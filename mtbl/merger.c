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

struct mtbl_merger_options {
	mtbl_merge_func			merge;
	void				*merge_clos;
};

struct mtbl_merger {
	bool				iterating;
	bool				finished;
	size_t				n_open;
	entry_vec			*vec;
	ubuf				*cur_key;
	ubuf				*cur_val;

	struct mtbl_merger_options	opt;
};

struct mtbl_merger_options *
mtbl_merger_options_init(void)
{
	return (my_calloc(1, sizeof(struct mtbl_merger_options)));
}

void
mtbl_merger_options_destroy(struct mtbl_merger_options **opt)
{
	if (*opt) {
		free(*opt);
		*opt = NULL;
	}
}

void
mtbl_merger_options_set_merge_func(struct mtbl_merger_options *opt,
				   mtbl_merge_func merge, void *clos)
{
	opt->merge = merge;
	opt->merge_clos = clos;
}

struct mtbl_merger *
mtbl_merger_init(const struct mtbl_merger_options *opt)
{
	struct mtbl_merger *m;

	m = my_calloc(1, sizeof(*m));
	m->vec = entry_vec_init(0);
	m->cur_key = ubuf_init(256);
	m->cur_val = ubuf_init(256);
	assert(opt != NULL);
	assert(opt->merge != NULL);
	memcpy(&m->opt, opt, sizeof(*opt));

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
mtbl_merger_add_reader(struct mtbl_merger *m, struct mtbl_reader *r)
{
	assert(!m->finished && !m->iterating);
	struct entry *e = my_calloc(1, sizeof(*e));
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

void
mtbl_merger_write(struct mtbl_merger *m, struct mtbl_writer *w)
{
	assert(!m->finished && !m->iterating);
	struct mtbl_iter *it = mtbl_merger_iter(m);
	const uint8_t *key, *val;
	size_t len_key, len_val;

	while (mtbl_iter_next(it, &key, &len_key, &val, &len_val))
		mtbl_writer_add(w, key, len_key, val, len_val);

	mtbl_iter_destroy(&it);

	m->finished = true;
}

static bool
merger_iter_next(void *v,
		 const uint8_t **out_key, size_t *out_len_key,
		 const uint8_t **out_val, size_t *out_len_val)
{
	struct mtbl_merger *m = (struct mtbl_merger *) v;
	if (m->finished)
		return (false);

	ubuf_clip(m->cur_key, 0);
	ubuf_clip(m->cur_val, 0);

	for (;;) {
		for (unsigned i = 0; i < entry_vec_size(m->vec); i++) {
			struct entry *e = entry_vec_value(m->vec, i);
			const uint8_t *key, *val;
			size_t len_key, len_val;

			if (e->r == NULL)
				break;

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

		if (m->n_open == 0) {
			m->finished = true;
			break;
		}

		struct entry **array = entry_vec_data(m->vec);
		qsort(array, entry_vec_size(m->vec), sizeof(void *), _mtbl_merger_compare);

		struct entry *e = entry_vec_value(m->vec, 0);

		if (ubuf_size(m->cur_key) == 0) {
			ubuf_clip(m->cur_val, 0);
			ubuf_append(m->cur_key, ubuf_data(e->key), ubuf_size(e->key));
			ubuf_append(m->cur_val, ubuf_data(e->val), ubuf_size(e->val));
			ubuf_clip(e->key, 0);
			continue;
		}

		if (bytes_compare(ubuf_data(m->cur_key), ubuf_size(m->cur_key),
				  ubuf_data(e->key), ubuf_size(e->key)) == 0)
		{
			uint8_t *merged_val;
			size_t len_merged_val;
			m->opt.merge(m->opt.merge_clos,
				     ubuf_data(m->cur_key), ubuf_size(m->cur_key),
				     ubuf_data(m->cur_val), ubuf_size(m->cur_val),
				     ubuf_data(e->val), ubuf_size(e->val),
				     &merged_val, &len_merged_val);
			ubuf_clip(m->cur_val, 0);
			ubuf_append(m->cur_val, merged_val, len_merged_val);
			free(merged_val);
			ubuf_clip(e->key, 0);
		} else {
			break;
		}
	}

	*out_key = ubuf_data(m->cur_key);
	*out_val = ubuf_data(m->cur_val);
	*out_len_key = ubuf_size(m->cur_key);
	*out_len_val = ubuf_size(m->cur_val);
	return (true);
}

static void
merger_iter_free(void *v)
{
	return;
}

struct mtbl_iter *
mtbl_merger_iter(struct mtbl_merger *m)
{
	m->iterating = true;
	m->n_open = entry_vec_size(m->vec);
	return (mtbl_iter_init(merger_iter_next, merger_iter_free, m));
}
