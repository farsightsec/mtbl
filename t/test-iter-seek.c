#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#include <mtbl.h>

#include "libmy/ubuf.h"
#include "mtbl-private.h"

#define NUM_KEYS	128

#define KEY_FMT		"%08x"
#define VAL_FMT		"%032d"

static void
init_mtbl(int fd);

static void
test_iter(struct mtbl_iter *);

static void
my_merge_func(void *clos,
        const uint8_t *key, size_t len_key,
        const uint8_t *val0, size_t len_val0,
        const uint8_t *val1, size_t len_val1,
        uint8_t **merged_val, size_t *len_merged_val);

int main(int argc, char ** argv) {
	FILE *tmp = tmpfile();
	assert(tmp != NULL);

	init_mtbl(dup(fileno(tmp)));

	struct mtbl_reader_options *reader_options = mtbl_reader_options_init();
	assert(reader_options != NULL);

	struct mtbl_reader *reader = mtbl_reader_init_fd(dup(fileno(tmp)), reader_options);
	assert(reader != NULL);

	const struct mtbl_source *source = mtbl_reader_source(reader);
	assert(source != NULL);

	struct mtbl_iter *iter = mtbl_source_iter(source);
	assert(iter != NULL);

	test_iter(iter);
	fprintf(stderr, "iter run successful\n");

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
	fprintf(stderr, "merger run 1 successful\n");

	mtbl_iter_destroy(&merger_iter);
	
	FILE *tmp2 = tmpfile();
	assert(tmp != NULL);

	init_mtbl(dup(fileno(tmp2)));

	struct mtbl_reader *reader2 = mtbl_reader_init_fd(dup(fileno(tmp2)), reader_options);
	assert(reader2 != NULL);

	mtbl_merger_add_source(merger, mtbl_reader_source(reader2));

	merger_iter = mtbl_source_iter(merger_source);
	assert(merger_iter != NULL);

	test_iter(merger_iter);
	fprintf(stderr, "merger run 2 successful\n");

	mtbl_iter_destroy(&merger_iter);

	mtbl_merger_destroy(&merger);
	mtbl_merger_options_destroy(&merger_options);
	mtbl_reader_destroy(&reader);
	mtbl_reader_destroy(&reader2);
	mtbl_reader_options_destroy(&reader_options);

	fclose(tmp);
	fclose(tmp2);
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
init_mtbl(int fd) {
	struct mtbl_writer_options *writer_options = mtbl_writer_options_init();
	assert(writer_options != NULL);

	mtbl_writer_options_set_block_size(writer_options, 1024);

	struct mtbl_writer *writer = mtbl_writer_init_fd(fd, writer_options);
	assert(writer != NULL);

	/* Populate the mtbl with hex(i)->i */
	for (uint32_t i = 0; i < NUM_KEYS; i++) {
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
