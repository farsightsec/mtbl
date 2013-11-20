/*
 * Copyright (c) 2013 by Farsight Security, Inc.
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

#include <stdbool.h>

#include "my_alloc.h"
#include "my_memory_barrier.h"

#include "my_queue.h"

#ifndef MY_HAVE_MEMORY_BARRIERS
# error memory barrier implementation required
#endif

#define MY_ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))

struct my_queue {
	void		**elems;
	unsigned	size;
	unsigned	head;
	unsigned	tail;
};

struct my_queue *
my_queue_init(unsigned size)
{
	struct my_queue *q;
	if (size < 2 || ((size - 1) & size) != 0)
		return (NULL);
	q = my_calloc(1, sizeof(*q));
	q->size = size;
	q->elems = my_calloc(size, sizeof(void *));
	return (q);
}

void
my_queue_destroy(struct my_queue **q)
{
	if (*q) {
		free((*q)->elems);
		free(*q);
		*q = NULL;
	}
}

const char *
my_queue_impl_type(void)
{
	return ("memory barrier");
}

static inline unsigned
q_space(unsigned head, unsigned tail, unsigned size)
{
	return ((tail - (head + 1)) & (size - 1));
}

static inline unsigned
q_count(unsigned head, unsigned tail, unsigned size)
{
	return ((head - tail) & (size - 1));
}

bool
my_queue_insert(struct my_queue *q, void *item, unsigned *pspace)
{
	bool res = false;
	unsigned head = q->head;
	unsigned tail = MY_ACCESS_ONCE(q->tail);
	unsigned space = q_space(head, tail, q->size);
	if (space >= 1) {
		q->elems[head] = item;
		smp_wmb();
		q->head = (head + 1) & (q->size - 1);
		smp_wmb();
		res = true;
		space--;
	}
	if (pspace != NULL)
		*pspace = space;
	return (res);
}

bool
my_queue_remove(struct my_queue *q, void **pitem, unsigned *pcount)
{
	bool res = false;
	unsigned head = MY_ACCESS_ONCE(q->head);
	unsigned tail = q->tail;
	unsigned count = q_count(head, tail, q->size);
	if (count >= 1) {
		*pitem = q->elems[tail];
		smp_mb();
		q->tail = (tail + 1) & (q->size - 1);
		res = true;
		count--;
	}
	if (pcount != NULL)
		*pcount = count;
	return (res);
}
