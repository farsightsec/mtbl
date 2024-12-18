#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>

#include <mtbl.h>

#include "libmy/ubuf.h"
#include "mtbl-private.h"

#define NAME		"test-sorted-merge"


#define NUM_SETS	2
#define NUM_KEYS	10
#define NUM_SORTED	18
#define T1	"abcd1"
#define T2	"apple pie"
#define T3	"farsight security"
#define T4	"mtbl is the best"
#define T5	"only the strong"
#define T6	"over the lazy dogs"
#define T7	"testing 123"
#define T8	"the quick brown fox"
#define T9	"unit testing"
#define T10	"xylophone"

#define T11	"abcd1"
#define T12	"brave new world"
#define T13	"far sight security"
#define T14	"glengarry glenn ross"
#define T15	"onwards and upwards"
#define T16	"over the rainbow"
#define T17	"send me to the moon"
#define T18	"testing 123"
#define T19	"xyz"
#define T20	"zebra"

#define MERGE_DUP	"deadbeef"

typedef struct sorter_tester {
	char *filename;
	struct mtbl_reader *reader;
	const struct mtbl_source *source;
	struct mtbl_iter *iter;
} sorter_tester_t;

sorter_tester_t testers[NUM_SETS];
char *tmpfname = NULL;

char *key_values[NUM_SETS][NUM_KEYS] = {
	{ T1,  T2,  T3,  T4,  T5,  T6,  T7,  T8,  T9,  T10 },
	{ T11, T12, T13, T14, T15, T16, T17, T18, T19, T20 }
};

char *sorted_values[NUM_SORTED] = {
	T1, T2, T12, T13, T3, T14, T4, T5, T15, T6, T16, T17, T18, T8, T9, T10, T19, T20
};

int sorted_dups[NUM_SORTED] = {
	1,  0,  0,   0,   0,  0,   0,  0,  0,   0,  0,   0,   1,   0,  0,  0,   0,   0
};


/* Suppress aggravating compiler warnings caused by calling plain old tmpnam() */
static char *
quiet_tmpnam(void)
{
	char template[] = "./tmp.mtbl.XXXXXX";
	int fd = mkstemp(template);

	assert(fd >= 0);
	close(fd);
	/* filename passed to mtbl_writer_init must not exist. */
	unlink(template);
	return strdup(template);
}

static void
init_mtbl(const char *filename, size_t idx);

static void
test_iter_sorted(struct mtbl_iter *, size_t idx, struct mtbl_sorter *sorter);

static void cleanup_func(void) {
	size_t i;

	for (i = 0; i < (sizeof(testers)/sizeof(testers[0])); i++) {

		if (testers[i].filename) {
			unlink(testers[i].filename);
			free(testers[i].filename);
			testers[i].filename = NULL;
		}

		if (testers[i].reader) {
			mtbl_reader_destroy(&(testers[i].reader));
			testers[i].reader = NULL;
		}

		if (testers[i].iter) {
			mtbl_iter_destroy(&(testers[i].iter));
			testers[i].iter = NULL;
		}

	}

	if (tmpfname != NULL) {
		unlink(tmpfname);
		free(tmpfname);
		tmpfname = NULL;
	}

}


static void
my_merge_func(void *clos,
        const uint8_t *key, size_t len_key,
        const uint8_t *val0, size_t len_val0,
        const uint8_t *val1, size_t len_val1,
        uint8_t **merged_val, size_t *len_merged_val);

int main(int argc, char ** argv) {
	size_t i, j = 0;
	struct mtbl_threadpool *pool = mtbl_threadpool_init(4);

	/* Make sure everything is cleaned up afterwards. */
	atexit(cleanup_func);

	for (j = 0; j < 2; j++) {
		struct mtbl_sorter_options *sorter_options;
		struct mtbl_sorter *sorter;
		struct mtbl_iter *sorter_iter;
		struct mtbl_reader_options *reader_options;
		struct mtbl_writer *writer;

		/* Reset the testers if necessary. */
		memset(testers, 0, sizeof(testers));

		/* Get the sorter ready to process data. */
		sorter_options = mtbl_sorter_options_init();
		assert(sorter_options != NULL);

		/* For some of the sorts, test the sorter's multithreading. */
		if (j % 2 == 0) {
			mtbl_sorter_options_set_threadpool(sorter_options, pool);
		}
		mtbl_sorter_options_set_merge_func(sorter_options, my_merge_func, NULL);

		sorter = mtbl_sorter_init(sorter_options);
		assert(sorter != NULL);

		/* Prepare the readers. */
		reader_options = mtbl_reader_options_init();
		assert(reader_options != NULL);

		/* Go through each of the data sets. */
		for (i = 0; i < NUM_SETS; i++) {
			/* Generate the temporary individual mtbl filename */
			testers[i].filename = quiet_tmpnam();
			assert(testers[i].filename != NULL);
			/* First write each individual mtbl component with key/value pairs. */
			init_mtbl(testers[i].filename, i);

			/* Then open up that temporary file for reading. */
			testers[i].reader = mtbl_reader_init(testers[i].filename, reader_options);
			assert(testers[i].reader != NULL);

			testers[i].source = mtbl_reader_source(testers[i].reader);
			assert(testers[i].source != NULL);

			testers[i].iter = mtbl_source_iter(testers[i].source);
			assert(testers[i].iter != NULL);

			/* Finally, double check its contents are what we expect. */
			test_iter_sorted(testers[i].iter, i, sorter);

		}

		fprintf(stderr, NAME ": PASS: %s initialization successful\n",
			(j ? "sorter writer" : "standard sorter"));

		mtbl_reader_options_destroy(&reader_options);

		/* Only the second half of the tests calls mtbl_sorter_write(). */
		if (j) {
			/* First open up a writer to accept the sorted values. */
			struct mtbl_writer_options *writer_options = mtbl_writer_options_init();
			assert(writer_options != NULL);

			mtbl_writer_options_set_block_size(writer_options, 1024);

			tmpfname = quiet_tmpnam();
			assert(tmpfname != NULL);

			writer = mtbl_writer_init(tmpfname, writer_options);
			assert(writer != NULL);

			/* Then flush the sorted entries out to disk. */
			assert(mtbl_sorter_write(sorter, writer) == mtbl_res_success);

			mtbl_writer_destroy(&writer);
			mtbl_writer_options_destroy(&writer_options);

			/* Everything is written. Now create an iterator for it. */
			struct mtbl_reader *iter_reader = mtbl_reader_init(tmpfname, reader_options);
                        assert(iter_reader != NULL);

			/* Flip that writer around and make it a reader.
			   For mtbl_sorter_write(), we validate the correctness
			   of the records written above by iterating over the new MTBL. */
			const struct mtbl_source *iter_source = mtbl_reader_source(iter_reader);
			assert(iter_source != NULL);

			sorter_iter = mtbl_source_iter(iter_source);
			assert(sorter_iter != NULL);

		} else {
			/* In the first half, our normal case scenario is to validate
			  the sorter's work directly by iterating over it. */
			sorter_iter = mtbl_sorter_iter(sorter);
			assert(sorter_iter != NULL);
		}

		size_t total = 0;

		while (1) {
			const uint8_t *key, *value;
			size_t len_key, len_value;

			/* For each expected value, read the key/value pair. */
			if (mtbl_iter_next(sorter_iter, &key, &len_key, &value, &len_value) != mtbl_res_success)
				break;

			total++;

			/* Also make sure we don't have any unexpected extra entries. */
			if (total > (sizeof(sorted_values) / sizeof(sorted_values[0])))
				break;

			/* Make sure the key values (and hence, order) are correct. */
			assert(bytes_compare((const uint8_t *)sorted_values[total - 1],
				strlen(sorted_values[total - 1]) + 1, key, len_key) == 0);

			/* Also verify the values. Merged keys have their values
			   set to MERGE_DUP, so check for this being the case
			   wherever we expect a merged value in the sorter output. */
			if (sorted_dups[total - 1]) {

				assert(strcmp((char *)value, MERGE_DUP) == 0);

			} else {

				assert(bytes_compare((const uint8_t *)sorted_values[total - 1],
					strlen(sorted_values[total - 1]) + 1, value, len_value) == 0);

			}

		}

		assert(total == (sizeof(sorted_values) / sizeof(sorted_values[0])));
		fprintf(stderr, NAME ": PASS: %s run successful\n",
			(j ? "sorter writer" : "standard sorter"));

		mtbl_iter_destroy(&sorter_iter);
		mtbl_sorter_destroy(&sorter);
		mtbl_sorter_options_destroy(&sorter_options);
		cleanup_func();
	}

	mtbl_threadpool_destroy(&pool);
	return 0;
}

static void
test_iter_sorted(struct mtbl_iter *iter, size_t idx, struct mtbl_sorter *sorter)
{
	const uint8_t *key, *value;
	size_t len_key, len_value;

	/* Iterate completely through the mtbl */
	for (size_t i = 0; i < NUM_KEYS; i++) {

		assert(mtbl_iter_next(iter, &key, &len_key, &value, &len_value) == mtbl_res_success);

		assert(bytes_compare((const uint8_t *)key_values[idx][i], strlen(key_values[idx][i]) + 1, key, len_key) == 0);

		assert(mtbl_sorter_add(sorter, key, len_key, value, len_value) == mtbl_res_success);

	}

	/* Ensure that we have completely iterated through the set. */
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

	if ((len_val0 == len_val1) && (!memcmp(val0, val1, len_val0))) {
		*merged_val = my_calloc(1, strlen(MERGE_DUP) + 1);
		strcpy(((char *)*merged_val), MERGE_DUP);
		*len_merged_val = strlen(MERGE_DUP) + 1;
	} else {
		*merged_val = my_calloc(1, len_val0);
		memcpy(*merged_val, val0, len_val0);
		*len_merged_val = len_val0;
	}

}

static void
init_mtbl(const char *filename, size_t idx) {
	struct mtbl_writer_options *writer_options = mtbl_writer_options_init();
	assert(writer_options != NULL);

	mtbl_writer_options_set_block_size(writer_options, 1024);

	struct mtbl_writer *writer = mtbl_writer_init(filename, writer_options);
	assert(writer != NULL);

	/* Populate the mtbl with hex(i)->i */
	for (uint32_t i = 0; i < NUM_KEYS; i++) {
		assert(mtbl_writer_add(writer, (const uint8_t *)key_values[idx][i], strlen(key_values[idx][i]) + 1,
			(const uint8_t *)key_values[idx][i], strlen(key_values[idx][i]) + 1) == mtbl_res_success);
	}

	mtbl_writer_destroy(&writer);
	mtbl_writer_options_destroy(&writer_options);
}
