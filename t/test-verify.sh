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

for good_fname in \
    "${top_srcdir}/t/test-gh1-snappy.data" \
    "${top_srcdir}/t/test-gh1-zlib.data" \
    "${top_srcdir}/t/test-foreign-prefix.data" \
    "${top_srcdir}/t/test-verify-good1.data" \
; do
    "${top_builddir}/src/mtbl_verify" "${good_fname}" 2>&1
    if [ "$?" -ne "0" ]; then
        exit 1
    fi
done

echo "The following lines should be error messages."

for bad_fname in \
    "${top_srcdir}/t/test-verify-bad1.data" \
    "${top_srcdir}/t/test-verify-bad2.data" \
    "${top_srcdir}/t/test-verify-bad-index-block-offset.data" \
; do
    "${top_builddir}/src/mtbl_verify" "${bad_fname}" 2>&1
    if [ "$?" -ne "1" ]; then
        exit 1
    fi
done

exit 0
