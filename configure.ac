AC_PREREQ(2.64)
AC_INIT([mtbl],
        [0.8.0],
        [https://github.com/farsightsec/mtbl/issues],
        [mtbl],
        [https://github.com/farsightsec/mtbl])
AC_CONFIG_SRCDIR([mtbl/mtbl.h])
AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([foreign 1.11 -Wall -Wno-portability silent-rules subdir-objects color-tests])
AC_PROG_CC_STDC
AC_USE_SYSTEM_EXTENSIONS
AC_SYS_LARGEFILE
AC_CONFIG_MACRO_DIR([m4])
AM_SILENT_RULES([yes])
LT_INIT

my_CFLAGS="-Wall \
-Wmissing-declarations -Wmissing-prototypes \
-Wnested-externs -Wpointer-arith \
-Wpointer-arith -Wsign-compare -Wchar-subscripts \
-Wstrict-prototypes -Wshadow \
-Wformat-security"
AC_SUBST([my_CFLAGS])

AC_CONFIG_HEADERS(config.h)
AC_CONFIG_FILES([Makefile mtbl/libmtbl.pc])

PKG_PROG_PKG_CONFIG
if test -n "$PKG_CONFIG"; then
    # Horrible hack for systems where the pkg-config install directory is simply wrong!
    if $PKG_CONFIG --variable=pc_path pkg-config 2>/dev/null | grep -q /libdata/; then
        PKG_INSTALLDIR(['${prefix}/libdata/pkgconfig'])
    else
        PKG_INSTALLDIR
    fi
else
    AC_MSG_ERROR([pkg-config is required!])
fi

PKG_CHECK_MODULES([liblz4], [liblz4 > 129])

AC_C_BIGENDIAN

AC_CHECK_FUNC([mkstemp], [], [
    AC_MSG_ERROR([required system function not found])
])

AC_CHECK_FUNCS([posix_madvise madvise])

AC_CHECK_HEADERS([sys/endian.h endian.h])

AC_CHECK_HEADER([snappy-c.h], [], [
    AC_MSG_ERROR([required header file not found])
])
AC_CHECK_LIB([snappy], [snappy_compress], [], [
    AC_MSG_ERROR([required library not found])
])

AC_CHECK_HEADER([zlib.h], [], [
    AC_MSG_ERROR([required header file not found])
])
AC_CHECK_LIB([z], [deflate], [], [
    AC_MSG_ERROR([required library not found])
])

AC_SEARCH_LIBS([dlopen], [dl])
AC_SEARCH_LIBS([clock_gettime], [rt])

AC_PATH_PROG([ASCIIDOC], [a2x])
AM_CONDITIONAL([BUILD_MAN], [test -n "$ASCIIDOC"])
if test -n "$ASCIIDOC"; then
    DOC_MAN_MSG="yes (asciidoc available)"
else
    DOC_MAN_MSG="no (asciidoc not available)"
fi

gl_LD_VERSION_SCRIPT

AC_OUTPUT
AC_MSG_RESULT([
    $PACKAGE $VERSION

        compiler:               ${CC}
        cflags:                 ${CFLAGS}
        ldflags:                ${LDFLAGS}
        libs:                   ${LIBS}

        prefix:                 ${prefix}
        sysconfdir:             ${sysconfdir}
        libdir:                 ${libdir}
        includedir:             ${includedir}
        pkgconfigdir:           ${pkgconfigdir}

        bigendian:              ${ac_cv_c_bigendian}

        building manpage docs:  ${DOC_MAN_MSG}
])
