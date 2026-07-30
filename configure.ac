AC_COPYRIGHT([[Copyright 2022-2026 Jeff Greer. All rights reserved. Restrictions apply, see COPYRIGHT and LICENSE files.]])
AC_INIT([SlimTS], [0.4.0])
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([include/config.h])
AM_INIT_AUTOMAKE

dnl ------------------------------------------------------------
dnl enforce minimual c++ standard
dnl ------------------------------------------------------------
CXXFLAGS="$CXXFLAGS -std=c++23"

AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug], [enable debug build flags])],
    [enable_debug="$enableval"],
    [enable_debug="no"]
)

if test "$enable_debug" = "yes"; then
    CXXFLAGS="$CXXFLAGS -g -O2"
fi

AC_ARG_ENABLE([release],
    [AS_HELP_STRING([--enable-release], [enable release build flags])],
    [enable_release="$enableval"],
    [enable_release="no"]
)

if test "$enable_release" = "yes"; then
    CXXFLAGS="$CXXFLAGS -O2 -DNDEBUG"
fi

AC_ARG_ENABLE([compact],
    [AS_HELP_STRING([--enable-compact], [enable compact build optimizations])],
    [enable_compact="$enableval"],
    [enable_compact="no"]
)

if test "$enable_compact" = "yes"; then
    CXXFLAGS="$CXXFLAGS -Os"
fi

AC_PROG_CXX
AM_PROG_AR
LT_INIT

AC_LANG_PUSH([C++])

AC_ARG_ENABLE([logging],
    [AS_HELP_STRING([--enable-logging], [enable logging support])],
    [enable_logging="$enableval"],
    [enable_logging="no"]
)

if test "$enable_logging" = "yes"; then
    AC_DEFINE([ENABLE_LOGGING], [1], [Define if logging is enabled])
fi

dnl ------------------------------------------------------------
dnl V8
dnl ------------------------------------------------------------
AC_ARG_WITH([v8],
    [AS_HELP_STRING([--with-v8=DIR], [path to V8 root directory])],
    [v8_root="$withval"],
    [AC_MSG_ERROR([--with-v8=DIR is required])]
)

v8_include="$v8_root/include"

dnl GN build variants
v8_obj_candidates="
$v8_root/out.gn/x64.release.sample/obj"

v8_obj=""
for d in $v8_obj_candidates; do
    if test -d "$d"; then
        v8_obj="$d"
        break
    fi
done

if test -z "$v8_obj"; then
    AC_MSG_ERROR([unable to locate V8 object directory under $v8_root])
fi

CPPFLAGS="-I$v8_include $CPPFLAGS"
LDFLAGS="-L$v8_obj $LDFLAGS"

AC_CHECK_HEADER([v8.h], [], [AC_MSG_ERROR([missing v8.h])])
AC_CHECK_HEADER([libplatform/libplatform.h], [], [AC_MSG_ERROR([missing libplatform headers])])

AC_CHECK_FILE([$v8_obj/libv8_monolith.a], [], [AC_MSG_ERROR([missing libv8_monolith.a])])
AC_CHECK_FILE([$v8_obj/libv8_libbase.a], [], [AC_MSG_ERROR([missing libv8_libbase.a])])
AC_CHECK_FILE([$v8_obj/libv8_libplatform.a], [], [AC_MSG_ERROR([missing libv8_libplatform.a])])

PKG_PROG_PKG_CONFIG

dnl ------------------------------------------------------------------
dnl OpenSSL (STATIC ONLY + --with-openssl=DIR)
dnl ------------------------------------------------------------------
AC_ARG_WITH([openssl],
    [AS_HELP_STRING([--with-openssl=DIR], [path to OpenSSL install prefix (static .a required)])],
    [],
    [with_openssl=""]
)

openssl_root="$with_openssl"
found_openssl="no"

dnl preferred search roots (in order)
ssl_include_dir=""
ssl_lib_dir=""

if test -n "$openssl_root"; then
    ssl_include_dir="$openssl_root/include"
    ssl_lib_dir="$openssl_root/lib"
else
    dnl fallback to /usr/local first, then system paths
    if test -d "/usr/local/include/openssl" -a -d "/usr/local/lib"; then
        ssl_include_dir="/usr/local/include"
        ssl_lib_dir="/usr/local/lib"
    else
        ssl_include_dir="/usr/include"
        ssl_lib_dir="/usr/lib/x86_64-linux-gnu"
    fi
fi

CPPFLAGS="-I$ssl_include_dir $CPPFLAGS"
LDFLAGS="-L$ssl_lib_dir $LDFLAGS"

dnl pkg-config (optional)
if test -n "$PKG_CONFIG"; then
    PKG_CHECK_MODULES([OPENSSL], [openssl], [
        CPPFLAGS="$CPPFLAGS $OPENSSL_CFLAGS"
        found_openssl="yes"
    ], [
        found_openssl="no"
    ])
fi

dnl static library fallback checks
if test "$found_openssl" = "no"; then
    ssl_lib="$ssl_lib_dir/libssl.a"
    crypto_lib="$ssl_lib_dir/libcrypto.a"

    AC_CHECK_FILE([$ssl_lib], [
        AC_CHECK_FILE([$crypto_lib], [
            found_openssl="yes"
        ])
    ])
fi

if test "$found_openssl" = "no"; then
    AC_MSG_ERROR([OpenSSL static libraries (.a) not found])
fi

AC_CHECK_HEADER([openssl/ssl.h], [], [AC_MSG_ERROR([missing OpenSSL headers])])
AC_CHECK_HEADER([openssl/crypto.h], [], [AC_MSG_ERROR([missing OpenSSL crypto headers])])

dnl ------------------------------------------------------------
dnl SlimCommon
dnl ------------------------------------------------------------
PKG_CHECK_MODULES([SLIMCOMMON], [slimcommon],
    [CPPFLAGS="$CPPFLAGS $SLIMCOMMON_CFLAGS"
     LIBS="$LIBS $SLIMCOMMON_LIBS"],
    [AC_MSG_ERROR([SlimCommon is required but not found])]
)

dnl ------------------------------------------------------------
dnl libtsgo
dnl ------------------------------------------------------------
PKG_CHECK_MODULES([LIBTSGO], [libtsgo],
    [CPPFLAGS="$CPPFLAGS $LIBTSGO_CFLAGS"
     LIBS="$LIBS $LIBTSGO_LIBS"],
    [AC_MSG_ERROR([libtsgo is required but not found])]
)

dnl ------------------------------------------------------------
dnl project include
dnl ------------------------------------------------------------
cwd=`pwd`
CPPFLAGS="-I$cwd/include $CPPFLAGS"

AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)

dnl ------------------------------------------------------------
dnl module install information
dnl ------------------------------------------------------------

AC_PROG_AWK
SLIMTS_VERSION_MAJOR=`echo $PACKAGE_VERSION | $AWK -F"." '{print $1}'`
SLIMTS_VERSION_MINOR=`echo $PACKAGE_VERSION | $AWK -F"." '{print $2}'`
SLIMTS_VERSION_MICRO=`echo $PACKAGE_VERSION | $AWK -F"." '{print $3}'`
MODULE_VERSION_INFO="$SLIMTS_VERSION_MAJOR:$SLIMTS_VERSION_MINOR:$SLIMTS_VERSION_MICRO"
AC_SUBST([MODULE_VERSION_INFO])

moduledir="$libdir/SlimTS"
AC_SUBST([moduledir])

AC_MSG_NOTICE([MODULE_VERSION_INFO = $MODULE_VERSION_INFO])
AC_MSG_NOTICE([MODULE_INSTALL_DIR = $moduledir])

PKG_INSTALLDIR

dnl ------------------------------------------------------------
dnl output
dnl ------------------------------------------------------------
AC_CONFIG_FILES([
    Makefile
    src/Makefile
    src/plugins/Makefile
    src/plugins/console/Makefile
    src/plugins/color-console/Makefile
    src/plugins/fs/Makefile
    src/plugins/http_package/Makefile
    src/plugins/http/Makefile
    src/plugins/memoryAdaptor/Makefile
    src/plugins/os/Makefile
    src/plugins/path/Makefile
    src/plugins/process/Makefile
    src/plugins/queue/Makefile
    slimts.pc:slimts.pc.in
])

dnl ------------------------------------------------------------
dnl node step
dnl ------------------------------------------------------------
(cd bin && node bundle_lib_files.mjs)

AC_OUTPUT
