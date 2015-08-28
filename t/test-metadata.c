#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <mtbl.h>

#include "metadata.c"

#define NAME	"test-metadata"

static int
test2(void)
{
	int ret = 0;
	struct mtbl_metadata t;
	uint8_t tbuf[MTBL_METADATA_SIZE];

	memset(tbuf, 0, sizeof(tbuf));

	if (metadata_read(tbuf, &t) != false) {
		fprintf(stderr, NAME ": metadata_read() should have failed but didn't\n");
		ret |= 1;
	}

	return (ret);
}

static int
test1(void)
{
	int ret = 0;
	struct mtbl_metadata m1, m2;
	uint8_t tbuf[MTBL_METADATA_SIZE];

	memset(tbuf, 0, sizeof(tbuf));

	m1.index_block_offset = 123;
	m1.data_block_size = 65536;
	m1.compression_algorithm = 1;
	m1.count_entries = 2;
	m1.count_data_blocks = 3;
	m1.bytes_data_blocks = 4;
	m1.bytes_index_block = 5;
	m1.bytes_keys = 6;
	m1.bytes_values = 7;

	metadata_write(&m1, tbuf);
	if (!metadata_read(tbuf, &m2)) {
		fprintf(stderr, NAME ": metadata_read() failed\n");
		ret |= 1;
	}

	if (memcmp(&m1, &m2, sizeof(struct mtbl_metadata)) != 0) {
		fprintf(stderr, NAME ": m1 != m2\n");
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
