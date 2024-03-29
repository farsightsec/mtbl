#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <mtbl.h>

#include "libmy/ubuf.h"
#include "mtbl-private.h"

#define NAME		"test-iter-seek"

#define NUM_KEYS	128
#define NUM_FILES	8
#define RESTART_INTERVAL	4

#define KEY_FMT		"%08x"
#define VAL_FMT		"%032d"
#define TEST2_LABEL_FMT		"test 2 jump %02d"
#define TEST2_LABEL_LEN		15

static void
init_mtbl(int fd, uint32_t num_keys, size_t block_size,
	size_t restart_interval);

static int
seek_and_check(struct mtbl_iter *, int32_t initial, int32_t target);

static int
test_iter(struct mtbl_iter *);

static void
my_merge_func(void *clos,
        const uint8_t *key, size_t len_key,
        const uint8_t *val0, size_t len_val0,
        const uint8_t *val1, size_t len_val1,
        uint8_t **merged_val, size_t *len_merged_val);

static int
test1(FILE *tmps[NUM_FILES]) {
	int i;
	struct mtbl_reader *readers[NUM_FILES];
	struct mtbl_reader_options *reader_options = mtbl_reader_options_init();
	assert(reader_options != NULL);

	for (i = 0; i < NUM_FILES; i++) {
		readers[i] = mtbl_reader_init_fd(dup(fileno(tmps[i])), reader_options);
		assert(readers[i] != NULL);
	}

	struct mtbl_iter *iter = mtbl_source_iter(mtbl_reader_source(readers[0]));
	assert(iter != NULL);

	if (test_iter(iter) != 0) {
		return 1;
	}
	fprintf(stderr, NAME ": PASS: iter run successful\n");

	mtbl_iter_destroy(&iter);

	struct mtbl_merger_options *merger_options = mtbl_merger_options_init();
	assert(merger_options != NULL);

	mtbl_merger_options_set_merge_func(merger_options, my_merge_func, NULL);

	struct mtbl_merger *merger = mtbl_merger_init(merger_options);
	assert(merger != NULL);

	mtbl_merger_add_source(merger, mtbl_reader_source(readers[0]));

	const struct mtbl_source *merger_source = mtbl_merger_source(merger);
	assert(merger_source != NULL);

	struct mtbl_iter *merger_iter = mtbl_source_iter(merger_source);
	assert(merger_iter != NULL);

	if (test_iter(merger_iter) != 0) {
		return 1;
	}
	fprintf(stderr, NAME ": PASS: merger run 1 successful\n");

	mtbl_iter_destroy(&merger_iter);

	for (i = 1; i < NUM_FILES; i++)
		mtbl_merger_add_source(merger, mtbl_reader_source(readers[i]));

	merger_iter = mtbl_source_iter(merger_source);
	assert(merger_iter != NULL);

	if (test_iter(merger_iter) != 0) {
		return 1;
	}
	fprintf(stderr, NAME ": PASS: merger run 2 successful\n");

	mtbl_iter_destroy(&merger_iter);

	for (i = 0; i < NUM_FILES; i++) {
		mtbl_reader_destroy(&readers[i]);
	}

	mtbl_merger_destroy(&merger);
	mtbl_merger_options_destroy(&merger_options);
	mtbl_reader_options_destroy(&reader_options);
	return 0;
}

static int
test2(FILE *tmp, int32_t num_keys, int32_t jump) {
	struct mtbl_reader_options *reader_options = mtbl_reader_options_init();

	struct mtbl_reader *reader = mtbl_reader_init_fd(dup(fileno(tmp)),
		reader_options);
	mtbl_reader_options_destroy(&reader_options);

	const struct mtbl_source *source = mtbl_reader_source(reader);

	struct mtbl_iter *iter = mtbl_source_iter(source);

	int32_t last_number = num_keys - 1;

	/* Loop forward from 0 and seek i + jump keys */
	for (int32_t i = 0; i < num_keys; i++) {
		if ((i + jump) < last_number) {
			int32_t target_number = i + jump;
			if (seek_and_check(iter, i, target_number) != 0) {
				return 1;
			};
		}
	}

	/* Loop backward from the last key and seek i - jump keys */
	for (int32_t i = last_number; i > 0; i--) {
		if ((i - jump) >= 0) {
			int32_t target_number = i - jump;
			if (seek_and_check(iter, i, target_number) != 0) {
				return 1;
			};
		}
	}

	mtbl_iter_destroy(&iter);
	mtbl_reader_destroy(&reader);
	return 0;
}


static int
test3(FILE *tmp, int32_t num_keys) {
	struct mtbl_reader_options *reader_options = mtbl_reader_options_init();

	struct mtbl_reader *reader = mtbl_reader_init_fd(dup(fileno(tmp)),
		reader_options);
	mtbl_reader_options_destroy(&reader_options);

	const struct mtbl_source *source = mtbl_reader_source(reader);

	struct mtbl_iter *iter = mtbl_source_iter(source);

	int32_t last_number = num_keys - 1;

	for (int32_t i = 0; i < num_keys; i++) {
		/* Test by seeking to the ith number and then to each key ahead
		 * of it until the very end of the data */
		for (int32_t j = i; j < num_keys; j++) {
			if (seek_and_check(iter, i, j) != 0) {
				return 1;
			};
		}
	}

	for (int32_t i = last_number; i > 0; i--) {
		/* Test by seeking to the ith number and then to each key before it
		 * until the very start of the data */
		for (int32_t j = i; j >= 0; j--) {
			if (seek_and_check(iter, i, j) != 0) {
				return 1;
			};
		}
	}

	mtbl_iter_destroy(&iter);
	mtbl_reader_destroy(&reader);
	return 0;
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

int main(int argc, char ** argv) {
	int i, ret = 0;
	FILE *tmps[NUM_FILES];

	for (i = 0; i < NUM_FILES; i++) {
		tmps[i] = tmpfile();
		init_mtbl(dup(fileno(tmps[i])), NUM_KEYS, 1024,
				DEFAULT_BLOCK_RESTART_INTERVAL);
	}

	ret |= check(test1(tmps), "test 1");

	for (i = 0; i < NUM_FILES; i++) {
		fclose(tmps[i]);
	}

	/* Make a larger tmp mtbl so we have larger restart arrays */
	int32_t test_2_num_keys = NUM_KEYS * 100;
	FILE *tmp3 = tmpfile();
	assert(tmp3 != NULL);
	assert(test_2_num_keys > 0);
	init_mtbl(dup(fileno(tmp3)), (uint32_t)test_2_num_keys, DEFAULT_BLOCK_SIZE,
		RESTART_INTERVAL);

	/* From each key, test seeking from 0 to RESTART_INTERVAL keys
	 * forward and backward */
	char test2_label[TEST2_LABEL_LEN];
	for (i = 0; i < RESTART_INTERVAL; i++) {
		snprintf(test2_label, TEST2_LABEL_LEN, TEST2_LABEL_FMT,
			RESTART_INTERVAL - i);
		ret |= check(test2(tmp3, test_2_num_keys, RESTART_INTERVAL - i),
			test2_label);
	}

	/* From each key, test seeking 2 and 3 restart points away forward and
	 * backward */
	snprintf(test2_label, TEST2_LABEL_LEN, TEST2_LABEL_FMT, RESTART_INTERVAL * 2);
	ret |= check(test2(tmp3, test_2_num_keys, RESTART_INTERVAL * 2),
		test2_label);
	snprintf(test2_label, TEST2_LABEL_LEN, TEST2_LABEL_FMT, RESTART_INTERVAL * 3);
	ret |= check(test2(tmp3, test_2_num_keys, RESTART_INTERVAL * 3),
		test2_label);
	fclose(tmp3);

	FILE *tmp4 = tmpfile();
	assert(tmp4 != NULL);
	/* Make a mtbl with a restart interval of 1 so that every key will be in
	 *the restart array */
	init_mtbl(dup(fileno(tmp4)), NUM_KEYS, DEFAULT_BLOCK_SIZE, 1);
	ret |= check(test3(tmp4, NUM_KEYS), "test 3");
	fclose(tmp4);

	if (ret)
		return (EXIT_FAILURE);
	return (EXIT_SUCCESS);
}

static int
seek_and_check(struct mtbl_iter *iter, int32_t initial, int32_t target) {
	/* Seek to the initial key first */
	ubuf *initial_key = ubuf_init(1);
	ubuf_add_fmt(initial_key, KEY_FMT, initial);
	if (mtbl_iter_seek(iter, ubuf_data(initial_key),
		ubuf_size(initial_key)) != mtbl_res_success) {
		return 1;
	}

	/* Seek to the target key */
	ubuf *expected_key = ubuf_init(1);
	ubuf_add_fmt(expected_key, KEY_FMT, target);
	if (mtbl_iter_seek(iter, ubuf_data(expected_key),
		ubuf_size(expected_key)) != mtbl_res_success) {
		return 1;
	}

	/* Check that the next key matches the target key */
	const uint8_t *key, *value;
	size_t len_key, len_value;
	if (mtbl_iter_next(iter, &key, &len_key, &value,
		&len_value) != mtbl_res_success) {
		return 1;
	}
	if (bytes_compare(ubuf_data(expected_key), ubuf_size(expected_key), key,
		len_key) != 0) {
		fprintf(stderr, NAME ": Expecting: %.*s | Got: %.*s\n",
			(int)ubuf_size(expected_key), ubuf_data(expected_key),
				(int)len_key, key);
		return 1;
	}

	ubuf_destroy(&initial_key);
	ubuf_destroy(&expected_key);
	return 0;
}

static int
test_iter(struct mtbl_iter *iter)
{
	/* Iterate completely through the mtbl */
	for (uint32_t i = 0; i < NUM_KEYS; i++) {
		ubuf *expected_key = ubuf_init(1);
		ubuf_add_fmt(expected_key, KEY_FMT, i);

		const uint8_t *key, *value;
		size_t len_key, len_value;

		if (mtbl_iter_next(iter, &key, &len_key, &value, &len_value) != mtbl_res_success) {
			return 1;
		}

		if (bytes_compare(ubuf_data(expected_key), ubuf_size(expected_key), key, len_key) != 0) {
			return 1;
		}

		ubuf_destroy(&expected_key);
	}

	/* Ensure that we have completely iterated through the set. */
	{
		const uint8_t *key, *value;
		size_t len_key, len_value;

		if (mtbl_iter_next(iter, &key, &len_key, &value, &len_value) != mtbl_res_failure) {
			return 1;
		}
	}

	/* Loop forward from 0 and seek i + jump keys */
	int jump = NUM_KEYS/8;
	for (int32_t i = 0; i < NUM_KEYS; i += jump) {
		if ((i + 4) < jump) {
			int32_t target_number = i + jump;
			if (seek_and_check(iter, i, target_number) != 0) {
				return 1;
			};
		}
	}

	/* Seek to a key, ensure that we get the one we want and that we go
	 * all the way to the end. */
	for (uint32_t i = NUM_KEYS; i-- > 0; ) {
		ubuf *seek_key = ubuf_init(1);
		ubuf_add_fmt(seek_key, KEY_FMT, i);

		const uint8_t *key, *value;
		size_t len_key, len_value;

		if (mtbl_iter_seek(iter, ubuf_data(seek_key), ubuf_size(seek_key)) != mtbl_res_success) {
			return 1;
		}

		for (uint32_t j = i; j < NUM_KEYS; j++) {
			ubuf *expected_key = ubuf_init(1);
			ubuf_add_fmt(expected_key, KEY_FMT, j);

			if (mtbl_iter_next(iter, &key, &len_key, &value, &len_value) != mtbl_res_success) {
				return 1;
			}

			if (bytes_compare(ubuf_data(expected_key), ubuf_size(expected_key), key, len_key) != 0) {
				return 1;
			}

			ubuf_destroy(&expected_key);
		}

		if (mtbl_iter_next(iter, &key, &len_key, &value, &len_value) != mtbl_res_failure) {
			return 1;
		}

		ubuf_destroy(&seek_key);
	}

	/* Attempt to seek past end of iterator */
	ubuf *seek_key = ubuf_init(1);
	const uint8_t *key, *value;
	size_t len_key, len_value;

	ubuf_add_fmt(seek_key, KEY_FMT, NUM_KEYS + 1);
	if (mtbl_iter_seek(iter, ubuf_data(seek_key), ubuf_size(seek_key)) != mtbl_res_success) {
		return 1;
	}
	if (mtbl_iter_next(iter, &key, &len_key, &value, &len_value) != mtbl_res_failure) {
		return 1;
	}
	ubuf_destroy(&seek_key);
	return 0;
}

static void
my_merge_func(void *clos,
        const uint8_t *key, size_t len_key,
        const uint8_t *val0, size_t len_val0,
        const uint8_t *val1, size_t len_val1,
        uint8_t **merged_val, size_t *len_merged_val)
{
	assert(len_val0 > 0);
	*merged_val = my_calloc(1, len_val0);
	memcpy(*merged_val, val0, len_val0);
	*len_merged_val = len_val0;
}

static void
init_mtbl(int fd, uint32_t num_keys, size_t block_size, size_t restart_interval) {
	struct mtbl_writer_options *writer_options = mtbl_writer_options_init();
	assert(writer_options != NULL);

	mtbl_writer_options_set_block_size(writer_options, block_size);
	mtbl_writer_options_set_block_restart_interval(writer_options, restart_interval);

	struct mtbl_writer *writer = mtbl_writer_init_fd(fd, writer_options);
	assert(writer != NULL);

	/* Populate the mtbl with hex(i)->i */
	for (uint32_t i = 0; i < num_keys; i++) {
		ubuf *key = ubuf_init(1);
		ubuf *value = ubuf_init(1);

		ubuf_add_fmt(key, KEY_FMT, i);
		ubuf_add_fmt(value, VAL_FMT, i);

		assert(mtbl_writer_add(writer, ubuf_data(key), ubuf_size(key), ubuf_data(value), ubuf_size(value)) == mtbl_res_success);

		ubuf_destroy(&key);
		ubuf_destroy(&value);
	}

	mtbl_writer_destroy(&writer);
	mtbl_writer_options_destroy(&writer_options);
}
