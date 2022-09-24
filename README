<!-- -*- mode: markdown; -*- vim: set tw=78 ts=4 sts=0 sw=4 noet ft=markdown norl: -->

# Trip Server Common

## Building

### Debian

For Debian versions 10 and 11.

Minimal packages required to build from the source distribution tarball:

- g++
- make
- uuid-dev
- libpqxx-dev

To build the application:

	$ ./configure
	$ make

Optionally, run the tests:

	$ make check

Optionally install:

	$ sudo make install

Minimal packages to build from Git clone:

- automake
- autoconf-archive

To re-create the required Gnu autotools files:

	$ autoreconf -i
	$ automake --add-missing --copy

Optionally install the `uuid-runtime` packages which runs a daemon
that `libuuid` uses to create secure UUIDs.

To run as a daemon, create a system user, e.g.

	$ sudo adduser trip --system --group --home /nonexistent --no-create-home

### macOS

Download, build and install the latest 6.x release of libpqxx from
<https://github.com/jtv/libpqxx/releases/tag/6.4.5>.

`libpqxx` needs the `doxygen` and `xmlto` packages installed to build the
refence documentation and tutorial.  Pass `--disable-documentation` to the
`./configure` command if you wish to skip building the documentation.

When running the `./configure` command to build this application, define
`PKG_CONFIG_PATH` to include where `libpqxx.pc` and d`libpq.pc` are installed,
e.g.:

	./configure PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:$(pg_config --libdir)/pkgconfig"

Add `CXXFLAGS='-g -O0'` to disable compiler optimisation.

To build from a Git clone, install the following ports from MacPorts:

- automake
- autoconf-archive

[MacPorts]: http://www.macports.org/ "MacPorts Home Page"
