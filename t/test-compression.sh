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
    zstd \
; do
	for thread_count in \
		0 \
		1 \
		2 \
		5 \
		8 \
		10 \
		24 \
		36 \
		50 \
		100 \
		500 \
		1000 \
	; do
		echo "$0: Testing compression type ${compression_type} ${thread_count}"
		"${top_builddir}/t/test-compression" "${compression_type}" "${thread_count}"
	done
done

exit 0
