# SYNOPSIS
#
#   my_PKG_CONFIG_FILES(pc-files-variable, pc-files-value)
#
# DESCRIPTION
#
#   Wrapper for PKG_INSTALLDIR that allows disabling the installation of .pc
#   files when explicitly configured with --without-pkgconfigdir. It also
#   selects a more sensible default for pkgconfigdir on platforms that place
#   pkg-config files in a "libdata" directory.
#
#   If .pc file installation is enabled, pc-files-variable will be set to
#   pc-files-value.
#
#   Example:
#
#     In configure.ac:
#
#       my_PKG_CONFIG_FILES([LIBEXAMPLE_PC], [src/libexample.pc])
#
#     In Makefile.am:
#
#       pkgconfig_DATA = ${LIBEXAMPLE_PC}
#       CLEANFILES += ${LIBEXAMPLE_PC}
#
#   Here, ${LIBEXAMPLE_PC} will normally expand to "src/libexample.pc", unless
#   configure was called with --without-pkgconfigdir, in which case it will
#   expand to "".
#

AC_DEFUN([my_PKG_CONFIG_FILES],
[
    PKG_PROG_PKG_CONFIG

    pkgconfig_dir='${libdir}/pkgconfig'
    if test -n "$PKG_CONFIG"; then
        if $PKG_CONFIG --variable=pc_path pkg-config 2>/dev/null | grep -q /libdata/; then
            pkgconfig_dir='${prefix}/libdata/pkgconfig'
        fi
    fi
    PKG_INSTALLDIR([$pkgconfig_dir])

    if test "x$pkgconfigdir" != "xno"; then
        if test -z "$PKG_CONFIG"; then
            AC_MSG_ERROR([pkg-config is required!])
        fi
        $1=$2
        AC_SUBST([$1])
    fi
])
