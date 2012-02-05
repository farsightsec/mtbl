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
	struct mtbl_reader		*r;
	struct mtbl_iter		*it;
	ubuf				*key;
	ubuf				*val;
};

VECTOR_GENERATE(entry_vec, struct entry *);

struct mtbl_merger {
	bool				finished;
	size_t				n_open;
	entry_vec			*vec;
	struct mtbl_writer		*w;
	mtbl_merge_func			merge;
	void				*merge_clos;
	ubuf				*cur_key;
	ubuf				*cur_val;
};

struct mtbl_merger *
mtbl_merger_init(void)
{
	struct mtbl_merger *m;

	m = calloc(1, sizeof(*m));
	assert(m != NULL);
	m->vec = entry_vec_init(0);
	m->cur_key = ubuf_init(256);
	m->cur_val = ubuf_init(256);

	return (m);
}

void
mtbl_merger_destroy(struct mtbl_merger **m)
{
	if (*m) {
		for (unsigned i = 0; i < entry_vec_size((*m)->vec); i++) {
			struct entry *e = entry_vec_value((*m)->vec, i);
			free(e);
		}
		entry_vec_destroy(&(*m)->vec);
		ubuf_destroy(&(*m)->cur_key);
		ubuf_destroy(&(*m)->cur_val);
		free(*m);
		*m = NULL;
	}
}

void
mtbl_merger_add(struct mtbl_merger *m, struct mtbl_reader *r)
{
	assert(!m->finished);
	struct entry *e = calloc(1, sizeof(*e));
	assert(e != NULL);
	e->r = r;
	e->it = mtbl_reader_iter(r);
	assert(e->it != NULL);
	e->key = ubuf_init(256);
	e->val = ubuf_init(256);

	entry_vec_add(m->vec, e);
}

static int
_mtbl_merger_compare(const void *va, const void *vb)
{
	const struct entry *a = *((const struct entry **) va);
	const struct entry *b = *((const struct entry **) vb);

	if (a->key == NULL && b->key == NULL)
		return (0);
	if (a->key == NULL)
		return (1);
	if (b->key == NULL)
		return (-1);

	return (bytes_compare(ubuf_data(a->key), ubuf_size(a->key),
			      ubuf_data(b->key), ubuf_size(b->key)));
}

static void
do_fill(struct mtbl_merger *m)
{
	for (unsigned i = 0; i < entry_vec_size(m->vec); i++) {
		struct entry *e = entry_vec_value(m->vec, i);
		const uint8_t *key, *val;
		size_t len_key, len_val;

		if (e->r == NULL)
			continue;

		if (ubuf_size(e->key) != 0)
			continue;

		if (mtbl_iter_next(e->it,
				   &key, &len_key,
				   &val, &len_val))
		{
			ubuf_clip(e->val, 0);
			ubuf_append(e->key, key, len_key);
			ubuf_append(e->val, val, len_val);
		} else {
			m->n_open -= 1;
			mtbl_iter_destroy(&e->it);
			mtbl_reader_destroy(&e->r);
			ubuf_destroy(&e->key);
			ubuf_destroy(&e->val);
		}
	}
}

static void
do_output_cur(struct mtbl_merger *m)
{
	mtbl_writer_add(m->w,
			ubuf_data(m->cur_key), ubuf_size(m->cur_key),
			ubuf_data(m->cur_val), ubuf_size(m->cur_val));
	ubuf_clip(m->cur_key, 0);
	ubuf_clip(m->cur_val, 0);
}

static void
do_output(struct mtbl_merger *m)
{
	struct entry *e = entry_vec_value(m->vec, 0);
	assert(e != NULL);

	if (ubuf_size(m->cur_key) == 0) {
		ubuf_clip(m->cur_val, 0);
		ubuf_append(m->cur_key, ubuf_data(e->key), ubuf_size(e->key));
		ubuf_append(m->cur_val, ubuf_data(e->val), ubuf_size(e->val));
		ubuf_clip(e->key, 0);
		return;
	}

	if (bytes_compare(ubuf_data(m->cur_key), ubuf_size(m->cur_key),
			  ubuf_data(e->key), ubuf_size(e->key)) == 0)
	{
		uint8_t *merged_val;
		size_t len_merged_val;

		m->merge(m->merge_clos,
			 ubuf_data(m->cur_key), ubuf_size(m->cur_key),
			 ubuf_data(m->cur_val), ubuf_size(m->cur_val),
			 ubuf_data(e->val), ubuf_size(e->val),
			 &merged_val, &len_merged_val);
		ubuf_clip(m->cur_val, 0);
		ubuf_append(m->cur_val, merged_val, len_merged_val);
		free(merged_val);
		ubuf_clip(e->key, 0);
	} else {
		do_output_cur(m);
	}
}

void
mtbl_merger_merge(struct mtbl_merger *m,
		  struct mtbl_writer *w,
		  mtbl_merge_func merge_fp, void *clos)
{
	struct entry **array = entry_vec_data(m->vec);
	m->w = w;
	m->merge = merge_fp;
	m->merge_clos = clos;
	m->n_open = entry_vec_size(m->vec);

	for (;;) {
		do_fill(m);
		if (m->n_open == 0)
			break;
		qsort(array, entry_vec_size(m->vec), sizeof(void *), _mtbl_merger_compare);
		do_output(m);
	}

	do_output_cur(m);
	m->finished = true;
}
