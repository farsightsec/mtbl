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
	mtbl/block_builder.c \
	mtbl/crc32c.c \
	mtbl/fixed.c \
	mtbl/memtable.c \
	mtbl/mtbl.h \
	mtbl/mtbl-private.h \
	mtbl/trailer.c \
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

TESTS = \
	src/test-memtable \
	src/test-mtbl \
	src/test-crc32c \
	src/test-fixed \
	src/test-trailer \
	src/test-varint

check_PROGRAMS = \
	src/test-memtable \
	src/test-mtbl \
	src/test-block_builder \
	src/test-crc32c \
	src/test-fixed \
	src/test-trailer \
	src/test-varint \
	src/test-vector \
	src/test-writer

src_test_mtbl_SOURCES = src/test-mtbl.c
src_test_mtbl_LDADD = mtbl/libmtbl.la
src_test_block_builder_SOURCES = src/test-block_builder.c
src_test_block_builder_LDADD = mtbl/libmtbl.la
src_test_crc32c_SOURCES = src/test-crc32c.c
src_test_crc32c_LDADD = mtbl/libmtbl.la
src_test_fixed_SOURCES = src/test-fixed.c
src_test_fixed_LDADD = mtbl/libmtbl.la
src_test_trailer_SOURCES = src/test-trailer.c
src_test_trailer_LDADD = mtbl/libmtbl.la
src_test_varint_SOURCES = src/test-varint.c
src_test_varint_LDADD = mtbl/libmtbl.la
src_test_vector_SOURCES = src/test-vector.c
src_test_vector_LDADD = mtbl/libmtbl.la
src_test_memtable_SOURCES = src/test-memtable.c
src_test_memtable_LDADD = mtbl/libmtbl.la
