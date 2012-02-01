#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <mtbl.h>

#include "trailer.c"

#define NAME	"test-trailer"

static int
test2(void)
{
	int ret = 0;
	struct trailer t;
	uint8_t tbuf[MTBL_TRAILER_SIZE];

	memset(tbuf, 0, sizeof(tbuf));

	if (trailer_read(tbuf, &t) != false) {
		fprintf(stderr, NAME ": trailer_read() should have failed but didn't\n");
		ret |= 1;
	}

	return (ret);
}

static int
test1(void)
{
	int ret = 0;
	struct trailer t1, t2;
	uint8_t tbuf[MTBL_TRAILER_SIZE];

	memset(tbuf, 0, sizeof(tbuf));

	t1.index_block_offset = 123;
	t1.compression_algorithm = 1;
	t1.count_entries = 2;
	t1.count_data_blocks = 3;
	t1.bytes_data_blocks = 4;
	t1.bytes_index_block = 5;
	t1.bytes_keys = 6;
	t1.bytes_values = 7;

	trailer_write(&t1, tbuf);
	if (!trailer_read(tbuf, &t2)) {
		fprintf(stderr, NAME ": trailer_read() failed\n");
		ret |= 1;
	}

	if (memcmp(&t1, &t2, sizeof(struct trailer) != 0)) {
		fprintf(stderr, NAME ": t1 != t2\n");
		ret |= 1;
	}

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

	ret |= check(test1(), "test1");
	ret |= check(test2(), "test2");

	if (ret)
		return (EXIT_FAILURE);
	return (EXIT_SUCCESS);
}
