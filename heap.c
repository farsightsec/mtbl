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

#include <string.h>

#include "my_alloc.h"
#include "heap.h"
#include "vector.h"

VECTOR_GENERATE(ptrvec, void *);

struct heap {
	ptrvec			*vec;
	heap_compare_func	cmp;
};

static inline int
cmp_wrapper(heap_compare_func cmp, const void *a, const void *b)
{
	return ((cmp(a, b) < 0) ? 1 : 0);
}

struct heap *
heap_init(heap_compare_func cmp)
{
	struct heap *h = my_calloc(1, sizeof(*h));
	h->cmp = cmp;
	h->vec = ptrvec_init(1);
	return (h);
}

void
heap_destroy(struct heap **h)
{
	if (*h != NULL) {
		ptrvec_destroy(&(*h)->vec);
		free(*h);
		*h = NULL;
	}
}

void heap_reset(struct heap *h)
{
	ptrvec_reset(h->vec);
}

void heap_clip(struct heap *h, size_t n_elems)
{
	ptrvec_clip(h->vec, n_elems);
}

static int
siftdown(struct heap *h, size_t startpos, size_t pos)
{
	assert(pos < ptrvec_size(h->vec));
	void *newitem = ptrvec_value(h->vec, pos);
	while (pos > startpos) {
		size_t parentpos = (pos - 1) >> 1;
		void *parent = ptrvec_value(h->vec, parentpos);
		int cmp = cmp_wrapper(h->cmp, newitem, parent);
		if (cmp == -1)
			return (-1);
		if (cmp == 0)
			break;
		ptrvec_data(h->vec)[pos] = parent;
		pos = parentpos;
	}
	ptrvec_data(h->vec)[pos] = newitem;
	return (0);
}

static int
siftup(struct heap *h, size_t pos)
{
	assert(pos < ptrvec_size(h->vec));
	void *newitem = ptrvec_value(h->vec, pos);
	size_t endpos = ptrvec_size(h->vec);
	size_t startpos = pos;
	size_t childpos = 2 * pos + 1;
	while (childpos < endpos) {
		size_t rightpos = childpos + 1;
		if (rightpos < endpos) {
			int cmp = cmp_wrapper(h->cmp,
					      ptrvec_value(h->vec, childpos),
					      ptrvec_value(h->vec, rightpos));
			if (cmp == -1)
				return (-1);
			if (cmp == 0)
				childpos = rightpos;
		}
		ptrvec_data(h->vec)[pos] = ptrvec_value(h->vec, childpos);
		pos = childpos;
		childpos = 2 * pos + 1;
	}
	ptrvec_data(h->vec)[pos] = newitem;
	return (siftdown(h, startpos, pos));
}

void
heap_heapify(struct heap *h)
{
	for (size_t i = ptrvec_size(h->vec)/2; i-- > 0; ) {
		siftup(h, i);
	}
}

void
heap_add(struct heap *h, void *item)
{
	ptrvec_add(h->vec, item);
}

void
heap_push(struct heap *h, void *item)
{
	ptrvec_add(h->vec, item);
	siftdown(h, 0, ptrvec_size(h->vec) - 1);
}

void *
heap_pop(struct heap *h)
{
	if (ptrvec_size(h->vec) < 1)
		return (NULL);
	void *returnitem;
	void *lastelt = ptrvec_value(h->vec, ptrvec_size(h->vec) - 1);
	ptrvec_clip(h->vec, ptrvec_size(h->vec) - 1);
	if (ptrvec_size(h->vec) > 0) {
		returnitem = ptrvec_value(h->vec, 0);
		ptrvec_data(h->vec)[0] = lastelt;
		siftup(h, 0);
	} else {
		returnitem = lastelt;
	}
	return (returnitem);
}

void *
heap_replace(struct heap *h, void *item)
{
	if (ptrvec_size(h->vec) < 1)
		return (NULL);
	void *returnitem = ptrvec_value(h->vec, 0);
	ptrvec_data(h->vec)[0] = item;
	siftup(h, 0);
	return (returnitem);
}

void *
heap_peek(struct heap *h)
{
	if (ptrvec_size(h->vec) < 1)
		return (NULL);
	return ptrvec_data(h->vec)[0];
}

void *
heap_get(struct heap *h, size_t i)
{
	if (i > ptrvec_size(h->vec) - 1)
		return (NULL);
	return (ptrvec_value(h->vec, i));
}

size_t
heap_size(struct heap *h)
{
	return (ptrvec_size(h->vec));
}
