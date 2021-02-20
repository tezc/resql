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
