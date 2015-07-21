#include <sys/types.h>
#include <errno.h>
#include <inttypes.h>
#include <libgen.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <mtbl.h>

#include "libmy/my_alloc.h"

#define NAME	"test-compression"

/**
 * Write a test key ('A' * 10000) + test value ('B' * 20000) to an mtbl_writer,
 * using the given compression type. Then re-open the file with an mtbl_reader
 * and verify that we get the exact same key/value entry back.
 *
 * This tests that data block compression/decompression is functioning
 * correctly.
 *
 * We use mkstemp() to create a temporary file, then immediately unlink() and
 * dup() it, which allows us to read from the file after we close it. This
 * avoids leaving the temporary file on the filesystem if the test fails.
 */
static int
test_compression(mtbl_compression_type c_type, const char *dirname)
{
	mtbl_res res;

	/* Initialize test key/value. */
	size_t len_key = 10000;
	size_t len_val = 20000;
	uint8_t *key = my_malloc(len_key);
	uint8_t *val = my_malloc(len_val);
	memset(key, 'A', len_key);
	memset(val, 'B', len_val);

	/* Create and open temporary file. */
	char fname[strlen(dirname) + 100];
	sprintf(fname, "%s/.mtbl." NAME ".%ld.XXXXXX", dirname, (long)getpid());
	int fd = mkstemp(fname);
	if (fd == -1) {
		fprintf(stderr, NAME ": mkstemp() failed: %s\n", strerror(errno));
		return 1;
	}

	/* Unlink the temporary filename. */
	int unlink_ret = unlink(fname);
	if (unlink_ret == -1) {
		fprintf(stderr, NAME ": unlink() on file '%s' failed: %s\n",
			fname, strerror(errno));
		return 1;
	}

	/**
	 * Duplicate the file descriptor.
	 * Used by the reader, since mtbl_writer_destroy() will close the fd,
	 * which will delete the file, since it has no links on the filesystem.
	 */
	int dup_fd = dup(fd);
	if (dup_fd == -1) {
		fprintf(stderr, NAME ": dup() failed: %s\n", strerror(errno));
		return 1;
	}

	/* Open a writer on the temporary file. */
	struct mtbl_writer_options *wopt = mtbl_writer_options_init();
	mtbl_writer_options_set_compression(wopt, c_type);
	struct mtbl_writer *w = mtbl_writer_init_fd(fd, wopt);
	mtbl_writer_options_destroy(&wopt);
	if (w == NULL) {
		fprintf(stderr, NAME ": mtbl_writer_init_fd() failed\n");
		return 1;
	}

	/* Write the test key/value entry. */
	res = mtbl_writer_add(w, key, len_key, val, len_val);
	if (res != mtbl_res_success) {
		fprintf(stderr, NAME ": mtbl_writer_add() failed\n");
		return 1;
	}

	/* Close the writer. */
	mtbl_writer_destroy(&w);
	fd = -1;

	/* Open the reader on the dup()'d file descriptor. */
	struct mtbl_reader *r = mtbl_reader_init_fd(dup_fd, NULL);
	if (r == NULL) {
		fprintf(stderr, NAME ": mtbl_reader_init_fd() failed\n");
		return 1;
	}

	/**
	 * Check that the compresison algorithm on the reader was what we set
	 * on the writer.
	 */
	const struct mtbl_metadata *m = mtbl_reader_metadata(r);
	uint64_t m_c_type = mtbl_metadata_compression_algorithm(m);
	if ((mtbl_compression_type) m_c_type != c_type) {
		fprintf(stderr, NAME ": mtbl_metadata_compression_algorithm() "
			"returned unexpected value %" PRIu64 " (!= %u)\n",
			m_c_type, c_type);
		return 1;
	}

	/* Retrieve the test key/value entry. */
	const struct mtbl_source *s = mtbl_reader_source(r);
	if (s == NULL) {
		fprintf(stderr, NAME ": mtbl_reader_source() failed\n");
		return 1;
	}
	struct mtbl_iter *it = mtbl_source_iter(s);
	const uint8_t *it_key, *it_val;
	size_t len_it_key, len_it_val;
	res = mtbl_iter_next(it, &it_key, &len_it_key, &it_val, &len_it_val);
	if (res != mtbl_res_success) {
		fprintf(stderr, NAME ": mtbl_iter_next() failed\n");
		return 1;
	}

	/* Check the test entry against our original entry. */
	if (len_it_key != len_key) {
		fprintf(stderr, NAME ": len_it_key != len_key\n");
		return 1;
	}
	if (len_it_val != len_val) {
		fprintf(stderr, NAME ": len_it_val != len_val\n");
		return 1;
	}
	if (memcmp(it_key, key, len_key) != 0) {
		fprintf(stderr, NAME ": it_key != key\n");
		return 1;
	}
	if (memcmp(it_val, val, len_val) != 0) {
		fprintf(stderr, NAME ": it_val != val\n");
		return 1;
	}

	/* Test that there are no more entries. */
	res = mtbl_iter_next(it, &it_key, &len_it_key, &it_val, &len_it_val);
	if (res != mtbl_res_failure) {
		fprintf(stderr, NAME ": mtbl_iter_next() returned an "
			"additional unexpected entry\n");
		return 1;
	}

	/* Cleanup. */
	mtbl_iter_destroy(&it);
	mtbl_reader_destroy(&r);
	free(key);
	free(val);

	return 0;
}

static int
check(int ret, const char *s1, const char *s2)
{
	if (ret == 0)
		fprintf(stderr, NAME ": PASS: %s %s\n", s1, s2);
	else
		fprintf(stderr, NAME ": FAIL: %s %s\n", s1, s2);
	return ret;
}

int
main(int argc, char **argv)
{
	int ret = 0;
	mtbl_compression_type c_type;
	mtbl_res res;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <COMPRESSION TYPE>\n", argv[0]);
		return EXIT_FAILURE;
	}

	res = mtbl_compression_type_from_str(argv[1], &c_type);
	if (res != mtbl_res_success) {
		fprintf(stderr, "%s: Error: Unknown compression type '%s'\n",
			argv[0], argv[1]);
		return EXIT_FAILURE;
	}

	ret |= check(test_compression(c_type, dirname(argv[1])), "test_compression", argv[1]);

	if (ret)
		return EXIT_FAILURE;
	return EXIT_SUCCESS;
}
