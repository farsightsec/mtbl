#include <libgen.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <mtbl.h>

#define NAME	"test-fileset-partition"

char * fileset;

static bool
cb(const char *fn)
{
	const char *bn;
	bn = basename((char*)fn);
	return strcmp(bn, "file1.mtbl") == 0;
}

static void
merge_func(void *clos,
	const uint8_t *key, size_t len_key,
	const uint8_t *val0, size_t len_val0,
	const uint8_t *val1, size_t len_val1,
	uint8_t **merged_val, size_t *len_merged_val)
{
}

static int
test1(void)
{
	int ret = 0;
	struct mtbl_fileset *fs;
	struct mtbl_fileset_options *opt;
	struct mtbl_merger *m1, *m2;
	const struct mtbl_source *s1, *s2;
	struct mtbl_iter *itr;
	const uint8_t *key, *val;
	size_t len_key, len_val;

	opt = mtbl_fileset_options_init();
	mtbl_fileset_options_set_merge_func(opt, merge_func, NULL);
	fs = mtbl_fileset_init(fileset, opt);

	mtbl_fileset_partition(fs, cb, &m1, &m2);

	s1 = mtbl_merger_source(m1);
	s2 = mtbl_merger_source(m2);

	itr = mtbl_source_iter(s1);
	if (mtbl_iter_next(itr, &key, &len_key, &val, &len_val) != mtbl_res_success) {
		fprintf (stderr, NAME ": m1 does not have any keys\n");
		ret++;
	} else {
		if (strncmp((const char*)key, "file1", len_key) != 0) {
			fprintf (stderr, NAME ": '%.*s' != 'file1'\n", (int)len_key, (char*)key);
		}
	}
	if (mtbl_iter_next(itr, &key, &len_key, &val, &len_val) == mtbl_res_success) {
		fprintf (stderr, NAME ": m1 has too many keys\n");
		ret++;
	}
	mtbl_iter_destroy(&itr);

	itr = mtbl_source_iter(s2);
	if (mtbl_iter_next(itr, &key, &len_key, &val, &len_val) != mtbl_res_success) {
		fprintf (stderr, NAME ": m2 does not have any keys\n");
		ret++;
	} else {
		if (strncmp((const char*)key, "file2", len_key) != 0) {
			fprintf (stderr, NAME ": '%.*s' != 'file2'\n", (int)len_key, (char*)key);
		}
	}
	if (mtbl_iter_next(itr, &key, &len_key, &val, &len_val) != mtbl_res_success) {
		fprintf (stderr, NAME ": m2 does not have enough keys\n");
		ret++;
	} else {
		if (strncmp((const char*)key, "file3", len_key) != 0) {
			fprintf (stderr, NAME ": '%.*s' != 'file3'\n", (int)len_key, (char*)key);
		}
	}
	if (mtbl_iter_next(itr, &key, &len_key, &val, &len_val) == mtbl_res_success) {
		fprintf (stderr, NAME ": m2 has too many keys\n");
		ret++;
	}
	mtbl_iter_destroy(&itr);

	mtbl_merger_destroy(&m1);
	mtbl_merger_destroy(&m2);
	mtbl_fileset_destroy(&fs);
	mtbl_fileset_options_destroy(&opt);

	return (ret);
}

static int
check(int ret, const char *s)
{
	if (ret == 0)
		fprintf(stderr, NAME ": PASS: %s\n", s);
	else
		fprintf(stderr, NAME ": FAIL: %s\n", s);
	return (ret);
}

int
main(int argc, char **argv)
{
	int ret = 0;
	if (argc < 2) {
		fprintf(stderr, NAME ": ERROR. Need fileset argument.\n");
		return (EXIT_FAILURE);
	}
	fileset = argv[1];

	ret |= check(test1(), "test1");

	if (ret)
		return (EXIT_FAILURE);
	return (EXIT_SUCCESS);
}
