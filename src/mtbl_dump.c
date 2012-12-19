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

#include <stdio.h>
#include <stdlib.h>

#include <mtbl.h>

#include "librsf/print_string.h"

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
