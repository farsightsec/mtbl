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

#include <lz4.h>
#include <lz4hc.h>
#include <snappy-c.h>
#include <zlib.h>

const char *
mtbl_compression_type_to_str(mtbl_compression_type compression_type)
{
	switch (compression_type) {
	case MTBL_COMPRESSION_NONE:
		return "none";
	case MTBL_COMPRESSION_SNAPPY:
		return "snappy";
	case MTBL_COMPRESSION_ZLIB:
		return "zlib";
	case MTBL_COMPRESSION_LZ4:
		return "lz4";
	case MTBL_COMPRESSION_LZ4HC:
		return "lz4hc";
	default:
		return NULL;
	}
}

mtbl_res
mtbl_compression_type_from_str(const char *s, mtbl_compression_type *t)
{
	if (strcasecmp(s, "none") == 0) {
		*t = MTBL_COMPRESSION_NONE;
		return mtbl_res_success;
	} else if (strcasecmp(s, "snappy") == 0) {
		*t = MTBL_COMPRESSION_SNAPPY;
		return mtbl_res_success;
	} else if (strcasecmp(s, "zlib") == 0) {
		*t = MTBL_COMPRESSION_ZLIB;
		return mtbl_res_success;
	} else if (strcasecmp(s, "lz4") == 0) {
		*t = MTBL_COMPRESSION_LZ4;
		return mtbl_res_success;
	} else if (strcasecmp(s, "lz4hc") == 0) {
		*t = MTBL_COMPRESSION_LZ4HC;
		return mtbl_res_success;
	}

	return mtbl_res_failure;
}

mtbl_res
mtbl_compress(
	mtbl_compression_type compression_type,
	const uint8_t *input,
	const size_t input_size,
	uint8_t **output,
	size_t *output_size)
{
	switch (compression_type) {
	case MTBL_COMPRESSION_NONE:
		return mtbl_res_failure;
	case MTBL_COMPRESSION_SNAPPY:
		return _mtbl_compress_snappy(input, input_size, output, output_size);
	case MTBL_COMPRESSION_ZLIB:
		return _mtbl_compress_zlib(input, input_size, output, output_size);
	case MTBL_COMPRESSION_LZ4:
		return _mtbl_compress_lz4(input, input_size, output, output_size);
	case MTBL_COMPRESSION_LZ4HC:
		return _mtbl_compress_lz4hc(input, input_size, output, output_size);
	default:
		return mtbl_res_failure;
	}
}

mtbl_res
mtbl_decompress(
	mtbl_compression_type compression_type,
	const uint8_t *input,
	const size_t input_size,
	uint8_t **output,
	size_t *output_size)
{
	switch (compression_type) {
	case MTBL_COMPRESSION_NONE:
		return mtbl_res_failure;
	case MTBL_COMPRESSION_SNAPPY:
		return _mtbl_decompress_snappy(input, input_size, output, output_size);
	case MTBL_COMPRESSION_ZLIB:
		return _mtbl_decompress_zlib(input, input_size, output, output_size);
	case MTBL_COMPRESSION_LZ4:
		/* Fall through, LZ4 and LZ4HC use the same decompressor. */
	case MTBL_COMPRESSION_LZ4HC:
		return _mtbl_decompress_lz4(input, input_size, output, output_size);
	default:
		return mtbl_res_failure;
	}
}

mtbl_res
_mtbl_compress_lz4(
	const uint8_t *input,
	const size_t input_size,
	uint8_t **output,
	size_t *output_size)
{
	int lz4_size;
	char *lz4_bytes;

	if (input_size > INT_MAX)
		return (mtbl_res_failure);

	lz4_size = LZ4_compressBound(input_size);

	*output_size = lz4_size + sizeof(uint32_t);
	*output = my_malloc(*output_size);
	lz4_bytes = (char *) (*output) + sizeof(uint32_t);

	lz4_size = LZ4_compress_default((const char *) input,
					lz4_bytes,
					(int) input_size,
					lz4_size);
	if (lz4_size == 0) {
		free(*output);
		return (mtbl_res_failure);
	}
	*output_size = lz4_size + sizeof(uint32_t);

	/**
	 * Prefix the compressed LZ4 block with a 32-bit little endian integer
	 * specifying the size of the uncompressed block. This makes
	 * decompression much easier.
	 */
	mtbl_fixed_encode32(*output, (uint32_t) input_size);

	return (mtbl_res_success);
}

mtbl_res
_mtbl_compress_lz4hc(
	const uint8_t *input,
	const size_t input_size,
	uint8_t **output,
	size_t *output_size)
{
	int lz4_size;
	char *lz4_bytes;

	if (input_size > INT_MAX)
		return (mtbl_res_failure);

	lz4_size = LZ4_compressBound(input_size);

	*output_size = lz4_size + sizeof(uint32_t);
	*output = my_malloc(*output_size);
	lz4_bytes = (char *) (*output) + sizeof(uint32_t);

	lz4_size = LZ4_compress_HC((const char *) input,
					lz4_bytes,
					(int) input_size,
					lz4_size,
					9 /* compressionLevel */);
	if (lz4_size == 0) {
		free(*output);
		return (mtbl_res_failure);
	}
	*output_size = lz4_size + sizeof(uint32_t);

	/**
	 * Prefix the compressed LZ4 block with a 32-bit little endian integer
	 * specifying the size of the uncompressed block. This makes
	 * decompression much easier.
	 */
	mtbl_fixed_encode32(*output, (uint32_t) input_size);

	return (mtbl_res_success);
}

mtbl_res
_mtbl_compress_snappy(
	const uint8_t *input,
	const size_t input_size,
	uint8_t **output,
	size_t *output_size)
{
	snappy_status res;

	*output_size = snappy_max_compressed_length(input_size);
	*output = my_malloc(*output_size);
	res = snappy_compress((const char *) input, input_size,
			      (char *) (*output), output_size);
	if (res != SNAPPY_OK) {
		free(*output);
		return (mtbl_res_failure);
	}

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
	if (zret != Z_OK) {
		free(*output);
		return (mtbl_res_failure);
	}

	return (mtbl_res_success);
}

mtbl_res
_mtbl_decompress_lz4(
	const uint8_t *input,
	const size_t input_size,
	uint8_t **output,
	size_t *output_size)
{
	int ret = 0;

	if (input_size > INT_MAX || input_size < sizeof(uint32_t))
		return (mtbl_res_failure);

	/**
	 * The first four bytes is a 32-bit little endian integer specifying
	 * the size of the uncompressed block.
	 */
	*output_size = mtbl_fixed_decode32(input);
	*output = my_malloc(*output_size);

	ret = LZ4_decompress_safe((char *) input + sizeof(uint32_t),
				  (char *) (*output),
				  input_size - sizeof(uint32_t),
				  *output_size);
	if (ret < 0) {
		free(*output);
		return (mtbl_res_failure);
	}

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
