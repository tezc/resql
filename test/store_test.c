/*
 *  Copyright (C) 2020 Resql Authors
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

#include "test_util.h"

#include "store.h"

#include <assert.h>
#include <string.h>

void store_open_test()
{
    file_clear_dir("/tmp/store", RS_STORE_EXTENSION);

    struct store store;

    store_init(&store, "/tmp/store", 0, 0);

    for (int i = 0; i < 10000; i++) {
        store_create_entry(&store, i, i, i, i, "test", strlen("test") + 1);
    }

    store_term(&store);

    struct store store2;
    store_init(&store2, "/tmp/store", 0, 0);

    assert(store2.last_index == 10000);
    store_term(&store2);
}

void store_test2()
{
    file_clear_dir("/tmp/store", RS_STORE_EXTENSION);

    struct store store;

    store_init(&store, "/tmp/store", 0, 0);

    for (int i = 0; i < 400000; i++) {
        store_create_entry(&store, i, i, i, i, "test", strlen("test") + 1);
    }

    store_term(&store);

    struct store store2;
    store_init(&store2, "/tmp/store", 0, 0);

    assert(store2.last_index == 400000);
    store_term(&store2);
}

static void store_expand_test()
{
    file_clear_dir("/tmp/store", RS_STORE_EXTENSION);

    struct store store;
    const int size = 32 * 1024 * 1024;
    store_init(&store, "/tmp/store", 0, 0);

    char* p = calloc(1, size);

    for (int i = 0; i < 2; i++) {
        store_create_entry(&store, i, i, i, i, p, size - 313);
    }

    free(p);

    store_term(&store);

    struct store store2;
    store_init(&store2, "/tmp/store", 0, 0);

    assert(store2.last_index == 2);
    store_term(&store2);
}

int main(void)
{
    test_execute(store_open_test);
    test_execute(store_test2);
    test_execute(store_expand_test);

    return 0;
}
