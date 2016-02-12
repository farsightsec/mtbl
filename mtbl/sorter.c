/*
 * Copyright (c) 2012-2016 by Farsight Security, Inc.
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

#include "libmy/ubuf.h"

VECTOR_GENERATE(reader_vec, struct mtbl_reader *);

struct sorter_iter {
	reader_vec			*readers;
	struct mtbl_merger		*m;
	struct mtbl_iter		*m_iter;
};

struct entry {
	uint32_t			len_key;
	uint32_t			len_val;
	uint8_t				data[];
};

#define entry_key(e) ((e)->data)
#define entry_val(e) ((e)->data + (e)->len_key)

VECTOR_GENERATE(entry_vec, struct entry *);

struct chunk {
	int				fd;
};

VECTOR_GENERATE(chunk_vec, struct chunk *);

struct mtbl_sorter_options {
	size_t				max_memory;
	char				*tmp_dname;
	mtbl_merge_func			merge;
	void				*merge_clos;
};

struct mtbl_sorter {
	chunk_vec			*chunks;
	entry_vec			*vec;
	size_t				entry_bytes;
	bool				iterating;

	struct mtbl_sorter_options	opt;
};

struct mtbl_sorter_options *
mtbl_sorter_options_init(void)
{
	struct mtbl_sorter_options *opt;
	opt = my_calloc(1, sizeof(*opt));
	opt->max_memory = DEFAULT_SORTER_MEMORY;
	mtbl_sorter_options_set_temp_dir(opt, DEFAULT_SORTER_TEMP_DIR);
	return (opt);
}

void
mtbl_sorter_options_destroy(struct mtbl_sorter_options **opt)
{
	if (*opt) {
		free((*opt)->tmp_dname);
		free(*opt);
		*opt = NULL;
	}
}

void
mtbl_sorter_options_set_merge_func(struct mtbl_sorter_options *opt,
				   mtbl_merge_func merge, void *clos)
{
	opt->merge = merge;
	opt->merge_clos = clos;
}

void
mtbl_sorter_options_set_temp_dir(struct mtbl_sorter_options *opt,
				 const char *temp_dir)
{
	free(opt->tmp_dname);
	opt->tmp_dname = strdup(temp_dir);
}

void
mtbl_sorter_options_set_max_memory(struct mtbl_sorter_options *opt,
				   size_t max_memory)
{
	if (max_memory < MIN_SORTER_MEMORY)
		max_memory = MIN_SORTER_MEMORY;
	opt->max_memory = max_memory;
}

struct mtbl_sorter *
mtbl_sorter_init(const struct mtbl_sorter_options *opt)
{
	struct mtbl_sorter *s;

	s = my_calloc(1, sizeof(*s));
	if (opt != NULL) {
		memcpy(&s->opt, opt, sizeof(*opt));
		s->opt.tmp_dname = strdup(opt->tmp_dname);
	}
	s->vec = entry_vec_init(INITIAL_SORTER_VEC_SIZE);
	s->chunks = chunk_vec_init(1);

	return (s);
}

void
mtbl_sorter_destroy(struct mtbl_sorter **s)
{
	if (*s) {
		for (unsigned i = 0; i < entry_vec_size((*s)->vec); i++) {
			struct entry *ent = entry_vec_value((*s)->vec, i);
			free(ent);
		}
		entry_vec_destroy(&((*s)->vec));
		for (unsigned i = 0; i < chunk_vec_size((*s)->chunks); i++) {
			struct chunk *c = chunk_vec_value((*s)->chunks, i);
			close(c->fd);
			free(c);
		}
		chunk_vec_destroy(&((*s)->chunks));
		free((*s)->opt.tmp_dname);
		free(*s);
		*s = NULL;
	}
}

static int
_mtbl_sorter_compare(const void *va, const void *vb)
{
	const struct entry *a = *((const struct entry **) va);
	const struct entry *b = *((const struct entry **) vb);

	return (bytes_compare(entry_key(a), a->len_key,
			      entry_key(b), b->len_key));
}

static mtbl_res
_mtbl_sorter_write_chunk(struct mtbl_sorter *s)
{
	mtbl_res res = mtbl_res_success;
	assert(!s->iterating);
	struct chunk *c = my_calloc(1, sizeof(*c));

	char template[64];
	sprintf(template, "/.mtbl.%ld.XXXXXX", (long)getpid());
	ubuf *tmp_fname = ubuf_init(strlen(s->opt.tmp_dname) + strlen(template) + 1);
	ubuf_append(tmp_fname, (uint8_t *) s->opt.tmp_dname, strlen(s->opt.tmp_dname));
	ubuf_append(tmp_fname, (uint8_t *) template, strlen(template));
	ubuf_append(tmp_fname, (const uint8_t *) "\x00", 1);

	c->fd = mkstemp((char *) ubuf_data(tmp_fname));
	assert(c->fd >= 0);
	int unlink_ret = unlink((char *) ubuf_data(tmp_fname));
	assert(unlink_ret == 0);
	ubuf_destroy(&tmp_fname);

	struct mtbl_writer_options *wopt = mtbl_writer_options_init();
	mtbl_writer_options_set_compression(wopt, MTBL_COMPRESSION_SNAPPY);
	struct mtbl_writer *w = mtbl_writer_init_fd(c->fd, wopt);
	mtbl_writer_options_destroy(&wopt);

	size_t entries_written = 0;
	struct entry **array = entry_vec_data(s->vec);
	qsort(array, entry_vec_size(s->vec), sizeof(void *), _mtbl_sorter_compare);
	for (unsigned i = 0; i < entry_vec_size(s->vec); i++) {
		struct entry *ent = entry_vec_value(s->vec, i);

		if (i + 1 < entry_vec_size(s->vec)) {
			struct entry *next_ent = entry_vec_value(s->vec, i + 1);
			struct entry *merge_ent = NULL;

			if (_mtbl_sorter_compare(&ent, &next_ent) == 0) {
				assert(s->opt.merge != NULL);
				uint8_t *merge_val = NULL;
				size_t len_merge_val = 0;
				s->opt.merge(s->opt.merge_clos,
					     entry_key(ent), ent->len_key,
					     entry_val(ent), ent->len_val,
					     entry_val(next_ent), next_ent->len_val,
					     &merge_val, &len_merge_val);
				if (merge_val == NULL) {
					free(c);
					mtbl_writer_destroy(&w);
					return (mtbl_res_failure);
				}
				size_t len = sizeof(struct entry) + ent->len_key + len_merge_val;
				merge_ent = my_malloc(len);
				merge_ent->len_key = ent->len_key;
				merge_ent->len_val = len_merge_val;
				memcpy(entry_key(merge_ent), entry_key(ent), ent->len_key);
				memcpy(entry_val(merge_ent), merge_val, len_merge_val);
				free(merge_val);
				free(ent);
				free(next_ent);
				entry_vec_data(s->vec)[i + 1] = merge_ent;
				continue;
			}
		}
		res = mtbl_writer_add(w,
				      entry_key(ent), ent->len_key,
				      entry_val(ent), ent->len_val);
		free(ent);
		entries_written += 1;
		if (res != mtbl_res_success)
			break;
	}
	mtbl_writer_destroy(&w);
	entry_vec_destroy(&s->vec);
	s->vec = entry_vec_init(INITIAL_SORTER_VEC_SIZE);
	s->entry_bytes = 0;
	chunk_vec_add(s->chunks, c);
	return (res);
}

mtbl_res
mtbl_sorter_write(struct mtbl_sorter *s, struct mtbl_writer *w)
{
	if (s->iterating)
		return (mtbl_res_failure);
	struct mtbl_iter *it = mtbl_sorter_iter(s);
	const uint8_t *key, *val;
	size_t len_key, len_val;
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

mtbl_res
mtbl_sorter_add(struct mtbl_sorter *s,
		const uint8_t *key, size_t len_key,
		const uint8_t *val, size_t len_val)
{
	mtbl_res res = mtbl_res_success;
	if (s->iterating)
		return (mtbl_res_failure);
	assert(len_key <= UINT_MAX);
	assert(len_val <= UINT_MAX);

	struct entry *ent;
	size_t entry_bytes;

	entry_bytes = sizeof(*ent) + len_key + len_val;
	ent = my_malloc(entry_bytes);
	ent->len_key = len_key;
	ent->len_val = len_val;
	memcpy(entry_key(ent), key, len_key);
	memcpy(entry_val(ent), val, len_val);
	entry_vec_append(s->vec, &ent, 1);
	s->entry_bytes += entry_bytes;

	if (s->entry_bytes + entry_vec_bytes(s->vec) >= s->opt.max_memory)
		res = _mtbl_sorter_write_chunk(s);
	return (res);
}

static mtbl_res
sorter_iter_seek(void *v,
		 const uint8_t *key, size_t len_key)
{
	struct sorter_iter *it = (struct sorter_iter *) v;
	return (mtbl_iter_seek(it->m_iter, key, len_key));
}

static mtbl_res
sorter_iter_next(void *v,
		 const uint8_t **key, size_t *len_key,
		 const uint8_t **val, size_t *len_val)
{
	struct sorter_iter *it = (struct sorter_iter *) v;
	return (mtbl_iter_next(it->m_iter, key, len_key, val, len_val));
}

static void
sorter_iter_free(void *v)
{
	struct sorter_iter *it = (struct sorter_iter *) v;
	if (it) {
		mtbl_iter_destroy(&it->m_iter);
		mtbl_merger_destroy(&it->m);
		for (size_t i = 0; i < reader_vec_size(it->readers); i++) {
			struct mtbl_reader *r = reader_vec_value(it->readers, i);
			mtbl_reader_destroy(&r);
		}
		reader_vec_destroy(&it->readers);
		free(it);
	}
}

struct mtbl_iter *
mtbl_sorter_iter(struct mtbl_sorter *s)
{
	mtbl_res res;
	struct sorter_iter *it = my_calloc(1, sizeof(*it));
	it->readers = reader_vec_init(0);

	struct mtbl_merger_options *mopt = mtbl_merger_options_init();
	mtbl_merger_options_set_merge_func(mopt, s->opt.merge, s->opt.merge_clos);
	it->m = mtbl_merger_init(mopt);
	mtbl_merger_options_destroy(&mopt);

	if (entry_vec_size(s->vec) > 0) {
		res = _mtbl_sorter_write_chunk(s);
		if (res != mtbl_res_success) {
			sorter_iter_free(it);
			return (NULL);
		}
	}

	for (unsigned i = 0; i < chunk_vec_size(s->chunks); i++) {
		struct chunk *c = chunk_vec_value(s->chunks, i);
		struct mtbl_reader *r;
		r = mtbl_reader_init_fd(c->fd, NULL);
		mtbl_merger_add_source(it->m, mtbl_reader_source(r));
		reader_vec_add(it->readers, r);
	}

	it->m_iter = mtbl_source_iter(mtbl_merger_source(it->m));
	s->iterating = true;
	return (_mtbl_iter_init(sorter_iter_seek, sorter_iter_next, sorter_iter_free, it));
}
