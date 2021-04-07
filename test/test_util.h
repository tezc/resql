/*
 * MIT License
 *
 * Copyright (c) 2021 Ozan Tezcan
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef RESQL_TEST_UTIL_H
#define RESQL_TEST_UTIL_H

#include "resql.h"
#include "rs.h"
#include "conf.h"

#define test_tmp_page0 "/tmp/resql_test/page.0.resql"
#define test_tmp_dir "/tmp/resql_test"
#define test_execute(A) (test_util_run(A, #A))

#define client_assert(c, b)                                                    \
    do {                                                                       \
        if (!(b)) {                                                            \
            rs_abort("%s \n", resql_errstr(c));                                \
        }                                                                      \
    } while (0)

void init_all();
void test_util_run(void (*test_fn)(void), const char *fn_name);

struct server *test_server_create_conf(struct conf *conf, int id);
struct server *test_server_create_auto(int cluster_size);
struct server *test_server_create(int id, int cluster_size);
struct server *test_server_start(int id, int cluster_size);
void test_server_destroy(int id);
void test_server_destroy_all();
void test_destroy_leader();

resql *test_client_create();
void test_client_destroy(resql *c);
void test_client_destroy_all();



#endif
