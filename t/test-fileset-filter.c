#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <mtbl.h>

#define NAME	"test-fileset-filter"

char * fileset;

static bool
filename_filter(const char *fn, void *clos) {
	const char *cn = (const char *)clos;
	const char *bn;
	bn = basename((char*)fn);
	return strcmp(bn, cn) == 0;
}

static bool
reader_filter(struct mtbl_reader *r, void *clos) {
	const struct mtbl_source *s;
	struct mtbl_iter *it;
	const uint8_t *key, *val;
	size_t klen, vlen;

	const char *filter = (char *)clos;
	size_t filter_len = strlen(filter);

	s = mtbl_reader_source((struct mtbl_reader *)r);
	it = mtbl_source_iter(s);

	while (mtbl_iter_next(it, &key, &klen, &val, &vlen)) {
		if (strncmp((const char *) val, filter, filter_len) == 0) {
			return true;
		}
	}

	mtbl_iter_destroy(&it);
	return false;
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
check_iter(struct mtbl_iter *itr, char **expected_keys,
		size_t expected_keys_len) {
	mtbl_res res;
	const uint8_t *key, *val;
	size_t len_key, len_val;
	char *expected;
	size_t expected_len;

	for(size_t i = 0; i < expected_keys_len; i++) {
		res = mtbl_iter_next(itr, &key, &len_key, &val, &len_val);
		if (res != mtbl_res_success) {
			fprintf (stderr, NAME ": source does not have enough keys\n");
			return 1;
		}
		expected = expected_keys[i];
		expected_len = strlen(expected);
		if (len_key != expected_len || strncmp((const char*)key, expected,
			expected_len) != 0) {
			fprintf (stderr, NAME ": '%.*s' != '%s'\n", (int)len_key,
				(char*)key, expected);
			return 1;
		}
	}
	// Check for extra keys
	if (mtbl_iter_next(itr, &key, &len_key, &val, &len_val) == mtbl_res_success) {
		fprintf (stderr, NAME ": source has too many keys\n");
		return 1;
	}
	return 0;
}

static int
test1(void)
{
	int ret = 0;
	struct mtbl_fileset *fs;
	struct mtbl_fileset_options *opt;
	const struct mtbl_source *s;
	struct mtbl_iter *itr;
	char *expected_keys[] = {"Blue Whale", "Cougar", "Red-eared Slider"};

	opt = mtbl_fileset_options_init();
	mtbl_fileset_options_set_merge_func(opt, merge_func, NULL);
	mtbl_fileset_options_set_filename_filter_func(opt, filename_filter, "animals-2.mtbl");
	fs = mtbl_fileset_init(fileset, opt);

	s = mtbl_fileset_source(fs);

	itr = mtbl_source_iter(s);

	ret = check_iter(itr, expected_keys, sizeof(expected_keys)/sizeof(*expected_keys));

	mtbl_iter_destroy(&itr);

	mtbl_fileset_options_destroy(&opt);
	mtbl_fileset_destroy(&fs);

	return (ret);
}

static int
test2(void) {
	int ret = 0;
	struct mtbl_fileset *fs;
	struct mtbl_fileset_options *opt;
	const struct mtbl_source *s;
	struct mtbl_iter *itr;
	const char *filter_val = "Vulnerable";
	char *expected_keys[] = {"Emperor Penguin", "Giraffe", "Hawksbill Sea Turtle"};

	opt = mtbl_fileset_options_init();
	mtbl_fileset_options_set_merge_func(opt, merge_func, NULL);
	mtbl_fileset_options_set_reader_filter_func(opt, reader_filter, (void *)filter_val);
	fs = mtbl_fileset_init(fileset, opt);

	s = mtbl_fileset_source(fs);

	itr = mtbl_source_iter(s);

	ret = check_iter(itr, expected_keys, sizeof(expected_keys)/sizeof(*expected_keys));

	mtbl_iter_destroy(&itr);

	mtbl_fileset_options_destroy(&opt);
	mtbl_fileset_destroy(&fs);

	return (ret);
}


static int
test3(void) {
	int ret = 0;
	struct mtbl_fileset *fs;
	struct mtbl_fileset_options *opt;
	const struct mtbl_source *s;
	struct mtbl_iter *itr;
	const uint8_t *key, *val;
	size_t len_key, len_val;
	const char *filter_val = "Vulnerable";

	opt = mtbl_fileset_options_init();
	mtbl_fileset_options_set_merge_func(opt, merge_func, NULL);
	mtbl_fileset_options_set_filename_filter_func(opt, filename_filter, "animals-1.mtbl");
	mtbl_fileset_options_set_reader_filter_func(opt, reader_filter, (void *)filter_val);
	fs = mtbl_fileset_init(fileset, opt);

	s = mtbl_fileset_source(fs);

	itr = mtbl_source_iter(s);

	// Check any extra keys. There should be none.
	if (mtbl_iter_next(itr, &key, &len_key, &val, &len_val) == mtbl_res_success) {
		fprintf (stderr, NAME ": source has too many keys\n");
		ret++;
	}

	mtbl_iter_destroy(&itr);

	mtbl_fileset_options_destroy(&opt);
	mtbl_fileset_destroy(&fs);

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
	ret |= check(test2(), "test2");
	ret |= check(test3(), "test3");

	if (ret)
		return (EXIT_FAILURE);
	return (EXIT_SUCCESS);
}
