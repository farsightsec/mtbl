#!/bin/sh -e

if [ -z "${top_srcdir}" ]; then
    echo "top_srcdir variable not set"
    exit 1
fi

if [ -z "${top_builddir}" ]; then
    echo "top_builddir variable not set"
    exit 1
fi

test "Hello, world!" = \
     "`head -c 13 "${top_srcdir}/t/test-foreign-prefix.data"`"
test '"some key\x0a" "1234567890123456"' = \
     "`"${top_builddir}/src/mtbl_dump" \
           "${top_srcdir}/t/test-foreign-prefix.data"`"
