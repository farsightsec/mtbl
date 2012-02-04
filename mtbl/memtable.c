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
#include "vector_types.h"

struct entry {
	uint32_t	len_key;
	uint32_t	len_val;
	uint8_t		data[];
};

#define entry_key(e) ((e)->data)
#define entry_val(e) ((e)->data + (e)->len_key)

VECTOR_GENERATE(entry_vec, struct entry *);

struct mtbl_memtable {
	entry_vec	*vec;
	bool		finished;
};

struct mtbl_memtable *
mtbl_memtable_init(void)
{
	struct mtbl_memtable *m;

	m = calloc(1, sizeof(*m));
	assert(m != NULL);
	m->vec = entry_vec_init(131072);

	return (m);
}

void
mtbl_memtable_destroy(struct mtbl_memtable **m)
{
	struct entry *ent;

	if (*m) {
		for (unsigned i = 0; i < entry_vec_size((*m)->vec); i++) {
			ent = entry_vec_value((*m)->vec, i);
			free(ent);
		}
		entry_vec_destroy(&((*m)->vec));
		free(*m);
		*m = NULL;
	}
}

size_t
mtbl_memtable_size(struct mtbl_memtable *m)
{
	return (entry_vec_size(m->vec));
}

static int
_mtbl_memtable_compare(const void *va, const void *vb)
{
	const struct entry *a = *((const struct entry **) va);
	const struct entry *b = *((const struct entry **) vb);

	size_t len = a->len_key > b->len_key ? b->len_key : a->len_key;
	return (memcmp(a->data, b->data, len));
}

void
mtbl_memtable_finish(struct mtbl_memtable *m)
{
	struct entry **array = entry_vec_data(m->vec);
	assert(!m->finished);
	qsort(array, entry_vec_size(m->vec), sizeof(void *), _mtbl_memtable_compare);
	m->finished = true;
}

void
mtbl_memtable_add(struct mtbl_memtable *m,
		  const uint8_t *key, size_t len_key,
		  const uint8_t *val, size_t len_val)
{
	struct entry *ent;

	assert(!m->finished);
	assert(len_key <= UINT_MAX);
	assert(len_val <= UINT_MAX);

	ent = malloc(sizeof(*ent) + len_key + len_val);
	assert(ent != NULL);
	ent->len_key = len_key;
	ent->len_val = len_val;
	memcpy(entry_key(ent), key, len_key);
	memcpy(entry_val(ent), val, len_val);
	entry_vec_append(m->vec, &ent, 1);
}

void
mtbl_memtable_get(struct mtbl_memtable *m, size_t idx,
		  uint8_t **key, size_t *len_key,
		  uint8_t **val, size_t *len_val)
{
	struct entry *ent;
	
	assert(idx < entry_vec_size(m->vec));
	ent = entry_vec_value(m->vec, idx);
	assert(ent != NULL);

	*key = entry_key(ent);
	*len_key = ent->len_key;

	if (val)
		*val = entry_val(ent);
	if (len_val)
		*len_val = ent->len_val;
}
