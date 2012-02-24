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

#include "mtbl-private.h"

struct mtbl_source {
	mtbl_source_iter_func		source_iter;
	mtbl_source_get_func		source_get;
	mtbl_source_get_prefix_func	source_get_prefix;
	mtbl_source_get_range_func	source_get_range;
	mtbl_source_free_func		source_free;
	void				*clos;
};

struct mtbl_source *
mtbl_source_init(mtbl_source_iter_func source_iter,
		 mtbl_source_get_func source_get,
		 mtbl_source_get_prefix_func source_get_prefix,
		 mtbl_source_get_range_func source_get_range,
		 mtbl_source_free_func source_free,
		 void *clos)
{
	assert(source_iter != NULL);
	assert(source_get != NULL);
	assert(source_get_prefix != NULL);
	assert(source_get_range != NULL);
	struct mtbl_source *s = my_calloc(1, sizeof(*s));
	s->source_iter = source_iter;
	s->source_get = source_get;
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
mtbl_source_iter(const struct mtbl_source *s)
{
	return (s->source_iter(s->clos));
}

struct mtbl_iter *
mtbl_source_get(const struct mtbl_source *s,
		const uint8_t *key, size_t len_key)
{
	return (s->source_get(s->clos, key, len_key));
}

struct mtbl_iter *
mtbl_source_get_prefix(const struct mtbl_source *s,
		       const uint8_t *key, size_t len_key)
{
	return (s->source_get_prefix(s->clos, key, len_key));
}

struct mtbl_iter *
mtbl_source_get_range(const struct mtbl_source *s,
		      const uint8_t *key0, size_t len_key0,
		      const uint8_t *key1, size_t len_key1)
{
	return (s->source_get_range(s->clos, key0, len_key0, key1, len_key1));
}

mtbl_res
mtbl_source_write(const struct mtbl_source *s, struct mtbl_writer *w)
{
	const uint8_t *key, *val;
	size_t len_key, len_val;
	struct mtbl_iter *it = mtbl_source_iter(s);
	mtbl_res res = mtbl_res_success;

	if (it == NULL)
		return (mtbl_res_failure);
	while (mtbl_iter_next(it, &key, &len_key, &val, &len_val) == mtbl_res_success) {
		res = mtbl_writer_add(w, key, len_key, val, len_val);
		if (res != mtbl_res_success)
			break;
	}
	mtbl_iter_destroy(&it);
	return (res);
}
