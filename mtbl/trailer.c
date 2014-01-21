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

void
trailer_write(struct trailer *t, uint8_t *buf)
{
	size_t padding;
	uint8_t *p = buf;

	p += mtbl_fixed_encode64(p, t->index_block_offset);
	p += mtbl_fixed_encode64(p, t->data_block_size);
	p += mtbl_fixed_encode64(p, t->compression_algorithm);
	p += mtbl_fixed_encode64(p, t->count_entries);
	p += mtbl_fixed_encode64(p, t->count_data_blocks);
	p += mtbl_fixed_encode64(p, t->bytes_data_blocks);
	p += mtbl_fixed_encode64(p, t->bytes_index_block);
	p += mtbl_fixed_encode64(p, t->bytes_keys);
	p += mtbl_fixed_encode64(p, t->bytes_values);

	padding = MTBL_TRAILER_SIZE - (p - buf) - sizeof(uint32_t);
	while (padding-- != 0)
		*(p++) = '\0';
	mtbl_fixed_encode32(buf + MTBL_TRAILER_SIZE - sizeof(uint32_t), MTBL_MAGIC);
}

bool
trailer_read(const uint8_t *buf, struct trailer *t)
{
	uint32_t magic;
	const uint8_t *p = buf;

	magic = mtbl_fixed_decode32(buf + MTBL_TRAILER_SIZE - sizeof(uint32_t));
	if (magic != MTBL_MAGIC)
		return (false);

	t->index_block_offset = mtbl_fixed_decode64(p); p += 8;
	t->data_block_size = mtbl_fixed_decode64(p); p += 8;
	t->compression_algorithm = mtbl_fixed_decode64(p); p += 8;
	t->count_entries = mtbl_fixed_decode64(p); p += 8;
	t->count_data_blocks = mtbl_fixed_decode64(p); p += 8;
	t->bytes_data_blocks = mtbl_fixed_decode64(p); p += 8;
	t->bytes_index_block = mtbl_fixed_decode64(p); p += 8;
	t->bytes_keys = mtbl_fixed_decode64(p); p += 8;
	t->bytes_values = mtbl_fixed_decode64(p); p += 8;

	return (true);

}
