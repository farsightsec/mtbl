/*
 * Copyright (c) 2012 by Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <time.h>

#include "mtbl-private.h"

#include "librsf/rsf_fileset.h"

struct mtbl_fileset_options {
	size_t				reload_frequency;
	mtbl_merge_func			merge;
	void				*merge_clos;
};

struct mtbl_fileset {
	uint32_t			reload_frequency;
	size_t				n_loaded, n_unloaded;
	struct timespec			last;
	struct rsf_fileset		*fs;
	struct mtbl_merger		*merger;
	struct mtbl_merger_options	*mopt;
	struct mtbl_source		*source;
};

static void fileset_check_reload(struct mtbl_fileset *f);

static struct mtbl_iter *
fileset_source_iter(void *clos)
{
	struct mtbl_fileset *f = (struct mtbl_fileset *) clos;
	fileset_check_reload(f);
	return mtbl_source_iter(mtbl_merger_source(f->merger));
}

static struct mtbl_iter *
fileset_source_get(void *clos, const uint8_t *key, size_t len_key)
{
	struct mtbl_fileset *f = (struct mtbl_fileset *) clos;
	fileset_check_reload(f);
	return mtbl_source_get(mtbl_merger_source(f->merger), key, len_key);
}

static struct mtbl_iter *
fileset_source_get_prefix(void *clos, const uint8_t *key, size_t len_key)
{
	struct mtbl_fileset *f = (struct mtbl_fileset *) clos;
	fileset_check_reload(f);
	return mtbl_source_get_prefix(mtbl_merger_source(f->merger), key, len_key);
}

static struct mtbl_iter *
fileset_source_get_range(void *clos,
			 const uint8_t *key0, size_t len_key0,
			 const uint8_t *key1, size_t len_key1)
{
	struct mtbl_fileset *f = (struct mtbl_fileset *) clos;
	fileset_check_reload(f);
	return mtbl_source_get_range(mtbl_merger_source(f->merger),
				     key0, len_key0, key1, len_key1);
}

struct mtbl_fileset_options *
mtbl_fileset_options_init(void)
{
	struct mtbl_fileset_options *opt;
	opt = my_calloc(1, sizeof(*opt));
	opt->reload_frequency = DEFAULT_FILESET_RELOAD_FREQ;
	return (opt);
}

void
mtbl_fileset_options_destroy(struct mtbl_fileset_options **opt)
{
	if (*opt) {
		free(*opt);
		*opt = NULL;
	}
}

void
mtbl_fileset_options_set_merge_func(struct mtbl_fileset_options *opt,
				    mtbl_merge_func merge, void *clos)
{
	opt->merge = merge;
	opt->merge_clos = clos;
}

void
mtbl_fileset_options_set_reload_frequency(struct mtbl_fileset_options *opt,
					  uint32_t reload_frequency)
{
	opt->reload_frequency = reload_frequency;
}

static void *
fs_load(struct rsf_fileset *fs, const char *fname)
{
	struct mtbl_fileset *f = (struct mtbl_fileset *) rsf_fileset_user(fs);
	f->n_loaded++;
	return (mtbl_reader_init(fname, NULL));
}

static void
fs_unload(struct rsf_fileset *fs, const char *fname, void *ptr)
{
	struct mtbl_fileset *f = (struct mtbl_fileset *) rsf_fileset_user(fs);
	struct mtbl_reader *r = (struct mtbl_reader *) ptr;
	f->n_unloaded++;
	mtbl_reader_destroy(&r);
}

struct mtbl_fileset *
mtbl_fileset_init(const char *fname, const struct mtbl_fileset_options *opt)
{
	assert(opt != NULL);
	assert(opt->merge != NULL);
	struct mtbl_fileset *f = my_calloc(1, sizeof(*f));
	f->reload_frequency = opt->reload_frequency;
	f->mopt = mtbl_merger_options_init();
	mtbl_merger_options_set_merge_func(f->mopt, opt->merge, opt->merge_clos);
	f->merger = mtbl_merger_init(f->mopt);
	f->fs = rsf_fileset_init(fname, fs_load, fs_unload, f);
	assert(f->fs != NULL);
	f->source = mtbl_source_init(fileset_source_iter,
				     fileset_source_get,
				     fileset_source_get_prefix,
				     fileset_source_get_range,
				     NULL, f);
	fileset_check_reload(f);
	return (f);
}

void
mtbl_fileset_destroy(struct mtbl_fileset **f)
{
	if (*f) {
		rsf_fileset_destroy(&(*f)->fs);
		mtbl_merger_destroy(&(*f)->merger);
		mtbl_merger_options_destroy(&(*f)->mopt);
		mtbl_source_destroy(&(*f)->source);
		free(*f);
		*f = NULL;
	}
}

const struct mtbl_source *
mtbl_fileset_source(struct mtbl_fileset *f)
{
	assert(f->merger != NULL);
	return mtbl_merger_source(f->merger);
}

static void
fs_reinit_merger(struct mtbl_fileset *f)
{
	const char *fname;
	struct mtbl_reader *reader;
	size_t i = 0;

	if (f->merger) {
		mtbl_merger_destroy(&f->merger);
		f->merger = mtbl_merger_init(f->mopt);
	}
	while (rsf_fileset_get(f->fs, i++, &fname, (void **) &reader))
		mtbl_merger_add_source(f->merger, mtbl_reader_source(reader));
}

static void
fileset_check_reload(struct mtbl_fileset *f)
{
	struct timespec now;
	int res;

	res = clock_gettime(CLOCK_MONOTONIC, &now);
	assert(res == 0);

	if (now.tv_sec - f->last.tv_sec > f->reload_frequency) {
		f->n_loaded = 0;
		f->n_unloaded = 0;
		rsf_fileset_reload(f->fs);
		if (f->n_loaded > 0 || f->n_unloaded > 0)
			fs_reinit_merger(f);
		f->last = now;
	}
}
