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

#ifndef MY_UBUF_PB_H
#define MY_UBUF_PB_H

#include <google/protobuf-c/protobuf-c.h>

#include "ubuf.h"

struct ubuf_protobuf_c_buffer {
	ProtobufCBuffer base;
	ubuf *u;
};

static inline void
ubuf_protobuf_c_buffer_append(ProtobufCBuffer *buffer,
			      size_t len,
			      const uint8_t *data)
{
	ubuf *u = ((struct ubuf_protobuf_c_buffer *) buffer)->u;
	ubuf_append(u, data, len);
}

#define UBUF_PROTOBUF_C_BUFFER_INIT(u) { { ubuf_protobuf_c_buffer_append }, u }

#endif /* MY_UBUF_PB_H */
