/*
 *  Resql
 *
 *  Copyright (C) 2021 Resql Authors
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

#define test_execute(A) (test_util_run(A, #A))

#define client_assert(c, b)                                                    \
    do {                                                                       \
        if (!(b)) {                                                            \
            rs_abort("%s \n", resql_errstr(c));                            \
        }                                                                      \
    } while (0)

void test_util_run(void (*test_fn)(void), const char *fn_name);

struct server* test_server_create(int id);
void test_server_stop(int id);

#endif
