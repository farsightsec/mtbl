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
# define my_queue_mb_init		my_queue_init
# define my_queue_mb_destroy		my_queue_destroy
# define my_queue_mb_impl_type		my_queue_impl_type
# define my_queue_mb_insert		my_queue_insert
# define my_queue_mb_remove		my_queue_remove
# include "my_queue_mb.c"
#else
# define my_queue_mutex_init		my_queue_init
# define my_queue_mutex_destroy		my_queue_destroy
# define my_queue_mutex_impl_type	my_queue_impl_type
# define my_queue_mutex_insert		my_queue_insert
# define my_queue_mutex_remove		my_queue_remove
# include "my_queue_mutex.c"
#endif
