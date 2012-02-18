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

struct mtbl_source {
	mtbl_source_get_prefix_func	source_get_prefix;
	mtbl_source_get_range_func	source_get_range;
	mtbl_source_free_func		source_free;
	void				*clos;
};

struct mtbl_source *
mtbl_source_init(mtbl_source_get_prefix_func source_get_prefix,
		 mtbl_source_get_range_func source_get_range,
		 mtbl_source_free_func source_free,
		 void *clos)
{
	assert(source_get_prefix != NULL);
	assert(source_get_range != NULL);
	struct mtbl_source *s = my_calloc(1, sizeof(*s));
	s->source_get_prefix = source_get_prefix;
	s->source_get_range = source_get_range;
	s->source_free = source_free;
	s->clos = clos;
	return (s);
}

void
mtbl_source_destroy(struct mtbl_source **s)
{
	if (*s) {
		if ((*s)->source_free != NULL)
			(*s)->source_free((*s)->clos);
		free(*s);
		*s = NULL;
	}
}

struct mtbl_iter *
mtbl_source_get_prefix(struct mtbl_source *s,
		       const uint8_t *key, size_t len_key)
{
	return (s->source_get_prefix(s->clos, key, len_key));
}

struct mtbl_iter *
mtbl_source_get_range(struct mtbl_source *s,
		      const uint8_t *key0, size_t len_key0,
		      const uint8_t *key1, size_t len_key1)
{
	return (s->source_get_range(s->clos, key0, len_key0, key1, len_key1));
}
