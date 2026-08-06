AC_COPYRIGHT([[Copyright 2022-2026 Jeff Greer. All rights reserved. Restrictions apply, see COPYRIGHT and LICENSE files.]])
AC_INIT([SlimTS], m4_esyscmd_s([git describe --always --tags 2>/dev/null | sed 's/^v//' | cut -d'-' -f1 || echo "0.0.0"]))
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([include/config.h])
AM_INIT_AUTOMAKE

dnl ------------------------------------------------------------
dnl enforce minimual c++ standard
dnl ------------------------------------------------------------
CXXFLAGS="$CXXFLAGS -std=c++23"

dnl ------------------------------------------------------------
dnl V8 required flags
dnl ------------------------------------------------------------
CXXFLAGS="$CXXFLAGS -DV8_COMPRESS_POINTERS -DV8_ENABLE_SANDBOX -fno-rtti"

AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug], [enable debug build flags])],
    [enable_debug="$enableval"],
    [enable_debug="no"]
)

if test "$enable_debug" = "yes"; then
    CXXFLAGS="$CXXFLAGS -g -O0 -fsanitize=address"
    AM_LDFLAGS="$AM_LDFLAGS -fsanitize=address"
    do_strip="no"
else
    AC_MSG_NOTICE([no build type specified, defaulting to release])
    CXXFLAGS="$CXXFLAGS -O2 -DNDEBUG"
    do_strip="yes"
fi

AM_CONDITIONAL([RELEASE_STRIP], [test "x$do_strip" = "xyes"])

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

dnl ------------------------------------------------------------
dnl BoringSSL (via pkg-config)
dnl ------------------------------------------------------------
PKG_CHECK_MODULES([BORINGSSL], [boringssl],
    [CPPFLAGS="$CPPFLAGS $BORINGSSL_CFLAGS"
     LIBS="$LIBS $BORINGSSL_LIBS"],
    [AC_MSG_ERROR([BoringSSL is required but not found via pkg-config])]
)

AC_CHECK_HEADER([openssl/ssl.h], [], [AC_MSG_ERROR([missing BoringSSL headers (openssl/ssl.h)])])
AC_CHECK_HEADER([openssl/crypto.h], [], [AC_MSG_ERROR([missing BoringSSL crypto headers (openssl/crypto.h)])])

dnl ------------------------------------------------------------
dnl SlimCommon
dnl ------------------------------------------------------------
PKG_CHECK_MODULES([SLIMCOMMON], [slimcommon],
    [CPPFLAGS="$CPPFLAGS $SLIMCOMMON_CFLAGS"],
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
AC_SUBST(AM_LDFLAGS)

dnl ------------------------------------------------------------
dnl module install information
dnl ------------------------------------------------------------

moduledir="$libdir/SlimTS"
AC_SUBST([moduledir])

AC_MSG_NOTICE([PACKAGE_VERSION = $PACKAGE_VERSION])
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
    src/plugins/module/Makefile
    src/plugins/memoryAdaptor/Makefile
    src/plugins/os/Makefile
    src/plugins/path/Makefile
    src/plugins/process/Makefile
    src/plugins/queue/Makefile
    src/plugins/require/Makefile
    slimts.pc:slimts.pc.in
])

AC_OUTPUT
