AC_CHECK_HEADERS([sys/endian.h endian.h])

AC_CHECK_DECLS([htole32, le32toh], [], [], [
#ifdef HAVE_ENDIAN_H
# include <endian.h>
#else
# ifdef HAVE_SYS_ENDIAN_H
#  include <sys/endian.h>
# endif
#endif
])

# modified from protobuf-c
AC_MSG_CHECKING([if machine is little endian])
AC_RUN_IFELSE(
    [
        AC_LANG_PROGRAM(
            [
            #include <inttypes.h>
            #include <string.h>
            ],
            [
            uint32_t v = 0x01020304;
            return memcmp (&v, "\4\3\2\1", 4) == 0 ? 0 : 1;
            ]
        )
    ],
    [
        is_little_endian=1; result=yes
    ],
    [
        is_little_endian=0; result=no
    ]
)
AC_MSG_RESULT($result)

AC_DEFINE_UNQUOTED([IS_LITTLE_ENDIAN],
                   $is_little_endian,
                   [Define to 1 if machine is little endian, 0 otherwise])
