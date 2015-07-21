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

for compression_type in \
    none \
    snappy \
    zlib \
    lz4 \
    lz4hc \
; do
    echo "$0: Testing compression type ${compression_type}"
    "${top_builddir}/t/test-compression" "${compression_type}"
done

exit 0
