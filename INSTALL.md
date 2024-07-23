Building
========

Dependancies
------------

To build this program, you need git, cmake, and a C++ compiler. You also need ncurses and cxxopts.
The easiest way to install ncurses is from your system's package manager. You need the wide version,
and want the development headers. All in all, the name should look something like libncursesw-dev.
cxxopts can be installed from [here](https://github.com/jarro2783/cxxopts). Clone it, and follow
it's [installation instructions](https://github.com/jarro2783/cxxopts/blob/master/INSTALL).

Actually building the thing
---------------------------

Go to the top level directory of the cloned repo. Then run `cmake --preset=rel -S src -B build`.
Then `cd` to the newly created build directory. Run `cmake --build .` to build the thing.

Installing
----------

Copy the generated `flappy-bird` binary to the directory you want. For a system-wide installation,
this is usually "/usr/local/bin". To only install it for yourself, it's usually "~/.local/bin". If
you want the manpage, copy "src/flappy-bird.1" to "/usr/local/share/man/man1" or
"/usr/share/man/man1".

