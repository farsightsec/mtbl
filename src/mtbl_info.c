/*
 * Copyright (c) 2012 by Farsight Security, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
