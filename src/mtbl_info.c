/*
 * Copyright (c) 2012 by Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include <mtbl.h>
#include "trailer.c"

static void
dump(const char *fname)
{
	int fd, ret;
	size_t trailer_offset;
	struct stat ss;
	struct trailer t;
	uint8_t buf[MTBL_TRAILER_SIZE];

	fd = open(fname, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "Error: unable to open file %s: %s\n", fname, strerror(errno));
		exit(EXIT_FAILURE);
	}

	ret = fstat(fd, &ss);
	if (ret < 0) {
		perror("Error: fstat");
		exit(EXIT_FAILURE);
	}
	trailer_offset = ss.st_size - MTBL_TRAILER_SIZE;

	off_t o = lseek(fd, trailer_offset, SEEK_SET);
	if (o == (off_t) -1) {
		fprintf(stderr, "Error: lseek() failed: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	ret = read(fd, buf, sizeof(buf));
	if (ret != sizeof(buf)) {
		fprintf(stderr, "Error: short read\n");
		exit(EXIT_FAILURE);
	}
	close(fd);

	if (!trailer_read(buf, &t)) {
		fprintf(stderr, "Error: unable to read trailer\n");
		exit(EXIT_FAILURE);
	}

	double p_data = 100.0 * t.bytes_data_blocks / ss.st_size;
	double p_index = 100.0 * t.bytes_index_block / ss.st_size;
	double compactness = 100.0 * ss.st_size / (t.bytes_keys + t.bytes_values);

	printf("file name:             %s\n", fname);
	printf("file size:             %'zd\n", (size_t) ss.st_size);
	printf("index bytes:           %'" PRIu64 " (%'.2f%%)\n", t.bytes_index_block, p_index);
	printf("data block bytes       %'" PRIu64 " (%'.2f%%)\n", t.bytes_data_blocks, p_data);
	printf("data block size:       %'" PRIu64 "\n", t.data_block_size);
	printf("data block count       %'" PRIu64 "\n", t.count_data_blocks);
	printf("entry count:           %'" PRIu64 "\n", t.count_entries);
	printf("key bytes:             %'" PRIu64 "\n", t.bytes_keys);
	printf("value bytes:           %'" PRIu64 "\n", t.bytes_values);
	printf("compression algorithm: ");

	if (t.compression_algorithm == MTBL_COMPRESSION_NONE) {
		puts("none");
	} else if (t.compression_algorithm == MTBL_COMPRESSION_SNAPPY) {
		puts("snappy");
	} else if (t.compression_algorithm == MTBL_COMPRESSION_ZLIB) {
		puts("zlib");
	} else {
		printf("%" PRIu64 "\n", t.compression_algorithm);
	}

	printf("compactness:           %'.2f%%\n", compactness);

	putchar('\n');
}

int
main(int argc, char **argv)
{
	setlocale(LC_ALL, "");

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <MTBL FILE> [<MTBL FILE>...]\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	for (int i = 1; i < argc; i++)
		dump(argv[i]);

	return (EXIT_SUCCESS);
}
