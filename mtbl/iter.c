/*
 * Copyright (c) 2012 by Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.	IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "mtbl-private.h"

struct mtbl_iter {
	iter_next_func	iter_next;
	iter_free_func	iter_free;
	void		*clos;
};

struct mtbl_iter *
iter_init(iter_next_func iter_next, iter_free_func iter_free, void *clos)
{
	assert(iter_next != NULL);
	assert(iter_free != NULL);
	struct mtbl_iter *it = calloc(1, sizeof(*it));
	assert(it != NULL);
	it->iter_next = iter_next;
	it->iter_free = iter_free;
	it->clos = clos;
	return (it);
}

void
mtbl_iter_destroy(struct mtbl_iter **it)
{
	if (*it) {
		(*it)->iter_free((*it)->clos);
		free(*it);
		*it = NULL;
	}
}

bool
mtbl_iter_next(struct mtbl_iter *it,
	       const uint8_t **key, size_t *len_key,
	       const uint8_t **val, size_t *len_val)
{
	return (it->iter_next(it->clos, key, len_key, val, len_val));
}
