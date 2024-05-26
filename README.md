rederr
======

Sometimes you just want to see stdin in red and stdout in white.

![rederr](https://github.com/sufianrhazi/rederr/assets/176408/a9d61bc7-69b5-4ef6-a600-d88c46020c03)


## Usage

    Usage: rederr [-h|--help] CMD [ARG] ...

        Runs CMD with stdout like this and stderr like this.

    Environment variables:
        REDERR_STDOUT_PREFIX
            bytes to write before stdout; default: ""
        REDERR_STDOUT_SUFFIX
            bytes to write after stdout; default: ""
        REDERR_STDERR_PREFIX
            bytes to write before stderr; default: "\e[31m" (foreground red)
        REDERR_STDERR_SUFFIX
            bytes to write after stderr; default: "\e[0m" (reset)

    Notes:
        CMD is launched with stdout and stderr as a pipe, not a virtual tty.

        If stderr and stdout are written close together, output may be out of order.

        rederr only adds additional output to stdout/stderr for formatting,
        if CMD emits ANSI escape codes, they will be present in the output.

        rederr exits with 111 in case of an internal errors.


## Building and installation

Binary builds of rederr can be found at: https://abstract.properties/misc/rederr and https://abstract.properties/misc/rederr.com. Both are identical and have a SHA-256 checksum of 4047fd8848875f9441fc22bd6bb674c273d68ddd10a9d800d6cd80fdf50239bd.

These are αcτµαlly pδrταblε εxεcµταblε builds built with cosmopolitan libc, which means they are fat binary executables that run on practically any platform: Linux + Mac + Windows + FreeBSD + OpenBSD + NetBSD + BIOS on AMD64 and ARM64.

Platform notes: 

* Windows: If you are running on Windows, the `.com` file extension is needed for the command to execute correctly.
* zsh / fish: If you use `zsh` or `fish` shells, you may need to update to a recent version (see [https://github.com/jart/cosmopolitan/issues/88](https://github.com/jart/cosmopolitan/issues/88)).
* MacOS: These binaries work on both Intel and Apple silicon.

End up using this on a non-Linux/Mac/Windows platform? Please reach out and let me know.

[https://abstract.properties/work/rederr.html](https://abstract.properties/work/rederr.html)
