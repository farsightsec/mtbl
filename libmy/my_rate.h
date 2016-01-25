/*
 * Copyright (c) 2008, 2009, 2013, 2014 by Farsight Security, Inc.
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

#ifndef MY_RATE_H
#define MY_RATE_H

struct my_rate;

struct my_rate *
my_rate_init(unsigned rate, unsigned freq);

void
my_rate_destroy(struct my_rate **);

void
my_rate_sleep(struct my_rate *);

#endif /* MY_RATE_H */
