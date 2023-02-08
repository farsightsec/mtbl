/*
 * Copyright (c) 2022 DomainTools LLC
 * Copyright (c) 2012-2019 by Farsight Security, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <time.h>

#include "mtbl-private.h"

#include "libmy/my_fileset.h"
#include "libmy/my_time.h"

struct mtbl_fileset_options {
	uint32_t			reload_interval;
	mtbl_merge_func			merge;
	void				*merge_clos;
	mtbl_dupsort_func		dupsort;
	void				*dupsort_clos;
	mtbl_filename_filter_func	fname_filter;
	void				*fname_filter_clos;
	mtbl_reader_filter_func		reader_filter;
	void				*reader_filter_clos;
};

struct shared_fileset {
	size_t 				n_loaded, n_unloaded, n_fs, n_iters;
	bool				reload_needed;
	struct timespec			fs_last;
	struct my_fileset		*my_fs;
};


struct mtbl_fileset {
	uint32_t			reload_interval;
	struct shared_fileset		*shared_fs;
	struct timespec			fs_last;
	struct mtbl_merger		*merger;
	struct mtbl_merger_options	*mopt;
	struct mtbl_source		*source;
	mtbl_filename_filter_func	fname_filter;
	void				*fname_filter_clos;
	mtbl_reader_filter_func		reader_filter;
	void				*reader_filter_clos;
};

struct fileset_iter {
	struct mtbl_fileset *fs;
	struct mtbl_iter *iter;
};

static mtbl_res
fileset_iter_seek(void *v, const uint8_t *key, size_t len)
{
	struct fileset_iter *it = (struct fileset_iter *)v;
	return mtbl_iter_seek(it->iter, key, len);
}

static mtbl_res
fileset_iter_next(void *v,
		const uint8_t **key, size_t *len_key,
		const uint8_t **val, size_t *len_val)
{
	struct fileset_iter *it = (struct fileset_iter *)v;
	return mtbl_iter_next(it->iter, key, len_key, val, len_val);
}

static void
fileset_iter_free(void *v)
{
	struct fileset_iter *it = (struct fileset_iter *)v;
	if (it) {
		it->fs->shared_fs->n_iters--;
		mtbl_iter_destroy(&it->iter);
		mtbl_fileset_reload(it->fs);
		free(it);
	}
}

static struct mtbl_iter *
fileset_iter_init(struct mtbl_fileset *f, struct mtbl_iter *mit)
{
	struct fileset_iter *it = my_calloc(1, sizeof(*it));
	f->shared_fs->n_iters++;
	it->iter = mit;
	it->fs = f;
	return mtbl_iter_init(fileset_iter_seek,
				fileset_iter_next,
				fileset_iter_free,
				it);
}


static struct mtbl_iter *
fileset_source_iter(void *clos)
{
	struct mtbl_fileset *f = (struct mtbl_fileset *) clos;
	mtbl_fileset_reload(f);
	return fileset_iter_init(f,
			mtbl_source_iter(mtbl_merger_source(f->merger)));
}

static struct mtbl_iter *
fileset_source_get(void *clos, const uint8_t *key, size_t len_key)
{
	struct mtbl_fileset *f = (struct mtbl_fileset *) clos;
	mtbl_fileset_reload(f);
	return fileset_iter_init(f,
			mtbl_source_get(mtbl_merger_source(f->merger),
					key, len_key));
}

static struct mtbl_iter *
fileset_source_get_prefix(void *clos, const uint8_t *key, size_t len_key)
{
	struct mtbl_fileset *f = (struct mtbl_fileset *) clos;
	mtbl_fileset_reload(f);
	return fileset_iter_init(f,
			mtbl_source_get_prefix(mtbl_merger_source(f->merger),
						key, len_key));
}

static struct mtbl_iter *
fileset_source_get_range(void *clos,
			 const uint8_t *key0, size_t len_key0,
			 const uint8_t *key1, size_t len_key1)
{
	struct mtbl_fileset *f = (struct mtbl_fileset *) clos;
	mtbl_fileset_reload(f);
	return fileset_iter_init(f,
			mtbl_source_get_range(mtbl_merger_source(f->merger),
				     key0, len_key0, key1, len_key1));
}

struct mtbl_fileset_options *
mtbl_fileset_options_init(void)
{
	struct mtbl_fileset_options *opt;
	opt = my_calloc(1, sizeof(*opt));
	opt->reload_interval = DEFAULT_FILESET_RELOAD_INTERVAL;
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
mtbl_fileset_options_set_dupsort_func(struct mtbl_fileset_options *opt,
				    mtbl_dupsort_func dupsort, void *clos)
{
	opt->dupsort = dupsort;
	opt->dupsort_clos = clos;
}

void
mtbl_fileset_options_set_filename_filter_func(struct mtbl_fileset_options *opt,
				    mtbl_filename_filter_func fname_filter, void *clos)
{
	opt->fname_filter = fname_filter;
	opt->fname_filter_clos = clos;
}

void
mtbl_fileset_options_set_reader_filter_func(struct mtbl_fileset_options *opt,
					mtbl_reader_filter_func reader_filter, void *clos)
{
	opt->reader_filter = reader_filter;
	opt->reader_filter_clos = clos;
}

void
mtbl_fileset_options_set_reload_interval(struct mtbl_fileset_options *opt,
					 uint32_t reload_interval)
{
	opt->reload_interval = reload_interval;
}

static void *
fs_load(struct my_fileset *fs, const char *fname)
{
	struct shared_fileset *f = (struct shared_fileset *) my_fileset_user(fs);
	f->n_loaded++;
	return (mtbl_reader_init(fname, NULL));
}

static void
fs_unload(struct my_fileset *fs, const char *fname, void *ptr)
{
	struct shared_fileset *f = (struct shared_fileset *) my_fileset_user(fs);
	struct mtbl_reader *r = (struct mtbl_reader *) ptr;
	f->n_unloaded++;
	mtbl_reader_destroy(&r);
}

static void
mtbl_fileset_set_options(struct mtbl_fileset *f, const struct mtbl_fileset_options *opt)
{
	assert(opt != NULL);

	f->reload_interval = opt->reload_interval;
	f->mopt = mtbl_merger_options_init();
	mtbl_merger_options_set_merge_func(f->mopt, opt->merge, opt->merge_clos);
	mtbl_merger_options_set_dupsort_func(f->mopt, opt->dupsort, opt->dupsort_clos);
	f->fname_filter = opt->fname_filter;
	f->fname_filter_clos = opt->fname_filter_clos;
	f->reader_filter = opt->reader_filter;
	f->reader_filter_clos = opt->reader_filter_clos;
	f->merger = mtbl_merger_init(f->mopt);
	f->source = mtbl_source_init(fileset_source_iter,
				     fileset_source_get,
				     fileset_source_get_prefix,
				     fileset_source_get_range,
				     NULL, f);
}

struct mtbl_fileset *
mtbl_fileset_init(const char *fname, const struct mtbl_fileset_options *opt)
{
	struct mtbl_fileset *f = my_calloc(1, sizeof(*f));

	f->shared_fs = my_calloc(1, sizeof(*(f->shared_fs)));
	f->shared_fs->n_fs = 1;
	f->shared_fs->reload_needed = true;
	f->shared_fs->my_fs = my_fileset_init(fname, fs_load, fs_unload, f->shared_fs);
	assert(f->shared_fs->my_fs != NULL);

	mtbl_fileset_set_options(f, opt);

	return (f);
}

struct mtbl_fileset *
mtbl_fileset_dup(struct mtbl_fileset *orig, const struct mtbl_fileset_options *opt)
{
	struct mtbl_fileset *f = my_calloc(1, sizeof(*f));

	f->shared_fs = orig->shared_fs;
	f->shared_fs->n_fs++;

	mtbl_fileset_set_options(f, opt);

	return (f);
}

void
mtbl_fileset_destroy(struct mtbl_fileset **f)
{
	if (*f) {
		if (--((*f)->shared_fs->n_fs) <= 0) {
			my_fileset_destroy(&(*f)->shared_fs->my_fs);
			free((*f)->shared_fs);
		}

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
	assert(f != NULL);
	assert(f->source != NULL);
	return (f->source);
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
	assert(f->merger != NULL);
	while (my_fileset_get(f->shared_fs->my_fs, i++, &fname, (void **) &reader)) {
		if (reader == NULL) {
			continue;
		}

		/*
		* Add the reader's source to the merger unless filtered out by the
		* fname_filter or reader_filter.
		*/
		if (
			(f->fname_filter == NULL
				|| f->fname_filter(fname, f->fname_filter_clos))
			&&
			(f->reader_filter == NULL
				|| f->reader_filter(reader, f->reader_filter_clos))
		   ) {
			mtbl_merger_add_source(f->merger, mtbl_reader_source(reader));
		}
	}
}

void
mtbl_fileset_reload(struct mtbl_fileset *f)
{
	assert(f != NULL);
	struct timespec now;

	/* if our merger is from an out of date fileset, reinitialize it. */
	if ((f->fs_last.tv_sec != f->shared_fs->fs_last.tv_sec) ||
	    (f->fs_last.tv_nsec != f->shared_fs->fs_last.tv_nsec)) {
		fs_reinit_merger(f);
		f->fs_last = f->shared_fs->fs_last;
	}

	/* if we loaded at least once and we are configured to *not* reload then do not reload */
	if (!f->shared_fs->reload_needed && f->reload_interval == MTBL_FILESET_RELOAD_INTERVAL_NEVER)
		return;

	/* if there are any open iterators under this fileset, do not reload now */
	if (f->shared_fs->n_iters > 0)
		return;

#if HAVE_CLOCK_GETTIME
	static const clockid_t clock = CLOCK_MONOTONIC;
#else
	static const int clock = -1;
#endif
	my_gettime(clock, &now);

	if (f->shared_fs->reload_needed || (now.tv_sec - f->shared_fs->fs_last.tv_sec > f->reload_interval)) {
		f->shared_fs->n_loaded = 0;
		f->shared_fs->n_unloaded = 0;
		assert(f->shared_fs->my_fs != NULL);
		my_fileset_reload(f->shared_fs->my_fs);
		if (f->shared_fs->n_loaded > 0 || f->shared_fs->n_unloaded > 0)
			fs_reinit_merger(f);
		f->shared_fs->fs_last = now;
		f->fs_last = now;
		f->shared_fs->reload_needed = false;
	}
}

void
mtbl_fileset_reload_now(struct mtbl_fileset *f)
{
	assert(f != NULL);

	struct timespec now;

	/*
	 * if there are any open iterators under this fileset,
	 * do not reload now
	 */
	if (f->shared_fs->n_iters > 0) {
		f->shared_fs->reload_needed = true;
		return;
	}

#if HAVE_CLOCK_GETTIME
	static const clockid_t clock = CLOCK_MONOTONIC;
#else
	static const int clock = -1;
#endif
	my_gettime(clock, &now);

	f->shared_fs->n_loaded = 0;
	f->shared_fs->n_unloaded = 0;
	assert(f->shared_fs->my_fs != NULL);
	my_fileset_reload(f->shared_fs->my_fs);
	if (f->shared_fs->n_loaded > 0 || f->shared_fs->n_unloaded > 0)
		fs_reinit_merger(f);
	f->shared_fs->fs_last = now;
	f->fs_last = now;
	f->shared_fs->reload_needed = false;
}

void
mtbl_fileset_partition(struct mtbl_fileset *f,
		mtbl_filename_filter_func cb,
		void *clos,
		struct mtbl_merger **m1,
		struct mtbl_merger **m2)
{
	const char *fname;
	struct mtbl_reader *reader;
	size_t i = 0;

	mtbl_fileset_reload(f);	/* open the fileset file if not already done */

	*m1 = mtbl_merger_init(f->mopt);
	*m2 = mtbl_merger_init(f->mopt);

	while (my_fileset_get(f->shared_fs->my_fs, i++, &fname, (void**) &reader)) {
		if (cb(fname, clos))
			mtbl_merger_add_source(*m1, mtbl_reader_source(reader));
		else
			mtbl_merger_add_source(*m2, mtbl_reader_source(reader));
	}
}
