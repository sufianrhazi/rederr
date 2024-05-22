#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>

int handle_line(const char *prefix, int prefix_len, const char *suffix, int suffix_len, int out_fd, int in_fd) {
    bool prefix_sent = false;

    char c;
    do {
        ssize_t num_read = read(in_fd, &c, 1);
        if (num_read == -1) goto err;
        if (num_read == 0) {
            close(out_fd);
            return 1;
        }

        if (!prefix_sent) {
            ssize_t prefix_written = write(out_fd, prefix, prefix_len);
            if (prefix_written == -1) goto err;
            if (prefix_written != prefix_len) goto err;
            prefix_sent = true;
        }

        ssize_t num_write = write(out_fd, &c, 1);
        if (num_write == -1) goto err;
        if (num_write == 0) goto err;
    } while (c != '\n');

    ssize_t suffix_written = write(out_fd, suffix, suffix_len);
    if (suffix_written == -1) goto err;
    if (suffix_written != suffix_len) goto err;

    return 0;
err:
    return -1;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s CMD [ARG] ...\n", argv[0]);
        printf("\n");
        printf(
            "    Runs CMD with "
            "\e[1m" // CSI 1 m -- bold
            "stdout as bold"
            "\e[0m" // CSI 0 m -- reset
            " and "
            "\e[31m" // CSI 31 m -- red
            "stderr as red"
            "\e[0m" // CSI 0 m -- reset
            "\n"
        );
        printf("\n");
        printf("Note: if multiple lines are sent to stderr and stdout at the same time,\n");
        printf("they may be output out of order.\n");
        return 1;
    }
    int child_stdout[2];
    int child_stderr[2];
    if (pipe(child_stdout) == -1) goto panic;
    if (pipe(child_stderr) == -1) goto panic;

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
        goto panic;
    } else {
        close(child_stdout[1]);
        close(child_stderr[1]);
        fd_set read_fds;
        struct timeval timeout;
        int maxfd = (child_stdout[0] < child_stderr[0] ? child_stderr[0] : child_stdout[0]) + 1;
        int stdout_closed = 0;
        int stderr_closed = 0;

        while (!stdout_closed && !stderr_closed) {
            FD_ZERO(&read_fds);

            if (!stdout_closed) {
                FD_SET(child_stdout[0], &read_fds);
            }
            if (!stderr_closed) {
                FD_SET(child_stderr[0], &read_fds);
            }

            timeout.tv_sec = 1;
            timeout.tv_usec = 0;

            int ready = select(maxfd, &read_fds, NULL, NULL, &timeout);
            if (ready == -1) goto panic_kill;
            if (ready) {
                if (FD_ISSET(child_stdout[0], &read_fds)) {
                    stdout_closed = handle_line("\e[1m", 4, "\e[0m", 4, 1, child_stdout[0]);
                    if (stdout_closed == -1) goto panic_kill;
                }
                if (FD_ISSET(child_stderr[0], &read_fds)) {
                    stderr_closed = handle_line("\e[31m", 5, "\e[0m", 4, 2, child_stderr[0]);
                    if (stderr_closed == -1) goto panic_kill;
                }
            }
        }
        int status;
        waitpid(child_pid, &status, 0);
        return status;
    }

panic_kill:
    kill(child_pid, SIGINT);
panic:
    return 111;
}
