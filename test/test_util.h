/*
 *  Resql
 *
 *  Copyright (C) 2021 Ozan Tezcan
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef RESQL_TEST_UTIL_H
#define RESQL_TEST_UTIL_H

#include "resql.h"
#include "rs.h"

#define test_execute(A) (test_util_run(A, #A))

#define client_assert(c, b)                                                    \
    do {                                                                       \
        if (!(b)) {                                                            \
            rs_abort("%s \n", resql_errstr(c));                                \
        }                                                                      \
    } while (0)

void test_util_run(void (*test_fn)(void), const char *fn_name);

struct server *test_server_create(int id, int cluster_size);
struct server *test_server_start(int id, int cluster_size);
void test_server_destroy(int id);
void test_server_destroy_all();
void test_destroy_leader();

resql *test_client_create();
void test_client_destroy(resql *c);
void test_client_destroy_all();



#endif
