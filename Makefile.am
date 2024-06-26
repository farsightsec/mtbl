AUTOMAKE_OPTIONS = parallel-tests

check_PROGRAMS =
bin_PROGRAMS =
TESTS =

EXTRA_DIST = COPYRIGHT LICENSE README.md
CLEANFILES =
ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

AM_CPPFLAGS = \
	-include $(top_builddir)/config.h \
	-I${top_srcdir}/mtbl
AM_CFLAGS = \
	${my_CFLAGS} \
	$(liblz4_CFLAGS) \
	$(libzstd_CFLAGS)
AM_LDFLAGS =

#
##
### code coverage
##
#

USE_LCOV=@USE_LCOV@
LCOV=@LCOV@
GENHTML=@GENHTML@

clean-coverage:
	@if [ $(USE_LCOV) = yes ] ; then \
		$(LCOV) --directory . --zerocounters ; \
		echo "Removing coverage info files and generated $(abs_top_builddir)/coverage-html/ directory" ; \
		rm -rf all.coverage report.coverage ; \
		rm -rf $(abs_top_builddir)/coverage-html/ ; \
	else \
		echo "Code coverage not enabled at configuration time." ; \
		echo "Use: ./configure --with-coverage" ; \
fi

report-coverage:
	@if [ $(USE_LCOV) = yes ] ; then \
		$(LCOV) --capture --directory . --output-file all.coverage ; \
		$(LCOV) --remove all.coverage \
			$(abs_top_srcdir)/t/\* \
			/usr/include/\* \
			--output report.coverage ; \
		$(GENHTML) --legend -o $(abs_top_builddir)/coverage-html report.coverage ; \
		echo "Generated Code Coverage report in HTML at $(abs_top_builddir)/coverage-html" ; \
	else \
		echo "Code coverage not enabled at configuration time." ; \
		echo "Use: ./configure --with-coverage" ; \
fi

#
##
### library
##
#

LIBMTBL_VERSION_INFO=3:2:2

include_HEADERS = mtbl/mtbl.h
lib_LTLIBRARIES = mtbl/libmtbl.la

mtbl_libmtbl_la_SOURCES = \
	libmy/crc32c.c libmy/crc32c.h libmy/crc32c-slicing.c libmy/crc32c-sse42.c \
	libmy/heap.c libmy/heap.h \
	libmy/my_alloc.h \
	libmy/my_byteorder.h \
	libmy/my_fileset.c libmy/my_fileset.h \
	libmy/my_time.h \
	libmy/ubuf.h \
	libmy/vector.h \
	mtbl/block.c \
	mtbl/block_builder.c \
	mtbl/bytes.h \
	mtbl/compression.c \
	mtbl/crc32c_wrap.c \
	mtbl/fileset.c \
	mtbl/fixed.c \
	mtbl/iter.c \
	mtbl/merger.c \
	mtbl/mtbl.h \
	mtbl/mtbl-private.h \
	mtbl/reader.c \
	mtbl/sorter.c \
	mtbl/source.c \
	mtbl/metadata.c \
	mtbl/varint.c \
	mtbl/writer.c

mtbl_libmtbl_la_LIBADD = -lsnappy -lz $(liblz4_LIBS) $(libzstd_LIBS)
mtbl_libmtbl_la_LDFLAGS = $(AM_LDFLAGS) \
	-no-undefined \
	-version-info $(LIBMTBL_VERSION_INFO)
if HAVE_LD_VERSION_SCRIPT
mtbl_libmtbl_la_LDFLAGS += \
	-Wl,--version-script=$(top_srcdir)/mtbl/libmtbl.sym
else
mtbl_libmtbl_la_LDFLAGS += \
	-export-symbols-regex "^(mtbl_[a-z].*)"
endif
EXTRA_DIST += mtbl/libmtbl.sym

pkgconfig_DATA = mtbl/libmtbl.pc
EXTRA_DIST += mtbl/libmtbl.pc.in
CLEANFILES += mtbl/libmtbl.pc

#
##
### programs
##
#

bin_PROGRAMS += src/mtbl_dump
src_mtbl_dump_SOURCES = src/mtbl_dump.c libmy/print_string.h libmy/hex_decode.h
src_mtbl_dump_LDADD = mtbl/libmtbl.la

bin_PROGRAMS += src/mtbl_info
src_mtbl_info_SOURCES = src/mtbl_info.c
src_mtbl_info_LDADD = mtbl/libmtbl.la

bin_PROGRAMS += src/mtbl_verify
src_mtbl_verify_SOURCES = src/mtbl_verify.c
src_mtbl_verify_LDADD = mtbl/libmtbl.la

bin_PROGRAMS += src/mtbl_merge
src_mtbl_merge_SOURCES = src/mtbl_merge.c libmy/getenv_int.h
src_mtbl_merge_LDADD = mtbl/libmtbl.la

#
##
### tests
##
#

AM_TESTS_ENVIRONMENT = top_builddir='$(top_builddir)'; top_srcdir='$(top_srcdir)'; export top_builddir top_srcdir;
TESTS_ENVIRONMENT = $(AM_TESTS_ENVIRONMENT)

TESTS += t/test-block_builder
check_PROGRAMS += t/test-block_builder
t_test_block_builder_SOURCES = t/test-block_builder.c
t_test_block_builder_LDADD = mtbl/libmtbl.la

TESTS += t/test-crc32c
check_PROGRAMS += t/test-crc32c
t_test_crc32c_SOURCES = t/test-crc32c.c
t_test_crc32c_LDADD = mtbl/libmtbl.la

TESTS += t/test-fileset-partition.sh
EXTRA_DIST += t/test-fileset-partition.sh
EXTRA_DIST += t/fileset-partition/test.fileset
EXTRA_DIST += t/fileset-partition/file1.mtbl
EXTRA_DIST += t/fileset-partition/file2.mtbl
EXTRA_DIST += t/fileset-partition/file3.mtbl
check_PROGRAMS += t/test-fileset-partition
t_test_fileset_partition_SOURCES = t/test-fileset-partition.c
t_test_fileset_partition_LDADD = mtbl/libmtbl.la
t/test-fileset-partition.sh: t/test-fileset-partition

TESTS += t/test-fileset-filter.sh
EXTRA_DIST += t/test-fileset-filter.sh
EXTRA_DIST += t/fileset-filter-data/animals.fileset
EXTRA_DIST += t/fileset-filter-data/animals-1.mtbl
EXTRA_DIST += t/fileset-filter-data/animals-2.mtbl
EXTRA_DIST += t/fileset-filter-data/animals-3.mtbl
check_PROGRAMS += t/test-fileset-filter
t_test_fileset_filter_SOURCES = t/test-fileset-filter.c
t_test_fileset_filter_LDADD = mtbl/libmtbl.la
t/test-fileset-filter.sh: t/test-fileset-filter

TESTS += t/test-fixed
check_PROGRAMS += t/test-fixed
t_test_fixed_SOURCES = t/test-fixed.c
t_test_fixed_LDADD = mtbl/libmtbl.la

TESTS += t/test-metadata
check_PROGRAMS += t/test-metadata
t_test_metadata_SOURCES = \
	t/test-metadata.c \
	libmy/b64_encode.c \
	libmy/b64_encode.h
t_test_metadata_LDADD = mtbl/libmtbl.la

TESTS += t/test-varint
check_PROGRAMS += t/test-varint
t_test_varint_SOURCES = t/test-varint.c
t_test_varint_LDADD = mtbl/libmtbl.la

TESTS += t/test-vector
check_PROGRAMS += t/test-vector
t_test_vector_SOURCES = t/test-vector.c
t_test_vector_LDADD = mtbl/libmtbl.la

TESTS += t/test-iter-seek
check_PROGRAMS += t/test-iter-seek
t_test_iter_seek_SOURCES = t/test-iter-seek.c
t_test_iter_seek_LDADD = mtbl/libmtbl.la

TESTS += t/test-sorted-merge
check_PROGRAMS += t/test-sorted-merge
t_test_sorted_merge_SOURCES = t/test-sorted-merge.c
t_test_sorted_merge_LDADD = mtbl/libmtbl.la

TESTS += t/test-verify.sh
EXTRA_DIST += t/test-verify.sh
EXTRA_DIST += t/test-verify-good1.data t/test-verify-bad1.data t/test-verify-bad2.data
t/test-verify.sh: src/mtbl_verify

TESTS += t/test-deb716628.sh
EXTRA_DIST += t/test-deb716628.sh
EXTRA_DIST += t/test-deb716628.data
t/test-deb716628.sh: src/mtbl_dump

TESTS += t/test-gh1.sh
EXTRA_DIST += t/test-gh1.sh
EXTRA_DIST += t/test-gh1-lz4.data
EXTRA_DIST += t/test-gh1-lz4hc.data
EXTRA_DIST += t/test-gh1-snappy.data
EXTRA_DIST += t/test-gh1-zlib.data
t/test-gh1.sh: src/mtbl_dump

TESTS += t/test-foreign-prefix.sh
EXTRA_DIST += t/test-foreign-prefix.sh
EXTRA_DIST += t/test-foreign-prefix.data
t/test-foreign-prefix.sh: src/mtbl_dump

TESTS += t/test-compression.sh
EXTRA_DIST += t/test-compression.sh
check_PROGRAMS += t/test-compression
t_test_compression_SOURCES = t/test-compression.c
t_test_compression_LDADD = mtbl/libmtbl.la
t/test-compression.sh: t/test-compression

#
##
### documentation
##
#

if BUILD_MAN
SUFFIXES = .1.txt .3.txt .7.txt .1 .3 .7

ASCIIDOC_PROCESS = $(AM_V_GEN) $(ASCIIDOC) -f manpage --no-xmllint --asciidoc-opt="-f man/asciidoc.conf" $<

.1.txt.1:
	$(ASCIIDOC_PROCESS)

.3.txt.3:
	$(ASCIIDOC_PROCESS)

.7.txt.7:
	$(ASCIIDOC_PROCESS)
endif

dist_man_MANS = \
	man/mtbl_dump.1 \
	man/mtbl_info.1 \
	man/mtbl_merge.1 \
	man/mtbl_verify.1 \
	man/mtbl_iter.3 \
	man/mtbl_metadata.3 \
	man/mtbl_source.3 \
	man/mtbl_reader.3 \
	man/mtbl_writer.3 \
	man/mtbl_merger.3 \
	man/mtbl_sorter.3 \
	man/mtbl_fileset.3 \
	man/mtbl_crc32c.3 \
	man/mtbl_fixed.3 \
	man/mtbl_varint.3 \
	man/mtbl.7

EXTRA_DIST += \
	man/mtbl_dump.1.txt \
	man/mtbl_info.1.txt \
	man/mtbl_merge.1.txt \
	man/mtbl_verify.1.txt \
	man/mtbl_iter.3.txt \
	man/mtbl_metadata.3.txt \
	man/mtbl_source.3.txt \
	man/mtbl_reader.3.txt \
	man/mtbl_writer.3.txt \
	man/mtbl_merger.3.txt \
	man/mtbl_sorter.3.txt \
	man/mtbl_fileset.3.txt \
	man/mtbl_crc32c.3.txt \
	man/mtbl_fixed.3.txt \
	man/mtbl_varint.3.txt \
	man/mtbl.7.txt
