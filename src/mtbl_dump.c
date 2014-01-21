/*
 * Copyright (c) 2012, 2014 by Farsight Security, Inc.
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

#include <stdio.h>
#include <stdlib.h>

#include <mtbl.h>

#include "libmy/print_string.h"

static bool
dump(const char *fname)
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
		print_string(key, len_key, stdout);
		fputc(' ', stdout);
		print_string(val, len_val, stdout);
		fputc('\n', stdout);
	}
	
	mtbl_iter_destroy(&it);
	mtbl_reader_destroy(&r);

	return (true);
}

int
main(int argc, char **argv)
{
	char *fname;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <MTBL FILE>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	fname = argv[1];

	if (!dump(fname))
		return (EXIT_FAILURE);
	return (EXIT_SUCCESS);
}
