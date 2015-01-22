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
#include <inttypes.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include <mtbl.h>

static void
print_info(const char *fname)
{
	int ret;
	struct stat ss;
	struct mtbl_reader *r;

	r = mtbl_reader_init(fname, NULL);
	if (r == NULL) {
		fprintf(stderr, "Error: mtbl_reader_init() on %s failed\n", fname);
		exit(EXIT_FAILURE);
	}

	ret = stat(fname, &ss);
	if (ret < 0) {
		perror("Error: stat");
		exit(EXIT_FAILURE);
	}
	
	uint64_t data_block_size = mtbl_reader_get_data_block_size(r);
	mtbl_compression_type compression_algorithm =
		mtbl_reader_get_compression_algorithm(r);
	uint64_t count_entries = mtbl_reader_get_count_entries(r);
	uint64_t count_data_blocks = mtbl_reader_get_count_data_blocks(r);
	uint64_t bytes_data_blocks = mtbl_reader_get_bytes_data_blocks(r);
	uint64_t bytes_index_block = mtbl_reader_get_bytes_index_block(r);
	uint64_t bytes_keys = mtbl_reader_get_bytes_keys(r);
	uint64_t bytes_values = mtbl_reader_get_bytes_values(r);

	double p_data = 100.0 * bytes_data_blocks / ss.st_size;
	double p_index = 100.0 * bytes_index_block / ss.st_size;
	double compactness = 100.0 * ss.st_size / (bytes_keys + bytes_values);

	printf("file name:             %s\n", fname);
	printf("file size:             %'zd\n", (size_t) ss.st_size);
	printf("index bytes:           %'" PRIu64 " (%'.2f%%)\n", bytes_index_block, p_index);
	printf("data block bytes       %'" PRIu64 " (%'.2f%%)\n", bytes_data_blocks, p_data);
	printf("data block size:       %'" PRIu64 "\n", data_block_size);
	printf("data block count       %'" PRIu64 "\n", count_data_blocks);
	printf("entry count:           %'" PRIu64 "\n", count_entries);
	printf("key bytes:             %'" PRIu64 "\n", bytes_keys);
	printf("value bytes:           %'" PRIu64 "\n", bytes_values);
	printf("compression algorithm: ");

	if (compression_algorithm == MTBL_COMPRESSION_NONE) {
		puts("none");
	} else if (compression_algorithm == MTBL_COMPRESSION_SNAPPY) {
		puts("snappy");
	} else if (compression_algorithm == MTBL_COMPRESSION_ZLIB) {
		puts("zlib");
	} else {
		printf("%u\n", compression_algorithm);
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
		print_info(argv[i]);

	return (EXIT_SUCCESS);
}
