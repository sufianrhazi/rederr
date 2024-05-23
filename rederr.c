#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 4096
char chunk_buffer[BUFFER_SIZE];

// write() can get interrupted, so this retries until the buffer is fully written
int write_all(int fd, const char *buffer, size_t len) {
    ssize_t written = 0;
    do {
        ssize_t wrote = write(fd, buffer + written, len - written);
        if (wrote == -1) return -1;
        written += wrote;
    } while (written < len);
    return written;
}

int handle_chunk(const char *prefix, int prefix_len, const char *suffix, int suffix_len, int out_fd, int in_fd) {
    bool prefix_sent = false;

    ssize_t num_read = read(in_fd, chunk_buffer, BUFFER_SIZE);
    if (num_read == -1) goto err;
    if (num_read == 0) {
        close(out_fd);
        return 1;
    }

    if (!prefix_sent) {
        if (prefix_len > 0) {
            ssize_t prefix_written = write_all(out_fd, prefix, prefix_len);
            if (prefix_written == -1) goto err;
        }
        prefix_sent = true;
    }

    ssize_t num_write = write_all(out_fd, chunk_buffer, num_read);
    if (num_write == -1) goto err;

    if (suffix_len > 0) {
        ssize_t suffix_written = write_all(out_fd, suffix, suffix_len);
        if (suffix_written == -1) goto err;
    }

    return 0;
err:
    return -1;
}

int main(int argc, char *argv[]) {
    char *stdout_prefix = getenv("REDERR_STDOUT_PREFIX");
    if (stdout_prefix == NULL) {
        stdout_prefix = "";
    }
    char stdout_prefix_len = strlen(stdout_prefix);
    char *stdout_suffix = getenv("REDERR_STDOUT_SUFFIX");
    if (stdout_suffix == NULL) {
        stdout_suffix = "";
    }
    char stdout_suffix_len = strlen(stdout_suffix);
    char *stderr_prefix = getenv("REDERR_STDERR_PREFIX");
    if (stderr_prefix == NULL) {
        stderr_prefix = "\e[31m";
    }
    char stderr_prefix_len = strlen(stderr_prefix);
    char *stderr_suffix = getenv("REDERR_STDERR_SUFFIX");
    if (stderr_suffix == NULL) {
        stderr_suffix = "\e[0m";
    }
    char stderr_suffix_len = strlen(stderr_suffix);

    if (argc < 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        printf("Usage: %s [-h|--help] CMD [ARG] ...\n", argv[0]);
        printf("\n");
        printf(
            "    Runs CMD with %sstdout like this%s and %sstderr like this%s.\n",
            stdout_prefix,
            stdout_suffix,
            stderr_prefix,
            stderr_suffix
        );
        if (argc >= 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
            printf("\n");
            printf("Version: 1.0.0\n");
            printf("\n");
            printf("Environment variables:\n");
            printf("    REDERR_STDOUT_PREFIX\n");
            printf("        bytes to write before stdout; default: \"\"\n");
            printf("    REDERR_STDOUT_SUFFIX\n");
            printf("        bytes to write after stdout; default: \"\"\n");
            printf("    REDERR_STDERR_PREFIX\n");
            printf("        bytes to write before stderr; default: \"\\e[31m\" (foreground red)\n");
            printf("    REDERR_STDERR_SUFFIX\n");
            printf("        bytes to write after stderr; default: \"\\e[0m\" (reset)\n");
            printf("\n");
            printf("Notes:\n");
            printf("    CMD is launched with stdout and stderr as a pipe, not a virtual tty.\n");
            printf("\n");
            printf("    If stderr and stdout are written close together, output may be out of order.\n");
            printf("\n");
            printf("    rederr only adds additional output to stdout/stderr for formatting,\n");
            printf("    if CMD emits ANSI escape codes, they will be present in the output.\n");
            printf("\n");
            printf("    rederr exits with 111 in case of an internal errors.\n");
        }
        return 1;
    }
    int child_stdout[2];
    int child_stderr[2];
    if (pipe(child_stdout) == -1) goto panic;
    if (pipe(child_stderr) == -1) goto panic;

    // Set a new process group to forward signals from the terminal to the group
    setpgid(0, 0);

    pid_t child_pid = fork();
    if (child_pid == -1) goto panic;
    if (child_pid == 0) {
        close(1);
        close(2);
        close(child_stdout[0]);
        close(child_stderr[0]);

        if (dup2(child_stdout[1], 1) == -1) goto panic;
        if (dup2(child_stderr[1], 2) == -1) goto panic;

        execvp(argv[1], &argv[1]);
        fprintf(stderr, "Failed to exec (%d): %s\n", errno, strerror(errno));
        return 111;
    } else {
        close(child_stdout[1]);
        close(child_stderr[1]);

        if (fcntl(child_stdout[0], F_SETFL, O_NONBLOCK) == -1) goto panic_kill;
        if (fcntl(child_stderr[0], F_SETFL, O_NONBLOCK) == -1) goto panic_kill;

        // Ignore signals a terminal can produce in the parent process, so the child process can handle them
        struct sigaction signal_ignore;
        signal_ignore.sa_handler = SIG_IGN;
        sigemptyset(&signal_ignore.sa_mask);
        signal_ignore.sa_flags = 0;

        if (sigaction(SIGINT, &signal_ignore, NULL) == -1) goto panic_kill;
        if (sigaction(SIGQUIT, &signal_ignore, NULL) == -1) goto panic_kill;

        fd_set read_fds;
        fd_set except_fds;
        struct timeval timeout;
        int maxfd = (child_stdout[0] < child_stderr[0] ? child_stderr[0] : child_stdout[0]) + 1;
        int stdout_closed = 0;
        int stderr_closed = 0;

        while (!stdout_closed || !stderr_closed) {
            FD_ZERO(&read_fds);
            FD_ZERO(&except_fds);

            if (!stdout_closed) {
                FD_SET(child_stdout[0], &read_fds);
                FD_SET(child_stdout[0], &except_fds);
            }
            if (!stderr_closed) {
                FD_SET(child_stderr[0], &read_fds);
                FD_SET(child_stderr[0], &except_fds);
            }

            timeout.tv_sec = 1;
            timeout.tv_usec = 0;

            int ready = select(maxfd, &read_fds, NULL, &except_fds, &timeout);
            if (ready == -1) goto panic_kill;
            if (ready) {
                if (!stdout_closed && FD_ISSET(child_stdout[0], &except_fds)) {
                    // Something went terribly wrong, close the pipe
                    close(child_stdout[0]);
                    stdout_closed = 1;
                }
                if (!stderr_closed && FD_ISSET(child_stderr[0], &except_fds)) {
                    // Something went terribly wrong, close the pipe
                    close(child_stderr[0]);
                    stderr_closed = 1;
                }
                if (!stdout_closed && FD_ISSET(child_stdout[0], &read_fds)) {
                    stdout_closed = handle_chunk(stdout_prefix, stdout_prefix_len, stdout_suffix, stdout_suffix_len, 1, child_stdout[0]);
                    if (stdout_closed == -1) goto panic_kill;
                }
                if (!stderr_closed && FD_ISSET(child_stderr[0], &read_fds)) {
                    stderr_closed = handle_chunk(stderr_prefix, stderr_prefix_len, stderr_suffix, stderr_suffix_len, 2, child_stderr[0]);
                    if (stderr_closed == -1) goto panic_kill;
                }
            }
        }

        int status;
        if (waitpid(child_pid, &status, 0) == -1) goto panic;
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        if (WIFSIGNALED(status)) {
            fprintf(stderr, "Child exited with signal %d\n", WTERMSIG(status));
            return 111;
        }
        return 111;
    }

panic_kill:
    kill(child_pid, SIGTERM);
    return 111;
panic:
    return 111;
}
