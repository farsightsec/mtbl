/*
 * Copyright (c) 2022 DomainTools LLC
 * Copyright (c) 2012-2016, 2019 by Farsight Security, Inc.
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

#include "mtbl-private.h"

#include "libmy/heap.h"
#include "libmy/ubuf.h"

struct entry {
	bool                            finished;
	struct mtbl_iter		*it;
	const uint8_t			*key, *val;
	size_t				len_key, len_val;
};

VECTOR_GENERATE(entry_vec, struct entry *);

VECTOR_GENERATE(iter_vec, struct mtbl_iter *);

VECTOR_GENERATE(source_vec, const struct mtbl_source *);

struct merger_iter {
	struct mtbl_merger		*m;
	struct heap			*h;	/* entries sorted by key/val */
	entry_vec			*entries;
	/* iters is maintained solely for resource management purposes */
	iter_vec                        *iters;
	ubuf				*cur_key;
	ubuf				*cur_val;
	bool				finished;
	bool				pending;
};

struct mtbl_merger_options {
	mtbl_merge_func			merge;
	void				*merge_clos;
	mtbl_dupsort_func		dupsort;
	void				*dupsort_clos;
};

struct mtbl_merger {
	source_vec			*sources;
	struct mtbl_source		*source;
	struct mtbl_merger_options	opt;
};

static struct mtbl_iter *
merger_iter(void *);

static struct mtbl_iter *
merger_get(void *, const uint8_t *, size_t);

static struct mtbl_iter *
merger_get_prefix(void *, const uint8_t *, size_t);

static struct mtbl_iter *
merger_get_range(void *, const uint8_t *, size_t, const uint8_t *, size_t);

static mtbl_res
merger_iter_next(void *, const uint8_t **, size_t *, const uint8_t **, size_t *);

static void
merger_iter_add_entry(struct merger_iter *it, struct mtbl_iter *ent_it);

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

void
mtbl_merger_options_set_dupsort_func(struct mtbl_merger_options *opt,
				   mtbl_dupsort_func dupsort, void *clos)
{
	opt->dupsort = dupsort;
	opt->dupsort_clos = clos;
}

struct mtbl_merger *
mtbl_merger_init(const struct mtbl_merger_options *opt)
{
	struct mtbl_merger *m;

	m = my_calloc(1, sizeof(*m));
	m->sources = source_vec_init(0);
	assert(opt != NULL);
	memcpy(&m->opt, opt, sizeof(*opt));
	m->source = mtbl_source_init(merger_iter,
				     merger_get,
				     merger_get_prefix,
				     merger_get_range,
				     NULL, m);
	return (m);
}

void
mtbl_merger_destroy(struct mtbl_merger **m)
{
	if (*m) {
		source_vec_destroy(&(*m)->sources);
		mtbl_source_destroy(&(*m)->source);
		free(*m);
		*m = NULL;
	}
}

const struct mtbl_source *
mtbl_merger_source(struct mtbl_merger *m)
{
	return (m->source);
}

void
mtbl_merger_add_source(struct mtbl_merger *m, const struct mtbl_source *s)
{
	source_vec_add(m->sources, s);
}

static int
_mtbl_merger_compare(const void *va, const void *vb, void *clos)
{
	const struct entry *a = (const struct entry *) va;
	const struct entry *b = (const struct entry *) vb;
	const struct mtbl_merger *m = (const struct mtbl_merger *)clos;
	int res;

	if (a->key == NULL && b->key == NULL)
		return (0);
	if (a->key == NULL)
		return (1);
	if (b->key == NULL)
		return (-1);

	res = bytes_compare(a->key, a->len_key,
			    b->key, b->len_key);

	if ((res == 0) && (m->opt.dupsort != NULL))
		res = m->opt.dupsort(m->opt.dupsort_clos,
				     a->key, a->len_key,
				     a->val, a->len_val,
				     b->val, b->len_val);

	return res;
}

static mtbl_res
entry_fill(struct entry *ent)
{
	mtbl_res res;

	res = mtbl_iter_next(ent->it, &ent->key, &ent->len_key,
				      &ent->val, &ent->len_val);
	ent->finished = (res != mtbl_res_success);
	return (res);
}

static mtbl_res
merger_iter_seek(void *v, const uint8_t *key, size_t len_key)
{
	struct merger_iter *it = (struct merger_iter *) v;
	struct entry *e;
	bool changed = false;
	mtbl_res res;

	it->finished = false;
	it->pending = false;

	e = heap_peek(it->h);

	/*
	 * If we are seeking backwards from our current key or the end of
	 * the iterator (e == NULL), seek all entries to the desired key
	 * and rebuild the heap.
	 */
	if (e == NULL || ubuf_size(it->cur_key) == 0 ||
	    bytes_compare(key, len_key, ubuf_data(it->cur_key), ubuf_size(it->cur_key)) < 0) {
		heap_clip(it->h, 0);
		for (size_t i = 0; i < entry_vec_size(it->entries); i++) {
			struct entry *ent = entry_vec_value(it->entries, i);
			res = mtbl_iter_seek(ent->it, key, len_key);
			if (res != mtbl_res_success)
				continue;
			res = entry_fill(ent);
			if (res != mtbl_res_success)
				continue;
			heap_add(it->h, ent);
		}
		heap_heapify(it->h);
		return (mtbl_res_success);
	}

	/*
	 * Otherwise, we are seeking forward from our current key, and only
	 * need to seek those entries which are behind the desired key.
	 */
	while (bytes_compare(key, len_key, e->key, e->len_key) > 0) {
		changed = true;
		res = mtbl_iter_seek(e->it, key, len_key);
		if (res == mtbl_res_success && entry_fill(e) == mtbl_res_success) {
			heap_replace(it->h, e);
			e = heap_peek(it->h);
		} else {
			heap_pop(it->h);
			e = heap_peek(it->h);
			if (e == NULL) {
				it->finished = true;
				break;
			}
		}
	}

	/*
	 * If the seek operation changed the internal state of the iterator,
	 * record the seek key for the purposes of detecting a backward seek.
	 */
	if (changed) {
		ubuf_clip(it->cur_val, 0);
		ubuf_clip(it->cur_key, 0);
		ubuf_append(it->cur_key, key, len_key);
	}

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
			if (e->finished)
				heap_pop(it->h);
			else
				break;
		}

		if (it->finished)
			break;

		if (ubuf_size(it->cur_key) == 0) {
			ubuf_clip(it->cur_val, 0);
			ubuf_append(it->cur_key, e->key, e->len_key);
			ubuf_append(it->cur_val, e->val, e->len_val);
			it->pending = true;
			res = entry_fill(e);
			if (res == mtbl_res_success)
				heap_replace(it->h, e);
			continue;
		}

		if (it->m->opt.merge == NULL)
			break;

		if (bytes_compare(ubuf_data(it->cur_key), ubuf_size(it->cur_key),
				  e->key, e->len_key) == 0)
		{
			uint8_t *merged_val = NULL;
			size_t len_merged_val = 0;
			it->m->opt.merge(it->m->opt.merge_clos,
					 ubuf_data(it->cur_key), ubuf_size(it->cur_key),
					 ubuf_data(it->cur_val), ubuf_size(it->cur_val),
					 e->val, e->len_val, &merged_val, &len_merged_val);
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

	if (it->pending) {
		it->pending = false;
		*out_key = ubuf_data(it->cur_key);
		*out_val = ubuf_data(it->cur_val);
		*out_len_key = ubuf_size(it->cur_key);
		*out_len_val = ubuf_size(it->cur_val);
		return (mtbl_res_success);
	} else {
		return (mtbl_res_failure);
	}
}

static void
merger_iter_free(void *v)
{
	struct merger_iter *it = (struct merger_iter *) v;
	if (it != NULL) {
		heap_destroy(&it->h);
		for (size_t i = 0; i < entry_vec_size(it->entries); i++) {
			struct entry *ent = entry_vec_value(it->entries, i);
			free(ent);
		}
		entry_vec_destroy(&it->entries);
		for (size_t i = 0; i < iter_vec_size(it->iters); i++) {
			struct mtbl_iter *iter = iter_vec_value(it->iters, i);
			mtbl_iter_destroy(&iter);
		}
		iter_vec_destroy(&it->iters);
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
	it->h = heap_init(_mtbl_merger_compare, m);
	it->entries = entry_vec_init(source_vec_size(m->sources));
	it->iters = iter_vec_init(source_vec_size(m->sources));
	it->cur_key = ubuf_init(256);
	it->cur_val = ubuf_init(256);
	return (it);
}

static void
merger_iter_add_entry(struct merger_iter *it, struct mtbl_iter *ent_it)
{
	struct entry *ent = my_calloc(1, sizeof(*ent));
	ent->it = ent_it;
	ent->finished = false;
	mtbl_res res = entry_fill(ent);
	if (res != mtbl_res_success) {
		free(ent);
	} else {
		heap_push(it->h, ent);
		entry_vec_add(it->entries, ent);
	}
}

static struct mtbl_iter *
merger_iter(void *clos)
{
	struct mtbl_merger *m = (struct mtbl_merger *) clos;
	struct merger_iter *it = merger_iter_init(m);
	for (size_t i = 0; i < source_vec_size(m->sources); i++) {
		const struct mtbl_source *s = source_vec_value(m->sources, i);
		struct mtbl_iter *s_it = mtbl_source_iter(s);
		iter_vec_add(it->iters, s_it);
		merger_iter_add_entry(it, s_it);
	}
	return (mtbl_iter_init(merger_iter_seek, merger_iter_next, merger_iter_free, it));
}

static struct mtbl_iter *
merger_get(void *clos, const uint8_t *key, size_t len_key)
{
	struct mtbl_merger *m = (struct mtbl_merger *) clos;
	struct merger_iter *it = merger_iter_init(m);
	for (size_t i = 0; i < source_vec_size(m->sources); i++) {
		const struct mtbl_source *s = source_vec_value(m->sources, i);
		struct mtbl_iter *s_it = mtbl_source_get_range(s, key, len_key, key, len_key);
		if (s_it != NULL) {
			iter_vec_add(it->iters, s_it);
			merger_iter_add_entry(it, s_it);
		}
	}
	if (entry_vec_size(it->entries) == 0) {
		merger_iter_free(it);
		return (NULL);
	}
	return (mtbl_iter_init(merger_iter_seek, merger_iter_next, merger_iter_free, it));
}

static struct mtbl_iter *
merger_get_range(void *clos,
		 const uint8_t *key0, size_t len_key0,
		 const uint8_t *key1, size_t len_key1)
{
	struct mtbl_merger *m = (struct mtbl_merger *) clos;
	struct merger_iter *it = merger_iter_init(m);
	for (size_t i = 0; i < source_vec_size(m->sources); i++) {
		const struct mtbl_source *s = source_vec_value(m->sources, i);
		struct mtbl_iter *s_it = mtbl_source_get_range(s, key0, len_key0, key1, len_key1);
		if (s_it != NULL) {
			iter_vec_add(it->iters, s_it);
			merger_iter_add_entry(it, s_it);
		}
	}
	if (entry_vec_size(it->entries) == 0) {
		merger_iter_free(it);
		return (NULL);
	}
	return (mtbl_iter_init(merger_iter_seek, merger_iter_next, merger_iter_free, it));
}

static struct mtbl_iter *
merger_get_prefix(void *clos,
		  const uint8_t *key, size_t len_key)
{
	struct mtbl_merger *m = (struct mtbl_merger *) clos;
	struct merger_iter *it = merger_iter_init(m);
	for (size_t i = 0; i < source_vec_size(m->sources); i++) {
		const struct mtbl_source *s = source_vec_value(m->sources, i);
		struct mtbl_iter *s_it = mtbl_source_get_prefix(s, key, len_key);
		if (s_it != NULL) {
			iter_vec_add(it->iters, s_it);
			merger_iter_add_entry(it, s_it);
		}
	}
	if (entry_vec_size(it->entries) == 0) {
		merger_iter_free(it);
		return (NULL);
	}
	return (mtbl_iter_init(merger_iter_seek, merger_iter_next, merger_iter_free, it));
}
