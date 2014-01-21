/*
 * Copyright (c) 2012 by Farsight Security, Inc.
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
