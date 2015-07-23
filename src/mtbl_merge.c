/*
 * Copyright (c) 2012, 2014-2015 by Farsight Security, Inc.
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

#include <sys/time.h>
#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <locale.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <mtbl.h>

#include "libmy/getenv_int.h"
#include "libmy/ubuf.h"

#define DEFAULT_BLOCK_SIZE	8192
#define STATS_INTERVAL		1000000

static const char		*program_name;

static const char		*mtbl_output_fname;

static mtbl_compression_type	opt_compression_type	= MTBL_COMPRESSION_ZLIB;
static size_t			opt_block_size		= DEFAULT_BLOCK_SIZE;

static const char		*merge_dso_path;
static const char		*merge_dso_prefix;

static mtbl_merge_init_func	user_func_init;
static mtbl_merge_free_func	user_func_free;
static mtbl_merge_func		user_func_merge;
static void			*user_clos;

static struct mtbl_merger	*merger;
static struct mtbl_writer	*writer;

static struct timespec		start_time;
static uint64_t			count;
static uint64_t			count_merged;

static void
usage(void)
{
	fprintf(stderr,
		"Usage: %s [-b <SIZE>] [-c <COMPRESSION>] <INPUT> [<INPUT>...] <OUTPUT>\n"
		"\n"
		"Merges one or more MTBL input files into a single output file.\n"
		"Requires a merge function provided by the user at runtime via a DSO.\n"
		"See mtbl_merge(1) for details.\n"
		"\n"
		"<SIZE> is the uncompressed data block size hint, in bytes.\n"
		"The default value if unspecified is 8192 bytes (8 kilobytes).\n"
		"\n"
		"<COMPRESSION> is one of none, snappy, zlib, lz4, or lz4hc.\n"
		"The default compression type if unspecified is zlib.\n"
		"\n"
		,
		program_name
	);
	exit(EXIT_FAILURE);
}

static void
my_timespec_get(struct timespec *now) {
	struct timeval tv;
	(void) gettimeofday(&tv, NULL);
	now->tv_sec = tv.tv_sec;
	now->tv_nsec = tv.tv_usec * 1000;
}

static void
my_timespec_sub(const struct timespec *a, struct timespec *b) {
	b->tv_sec -= a->tv_sec;
	b->tv_nsec -= a->tv_nsec;
	if (b->tv_nsec < 0) {
		b->tv_sec -= 1;
		b->tv_nsec += 1000000000;
	}
}

static double
my_timespec_to_double(const struct timespec *ts) {
	return (ts->tv_sec + ts->tv_nsec / 1E9);
}

static void
print_stats(void)
{
	struct timespec dur;
	double t_dur;

	my_timespec_get(&dur);
	my_timespec_sub(&start_time, &dur);
	t_dur = my_timespec_to_double(&dur);

	fprintf(stderr,
		"%s: wrote %'" PRIu64 " entries (%'" PRIu64 " merged) "
		"in %'.2f sec, %'d ent/sec, %'d merge/sec\n",
		program_name,
		count,
		count_merged,
		t_dur,
		(int) (count / t_dur),
		(int) (count_merged / t_dur)
	);
}

static void
merge_func(void *clos,
	   const uint8_t *key, size_t len_key,
	   const uint8_t *val0, size_t len_val0,
	   const uint8_t *val1, size_t len_val1,
	   uint8_t **merged_val, size_t *len_merged_val)
{
	user_func_merge(clos,
			key, len_key,
			val0, len_val0,
			val1, len_val1,
			merged_val, len_merged_val);
	count_merged += 1;
}

static void
merge(void)
{
	const uint8_t *key, *val;
	size_t len_key, len_val;
	struct mtbl_iter *it;

	it = mtbl_source_iter(mtbl_merger_source(merger));
	assert(it != NULL);
	while (mtbl_iter_next(it, &key, &len_key, &val, &len_val) == mtbl_res_success) {
		mtbl_res res = mtbl_writer_add(writer, key, len_key, val, len_val);
		assert(res == mtbl_res_success);
		if ((++count % STATS_INTERVAL) == 0)
			print_stats();
	}
	mtbl_iter_destroy(&it);
	mtbl_merger_destroy(&merger);
	mtbl_writer_destroy(&writer);
}

static void
init_dso(void)
{
	merge_dso_path = getenv("MTBL_MERGE_DSO");
	merge_dso_prefix = getenv("MTBL_MERGE_FUNC_PREFIX");

	if (merge_dso_path == NULL) {
		fprintf(stderr, "Error: MTBL_MERGE_DSO environment variable not set.\n\n");
		usage();
	}
	if (merge_dso_prefix == NULL) {
		fprintf(stderr, "Error: MTBL_MERGE_FUNC_PREFIX environment variable not set.\n\n");
		usage();
	}

	dlerror();
	void *handle = dlopen(merge_dso_path, RTLD_NOW);
	if (handle == NULL) {
		fprintf(stderr, "Error: dlopen() failed: %s\n", dlerror());
		exit(EXIT_FAILURE);
	}

	/* merge func */
	ubuf *func_name = ubuf_init(0);
	ubuf_append(func_name,
		    (const uint8_t *) merge_dso_prefix,
		    strlen(merge_dso_prefix));
	ubuf_append(func_name,
		    (const uint8_t *) "_func",
		    sizeof("_func"));
	user_func_merge = dlsym(handle, (const char *) ubuf_data(func_name));
	if (user_func_merge == NULL) {
		fprintf(stderr, "Error: user merge function required but not found in DSO.\n\n");
		usage();
	}

	/* init func */
	ubuf_clip(func_name, 0);
	ubuf_append(func_name,
		    (const uint8_t *) merge_dso_prefix,
		    strlen(merge_dso_prefix));
	ubuf_append(func_name,
		    (const uint8_t *) "_init_func",
		    sizeof("_init_func"));
	user_func_init = dlsym(handle, (const char *) ubuf_data(func_name));
	if (user_func_init != NULL)
		user_clos = user_func_init();

	/* free func */
	ubuf_clip(func_name, 0);
	ubuf_append(func_name,
		    (const uint8_t *) merge_dso_prefix,
		    strlen(merge_dso_prefix));
	ubuf_append(func_name,
		    (const uint8_t *) "_free_func",
		    sizeof("_free_func"));
	user_func_free = dlsym(handle, (const char *) ubuf_data(func_name));

	/* cleanup */
	ubuf_destroy(&func_name);
}

static size_t
get_block_size(void)
{
	uint64_t sz;
	if (getenv_int("MTBL_MERGE_BLOCK_SIZE", &sz))
		return ((size_t) sz);
	return (DEFAULT_BLOCK_SIZE);
}

static void
init_mtbl(void)
{
	struct mtbl_merger_options *mopt;
	struct mtbl_writer_options *wopt;

	mopt = mtbl_merger_options_init();
	wopt = mtbl_writer_options_init();

	mtbl_merger_options_set_merge_func(mopt, merge_func, user_clos);
	mtbl_writer_options_set_compression(wopt, opt_compression_type);
	mtbl_writer_options_set_block_size(wopt, opt_block_size);

	merger = mtbl_merger_init(mopt);
	assert(merger != NULL);

	fprintf(stderr, "%s: opening output file %s\n", program_name, mtbl_output_fname);
	writer = mtbl_writer_init(mtbl_output_fname, wopt);
	if (writer == NULL) {
		fprintf(stderr, "Error: mtbl_writer_init() failed.\n\n");
		usage();
	}

	mtbl_merger_options_destroy(&mopt);
	mtbl_writer_options_destroy(&wopt);
}

static bool
parse_long(const char *str, long int *val)
{
	char *endptr;

	errno = 0;
	*val = strtol(str, &endptr, 0);

	if ((errno == ERANGE && (*val == LONG_MAX || *val == LONG_MIN))
	    || (errno != 0 && *val == 0))
	{
		return false;
	}

	if (endptr == str || *endptr != '\0')
		return false;

	return true;
}

static bool
parse_arg_block_size(const char *arg)
{
	long int arg_block_size = 0;

	if (getenv("MTBL_MERGE_BLOCK_SIZE") != NULL) {
		fprintf(stderr,
			"%s: Error: specify block size via command-line or environment, not both\n",
			program_name);
		return false;
	}

	if (!parse_long(arg, &arg_block_size))
		return false;

	opt_block_size = arg_block_size;
	return true;
}

static bool
parse_arg_compression(const char *arg)
{
	mtbl_res res;

	res = mtbl_compression_type_from_str(arg, &opt_compression_type);
	return res == mtbl_res_success;
}

int
main(int argc, char **argv)
{
	setlocale(LC_ALL, "");
	program_name = argv[0];

	opt_block_size = get_block_size();

	int c;
	while ((c = getopt(argc, argv, "b:c:")) != -1) {
		switch (c) {
		case 'b':
			if (!parse_arg_block_size(optarg))
				usage();
			break;
		case 'c':
			if (!parse_arg_compression(optarg))
				usage();
			break;
		default:
			usage();
		}
	}

	if (argc - optind < 2)
		usage();
	mtbl_output_fname = argv[argc - 1];

	/* open user dso */
	init_dso();

	/* open merger, writer */
	init_mtbl();

	/* open readers */
	const size_t n_readers = argc - 1 - optind;
	struct mtbl_reader *readers[n_readers];
	for (size_t i = 0; i < n_readers; i++) {
		const char *fname = argv[i + optind];
		fprintf(stderr, "%s: opening input file %s\n", program_name, fname);
		readers[i] = mtbl_reader_init(fname, NULL);
		if (readers[i] == NULL) {
			fprintf(stderr, "Error: mtbl_reader_init() failed.\n\n");
			usage();
		}
		mtbl_merger_add_source(merger, mtbl_reader_source(readers[i]));
	}

	/* do merge */
	my_timespec_get(&start_time);
	merge();

	/* cleanup readers */
	for (size_t i = 0; i < n_readers; i++)
		mtbl_reader_destroy(&readers[i]);

	/* call user cleanup */
	if (user_func_free != NULL)
		user_func_free(user_clos);

	print_stats();

	return (EXIT_SUCCESS);
}
