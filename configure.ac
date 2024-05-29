#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([example], [0.1.0], [frank.dean@fdsd.co.uk])
AC_COPYRIGHT([Copyright (C) 2022-2024 Frank Dean])
dnl May fix an issue where the make files perform a rebuild of configure.ac,
dnl Makefile.am etc.  Run ./configure with --enable-maintainer-mode to
dnl automatically re-run autoconf scripts.
AM_MAINTAINER_MODE
AM_INIT_AUTOMAKE([-Wall -Werror foreign dist-zip subdir-objects info-in-builddir])
AC_CONFIG_SRCDIR([src/http_request_factory.cpp])
AC_CONFIG_HEADERS([config.h])

AC_CONFIG_TESTDIR([tests])
AC_CONFIG_FILES([tests/Makefile])

AC_ARG_ENABLE([postgresql],
    AS_HELP_STRING([--disable-postgresql],
	[Build to include libpqxx used by the PostgreSQL pool manger (default yes)]),
	[use_postgresql=${enableval}],
	[use_postgresql=yes])
AC_ARG_ENABLE([keep-alive],
    AS_HELP_STRING([--enable-keep-alive],
	[Build to enable HTTP keep-alive (default no)]),
	[use_keep_alive=${enableval}],
	[use_keep_alive=no])
AC_ARG_ENABLE([static-files],
    AS_HELP_STRING([--enable-static-files],
	[Build to allow HTTP serving of static files (default no)]),
	[use_static_files=${enableval}],
	[use_static_files=no])
AC_ARG_ENABLE([directory-listing],
    AS_HELP_STRING([--enable-directory-listing],
	[Build to allow HTTP directory listings (default no)]),
	[use_directory_listing=${enableval}],
	[use_directory_listing=no])

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
if test "x${use_postgresql}" == xyes; then
   if test "${have_libpqxx7}" -eq 1; then
      AC_DEFINE_UNQUOTED([HAVE_LIBPQXX7], [${have_libpqxx7}], [Build using libpqxx v7 instead of v6])
   else
      PKG_CHECK_MODULES([LIBPQXX], [libpqxx >= 6.2.5 libpqxx < 7 libpq >= 11.14])
   fi
fi

case $host in
*-*-freebsd* | *-*-linux-*)
    $PKG_CONFIG --exists uuid
    if [[ $? -eq 0 ]]; then
       have_libuuid=yes
       LIBUUID_CFLAGS=$($PKG_CONFIG --cflags uuid)
       LIBUUID_LIBS=$($PKG_CONFIG --libs uuid)
       dnl AC_DEFINE([HAVE_LINUX_UUID], [1], [Build with support for Linux libuuid implementation])
    else
	$PKG_CONFIG --exists ossp-uuid
	if [[ $? -eq 0 ]]; then
	   have_libosspuuid=yes
	   LIBUUID_CFLAGS=$($PKG_CONFIG --cflags ossp-uuid)
	   LIBUUID_LIBS=$($PKG_CONFIG --libs ossp-uuid)
	fi
    fi
;;
*-*-darwin*)
    # uuid in system library - man uuid_generate
    have_libuuid=yes
;;
*-*-mingw32* | *-*-windows | *-*-cygwin*)
# Windows not supported
;;
*)
esac

if test "x${have_libuuid}" = xyes; then
    AC_DEFINE([HAVE_LIBUUID], [1], [Build for libuuid implementation])
elif test "x${have_libosspuuid}" = xyes; then
    AC_DEFINE([HAVE_OSSP_UUID], [1], [Build for ossp-uuid implementation])
else
    AC_MSG_ERROR([Cannot find either libuuid or ossp-uuid libraries])
fi

if test "x${use_static_files}" == xyes; then
   AC_DEFINE([ALLOW_STATIC_FILES], [1], [Serve static files from user specified root])
fi
if test "x${use_static_files}" == xyes && test "x${use_directory_listing}" == xyes; then
   AC_DEFINE([ALLOW_DIRECTORY_LISTING], [1], [Allow directory listings])
fi
if test "x${use_keep_alive}" == xyes; then
   AC_DEFINE([ENABLE_KEEP_ALIVE], [1], [Enable HTTP keep alive])
fi

CXXFLAGS="${CXXFLAGS} -Wno-deprecated-declarations -Wsuggest-override ${LIBPQXX_CFLAGS} ${LIBUUID_CFLAGS}"
dnl CPPFLAGS="${CPPFLAGS} ${XXXXXX_CPPFLAGS}"
LIBS="${LIBS} ${LIBPQXX_LIBS} ${LIBUUID_LIBS}"

# Checks for header files.
dnl echo "Host_os: ${host_os}"
dnl echo "Host: ${host}"
dnl
AC_HEADER_ASSERT
dnl The following headers check is just to ensure the headers have been
dnl installed in addition to the actual library
if test "x${use_postgresql}" == xyes; then
   AC_CHECK_HEADERS([pqxx/config-public-compiler.h], [have_pqxx_h=yes])
   case $host in
     *-*-linux-*)
	   if test "x${have_pqxx_h}" != xyes; then
	      AC_MSG_ERROR([Cannot find PostgreSQL C++ client header files, pqxx/config-public-compiler.h])
	   fi
	   ;;
     *-*-darwin*)
	   if test "x${have_pqxx_h}" != xyes; then
	      AC_MSG_WARN([Cannot find PostgreSQL C++ client header files, pqxx/config-public-compiler.h])
	   fi
	   ;;
     *-*-mingw32* | *-*-windows | *-*-cygwin*)
	   # Windows not supported
	   ;;
     *)
   esac

fi

AC_CHECK_HEADERS([getopt.h])
if test "x${use_postgresql}" == xyes; then
   AC_CHECK_HEADERS([pqxx/config-public-compiler.h], [have_pqxx_h=yes])
fi

# Check if the version of libuuid supports the uuid_generate_time_safe method
if test "x${have_libuuid}" = xyes; then
    SAFE_UUID_CHECK="#include <uuid/uuid.h>
      int main(int argc, char** argv) {
	uuid_t uuid;
	int safe = uuid_generate_time_safe(uuid);
      return 0;
    }"
    AC_MSG_CHECKING([if libuuid supports uuid_generate_time_safe])
    AC_LINK_IFELSE(
    [AC_LANG_SOURCE($SAFE_UUID_CHECK)],
    [AC_MSG_RESULT([yes]) AC_DEFINE([HAVE_LINUX_UUID], [1], [Build with support for Linux libuuid implementation])],
    [AC_MSG_RESULT([no])]
    )
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_CHECK_HEADER([string],,
    [AC_MSG_ERROR([Standard C++ library not found])])
AC_C_INLINE

# Checks for library functions.
AC_FUNC_MKTIME
AC_CHECK_FUNCS([strerror])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
