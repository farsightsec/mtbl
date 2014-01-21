AC_DEFUN([MY_CHECK_LIBPCAP], [
    libpcap_CFLAGS=""
    libpcap_LIBS="-lpcap"

    AC_ARG_WITH(
        [libpcap],
        AC_HELP_STRING([--with-libpcap=DIR], [libpcap installation path]),
        [],
        [withval="yes"]
    )
    if test "$withval" = "yes"; then
        withval="/usr /usr/local"
    fi

    libpcap_dir=""

    AC_MSG_CHECKING([for libpcap headers])
    for dir in $withval; do
        if test -f "$dir/include/pcap.h"; then
            libpcap_dir="$dir"
            if test "$dir" != "/usr"; then
                libpcap_CFLAGS="-I$dir/include"
            fi
            break
        fi
    done
    if test -n "$libpcap_dir"; then
        AC_MSG_RESULT([$libpcap_dir])
    else
        AC_MSG_ERROR([cannot find pcap.h in $withval])
    fi

    save_LDFLAGS="$LDFLAGS"
    save_LIBS="$LIBS"
    if test "$libpcap_dir" != "/usr"; then
        libpcap_LIBS="$libpcap_LIBS -L$libpcap_dir/lib"
        LDFLAGS="-L$libpcap_dir/lib"
    fi
    AC_CHECK_LIB(
        [pcap],
        [pcap_open_offline],
        [],
        [AC_MSG_ERROR([required library not found])]
    )
    AC_SEARCH_LIBS(
        [pcap_create],
        [pcap],
        AC_DEFINE([HAVE_PCAP_CREATE], [1], [Define to 1 if pcap_create() is available.])
    )
    LDFLAGS="$save_LDFLAGS"
    LIBS="$save_LIBS"

    AC_SUBST([libpcap_CFLAGS])
    AC_SUBST([libpcap_LIBS])
])
