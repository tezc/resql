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

#include "page.h"

#include "entry.h"
#include "file.h"
#include "rs.h"
#include "test_util.h"

#include <string.h>

static void page_open_test(void)
{
    int rc;
    size_t page_size;
    struct page page;

    rc = page_init(&page, test_tmp_page0, -1, 5000);
    rs_assert(rc == RS_OK);

    page_size = page.map.len;
    rs_assert(page_fsync(&page, page_last_index(&page)) == RS_OK);
    rs_assert(page_term(&page) == RS_OK);

    rc = page_init(&page, test_tmp_page0, -1, 0);
    rs_assert(rc == RS_OK);
    rs_assert(page.map.len == page_size);
    rs_assert(page.prev_index == 5000);
    rs_assert(page_term(&page) == RS_OK);
}

static void page_reopen_test(void)
{
    int rc;
    const int prev_index = 5000;
    char *data = "data";
    unsigned char *entry;
    struct page page;

    rc = page_init(&page, test_tmp_page0, -1, prev_index);
    rs_assert(rc == RS_OK);

    for (int i = 0; i < 1000; i++) {
        page_create_entry(&page, i, i, i, i, data, strlen(data) + 1);

        if (i % 2 == 0) {
            page_fsync(&page, 5000 + 500);
        }
    }

    rs_assert(page_term(&page) == RS_OK);

    rc = page_init(&page, test_tmp_page0, -1, 0);
    rs_assert(rc == RS_OK);

    for (uint64_t i = 0; i < 1000; i++) {
        entry = page_entry_at(&page, prev_index + 1 + i);
        rs_assert(entry != NULL);
        rs_assert(entry_term(entry) == i);
        rs_assert(entry_flags(entry) == i);
        rs_assert(entry_cid(entry) == i);
        rs_assert(entry_seq(entry) == i);
        rs_assert(strcmp(entry_data(entry), data) == 0);
    }

    rs_assert(page_term(&page) == RS_OK);
}

static void page_expand_test(void)
{
    int rc;
    const int prev_index = 5000;
    char *data = "data";
    unsigned char *entry;
    struct page page;

    rc = page_init(&page, test_tmp_page0, -1, prev_index);
    rs_assert(rc == RS_OK);

    for (int i = 0; i < 1000; i++) {
        page_create_entry(&page, i, i, i, i, data, strlen(data) + 1);

        if (i % 2 == 0) {
            page_fsync(&page, 5000 + 500);
        }
    }

    rs_assert(page_expand(&page) == RS_OK);

    for (uint64_t i = 0; i < 1000; i++) {
        entry = page_entry_at(&page, prev_index + 1 + i);
        rs_assert(entry != NULL);
        rs_assert(entry_term(entry) == i);
        rs_assert(entry_flags(entry) == i);
        rs_assert(entry_cid(entry) == i);
        rs_assert(entry_seq(entry) == i);
        rs_assert(strcmp(entry_data(entry), data) == 0);
    }

    rs_assert(page_term(&page) == RS_OK);
}

static void page_remove_after_test(void)
{
    int rc;
    const int prev_index = 5000;
    char *data = "data";
    unsigned char *entry;
    struct page page;

    rc = page_init(&page, test_tmp_page0, -1, prev_index);
    rs_assert(rc == RS_OK);

    for (int i = 0; i < 1000; i++) {
        page_create_entry(&page, i, i, i, i, data, strlen(data) + 1);
    }

    page_remove_after(&page, prev_index + 501);

    rs_assert(page_term(&page) == RS_OK);

    rc = page_init(&page, test_tmp_page0, -1, 0);
    rs_assert(rc == RS_OK);
    rs_assert(page_entry_count(&page) == 501);

    for (uint64_t i = 0; i < 501; i++) {
        entry = page_entry_at(&page, prev_index + 1 + i);
        rs_assert(entry != NULL);
        rs_assert(entry_term(entry) == i);
        rs_assert(entry_flags(entry) == i);
        rs_assert(entry_cid(entry) == i);
        rs_assert(entry_seq(entry) == i);
        rs_assert(strcmp(entry_data(entry), data) == 0);
    }

    rs_assert(page_term(&page) == RS_OK);
}

int main(void)
{
    test_execute(page_open_test);
    test_execute(page_reopen_test);
    test_execute(page_remove_after_test);
    test_execute(page_expand_test);

    return 0;
}
