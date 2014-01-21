AC_DEFUN([MY_CHECK_LIBPROTOBUF_C], [
    libprotobuf_c_CFLAGS=""
    libprotobuf_c_LIBS="-lprotobuf-c"

    AC_ARG_WITH(
        [libprotobuf_c],
        AC_HELP_STRING([--with-libprotobuf_c=DIR], [libprotobuf-c installation path]),
        [],
        [withval="yes"]
    )
    if test "$withval" = "yes"; then
        withval="/usr /usr/local"
    fi

    libprotobuf_c_dir=""

    AC_MSG_CHECKING([for libprotobuf-c headers])
    for dir in $withval; do
        if test -f "$dir/include/protobuf-c/protobuf-c.h"; then
            libprotobuf_c_dir="$dir"
            if test "$dir" != "/usr"; then
                libprotobuf_c_CFLAGS="-I$dir/include"
            fi
            break
        elif test -f "$dir/include/google/protobuf-c/protobuf-c.h"; then
            libprotobuf_c_dir="$dir"
            libprotobuf_c_CFLAGS="-I$dir/include/google"
            break
        fi
    done
    if test -n "$libprotobuf_c_dir"; then
        AC_MSG_RESULT([$libprotobuf_c_dir])
    else
        AC_MSG_ERROR([cannot find protobuf-c.h in $withval])
    fi

    save_LDFLAGS="$LDFLAGS"
    save_LIBS="$LIBS"
    if test "$libprotobuf_c_dir" != "/usr"; then
        libprotobuf_c_LIBS="$libprotobuf_c_LIBS -L$libprotobuf_c_dir/lib"
        LDFLAGS="-L$libprotobuf_c_dir/lib"
    fi
    AC_CHECK_LIB(
        [protobuf-c],
        [protobuf_c_message_pack], 
        [],
        [AC_MSG_ERROR([required library not found])]
    )
    LDFLAGS="$save_LDFLAGS"
    LIBS="$save_LIBS"

    AC_SUBST([libprotobuf_c_CFLAGS])
    AC_SUBST([libprotobuf_c_LIBS])

    AC_PATH_PROG([PROTOC_C], [protoc-c])
    if test -z "$PROTOC_C"; then
        AC_MSG_ERROR([The protoc-c program was not found. Please install the protobuf-c compiler!])
    fi
])
