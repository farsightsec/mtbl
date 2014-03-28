#!/bin/sh

if [ -z "${top_srcdir}" ]; then
    echo "top_srcdir variable not set"
    exit 1
fi

if [ -z "${top_builddir}" ]; then
    echo "top_builddir variable not set"
    exit 1
fi

ulimit -c 0

echo "The following line should be an error message."

"${top_builddir}/src/mtbl_dump" "${top_srcdir}/t/deb716628.data" 2>&1
if [ "$?" -eq "1" ]; then
    exit 0
fi

exit 1
