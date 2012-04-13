EXTRA_DIST =
CLEANFILES =
ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

AM_CPPFLAGS = \
	-include $(top_builddir)/config.h \
	-I${top_srcdir}/mtbl

AM_CFLAGS = ${my_CFLAGS}

AM_LDFLAGS = -Wl,--as-needed

SED_PROCESS = \
	$(AM_V_GEN)$(MKDIR_P) $(dir $@) && $(SED) \
	-e 's,@VERSION\@,$(VERSION),g' \
	-e 's,@prefix\@,$(prefix),g' \
	-e 's,@exec_prefix\@,$(exec_prefix),g' \
	-e 's,@libdir\@,$(libdir),g' \
	-e 's,@includedir\@,$(includedir),g' \
	< $< > $@ || rm $@

%.pc: %.pc.in Makefile
	$(SED_PROCESS)

LIBMTBL_CURRENT=0
LIBMTBL_REVISION=0
LIBMTBL_AGE=0

include_HEADERS = mtbl/mtbl.h
lib_LTLIBRARIES = mtbl/libmtbl.la

mtbl_libmtbl_la_SOURCES = \
	mtbl/block.c \
	mtbl/block_builder.c \
	mtbl/bytes.h \
	mtbl/crc32c.c \
	mtbl/fixed.c \
	mtbl/heap.c \
	mtbl/iter.c \
	mtbl/merger.c \
	mtbl/mtbl.h \
	mtbl/mtbl-private.h \
	mtbl/reader.c \
	mtbl/sorter.c \
	mtbl/source.c \
	mtbl/trailer.c \
	mtbl/vector.h \
	mtbl/vector_types.h \
	mtbl/varint.c \
	mtbl/writer.c

mtbl_libmtbl_la_LIBADD = -lsnappy -lz
mtbl_libmtbl_la_LDFLAGS = $(AM_LDFLAGS) \
	-version-info $(LIBMTBL_CURRENT):$(LIBMTBL_REVISION):$(LIBMTBL_AGE) \
	-export-symbols-regex "^(mtbl_[a-z].*)"

install-exec-hook:
	rm -f $(DESTDIR)$(libdir)/libmtbl.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mtbl/libmtbl.pc
EXTRA_DIST += mtbl/libmtbl.pc.in
CLEANFILES += mtbl/libmtbl.pc

TESTS =
check_PROGRAMS =
bin_PROGRAMS =

bin_PROGRAMS += src/mtbl_dump
src_mtbl_dump_SOURCES = src/mtbl_dump.c librsf/print_string.h
src_mtbl_dump_LDADD = mtbl/libmtbl.la

bin_PROGRAMS += src/mtbl_info
src_mtbl_info_SOURCES = src/mtbl_info.c
src_mtbl_info_LDADD = mtbl/libmtbl.la

bin_PROGRAMS += src/mtbl_merge
src_mtbl_merge_SOURCES = src/mtbl_merge.c
src_mtbl_merge_LDADD = mtbl/libmtbl.la

TESTS += src/test-block_builder
check_PROGRAMS += src/test-block_builder
src_test_block_builder_SOURCES = src/test-block_builder.c
src_test_block_builder_LDADD = mtbl/libmtbl.la

TESTS += src/test-crc32c
check_PROGRAMS += src/test-crc32c
src_test_crc32c_SOURCES = src/test-crc32c.c
src_test_crc32c_LDADD = mtbl/libmtbl.la

TESTS += src/test-fixed
check_PROGRAMS += src/test-fixed
src_test_fixed_SOURCES = src/test-fixed.c
src_test_fixed_LDADD = mtbl/libmtbl.la

TESTS += src/test-trailer
check_PROGRAMS += src/test-trailer
src_test_trailer_SOURCES = src/test-trailer.c
src_test_trailer_LDADD = mtbl/libmtbl.la

TESTS += src/test-varint
check_PROGRAMS += src/test-varint
src_test_varint_SOURCES = src/test-varint.c
src_test_varint_LDADD = mtbl/libmtbl.la

TESTS += src/test-vector
check_PROGRAMS += src/test-vector
src_test_vector_SOURCES = src/test-vector.c
src_test_vector_LDADD = mtbl/libmtbl.la

SUFFIXES = .1.txt .3.txt .7.txt .1 .3 .7

ASCIIDOC_PROCESS = a2x -f manpage --asciidoc-opt="-f man/asciidoc.conf" $<

.1.txt.1:
	$(ASCIIDOC_PROCESS)

.3.txt.3:
	$(ASCIIDOC_PROCESS)

.7.txt.7:
	$(ASCIIDOC_PROCESS)

dist_man_MANS = \
	man/mtbl_dump.1 \
	man/mtbl_info.1 \
	man/mtbl_merge.1 \
	man/mtbl_iter.3 \
	man/mtbl_source.3 \
	man/mtbl_reader.3 \
	man/mtbl_writer.3 \
	man/mtbl_merger.3 \
	man/mtbl_sorter.3 \
	man/mtbl_crc32c.3 \
	man/mtbl_fixed.3 \
	man/mtbl_varint.3 \
	man/mtbl.7

EXTRA_DIST += \
	man/mtbl_dump.1.txt \
	man/mtbl_info.1.txt \
	man/mtbl_merge.1.txt \
	man/mtbl_iter.3.txt \
	man/mtbl_source.3.txt \
	man/mtbl_reader.3.txt \
	man/mtbl_writer.3.txt \
	man/mtbl_merger.3.txt \
	man/mtbl_sorter.3.txt \
	man/mtbl_crc32c.3.txt \
	man/mtbl_fixed.3.txt \
	man/mtbl_varint.3.txt \
	man/mtbl.7.txt
