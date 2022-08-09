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

static void
test_iter(struct mtbl_iter *);

static void
my_merge_func(void *clos,
        const uint8_t *key, size_t len_key,
        const uint8_t *val0, size_t len_val0,
        const uint8_t *val1, size_t len_val1,
        uint8_t **merged_val, size_t *len_merged_val);

static void
test1(FILE *tmp, FILE *tmp2) {
	struct mtbl_reader_options *reader_options = mtbl_reader_options_init();
	assert(reader_options != NULL);
	mtbl_reader_options_set_block_search(reader_options, true);

	struct mtbl_reader *reader = mtbl_reader_init_fd(dup(fileno(tmp)), reader_options);
	assert(reader != NULL);

	const struct mtbl_source *source = mtbl_reader_source(reader);
	assert(source != NULL);

	struct mtbl_iter *iter = mtbl_source_iter(source);
	assert(iter != NULL);

	test_iter(iter);
	fprintf(stderr, NAME ": PASS: iter run successful\n");

	mtbl_iter_destroy(&iter);

	struct mtbl_merger_options *merger_options = mtbl_merger_options_init();
	assert(merger_options != NULL);

	mtbl_merger_options_set_merge_func(merger_options, my_merge_func, NULL);

	struct mtbl_merger *merger = mtbl_merger_init(merger_options);
	assert(merger != NULL);

	mtbl_merger_add_source(merger, source);

	const struct mtbl_source *merger_source = mtbl_merger_source(merger);
	assert(merger_source != NULL);

	struct mtbl_iter *merger_iter = mtbl_source_iter(merger_source);
	assert(merger_iter != NULL);

	test_iter(merger_iter);
	fprintf(stderr, NAME ": PASS: merger run 1 successful\n");

	mtbl_iter_destroy(&merger_iter);

	struct mtbl_reader *reader2 = mtbl_reader_init_fd(dup(fileno(tmp2)), reader_options);
	assert(reader2 != NULL);

	mtbl_merger_add_source(merger, mtbl_reader_source(reader2));

	merger_iter = mtbl_source_iter(merger_source);
	assert(merger_iter != NULL);

	test_iter(merger_iter);
	fprintf(stderr, NAME ": PASS: merger run 2 successful\n");

	mtbl_iter_destroy(&merger_iter);

	mtbl_merger_destroy(&merger);
	mtbl_merger_options_destroy(&merger_options);
	mtbl_reader_destroy(&reader);
	mtbl_reader_destroy(&reader2);
	mtbl_reader_options_destroy(&reader_options);
}

static int
test2(FILE *tmp, int32_t num_keys, int32_t jump) {
	struct mtbl_reader_options *reader_options = mtbl_reader_options_init();
	mtbl_reader_options_set_block_search(reader_options, true);

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
			if(seek_and_check(iter, i, target_number) != 0) {
				return 1;
			};
		}
	}

	/* Loop backward from the last key and seek i - jump keys */
	for (int32_t i = last_number; i > 0; i--) {
		if ((i - jump) >= 0) {
			int32_t target_number = i - jump;
			if(seek_and_check(iter, i, target_number) != 0) {
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
	int ret = 0;
	FILE *tmp = tmpfile();
	assert(tmp != NULL);
	init_mtbl(dup(fileno(tmp)), NUM_KEYS, 1024, 
		DEFAULT_BLOCK_RESTART_INTERVAL);
	
	FILE *tmp2 = tmpfile();
	assert(tmp2 != NULL);
	init_mtbl(dup(fileno(tmp2)), NUM_KEYS, 1024, 
		DEFAULT_BLOCK_RESTART_INTERVAL);

	test1(tmp, tmp2);
	fclose(tmp);
	fclose(tmp2);

	/* Make a larget tmp mtbl so we have larger restart arrays */
	int32_t test_2_num_keys = NUM_KEYS * 100;
	FILE *tmp3 = tmpfile();
	assert(tmp3 != NULL);
	assert(test_2_num_keys > 0);
	init_mtbl(dup(fileno(tmp3)), (uint32_t)test_2_num_keys, DEFAULT_BLOCK_SIZE,
		RESTART_INTERVAL);

	/* From each key, test seeking from 0 to RESTART_INTERVAL keys
	 * forward and backward */
	char test2_label[TEST2_LABEL_LEN];
	for (uint8_t i = 0; i < RESTART_INTERVAL; i++) {
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

	if (ret)
		return (EXIT_FAILURE);
	return (EXIT_SUCCESS);
}

static int
seek_and_check(struct mtbl_iter *iter, int32_t initial, int32_t target) {
	/* Seek to the initial key first */
	ubuf *initial_key = ubuf_init(1);
	ubuf_add_fmt(initial_key, KEY_FMT, initial);
	if(mtbl_iter_seek(iter, ubuf_data(initial_key),
		ubuf_size(initial_key)) != mtbl_res_success) {
		return 1;
	}

	/* Seek to the target key */
	ubuf *expected_key = ubuf_init(1);
	ubuf_add_fmt(expected_key, KEY_FMT, target);
	if(mtbl_iter_seek(iter, ubuf_data(expected_key),
		ubuf_size(expected_key)) != mtbl_res_success) {
		return 1;
	}

	/* Check that the next key matches the target key */
	const uint8_t *key, *value;
	size_t len_key, len_value;
	if(mtbl_iter_next(iter, &key, &len_key, &value,
		&len_value) != mtbl_res_success) {
		return 1;
	}
	if(bytes_compare(ubuf_data(expected_key), ubuf_size(expected_key), key,
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

static void
test_iter(struct mtbl_iter *iter)
{
	/* Iterate completely through the mtbl */
	for (uint32_t i = 0; i < NUM_KEYS; i++) {
		ubuf *expected_key = ubuf_init(1);
		ubuf_add_fmt(expected_key, KEY_FMT, i);

		const uint8_t *key, *value;
		size_t len_key, len_value;

		assert(mtbl_iter_next(iter, &key, &len_key, &value, &len_value) == mtbl_res_success);
		
		assert(bytes_compare(ubuf_data(expected_key), ubuf_size(expected_key), key, len_key) == 0);

		ubuf_destroy(&expected_key);
	}

	/* Ensure that we have completely iterated through the set. */
	{
		const uint8_t *key, *value;
		size_t len_key, len_value;

		assert(mtbl_iter_next(iter, &key, &len_key, &value, &len_value) == mtbl_res_failure);
	}

	/* Seek to a key, ensure that we get the one we want and that we go
	 * all the way to the end. */
	for (uint32_t i = NUM_KEYS; i-- > 0; ) {
		ubuf *seek_key = ubuf_init(1);
		ubuf_add_fmt(seek_key, KEY_FMT, i);

		const uint8_t *key, *value;
		size_t len_key, len_value;

		assert(mtbl_iter_seek(iter, ubuf_data(seek_key), ubuf_size(seek_key)) == mtbl_res_success);
		
		for (uint32_t j = i; j < NUM_KEYS; j++) {
			ubuf *expected_key = ubuf_init(1);
			ubuf_add_fmt(expected_key, KEY_FMT, j);

			assert(mtbl_iter_next(iter, &key, &len_key, &value, &len_value) == mtbl_res_success);
			
			assert(bytes_compare(ubuf_data(expected_key), ubuf_size(expected_key), key, len_key) == 0);

			ubuf_destroy(&expected_key);
		}

		assert(mtbl_iter_next(iter, &key, &len_key, &value, &len_value) == mtbl_res_failure);

		ubuf_destroy(&seek_key);
	}

	/* Attempt to seek past end of iterator */
	ubuf *seek_key = ubuf_init(1);
	const uint8_t *key, *value;
	size_t len_key, len_value;

	ubuf_add_fmt(seek_key, KEY_FMT, NUM_KEYS + 1);
	assert(mtbl_iter_seek(iter, ubuf_data(seek_key), ubuf_size(seek_key)) == mtbl_res_success);
	assert(mtbl_iter_next(iter, &key, &len_key, &value, &len_value) == mtbl_res_failure);
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
