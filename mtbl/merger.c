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
	struct mtbl_iter		*it;
	ubuf				*key;
	ubuf				*val;
};

VECTOR_GENERATE(entry_vec, struct entry *);

VECTOR_GENERATE(reader_vec, struct mtbl_reader *);

struct merger_iter {
	struct mtbl_merger		*m;
	struct heap			*h;
	entry_vec			*entries;
	ubuf				*cur_key;
	ubuf				*cur_val;
	bool				finished;
};

struct mtbl_merger_options {
	mtbl_merge_func			merge;
	void				*merge_clos;
};

struct mtbl_merger {
	reader_vec			*readers;
	struct mtbl_merger_options	opt;
	struct mtbl_source		*source;
};

static struct mtbl_iter *
merger_get_prefix(void *, const uint8_t *, size_t);

static struct mtbl_iter *
merger_get_range(void *, const uint8_t *, size_t, const uint8_t *, size_t);

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
	m->readers = reader_vec_init(0);
	assert(opt != NULL);
	assert(opt->merge != NULL);
	memcpy(&m->opt, opt, sizeof(*opt));
	m->source = mtbl_source_init(merger_get_prefix, merger_get_range, NULL, m);

	return (m);
}

void
mtbl_merger_destroy(struct mtbl_merger **m)
{
	if (*m) {
		reader_vec_destroy(&(*m)->readers);
		mtbl_source_destroy(&(*m)->source);
		free(*m);
		*m = NULL;
	}
}

struct mtbl_source *
mtbl_merger_source(struct mtbl_merger *m)
{
	return (m->source);
}

void
mtbl_merger_add_reader(struct mtbl_merger *m, struct mtbl_reader *r)
{
	reader_vec_add(m->readers, r);
}

static int
_mtbl_merger_compare(const void *va, const void *vb)
{
	const struct entry *a = (const struct entry *) va;
	const struct entry *b = (const struct entry *) vb;

	if (a->key == NULL && b->key == NULL)
		return (0);
	if (a->key == NULL)
		return (1);
	if (b->key == NULL)
		return (-1);

	return (bytes_compare(ubuf_data(a->key), ubuf_size(a->key),
			      ubuf_data(b->key), ubuf_size(b->key)));
}

static mtbl_res
entry_fill(struct entry *ent)
{
	assert(ent->it != NULL);

	const uint8_t *key, *val;
	size_t len_key, len_val;
	mtbl_res res;

	ubuf_clip(ent->key, 0);
	ubuf_clip(ent->val, 0);
	res = mtbl_iter_next(ent->it, &key, &len_key, &val, &len_val);
	if (res == mtbl_res_success) {
		ubuf_append(ent->key, key, len_key);
		ubuf_append(ent->val, val, len_val);
	} else {
		mtbl_iter_destroy(&ent->it);
	}
	return (res);
}

mtbl_res
mtbl_merger_write(struct mtbl_merger *m, struct mtbl_writer *w)
{
	struct mtbl_iter *it = mtbl_merger_iter(m);
	const uint8_t *key, *val;
	size_t len_key, len_val;

	while (mtbl_iter_next(it, &key, &len_key, &val, &len_val))
		mtbl_writer_add(w, key, len_key, val, len_val);
	mtbl_iter_destroy(&it);
	return (mtbl_res_success);
}

static mtbl_res
merger_iter_next(void *v,
		 const uint8_t **out_key, size_t *out_len_key,
		 const uint8_t **out_val, size_t *out_len_val)
{
	struct merger_iter *it = (struct merger_iter *) v;
	struct entry *e;
	mtbl_res res;

	if (it->finished)
		return (mtbl_res_failure);

	ubuf_clip(it->cur_key, 0);
	ubuf_clip(it->cur_val, 0);

	for (;;) {
		for (;;) {
			e = heap_peek(it->h);
			if (e == NULL) {
				it->finished = true;
				break;
			}
			if (e->it == NULL)
				heap_pop(it->h);
			else
				break;
		}

		if (it->finished)
			break;

		if (ubuf_size(it->cur_key) == 0) {
			ubuf_clip(it->cur_val, 0);
			ubuf_append(it->cur_key, ubuf_data(e->key), ubuf_size(e->key));
			ubuf_append(it->cur_val, ubuf_data(e->val), ubuf_size(e->val));
			res = entry_fill(e);
			if (res == mtbl_res_success)
				heap_replace(it->h, e);
			continue;
		}

		if (bytes_compare(ubuf_data(it->cur_key), ubuf_size(it->cur_key),
				  ubuf_data(e->key), ubuf_size(e->key)) == 0)
		{
			uint8_t *merged_val = NULL;
			size_t len_merged_val = 0;
			it->m->opt.merge(it->m->opt.merge_clos,
					 ubuf_data(it->cur_key), ubuf_size(it->cur_key),
					 ubuf_data(it->cur_val), ubuf_size(it->cur_val),
					 ubuf_data(e->val), ubuf_size(e->val),
					 &merged_val, &len_merged_val);
			if (merged_val == NULL)
				return (mtbl_res_failure);
			ubuf_clip(it->cur_val, 0);
			ubuf_append(it->cur_val, merged_val, len_merged_val);
			free(merged_val);
			res = entry_fill(e);
			if (res == mtbl_res_success)
				heap_replace(it->h, e);
		} else {
			break;
		}
	}

	*out_key = ubuf_data(it->cur_key);
	*out_val = ubuf_data(it->cur_val);
	*out_len_key = ubuf_size(it->cur_key);
	*out_len_val = ubuf_size(it->cur_val);

	return (mtbl_res_success);
}

static void
merger_iter_free(void *v)
{
	struct merger_iter *it = (struct merger_iter *) v;
	if (it != NULL) {
		heap_destroy(&it->h);
		for (size_t i = 0; i < entry_vec_size(it->entries); i++) {
			struct entry *ent = entry_vec_value(it->entries, i);
			ubuf_destroy(&ent->key);
			ubuf_destroy(&ent->val);
			mtbl_iter_destroy(&ent->it);
			free(ent);
		}
		entry_vec_destroy(&it->entries);
		ubuf_destroy(&it->cur_key);
		ubuf_destroy(&it->cur_val);
		free(it);
	}
}

static struct merger_iter *
merger_iter_init(struct mtbl_merger *m)
{
	struct merger_iter *it = my_calloc(1, sizeof(*it));
	it->m = m;
	it->h = heap_init(_mtbl_merger_compare);
	it->entries = entry_vec_init(reader_vec_size(m->readers));
	it->cur_key = ubuf_init(256);
	it->cur_val = ubuf_init(256);
	return (it);
}

static void
merger_iter_add_entry(struct merger_iter *it, struct mtbl_iter *ent_it)
{
	struct entry *ent = my_calloc(1, sizeof(*ent));
	ent->key = ubuf_init(256);
	ent->val = ubuf_init(256);
	ent->it = ent_it;
	entry_fill(ent);
	heap_push(it->h, ent);
	entry_vec_add(it->entries, ent);
}

struct mtbl_iter *
mtbl_merger_iter(struct mtbl_merger *m)
{
	struct merger_iter *it = merger_iter_init(m);
	for (size_t i = 0; i < reader_vec_size(m->readers); i++) {
		struct mtbl_reader *r = reader_vec_value(m->readers, i);
		merger_iter_add_entry(it, mtbl_reader_iter(r));
	}
	return (mtbl_iter_init(merger_iter_next, merger_iter_free, it));
}

struct mtbl_iter *
mtbl_merger_get(struct mtbl_merger *m, const uint8_t *key, size_t len_key)
{
	struct merger_iter *it = merger_iter_init(m);
	for (size_t i = 0; i < reader_vec_size(m->readers); i++) {
		struct mtbl_reader *r = reader_vec_value(m->readers, i);
		struct mtbl_iter *r_it = mtbl_source_get_range(mtbl_reader_source(r),
							       key, len_key, key, len_key);
		if (r_it != NULL)
			merger_iter_add_entry(it, r_it);
	}
	if (entry_vec_size(it->entries) == 0) {
		merger_iter_free(it);
		return (NULL);
	}
	return (mtbl_iter_init(merger_iter_next, merger_iter_free, it));
}

static struct mtbl_iter *
merger_get_range(void *clos,
		 const uint8_t *key0, size_t len_key0,
		 const uint8_t *key1, size_t len_key1)
{
	struct mtbl_merger *m = (struct mtbl_merger *) clos;;
	struct merger_iter *it = merger_iter_init(m);
	for (size_t i = 0; i < reader_vec_size(m->readers); i++) {
		struct mtbl_reader *r = reader_vec_value(m->readers, i);
		struct mtbl_iter *r_it = mtbl_source_get_range(mtbl_reader_source(r),
							       key0, len_key0, key1, len_key1);
		if (r_it != NULL)
			merger_iter_add_entry(it, r_it);
	}
	if (entry_vec_size(it->entries) == 0) {
		merger_iter_free(it);
		return (NULL);
	}
	return (mtbl_iter_init(merger_iter_next, merger_iter_free, it));
}

static struct mtbl_iter *
merger_get_prefix(void *clos,
		  const uint8_t *key, size_t len_key)
{
	struct mtbl_merger *m = (struct mtbl_merger *) clos;
	struct merger_iter *it = merger_iter_init(m);
	for (size_t i = 0; i < reader_vec_size(m->readers); i++) {
		struct mtbl_reader *r = reader_vec_value(m->readers, i);
		struct mtbl_iter *r_it = mtbl_source_get_prefix(mtbl_reader_source(r),
								key, len_key);
		if (r_it != NULL)
			merger_iter_add_entry(it, r_it);
	}
	if (entry_vec_size(it->entries) == 0) {
		merger_iter_free(it);
		return (NULL);
	}
	return (mtbl_iter_init(merger_iter_next, merger_iter_free, it));
}
