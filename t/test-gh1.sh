#!/bin/sh -e

if [ -z "${top_srcdir}" ]; then
    echo "top_srcdir variable not set"
    exit 1
fi

if [ -z "${top_builddir}" ]; then
    echo "top_builddir variable not set"
    exit 1
fi

ulimit -c 0

"${top_builddir}/src/mtbl_dump" "${top_srcdir}/t/test-gh1-snappy.data"
"${top_builddir}/src/mtbl_dump" "${top_srcdir}/t/test-gh1-zlib.data"

exit 0
