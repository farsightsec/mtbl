#!/bin/sh

if [ -z "${top_srcdir}" ]; then
    echo "top_srcdir variable not set"
    exit 1
fi

if [ -z "${top_builddir}" ]; then
    echo "top_builddir variable not set"
    exit 1
fi

exec "${top_builddir}/t/test-fileset-filter" "${top_srcdir}/t/fileset-filter-data/animals.fileset"
