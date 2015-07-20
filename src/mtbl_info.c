/*
 * Copyright (c) 2012, 2015 by Farsight Security, Inc.
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
#include <string.h>
#include <unistd.h>

#include <mtbl.h>

static void
print_info(const char *fname)
{
	int fd, ret;
	struct stat ss;
	struct mtbl_reader *r;
	const struct mtbl_metadata *m;

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

	r = mtbl_reader_init_fd(fd, NULL);
	if (r == NULL) {
		fprintf(stderr, "Error: mtbl_reader_init_fd() on %s failed\n", fname);
		exit(EXIT_FAILURE);
	}

	m = mtbl_reader_metadata(r);

	uint64_t data_block_size = mtbl_metadata_data_block_size(m);
	mtbl_compression_type compression_algorithm =
		mtbl_metadata_compression_algorithm(m);
	uint64_t count_entries = mtbl_metadata_count_entries(m);
	uint64_t count_data_blocks = mtbl_metadata_count_data_blocks(m);
	uint64_t bytes_data_blocks = mtbl_metadata_bytes_data_blocks(m);
	uint64_t bytes_index_block = mtbl_metadata_bytes_index_block(m);
	uint64_t bytes_keys = mtbl_metadata_bytes_keys(m);
	uint64_t bytes_values = mtbl_metadata_bytes_values(m);
	uint64_t index_block_offset = mtbl_metadata_index_block_offset(m);

	double p_data = 100.0 * bytes_data_blocks / ss.st_size;
	double p_index = 100.0 * bytes_index_block / ss.st_size;
	double compactness = 100.0 * ss.st_size / (bytes_keys + bytes_values);

	printf("file name:             %s\n", fname);
	printf("file size:             %'zd\n", (size_t) ss.st_size);
	printf("index block offset:    %'" PRIu64 "\n", index_block_offset);
	printf("index bytes:           %'" PRIu64 " (%'.2f%%)\n", bytes_index_block, p_index);
	printf("data block bytes       %'" PRIu64 " (%'.2f%%)\n", bytes_data_blocks, p_data);
	printf("data block size:       %'" PRIu64 "\n", data_block_size);
	printf("data block count       %'" PRIu64 "\n", count_data_blocks);
	printf("entry count:           %'" PRIu64 "\n", count_entries);
	printf("key bytes:             %'" PRIu64 "\n", bytes_keys);
	printf("value bytes:           %'" PRIu64 "\n", bytes_values);

	printf("compression algorithm: ");
	const char *compression = mtbl_compression_type_to_str(compression_algorithm);
	if (compression != NULL)
		puts(compression);
	else
		printf("%u\n", compression_algorithm);

	printf("compactness:           %'.2f%%\n", compactness);

	putchar('\n');

	mtbl_reader_destroy(&r);
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
