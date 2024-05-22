rederr
======

Sometimes you just want to see stdin in red and stdout in white.

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

To build, run `make`. It assumes `cc` is an alias to a C compiler. 

To install, place the produced `rederr` binary somewhere in your `PATH`.
