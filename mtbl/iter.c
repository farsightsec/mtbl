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

struct mtbl_iter {
	mtbl_iter_seek_func	iter_seek;
	mtbl_iter_next_func	iter_next;
	mtbl_iter_free_func	iter_free;
	void			*clos;
};

struct mtbl_iter *
_mtbl_iter_init(mtbl_iter_seek_func iter_seek,
		mtbl_iter_next_func iter_next,
		mtbl_iter_free_func iter_free,
		void *clos)
{
	assert(iter_seek != NULL);
	assert(iter_next != NULL);
	struct mtbl_iter *it = my_calloc(1, sizeof(*it));
	it->iter_seek = iter_seek;
	it->iter_next = iter_next;
	it->iter_free = iter_free;
	it->clos = clos;
	return (it);
}

void
mtbl_iter_destroy(struct mtbl_iter **it)
{
	if (*it) {
		if ((*it)->iter_free != NULL)
			(*it)->iter_free((*it)->clos);
		free(*it);
		*it = NULL;
	}
}

mtbl_res
mtbl_iter_seek(struct mtbl_iter *it,
	       const uint8_t *key, size_t len_key)
{
	if (it == NULL)
		return (mtbl_res_failure);
	return (it->iter_seek(it->clos, key, len_key));
}

mtbl_res
mtbl_iter_next(struct mtbl_iter *it,
	       const uint8_t **key, size_t *len_key,
	       const uint8_t **val, size_t *len_val)
{
	if (it == NULL)
		return (mtbl_res_failure);
	return (it->iter_next(it->clos, key, len_key, val, len_val));
}
