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

#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "my_alloc.h"

#include "my_queue.h"

#if defined(__GNUC__)
# define _aligned __attribute__((aligned(64)))
#else
# define _aligned
#endif

struct my_queue {
	uint8_t		*data;
	unsigned	num_elems;
	unsigned	sizeof_elem;
	unsigned	head;
	unsigned	tail;
	pthread_mutex_t	lock _aligned;
};

struct my_queue *
my_queue_mutex_init(unsigned, unsigned);

void
my_queue_mutex_destroy(struct my_queue **);

const char *
my_queue_mutex_impl_type(void);

bool
my_queue_mutex_insert(struct my_queue *, void *, unsigned *);

bool
my_queue_mutex_remove(struct my_queue *, void *, unsigned *);

struct my_queue *
my_queue_mutex_init(unsigned num_elems, unsigned sizeof_elem)
{
	struct my_queue *q;
	if (num_elems < 2 || ((num_elems - 1) & num_elems) != 0)
		return (NULL);
	q = my_calloc(1, sizeof(*q));
	q->num_elems = num_elems;
	q->sizeof_elem = sizeof_elem;
	q->data = my_calloc(q->num_elems, q->sizeof_elem);
	int rc = pthread_mutex_init(&q->lock, NULL);
	assert(rc == 0);
	return (q);
}

void
my_queue_mutex_destroy(struct my_queue **q)
{
	if (*q) {
		pthread_mutex_destroy(&(*q)->lock);
		free((*q)->data);
		free(*q);
		*q = NULL;
	}
}

const char *
my_queue_mutex_impl_type(void)
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
my_queue_mutex_insert(struct my_queue *q, void *item, unsigned *pspace)
{
	q_lock(q);
	bool res = false;
	unsigned head = q->head;
	unsigned tail = q->tail;
	unsigned space = q_space(head, tail, q->num_elems);
	if (space >= 1) {
		memcpy(&q->data[head * q->sizeof_elem], item, q->sizeof_elem);
		q->head = (head + 1) & (q->num_elems - 1);
		res = true;
		space--;
	}
	q_unlock(q);
	if (pspace)
		*pspace = space;
	return (res);
}

bool
my_queue_mutex_remove(struct my_queue *q, void *item, unsigned *pcount)
{
	q_lock(q);
	bool res = false;
	unsigned head = q->head;
	unsigned tail = q->tail;
	unsigned count = q_count(head, tail, q->num_elems);
	if (count >= 1) {
		memcpy(item, &q->data[tail * q->sizeof_elem], q->sizeof_elem);
		q->tail = (tail + 1) & (q->num_elems - 1);
		res = true;
		count--;
	}
	q_unlock(q);
	if (pcount)
		*pcount = count;
	return (res);
}

const struct my_queue_ops my_queue_mutex_ops = {
	.init =
		my_queue_mutex_init,
	.destroy =
		my_queue_mutex_destroy,
	.impl_type =
		my_queue_mutex_impl_type,
	.insert =
		my_queue_mutex_insert,
	.remove =
		my_queue_mutex_remove,
};
