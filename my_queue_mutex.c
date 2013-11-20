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

#include <assert.h>
#include <pthread.h>
#include <stdbool.h>

#include "my_alloc.h"

#include "my_queue.h"

#if defined(__GNUC__)
# define _aligned __attribute__((aligned(64)))
#else
# define _aligned
#endif

struct my_queue {
	void		**elems;
	unsigned	size;
	unsigned	head;
	unsigned	tail;
	pthread_mutex_t	lock _aligned;
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
	pthread_mutex_init(&q->lock, NULL);
	return (q);
}

void
my_queue_destroy(struct my_queue **q)
{
	if (*q) {
		pthread_mutex_destroy(&(*q)->lock);
		free((*q)->elems);
		free(*q);
		*q = NULL;
	}
}

const char *
my_queue_impl_type(void)
{
	return ("pthread mutex");
}

static inline void
q_lock(struct my_queue *q)
{
	int rc = pthread_mutex_lock(&q->lock);
	assert(rc == 0);
}

static inline void
q_unlock(struct my_queue *q)
{
	int rc = pthread_mutex_unlock(&q->lock);
	assert(rc == 0);
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
	q_lock(q);
	bool res = false;
	unsigned head = q->head;
	unsigned tail = q->tail;
	unsigned space = q_space(head, tail, q->size);
	if (space >= 1) {
		q->elems[head] = item;
		q->head = (head + 1) & (q->size - 1);
		res = true;
		space--;
	}
	q_unlock(q);
	if (pspace)
		*pspace = space;
	return (res);
}

bool
my_queue_remove(struct my_queue *q, void **pitem, unsigned *pcount)
{
	q_lock(q);
	bool res = false;
	unsigned head = q->head;
	unsigned tail = q->tail;
	unsigned count = q_count(head, tail, q->size);
	if (count >= 1) {
		*pitem = q->elems[tail];
		q->tail = (tail + 1) & (q->size - 1);
		res = true;
		count--;
	}
	q_unlock(q);
	if (pcount)
		*pcount = count;
	return (res);
}
