AC_DEFUN([MY_CHECK_BYTEORDER],
    [
        AC_C_BIGENDIAN(
            [AC_DEFINE([IS_LITTLE_ENDIAN], [0], [Define to 1 if system is little endian])],
            [AC_DEFINE([IS_LITTLE_ENDIAN], [1], [Define to 1 if system is little endian])],
            [AC_MSG_FAILURE([unable to determine endianness])]
        )
    ]
)
