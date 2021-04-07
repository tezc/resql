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
#include "store.h"

#include "file.h"
#include "test_util.h"

#include <string.h>

void store_open_test()
{
    struct store store;

    store_init(&store, test_tmp_dir, 0, 0);

    for (int i = 0; i < 10000; i++) {
        store_create_entry(&store, i, i, i, i, "test", strlen("test") + 1);
    }

    store_term(&store);

    struct store store2;
    store_init(&store2, test_tmp_dir, 0, 0);

    rs_assert(store2.last_index == 10000);
    store_term(&store2);
}

void store_test2()
{
    struct store store;

    store_init(&store, test_tmp_dir, 0, 0);

    for (int i = 0; i < 400000; i++) {
        store_create_entry(&store, i, i, i, i, "test", strlen("test") + 1);
    }

    store_term(&store);

    struct store store2;
    store_init(&store2, test_tmp_dir, 0, 0);

    rs_assert(store2.last_index == 400000);
    store_term(&store2);
}

static void store_expand_test()
{
    struct store store;
    const int size = 32 * 1024 * 1024;
    store_init(&store, test_tmp_dir, 0, 0);

    char *p = calloc(1, size);

    for (int i = 0; i < 2; i++) {
        store_create_entry(&store, i, i, i, i, p, size - 313);
    }

    free(p);

    store_term(&store);

    struct store store2;
    store_init(&store2, test_tmp_dir, 0, 0);

    rs_assert(store2.last_index == 2);
    store_term(&store2);
}

int main(void)
{
    test_execute(store_open_test);
    test_execute(store_test2);
    test_execute(store_expand_test);

    return 0;
}
