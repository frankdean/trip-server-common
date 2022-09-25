<!-- -*- mode: markdown; -*- vim: set tw=78 ts=4 sts=0 sw=4 noet ft=markdown norl: -->

# Trip Server Common

This package contains code from the Trip Server 2 application which is
currently being developed as a port of [Trip Server][trip-server].  The code
in this module is intended to be useful common code for building web servers
generally.

Currently it is under active development and potentially subject to large
changes, and therefore not recommended for use on other projects at this time.

`./src/example.cpp` contains a fairly minimal example application that
demonstrates serving pages that do or do not require authentication and
session management.

## Building

These instructions are for building and installing from the source
distribution tarball, which contains additional artefacts to those maintained
under Git source control.  Building from a cloned Git repository requires
additional packages to be installed as described below.

### Debian

For Debian version 11 (Bullseye).

Minimal packages required to build from the source distribution tarball:

- g++
- gawk
- libpqxx-dev
- make
- uuid-dev

To build the application:

	$ ./configure
	$ make

Optionally, run the tests:

	$ make check

Install:

	$ sudo make install

Additional packages required to build from Git clone:

- automake
- autoconf-archive

To re-create the required Gnu autotools files:

	$ autoreconf -i
	$ automake --add-missing --copy

Optionally install the `uuid-runtime` package which runs a daemon that
`libuuid` uses to create secure UUIDs.

### macOS

Download, build and install the latest 6.x release of libpqxx from
<https://github.com/jtv/libpqxx/releases/tag/6.4.8>.

`libpqxx` needs the `doxygen` and `xmlto` packages installed to build the
refence documentation and tutorial.  Pass `--disable-documentation` to the
`./configure` command if you wish to skip building the documentation.

When running the `./configure` command to build this application, define
`PKG_CONFIG_PATH` to include where `libpqxx.pc` and d`libpq.pc` are installed,
e.g.:

	./configure PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:$(pg_config --libdir)/pkgconfig"

Add `CXXFLAGS='-g -O0'` to disable compiler optimisation.

To build from a Git clone, install the following ports from [MacPorts][]:

- autoconf
- automake
- autoconf-archive
- gawk
- pkgconfig

[MacPorts]: http://www.macports.org/ "MacPorts Home Page"
