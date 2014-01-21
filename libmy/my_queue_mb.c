/*
 * Copyright (c) 2013, 2014 by Farsight Security, Inc.
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

#include "my_memory_barrier.h"

#ifdef MY_HAVE_MEMORY_BARRIERS

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "my_alloc.h"

#include "my_queue.h"

#define MY_ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))

struct my_queue *
my_queue_mb_init(unsigned, unsigned);

void
my_queue_mb_destroy(struct my_queue **);

const char *
my_queue_mb_impl_type(void);

bool
my_queue_mb_insert(struct my_queue *, void *, unsigned *);

bool
my_queue_mb_remove(struct my_queue *, void *, unsigned *);

struct my_queue {
	uint8_t		*data;
	unsigned	num_elems;
	unsigned	sizeof_elem;
	unsigned	head;
	unsigned	tail;
};

struct my_queue *
my_queue_mb_init(unsigned num_elems, unsigned sizeof_elem)
{
	struct my_queue *q;
	if (num_elems < 2 || ((num_elems - 1) & num_elems) != 0)
		return (NULL);
	q = my_calloc(1, sizeof(*q));
	q->num_elems = num_elems;
	q->sizeof_elem = sizeof_elem;
	q->data = my_calloc(q->num_elems, q->sizeof_elem);
	return (q);
}

void
my_queue_mb_destroy(struct my_queue **q)
{
	if (*q) {
		free((*q)->data);
		free(*q);
		*q = NULL;
	}
}

const char *
my_queue_mb_impl_type(void)
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
my_queue_mb_insert(struct my_queue *q, void *item, unsigned *pspace)
{
	bool res = false;
	unsigned head = q->head;
	unsigned tail = MY_ACCESS_ONCE(q->tail);
	unsigned space = q_space(head, tail, q->num_elems);
	if (space >= 1) {
		memcpy(&q->data[head * q->sizeof_elem], item, q->sizeof_elem);
		smp_wmb();
		q->head = (head + 1) & (q->num_elems - 1);
		smp_wmb();
		res = true;
		space--;
	}
	if (pspace != NULL)
		*pspace = space;
	return (res);
}

bool
my_queue_mb_remove(struct my_queue *q, void *item, unsigned *pcount)
{
	bool res = false;
	unsigned head = MY_ACCESS_ONCE(q->head);
	unsigned tail = q->tail;
	unsigned count = q_count(head, tail, q->num_elems);
	if (count >= 1) {
		memcpy(item, &q->data[tail * q->sizeof_elem], q->sizeof_elem);
		smp_mb();
		q->tail = (tail + 1) & (q->num_elems - 1);
		res = true;
		count--;
	}
	if (pcount != NULL)
		*pcount = count;
	return (res);
}

const struct my_queue_ops my_queue_mb_ops = {
	.init =
		my_queue_mb_init,
	.destroy =
		my_queue_mb_destroy,
	.impl_type =
		my_queue_mb_impl_type,
	.insert =
		my_queue_mb_insert,
	.remove =
		my_queue_mb_remove,
};

#endif /* MY_HAVE_MEMORY_BARRIERS */
