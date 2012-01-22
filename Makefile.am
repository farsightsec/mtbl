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
	mtbl/mtbl.h \
	mtbl/mtbl-private.h

mtbl_libmtbl_la_LDFLAGS = $(AM_LDFLAGS) \
	-version-info $(LIBMTBL_CURRENT):$(LIBMTBL_REVISION):$(LIBMTBL_AGE) \
	-export-symbols-regex "^(mtbl_[a-z].*)"

install-exec-hook:
	rm -f $(DESTDIR)$(libdir)/libmtbl.la

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mtbl/libmtbl.pc
EXTRA_DIST += mtbl/libmtbl.pc.in
CLEANFILES += mtbl/libmtbl.pc

TESTS = src/test-mtbl

check_PROGRAMS = src/test-mtbl
src_test_mtbl_SOURCES = src/test-mtbl.c
src_test_mtbl_LDADD = mtbl/libmtbl.la
