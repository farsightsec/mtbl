/*
 * Copyright (c) 2012 by Internet Systems Consortium, Inc. ("ISC")
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS.	IN NO EVENT SHALL ISC BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "mtbl-private.h"

void
trailer_write(struct mtbl_trailer *t, uint8_t *buf)
{
	size_t padding;
	uint8_t *p = buf;

	mtbl_fixed_encode64(p, t->compression_algorithm);
	p += 8;
	mtbl_fixed_encode64(p, t->count_data_blocks);
	p += 8;
	mtbl_fixed_encode64(p, t->bytes_data_blocks_compressed);
	p += 8;
	mtbl_fixed_encode64(p, t->bytes_data_blocks_uncompressed);
	p += 8;
	mtbl_fixed_encode64(p, t->bytes_index_block);
	p += 8;
	mtbl_fixed_encode64(p, t->count_entries);
	p += 8;
	mtbl_fixed_encode64(p, t->bytes_keys);
	p += 8;
	mtbl_fixed_encode64(p, t->bytes_values);
	p += 8;

	padding = MTBL_TRAILER_SIZE - (p - buf) - sizeof(uint32_t);
	while (padding-- != 0)
		*(p++) = '\0';
	mtbl_fixed_encode32(buf + MTBL_TRAILER_SIZE - sizeof(uint32_t), MTBL_MAGIC);
}

bool
trailer_read(const uint8_t *buf, struct mtbl_trailer *t)
{
	uint32_t magic;
	const uint8_t *p = buf;

	magic = mtbl_fixed_decode32(buf + MTBL_TRAILER_SIZE - sizeof(uint32_t));
	if (magic != MTBL_MAGIC)
		return (false);

	t->compression_algorithm = mtbl_fixed_decode64(p);
	p += 8;
	t->count_data_blocks = mtbl_fixed_decode64(p);
	p += 8;
	t->bytes_data_blocks_compressed = mtbl_fixed_decode64(p);
	p += 8;
	t->bytes_data_blocks_uncompressed = mtbl_fixed_decode64(p);
	p += 8;
	t->bytes_index_block = mtbl_fixed_decode64(p);
	p += 8;
	t->count_entries = mtbl_fixed_decode64(p);
	p += 8;
	t->bytes_keys = mtbl_fixed_decode64(p);
	p += 8;
	t->bytes_values = mtbl_fixed_decode64(p);
	p += 8;

	return (true);

}
