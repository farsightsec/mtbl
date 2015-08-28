/*
 * Copyright (c) 2012, 2014-2015 by Farsight Security, Inc.
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

#include <mtbl.h>

#include "libmy/print_string.h"

static bool
dump(const char *fname, const bool silent)
{
	const uint8_t *key, *val;
	size_t len_key, len_val;
	struct mtbl_reader *r;
	struct mtbl_iter *it;

	r = mtbl_reader_init(fname, NULL);
	if (r == NULL) {
		fprintf(stderr, "Error: mtbl_reader_init() on %s failed\n", fname);
		return (false);
	}

	it = mtbl_source_iter(mtbl_reader_source(r));
	while (mtbl_iter_next(it, &key, &len_key, &val, &len_val)) {
		if (silent)
			continue;
		print_string(key, len_key, stdout);
		fputc(' ', stdout);
		print_string(val, len_val, stdout);
		fputc('\n', stdout);
	}
	
	mtbl_iter_destroy(&it);
	mtbl_reader_destroy(&r);

	return (true);
}

static void
usage(void)
{
	fprintf(stderr, "Usage: mtbl_dump [-s] <MTBL FILE>\n");
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	char *fname;
	bool silent = false;

	int c;
	while ((c = getopt(argc, argv, "s")) != -1) {
		switch (c) {
		case 's':
			silent = true;
			break;
		default:
			usage();
		}
	}
	if (optind >= argc)
		usage();
	fname = argv[optind];

	if (!dump(fname, silent))
		return (EXIT_FAILURE);
	return (EXIT_SUCCESS);
}
