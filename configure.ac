AC_COPYRIGHT([[Copyright 2022-2026 Jeff Greer. All rights reserved. Restrictions apply, see COPYRIGHT and LICENSE files.]])
AC_INIT([slimTS], [0.2.0])

AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([include/config.h])

AM_INIT_AUTOMAKE
AC_PROG_CXX
LT_INIT
AM_PROG_AR

AC_LANG_PUSH([C++])

dnl ------------------------------------------------------------------
dnl Force C++23 required
dnl ------------------------------------------------------------------
CXXFLAGS="$CXXFLAGS -std=c++23"

dnl ------------------------------------------------------------------
dnl Google V8 (header + library checks, required)
dnl ------------------------------------------------------------------
AC_ARG_WITH([google-v8-dir],
    [AS_HELP_STRING([--with-google-v8-dir=DIR], [path to installed Google V8 directory])],
    [],
    [with_google_v8_dir=""]
)

if test -z "$with_google_v8_dir"; then
    AC_MSG_ERROR([--with-google-v8-dir=DIR is required])
fi

v8_root="$with_google_v8_dir"
v8_obj="$v8_root/out.gn/x64.release.sample/obj"

CPPFLAGS="-I$v8_root/include $CPPFLAGS"
LDFLAGS="-L$v8_obj $LDFLAGS"

AC_CHECK_HEADER([v8.h], [],
    [AC_MSG_ERROR([missing v8.h])]
)

AC_CHECK_HEADER([libplatform/libplatform.h], [],
    [AC_MSG_ERROR([missing libplatform header])]
)

AC_CHECK_FILE([$v8_obj/libv8_libbase.a], [],
    [AC_MSG_ERROR([missing libv8_libbase.a])]
)

AC_CHECK_FILE([$v8_obj/libv8_monolith.a], [],
    [AC_MSG_ERROR([missing libv8_monolith.a])]
)

AC_CHECK_FILE([$v8_obj/libv8_libplatform.a], [],
    [AC_MSG_ERROR([missing libv8_libplatform.a])]
)

dnl ------------------------------------------------------------------
dnl librdkafka auto-detection
dnl ------------------------------------------------------------------
PKG_PROG_PKG_CONFIG

found_librdkafka="no"

if test -n "$PKG_CONFIG"; then
    PKG_CHECK_MODULES([RDKAFKA], [rdkafka++], [
        CPPFLAGS="$CPPFLAGS $RDKAFKA_CFLAGS"
        LIBS="$LIBS $RDKAFKA_LIBS"
        found_librdkafka="yes"
    ], [
        found_librdkafka="no"
    ])
fi

if test "$found_librdkafka" = "no"; then
    AC_CHECK_HEADER([librdkafka/rdkafkacpp.h], [
        AC_CHECK_LIB([rdkafka++], [main], [
            LIBS="$LIBS -lrdkafka++ -lrdkafka"
            found_librdkafka="yes"
        ])
    ])
fi

if test "$found_librdkafka" = "no"; then
    AC_MSG_ERROR([librdkafka not found])
fi

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

dnl ------------------------------------------------------------------
dnl project include path
dnl ------------------------------------------------------------------
cwd=`pwd`
CPPFLAGS="-I$cwd/include $CPPFLAGS"

AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)

dnl ------------------------------------------------------------------
dnl output files
dnl ------------------------------------------------------------------
AC_CONFIG_FILES([
    Makefile
    src/Makefile
    src/plugins/console/Makefile
    src/plugins/csv/Makefile
    src/plugins/fs/Makefile
    src/plugins/http_package/Makefile
    src/plugins/http_server/Makefile
    src/plugins/kafka/Makefile
    src/plugins/memoryAdaptor/Makefile
    src/plugins/os/Makefile
    src/plugins/path/Makefile
    src/plugins/process/Makefile
    src/plugins/queue/Makefile
])

dnl ------------------------------------------------------------------
dnl node bundle step
dnl ------------------------------------------------------------------
(cd bin && node bundle_lib_files.mjs)

AC_OUTPUT