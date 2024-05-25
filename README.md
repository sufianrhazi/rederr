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

Binary [αcτµαlly pδrταblε εxεcµταblε](https://justine.lol/ape.html) builds of rederr can be found at:
[https://abstract.properties/misc/rederr](https://abstract.properties/misc/rederr) and
[https://abstract.properties/misc/rederr.com](https://abstract.properties/misc/rederr.com). They both are identical and have
a SHA-256 checksum of `4047fd8848875f9441fc22bd6bb674c273d68ddd10a9d800d6cd80fdf50239bd`.

If you are running on Windows, use `.com` file extension is needed for the command to execute correctly. If you are
running on MacOS, Linux, or any \*BSD (note: I have not manually confirmed \*BSD support), you can use either. If you
use `zsh` or `fish`, you may need to update to a recent version (see
[https://github.com/jart/cosmopolitan/issues/88](https://github.com/jart/cosmopolitan/issues/88)).

To build it yourself, run `make`. You can set the `CC` environment variable to specify your compiler of choice. 
