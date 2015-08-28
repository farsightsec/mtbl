/*
 * Copyright (c) 2012-2015 by Farsight Security, Inc.
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
metadata_write(const struct mtbl_metadata *m, uint8_t *buf)
{
	size_t padding;
	uint8_t *p = buf;

	p += mtbl_fixed_encode64(p, m->index_block_offset);
	p += mtbl_fixed_encode64(p, m->data_block_size);
	p += mtbl_fixed_encode64(p, m->compression_algorithm);
	p += mtbl_fixed_encode64(p, m->count_entries);
	p += mtbl_fixed_encode64(p, m->count_data_blocks);
	p += mtbl_fixed_encode64(p, m->bytes_data_blocks);
	p += mtbl_fixed_encode64(p, m->bytes_index_block);
	p += mtbl_fixed_encode64(p, m->bytes_keys);
	p += mtbl_fixed_encode64(p, m->bytes_values);

	padding = MTBL_METADATA_SIZE - (p - buf) - sizeof(uint32_t);
	while (padding-- != 0)
		*(p++) = '\0';
	mtbl_fixed_encode32(buf + MTBL_METADATA_SIZE - sizeof(uint32_t), MTBL_MAGIC);
}

bool
metadata_read(const uint8_t *buf, struct mtbl_metadata *m)
{
	uint32_t magic;
	const uint8_t *p = buf;

	magic = mtbl_fixed_decode32(buf + MTBL_METADATA_SIZE - sizeof(uint32_t));
	if (magic != MTBL_MAGIC)
		return (false);

	m->index_block_offset = mtbl_fixed_decode64(p); p += 8;
	m->data_block_size = mtbl_fixed_decode64(p); p += 8;
	m->compression_algorithm = mtbl_fixed_decode64(p); p += 8;
	m->count_entries = mtbl_fixed_decode64(p); p += 8;
	m->count_data_blocks = mtbl_fixed_decode64(p); p += 8;
	m->bytes_data_blocks = mtbl_fixed_decode64(p); p += 8;
	m->bytes_index_block = mtbl_fixed_decode64(p); p += 8;
	m->bytes_keys = mtbl_fixed_decode64(p); p += 8;
	m->bytes_values = mtbl_fixed_decode64(p); p += 8;

	return (true);

}

uint64_t
mtbl_metadata_index_block_offset(const struct mtbl_metadata *m)
{
	return m->index_block_offset;
}

uint64_t
mtbl_metadata_data_block_size(const struct mtbl_metadata *m)
{
	return m->data_block_size;
}

uint64_t
mtbl_metadata_compression_algorithm(const struct mtbl_metadata *m)
{
	return m->compression_algorithm;
}

uint64_t
mtbl_metadata_count_entries(const struct mtbl_metadata *m)
{
	return m->count_entries;
}

uint64_t
mtbl_metadata_count_data_blocks(const struct mtbl_metadata *m)
{
	return m->count_data_blocks;
}

uint64_t
mtbl_metadata_bytes_data_blocks(const struct mtbl_metadata *m)
{
	return m->bytes_data_blocks;
}

uint64_t
mtbl_metadata_bytes_index_block(const struct mtbl_metadata *m)
{
	return m->bytes_index_block;
}

uint64_t
mtbl_metadata_bytes_keys(const struct mtbl_metadata *m)
{
	return m->bytes_keys;
}

uint64_t
mtbl_metadata_bytes_values(const struct mtbl_metadata *m)
{
	return m->bytes_values;
}
