/*
 * Copyright (c) 2012, 2014-2015 by Farsight Security, Inc.
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

#include <snappy-c.h>
#include <zlib.h>

mtbl_res
_mtbl_compress_snappy(
	const uint8_t *input,
	const size_t input_size,
	uint8_t **output,
	size_t *output_size)
{
	snappy_status res;

	*output = my_malloc(snappy_max_compressed_length(input_size));
	res = snappy_compress((const char *) input, input_size,
			      (char *) (*output), output_size);
	if (res != SNAPPY_OK)
		return (mtbl_res_failure);

	return (mtbl_res_success);
}

mtbl_res
_mtbl_compress_zlib(
	const uint8_t *input,
	const size_t input_size,
	uint8_t **output,
	size_t *output_size)
{
	int zret;
	z_stream zs = {
		.opaque	= Z_NULL,
		.zalloc	= Z_NULL,
		.zfree	= Z_NULL,
	};

	*output_size = 2 * input_size;
	*output = my_malloc(*output_size);
	zret = deflateInit(&zs, Z_DEFAULT_COMPRESSION);
	assert(zret == Z_OK);
	zs.avail_in = input_size;
	zs.next_in = (uint8_t *) input;
	zs.avail_out = *output_size;
	zs.next_out = *output;
	zret = deflate(&zs, Z_FINISH);
	assert(zret == Z_STREAM_END);
	assert(zs.avail_in == 0);
	*output_size = zs.total_out;
	zret = deflateEnd(&zs);
	if (zret != Z_OK)
		return (mtbl_res_failure);

	return (mtbl_res_success);
}

mtbl_res
_mtbl_decompress_snappy(
	const uint8_t *input,
	const size_t input_size,
	uint8_t **output,
	size_t *output_size)
{
	snappy_status res;

	res = snappy_uncompressed_length((const char *) input, input_size, output_size);
	if (res != SNAPPY_OK)
		return (mtbl_res_failure);

	*output = my_malloc(*output_size);
	res = snappy_uncompress((const char *) input, input_size,
				(char *) (*output), output_size);
	if (res != SNAPPY_OK) {
		free(*output);
		return (mtbl_res_failure);
	}

	return (mtbl_res_success);
}

mtbl_res
_mtbl_decompress_zlib(
	const uint8_t *input,
	const size_t input_size,
	uint8_t **output,
	size_t *output_size)
{
	int zret;
	z_stream zs = {
		.avail_in	= 0,
		.next_in	= Z_NULL,
		.opaque		= Z_NULL,
		.zalloc		= Z_NULL,
		.zfree		= Z_NULL,
	};

	/**
	 * Initial guess of how large the decompressed output will be.
	 * A good guess will decrease the amount of looping and reallocation
	 * that has to be done if the buffer is too small.
	 */
	*output_size = 4 * input_size;

	/**
	 * Round up to the nearest kilobyte boundary.
	 */
	*output_size -= ((*output_size) % 1024);
	*output_size += 1024;

	*output = my_malloc(*output_size);

	zret = inflateInit(&zs);
	assert(zret == Z_OK);

	zs.avail_in = input_size;
	zs.next_in = (uint8_t *) input;
	zs.avail_out = *output_size;
	zs.next_out = *output;

	do {
		zret = inflate(&zs, Z_FINISH);
		assert(zret == Z_STREAM_END || zret == Z_BUF_ERROR);
		if (zret != Z_STREAM_END) {
			*output = my_realloc(*output, *output_size * 2);
			zs.next_out = *output + *output_size;
			zs.avail_out = *output_size;
			*output_size *= 2;
		}
	} while (zret != Z_STREAM_END);

	*output_size = zs.total_out;
	inflateEnd(&zs);

	return (mtbl_res_success);
}
