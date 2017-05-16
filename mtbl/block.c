/*
 * Copyright (c) 2012, 2014 by Farsight Security, Inc.
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

// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//
//    * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include "mtbl-private.h"

#include "libmy/ubuf.h"

struct block {
	uint8_t		*data;
	size_t		size;
	uint64_t	restart_offset;
	bool		needs_free;
};

struct block_iter {
	struct block	*block;
	uint8_t		*data;
	uint64_t	restarts;
	uint32_t	num_restarts;
	uint64_t	current;
	uint32_t	restart_index;
	uint8_t		*next;
	ubuf		*key;
	uint8_t		*val;
	uint32_t	val_len;
};

static inline uint32_t
num_restarts(struct block *b)
{
	assert(b->size >= 2*sizeof(uint32_t));
	return (mtbl_fixed_decode32(b->data + b->size - sizeof(uint32_t)));
}

static inline uint8_t *
decode_entry(uint8_t *p, uint8_t *limit,
	     uint32_t *shared, uint32_t *non_shared, uint32_t *value_length)
{
	if (limit - p < 3)
		return (NULL);
	*shared = p[0];
	*non_shared = p[1];
	*value_length = p[2];
	if ((*shared | *non_shared | *value_length) < 128) {
		/* fast path */
		p += 3;
	} else {
		p += mtbl_varint_decode32(p, shared);
		p += mtbl_varint_decode32(p, non_shared);
		p += mtbl_varint_decode32(p, value_length);
		assert (p <= limit);
	}
	assert(!((limit - p) < (*non_shared + *value_length)));
	return (p);
}

struct block *
block_init(uint8_t *data, size_t size, bool needs_free)
{
	struct block *b = my_calloc(1, sizeof(*b));
	b->data = data;
	b->size = size;
	if (size < sizeof(uint32_t)) {
		b->size = 0;
	} else {
		b->restart_offset = size - (1 + num_restarts(b)) * sizeof(uint32_t);
		if (b->restart_offset > size - sizeof(uint32_t)) {
			b->size = 0;
		}
	}
	b->needs_free = needs_free;
	return (b);
}

void
block_destroy(struct block **b)
{
	if (*b != NULL) {
		if ((*b)->needs_free)
			free((*b)->data);
		free(*b);
		*b = NULL;
	}
}

struct block_iter *
block_iter_init(struct block *b)
{
	assert(b->size >= 2 * sizeof(uint32_t));
	struct block_iter *bi = my_calloc(1, sizeof(*bi));
	bi->block = b;
	bi->data = b->data;
	bi->restarts = b->restart_offset;
	bi->num_restarts = num_restarts(b);
	bi->current = bi->restarts;
	bi->restart_index = bi->num_restarts;
	assert(bi->num_restarts > 0);
	bi->key = ubuf_init(64);
	return (bi);
}

void
block_iter_destroy(struct block_iter **bi)
{
	if (*bi != NULL) {
		ubuf_destroy(&(*bi)->key);
		free(*bi);
		*bi = NULL;
	}
}

static inline uint64_t
next_entry_offset(struct block_iter *bi)
{
	/* return the offset in ->data just past the end of the current entry */
	return (bi->next - bi->data);
}

static inline uint64_t
get_restart_point(struct block_iter *bi, uint32_t idx)
{
	assert(idx < bi->num_restarts);
	return (mtbl_fixed_decode32(bi->data + bi->restarts + idx * sizeof(uint32_t)));
}

static inline void
seek_to_restart_point(struct block_iter *bi, uint32_t idx)
{
	ubuf_reset(bi->key);
	bi->restart_index = idx;
	uint64_t offset = get_restart_point(bi, idx);
	bi->next = bi->data + offset;
}

static bool
parse_next_key(struct block_iter *bi)
{
	bi->current = next_entry_offset(bi);
	uint8_t *p = bi->data + bi->current;
	uint8_t *limit = bi->data + bi->restarts;
	if (p >= limit) {
		/* no more entries to return, mark as invalid */
		bi->current = bi->restarts;
		bi->restart_index = bi->num_restarts;
		return (false);
	}

	/* decode next entry */
	uint32_t shared, non_shared, value_length;
	p = decode_entry(p, limit, &shared, &non_shared, &value_length);
	assert(!(p == NULL || ubuf_size(bi->key) < shared));
	
	ubuf_clip(bi->key, shared);
	ubuf_append(bi->key, p, non_shared);
	bi->next = p + non_shared + value_length;
	bi->val = p + non_shared;
	bi->val_len = value_length;
	while (bi->restart_index + 1 < bi->num_restarts &&
	       get_restart_point(bi, bi->restart_index + 1) < bi->current)
	{
		bi->restart_index += 1;
	}
	return (true);
}

bool
block_iter_valid(const struct block_iter *bi)
{
	return (bi->current < bi->restarts);
}

void
block_iter_seek_to_first(struct block_iter *bi)
{
	seek_to_restart_point(bi, 0);
	parse_next_key(bi);
}

void 
block_iter_seek_to_last(struct block_iter *bi)
{
	seek_to_restart_point(bi, bi->num_restarts - 1);
	while (parse_next_key(bi) && next_entry_offset(bi) < bi->restarts) {
		/* keep skipping */
	}
}

void 
block_iter_seek(struct block_iter *bi, const uint8_t *target, size_t target_len)
{
	/* binary search in restart array to find the first restart point
	 * with a key >= target
	 */
	uint32_t left = 0;
	uint32_t right = bi->num_restarts - 1;
	while (left < right) {
		uint32_t mid = (left + right + 1) / 2;
		uint64_t region_offset = get_restart_point(bi, mid);
		uint32_t shared, non_shared, value_length;
		const uint8_t *key_ptr = decode_entry(bi->data + region_offset,
						      bi->data + bi->restarts,
						      &shared, &non_shared, &value_length);
		if (key_ptr == NULL || (shared != 0)) {
			/* corruption */
			return;
		}
		if (bytes_compare(key_ptr, non_shared, target, target_len) < 0) {
			/* key at "mid" is smaller than "target", therefore all
			 * keys before "mid" are uninteresting
			 */
			left = mid;
		} else {
			/* key at "mid" is larger than "target", therefore all
			 * keys at or before "mid" are uninteresting
			 */
			right = mid - 1;
		}
	}

	/* linear search within restart block for first key >= target */
	seek_to_restart_point(bi, left);
	for (;;) {
		if (!parse_next_key(bi))
			return;
		if (bytes_compare(ubuf_data(bi->key), ubuf_size(bi->key),
				       target, target_len) >= 0)
		{
			return;
		}
	}
}

bool
block_iter_next(struct block_iter *bi)
{
	if (!block_iter_valid(bi))
		return (false);
	parse_next_key(bi);
	return (block_iter_valid(bi));
}

void 
block_iter_prev(struct block_iter *bi)
{
	assert(block_iter_valid(bi));
	const uint64_t original = bi->current;
	while (get_restart_point(bi, bi->restart_index) >= original) {
		if (bi->restart_index == 0) {
			/* no more entries */
			bi->current = bi->restarts;
			bi->restart_index = bi->num_restarts;
			return;
		}
		bi->restart_index -= 1;
	}

	seek_to_restart_point(bi, bi->restart_index);
	do {
		/* loop until end of current entry hits the start of original entry */
	} while (parse_next_key(bi) && next_entry_offset(bi) < original);
}

bool
block_iter_get(struct block_iter *bi,
	       const uint8_t **key, size_t *key_len,
	       const uint8_t **val, size_t *val_len)
{
	if (!block_iter_valid(bi))
		return (false);
	if (key) {
		*key = ubuf_data(bi->key);
		*key_len = ubuf_size(bi->key);
	}
	if (val) {
		*val = bi->val;
		*val_len = bi->val_len;
	}
	return (true);
}
