/*
 * Copyright (c) 2012-2014 by Farsight Security, Inc.
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

/* OS X compatibility support based on code from Mathias Panzenböck */

#ifdef __MACH__
#include "mach_gettime.h"
#include <mach/mach_time.h>

#define MT_NANO (+1.0E-9)
#define MT_GIGA UINT64_C(1000000000)

static double mt_timebase = 0.0;
static uint64_t mt_timestart = 0;

int clock_gettime(clockid_t clk_id, struct timespec *tp) {
    kern_return_t retval = KERN_SUCCESS;
    if( clk_id == TIMER_ABSTIME) {
        if (!mt_timestart) { 
            mach_timebase_info_data_t tb = { 0 };
            mach_timebase_info(&tb);
            mt_timebase = tb.numer;
            mt_timebase /= tb.denom;
            mt_timestart = mach_absolute_time();
        }

        double diff = (mach_absolute_time() - mt_timestart) * mt_timebase;
        tp->tv_sec = diff * MT_NANO;
        tp->tv_nsec = diff - (tp->tv_sec * MT_GIGA);
    }
    else { 
        clock_serv_t cclock;
        mach_timespec_t mts;

        host_get_clock_service(mach_host_self(), clk_id, &cclock);
        retval = clock_get_time(cclock, &mts);
        mach_port_deallocate(mach_task_self(), cclock);

        tp->tv_sec = mts.tv_sec;
        tp->tv_nsec = mts.tv_nsec;
    }

    return retval;
}
#endif

struct mtbl_fileset_options {
	size_t				reload_interval;
	mtbl_merge_func			merge;
	void				*merge_clos;
};

struct mtbl_fileset {
	uint32_t			reload_interval;
	size_t				n_loaded, n_unloaded;
	struct timespec			last;
	struct my_fileset		*fs;
	struct mtbl_merger		*merger;
	struct mtbl_merger_options	*mopt;
	struct mtbl_source		*source;
};

static struct mtbl_iter *
fileset_source_iter(void *clos)
{
	struct mtbl_fileset *f = (struct mtbl_fileset *) clos;
	mtbl_fileset_reload(f);
	return mtbl_source_iter(mtbl_merger_source(f->merger));
}

static struct mtbl_iter *
fileset_source_get(void *clos, const uint8_t *key, size_t len_key)
{
	struct mtbl_fileset *f = (struct mtbl_fileset *) clos;
	mtbl_fileset_reload(f);
	return mtbl_source_get(mtbl_merger_source(f->merger), key, len_key);
}

static struct mtbl_iter *
fileset_source_get_prefix(void *clos, const uint8_t *key, size_t len_key)
{
	struct mtbl_fileset *f = (struct mtbl_fileset *) clos;
	mtbl_fileset_reload(f);
	return mtbl_source_get_prefix(mtbl_merger_source(f->merger), key, len_key);
}

static struct mtbl_iter *
fileset_source_get_range(void *clos,
			 const uint8_t *key0, size_t len_key0,
			 const uint8_t *key1, size_t len_key1)
{
	struct mtbl_fileset *f = (struct mtbl_fileset *) clos;
	mtbl_fileset_reload(f);
	return mtbl_source_get_range(mtbl_merger_source(f->merger),
				     key0, len_key0, key1, len_key1);
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
mtbl_fileset_options_set_reload_interval(struct mtbl_fileset_options *opt,
					 uint32_t reload_interval)
{
	opt->reload_interval = reload_interval;
}

static void *
fs_load(struct my_fileset *fs, const char *fname)
{
	struct mtbl_fileset *f = (struct mtbl_fileset *) my_fileset_user(fs);
	f->n_loaded++;
	return (mtbl_reader_init(fname, NULL));
}

static void
fs_unload(struct my_fileset *fs, const char *fname, void *ptr)
{
	struct mtbl_fileset *f = (struct mtbl_fileset *) my_fileset_user(fs);
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
	f->reload_interval = opt->reload_interval;
	f->mopt = mtbl_merger_options_init();
	mtbl_merger_options_set_merge_func(f->mopt, opt->merge, opt->merge_clos);
	f->merger = mtbl_merger_init(f->mopt);
	f->fs = my_fileset_init(fname, fs_load, fs_unload, f);
	assert(f->fs != NULL);
	f->source = mtbl_source_init(fileset_source_iter,
				     fileset_source_get,
				     fileset_source_get_prefix,
				     fileset_source_get_range,
				     NULL, f);
	mtbl_fileset_reload(f);
	return (f);
}

void
mtbl_fileset_destroy(struct mtbl_fileset **f)
{
	if (*f) {
		my_fileset_destroy(&(*f)->fs);
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
	while (my_fileset_get(f->fs, i++, &fname, (void **) &reader))
		mtbl_merger_add_source(f->merger, mtbl_reader_source(reader));
}

void
mtbl_fileset_reload(struct mtbl_fileset *f)
{
	assert(f != NULL);
	struct timespec now;
	int res;

	res = clock_gettime(CLOCK_MONOTONIC, &now);
	assert(res == 0);

	if (now.tv_sec - f->last.tv_sec > f->reload_interval) {
		f->n_loaded = 0;
		f->n_unloaded = 0;
		assert(f->fs != NULL);
		my_fileset_reload(f->fs);
		if (f->n_loaded > 0 || f->n_unloaded > 0)
			fs_reinit_merger(f);
		f->last = now;
	}
}

void
mtbl_fileset_reload_now(struct mtbl_fileset *f)
{
	assert(f != NULL);
	struct timespec now;
	int res;

	res = clock_gettime(CLOCK_MONOTONIC, &now);
	assert(res == 0);

	f->n_loaded = 0;
	f->n_unloaded = 0;
	assert(f->fs != NULL);
	my_fileset_reload(f->fs);
	if (f->n_loaded > 0 || f->n_unloaded > 0)
		fs_reinit_merger(f);
	f->last = now;
}
