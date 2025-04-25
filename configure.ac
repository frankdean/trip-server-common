#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([example], [0.1.0], [frank.dean@fdsd.co.uk])
TRIP_SOURCE_URL=[https://www.fdsd.co.uk/trip-server-2/]
AC_SUBST([TRIP_SOURCE_URL])
AC_DEFINE_UNQUOTED([TRIP_SOURCE_URL], ["${TRIP_SOURCE_URL}"], [The download URL for this application's source code])
AC_COPYRIGHT([Copyright (C) 2022-2025 Frank Dean])
dnl May fix an issue where the make files perform a rebuild of configure.ac,
dnl Makefile.am etc.  Run ./configure with --enable-maintainer-mode to
dnl automatically re-run autoconf scripts.
AM_MAINTAINER_MODE
AM_INIT_AUTOMAKE([-Wall -Werror foreign dist-zip subdir-objects info-in-builddir])
AC_CONFIG_SRCDIR([src/http_request_factory.cpp])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_TESTDIR([tests])
AC_CONFIG_MACRO_DIRS([m4])

AC_ARG_ENABLE([postgresql],
    AS_HELP_STRING([--disable-postgresql],
	[Disable building to include libpqxx used by the PostgreSQL pool manger]))
dnl provide options to enable static files, directory listings and keep alive
TRIP_WEB_OPTION_STATIC_FILES([no])
TRIP_WEB_OPTION_DIRECTORY_LISTING([no])
TRIP_WEB_OPTION_KEEP_ALIVE([no])

# Checks for programs.
AC_LANG(C++)
dnl Must set the compiler to use before AX_CXX_COMPILE_STDCXX_17
#AC_PROG_CXX([clang++ g++ c++ gpp aCC CC cxx cc++ cl.exe FCC KCC RCC xlC_r xlC])
AC_PROG_CXX
AX_CXX_COMPILE_STDCXX_17
AC_PROG_INSTALL

# Set $host variable
AC_CANONICAL_HOST

# Checks for libraries.
AC_SEARCH_LIBS([pthread_create], [pthread])

PKG_CHECK_MODULES([LIBPQXX], [libpqxx >= 7.7.5 libpqxx < 8 libpq >= 11.14],
			     [AC_SUBST([have_libpqxx7], [1])],
			     [AC_SUBST([have_libpqxx7], [0])]
			     )
AS_IF([test "x${enable_postgresql}" != xno], [
   AS_IF([test "${have_libpqxx7}" -eq 1],
      [AC_DEFINE_UNQUOTED([HAVE_LIBPQXX7], [${have_libpqxx7}], [Build using libpqxx v7 instead of v6])],
      [PKG_CHECK_MODULES([LIBPQXX], [libpqxx >= 6.2.5 libpqxx < 7 libpq >= 11.14])]
   )
   AC_CHECK_HEADERS([pqxx/config-public-compiler.h],,
      AC_MSG_ERROR([[Cannot find PostgreSQL C++ client header files, pqxx/config-public-compiler.h]]))
])

dnl Check if the version of libuuid supports the uuid_generate_time_safe method
TRIP_CHECK_UUID_LIBS

CXXFLAGS="${CXXFLAGS} -Wno-deprecated-declarations -Wsuggest-override ${LIBPQXX_CFLAGS} ${LIBUUID_CFLAGS}"
dnl CPPFLAGS="${CPPFLAGS} ${XXXXXX_CPPFLAGS}"
LIBS="${LIBS} ${LIBPQXX_LIBS} ${LIBUUID_LIBS}"

dnl echo "Host_os: ${host_os}"
dnl echo "Host: ${host}"
dnl
# Checks for header files.
AC_HEADER_ASSERT
AC_CHECK_HEADERS([getopt.h])
dnl The following headers check is just to ensure the headers have been
dnl installed in addition to the actual library
dnl Check we have a UUID library and header
TRIP_CHECK_UUID_FUNC

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_CHECK_HEADER([string],,
    [AC_MSG_ERROR([Standard C++ library not found])])
AC_C_INLINE

# Checks for library functions.
AC_FUNC_MKTIME
AC_CHECK_FUNCS([strerror])

AC_CONFIG_FILES([Makefile
                 src/Makefile
		 tests/Makefile
		 ])
AC_OUTPUT
