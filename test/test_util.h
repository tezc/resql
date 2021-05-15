/*
 * BSD-3-Clause
 *
 * Copyright 2021 Ozan Tezcan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef RESQL_TEST_UTIL_H
#define RESQL_TEST_UTIL_H

#include "conf.h"
#include "resql.h"
#include "rs.h"

#define test_tmp_page0	"/tmp/resql_test/page.0.resql"
#define test_tmp_dir	"/tmp/resql_test"
#define test_tmp_page0	"/tmp/resql_test/page.0.resql"
#define test_tmp_dir2	"/tmp/resql_test2"
#define test_execute(A) (test_util_run(A, #A))

#define client_assert(c, b)                                                    \
	do {                                                                   \
		if (!(b)) {                                                    \
			rs_abort("%s \n", resql_errstr(c));                    \
		}                                                              \
	} while (0)

void init_all();
void test_util_run(void (*test_fn)(void), const char *fn_name);

struct server *test_server_create_conf(struct conf *conf, int id);
struct server *test_server_create_auto(bool in_memory, int cluster_size);
struct server *test_server_create(bool in_memory, int id, int cluster_size);
struct server *test_server_start_auto(bool in_memory, int cluster_size);
struct server *test_server_start(bool in_memory, int id, int cluster_size);
struct server *test_server_add_auto(bool in_memory);

void test_wait_until_size(int size);
void test_server_add(bool in_memory, int id, int cluster_size);
void test_server_remove(int id);

void test_server_destroy(int id);
void test_server_destroy_all();
void test_server_destroy_leader();

resql *test_client_create();
void test_client_destroy(resql *c);
void test_client_destroy_all();

#endif
