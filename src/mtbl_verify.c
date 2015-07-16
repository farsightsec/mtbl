/*
 * Copyright (c) 2015 by Farsight Security, Inc.
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

#include <sys/mman.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <locale.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <mtbl.h>

static const int block_progress_interval = 10000;

static void
clear_line_stdout(void)
{
	const char *clear_line_sequence =
		"\033[0G" /* Move the cursor to column 0. */
		"\033[2K" /* Erase the entire line. */
	;

	if (isatty(STDOUT_FILENO)) {
		fputs(clear_line_sequence, stdout);
		fflush(stdout);
	}
}

static bool
verify_data_blocks(
	int fd,
	const char *prefix,
	const uint64_t file_offset,
	const uint64_t bytes_data_blocks,
	const uint64_t count_data_blocks)
{
	bool res = true;
	size_t len_mmap_data;
	uint8_t *mmap_data;
	uint8_t *data;
	uint64_t offset = 0;
	uint64_t bytes_consumed = 0;

	/* Handle MTBL files with no data entries. */
	if (bytes_data_blocks == 0 && count_data_blocks == 0)
		return true;

	/* Map the data blocks. */
	len_mmap_data = file_offset + bytes_data_blocks;
	mmap_data = mmap(NULL, len_mmap_data, PROT_READ, MAP_PRIVATE, fd, 0);
	if (mmap_data == MAP_FAILED) {
		fprintf(stderr, "%s: mmap() failed\n", prefix);
		return false;
	}
	data = mmap_data + file_offset;

	/* Verify each data block. */
	for (uint64_t block = 0; block < count_data_blocks; block++) {
		uint32_t block_crc, calc_crc;
		uint32_t raw_contents_size;
		uint8_t *raw_contents;

		/* Get the data block size. */
		raw_contents_size = mtbl_fixed_decode32(&data[offset + 0]);

		/* Bounds check. */
		bytes_consumed += 2 * sizeof(uint32_t);
		bytes_consumed += raw_contents_size;
		if (bytes_consumed > bytes_data_blocks) {
			clear_line_stdout();
			fprintf(stderr, "%s: Error: Block length (%u bytes) exceeds "
				"total data length at "
				"data block %" PRIu64 " (%" PRIu64 " bytes into file)\n",
				prefix,
				raw_contents_size,
				block,
				offset);
			return false;
		}

		/* CRC32C check. */
		raw_contents = &data[offset + 2 * sizeof(uint32_t)];
		block_crc = mtbl_fixed_decode32(&data[offset + 1 * sizeof(uint32_t)]);
		calc_crc = mtbl_crc32c(raw_contents, raw_contents_size);
		if (block_crc != calc_crc) {
			clear_line_stdout();
			fprintf(stderr, "%s: Error: block_crc (%08x) != calc_crc (%08x) at "
				"data block %" PRIu64 " (%" PRIu64 " bytes into file)\n",
				prefix,
				block_crc,
				calc_crc,
				block,
				offset);
			res = false;
			break;
		}

		/* Progress report. */
		if ((block % block_progress_interval) == 0) {
			if (isatty(STDOUT_FILENO)) {
				clear_line_stdout();
				printf("%s: %'" PRIu64 " out of %'" PRIu64 " blocks (%.2f%%) OK",
				       prefix,
				       block,
				       count_data_blocks,
				       100.0 * (block + 0.0) / (count_data_blocks + 0.0));
				fflush(stdout);
			}
		}

		/* Update 'offset' to the next data block. */
		offset += 2 * sizeof(uint32_t);
		offset += raw_contents_size;
	}

	/* Unmap the data blocks. */
	munmap(mmap_data, len_mmap_data);

	clear_line_stdout();
	return res;
}

static bool
verify_file(const char *fname)
{
	bool res = true;
	int fd;
	struct mtbl_reader *r;
	const struct mtbl_metadata *m;

	fd = open(fname, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "%s: open() failed: %s\n", fname, strerror(errno));
		return false;
	}

	r = mtbl_reader_init_fd(fd, NULL);
	if (r == NULL) {
		close(fd);
		fprintf(stderr, "%s: mtbl_reader_init_fd() failed\n", fname);
		return false;
	}

	m = mtbl_reader_metadata(r);

	uint64_t count_data_blocks = mtbl_metadata_count_data_blocks(m);
	uint64_t bytes_data_blocks = mtbl_metadata_bytes_data_blocks(m);
	uint64_t index_offset = mtbl_metadata_index_block_offset(m);

	uint64_t data_offset = index_offset - bytes_data_blocks;

	if (verify_data_blocks(fd, fname, data_offset, bytes_data_blocks, count_data_blocks)) {
		printf("%s: OK\n", fname);
		res = true;
	} else {
		printf("%s: FAILED\n", fname);
		res = false;
	}

	mtbl_reader_destroy(&r);

	return res;
}

int
main(int argc, char **argv)
{
	int n_failed = 0;

	setlocale(LC_ALL, "");

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <MTBL FILE> [<MTBL FILE>...]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	for (int i = 1; i < argc; i++) {
		if (!verify_file(argv[i]))
			n_failed += 1;
	}

	if (n_failed > 0) {
		fprintf(stderr, "%s: WARNING: %d file%s did NOT successfully verify\n",
			argv[0],
			n_failed,
			n_failed > 1 ? "s" : ""
		);
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
