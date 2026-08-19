/*
 * Copyright (c) 2026 DomainTools LLC
 * Copyright (c) 2012, 2014-2015, 2021 by Farsight Security, Inc.
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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <inttypes.h>
#include <mtbl.h>

#include "libmy/print_string.h"
#include "libmy/hex_decode.h"

static void print_hex_string(const void *data, size_t len, FILE *out);

/* print data formatted as an 8 digit hex length, colon, the hex digit pairs separated by a dash */
static void print_hex_string(const void *data, size_t len, FILE *out)
{
	uint8_t *str = (uint8_t *) data;
	assert(len < 4294967295);
	fprintf(out, "%08x:", (unsigned int)len);
	while (len-- != 0) {
		unsigned c = *(str++);
		fprintf(out, "%02x", c);
		if (len > 0)
			fputc('-', out);
	}
}


static bool
dump(const char *fname, const bool silent, bool hex,
     const uint8_t *key_prefix, size_t key_prefix_len,
     const uint8_t *val_prefix, size_t val_prefix_len,
     size_t key_min_len, size_t val_min_len)
{
	const uint8_t *key, *val;
	size_t key_len, val_len;
	uint64_t count = 0, expected;
	struct mtbl_reader *r;
	struct mtbl_iter *it;

	r = mtbl_reader_init(fname, NULL);
	if (r == NULL) {
		fprintf(stderr, "Error: mtbl_reader_init() on %s failed\n", fname);
		return (false);
	}

	expected = mtbl_metadata_count_entries(mtbl_reader_metadata(r));
	it = mtbl_source_iter(mtbl_reader_source(r));
	while (mtbl_iter_next(it, &key, &key_len, &val, &val_len)) {
		count++;
		if (key_prefix != 0
		    && (key_len < key_prefix_len
			|| 0 != bcmp(key, key_prefix, key_prefix_len)))
			continue;
		if (val_prefix != 0
		    && (val_len < val_prefix_len
			|| 0 != bcmp(val, val_prefix, val_prefix_len)))
			continue;
		if (key_len < key_min_len || val_len < val_min_len)
			continue;
		if (silent)
			continue;
		if (hex) {
			print_hex_string(key, key_len, stdout);
			fputc(' ', stdout);
			print_hex_string(val, val_len, stdout);
			fputc('\n', stdout);
		} else {
			print_string(key, key_len, stdout);
			fputc(' ', stdout);
			print_string(val, val_len, stdout);
			fputc('\n', stdout);
		}
	}

	mtbl_iter_destroy(&it);

	if (count != expected) {
		fprintf(stderr, "%s: error: read %" PRIu64 " of %" PRIu64 " expected entries;"
			" file may be truncated or corrupt\n", fname, count, expected);
		mtbl_reader_destroy(&r);
		return (false);
	}

	mtbl_reader_destroy(&r);

	return (true);
}

static void
usage(void)
{
	fprintf(stderr, "Usage: mtbl_dump [-s] [-x] [-k ABCD...] [-v ABCD...] [-K #] [-V #] <MTBL FILE>\n");
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	char *fname;
	bool silent = false;
	bool hex = false;
	uint8_t *key_prefix = NULL;
	size_t key_prefix_len = 0;
	uint8_t *val_prefix = NULL;
	size_t val_prefix_len = 0;
	size_t key_min_len = 0;
	size_t val_min_len = 0;

	int c;

	while ((c = getopt(argc, argv, "sxk:v:K:V:")) != -1) {
		switch (c) {
		case 's':
			silent = true;
			break;
		case 'x':
			hex = true;
			break;
		case 'k':
			if (strlen(optarg) == 0) {
				fprintf(stderr, "Need a non-empty argument to -k\n");
				return (EXIT_FAILURE);
			}
			if (hex_decode(optarg, &key_prefix, &key_prefix_len) == false) {
				fprintf(stderr, "hex decoding of %s failed\n", optarg);
				return (EXIT_FAILURE);
			}
			break;
		case 'v':
			if (strlen(optarg) == 0) {
				fprintf(stderr, "Need a non-empty argument to -v\n");
				return (EXIT_FAILURE);
			}
			if (hex_decode(optarg, &val_prefix, &val_prefix_len) == false) {
				fprintf(stderr, "hex decoding of %s failed\n", optarg);
				return (EXIT_FAILURE);
			}
			break;
		case 'K':
		{
			char *endptr;
			long val = strtol(optarg, &endptr, 10);
			if (endptr == optarg || *endptr != '\0' || val < 1) {
				fprintf(stderr, "Invalid minimum key length: %s\n", optarg);
				return (EXIT_FAILURE);
			}
			key_min_len = (size_t) val;
			break;
		}
		case 'V':
		{
			char *endptr;
			long val = strtol(optarg, &endptr, 10);
			if (endptr == optarg || *endptr != '\0' || val < 1) {
				fprintf(stderr, "Invalid minimum val length: %s\n", optarg);
				return (EXIT_FAILURE);
			}
			val_min_len = (size_t) val;
			break;
		}
		default:
			usage();
		}
	}

	if (optind >= argc)
		usage();
	fname = argv[optind];

	if (!dump(fname, silent, hex,
		  key_prefix, key_prefix_len,
		  val_prefix, val_prefix_len,
		  key_min_len, val_min_len))
		return (EXIT_FAILURE);
	return (EXIT_SUCCESS);
}
