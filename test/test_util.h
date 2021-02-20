/*
 *  Copyright (C) 2020 reSQL Authors
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
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

#endif
