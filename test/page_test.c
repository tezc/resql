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

#include "test_util.h"

#include "entry.h"
#include "file.h"
#include "page.h"
#include "rs.h"

#include <assert.h>
#include <string.h>


void* x()
{
    return NULL;
}

void a()
{

}

static void page_open_test(void)
{
    int rc;
    struct page page;

    file_mkdir("/tmp/store");
    file_clear_dir("/tmp/store", RS_STORE_EXTENSION);

    rc = page_init(&page, "/tmp/store/page.resql", -1, 5000);
    assert(rc == RS_OK);

    size_t page_size = page.map.len;
    assert(page_fsync(&page, page_last_index(&page)) == RS_OK);
    assert(page_term(&page) == RS_OK);

    rc = page_init(&page, "/tmp/store/page.resql", -1, 0);
    assert(rc == RS_OK);
    assert(page.map.len == page_size);
    assert(page.prev_index == 5000);
    assert(page_term(&page) == RS_OK);
}

static void page_reopen_test(void)
{
    file_mkdir("/tmp/store");
    file_clear_dir("/tmp/store", RS_STORE_EXTENSION);

    int rc;
    const int prev_index = 5000;
    struct page page;

    rc = page_init(&page, "/tmp/store/page.resql", -1, prev_index);
    assert(rc == RS_OK);

    char *data = "data";
    char *entry;

    for (int i = 0; i < 1000; i++) {
        page_create_entry(&page, i, i, i, i, data, strlen(data) + 1);

        if (i % 2 == 0) {
            page_fsync(&page, 5000 + 500);
        }
    }

    assert(page_term(&page) == RS_OK);

    rc = page_init(&page, "/tmp/store/page.resql", -1, 0);
    assert(rc == RS_OK);

    for (uint64_t i = 0; i < 1000; i++) {
        entry = page_entry_at(&page, prev_index + 1 + i);
        assert(entry != NULL);
        assert(entry_term(entry) == i);
        assert(entry_flags(entry) == i);
        assert(entry_cid(entry) == i);
        assert(entry_seq(entry) == i);
        assert(strcmp(entry_data(entry), data) == 0);
    }

    assert(page_term(&page) == RS_OK);
}

static void page_expand_test(void)
{
    file_mkdir("/tmp/store");
    file_clear_dir("/tmp/store", RS_STORE_EXTENSION);

    int rc;
    const int prev_index = 5000;
    struct page page;

    rc = page_init(&page, "/tmp/store/page.resql", -1, prev_index);
    assert(rc == RS_OK);

    char *data = "data";
    char *entry;

    for (int i = 0; i < 1000; i++) {
        page_create_entry(&page, i, i, i, i, data, strlen(data) + 1);

        if (i % 2 == 0) {
            page_fsync(&page, 5000 + 500);
        }
    }

    assert(page_expand(&page) == RS_OK);

    for (uint64_t i = 0; i < 1000; i++) {
        entry = page_entry_at(&page, prev_index + 1 + i);
        assert(entry != NULL);
        assert(entry_term(entry) == i);
        assert(entry_flags(entry) == i);
        assert(entry_cid(entry) == i);
        assert(entry_seq(entry) == i);
        assert(strcmp(entry_data(entry), data) == 0);
    }

    assert(page_term(&page) == RS_OK);
}

static void page_remove_after_test(void)
{
    file_mkdir("/tmp/store");
    file_clear_dir("/tmp/store", RS_STORE_EXTENSION);

    int rc;
    const int prev_index = 5000;
    struct page page;

    rc = page_init(&page, "/tmp/store/page.resql", -1, prev_index);
    assert(rc == RS_OK);

    char *data = "data";
    char *entry;

    for (int i = 0; i < 1000; i++) {
        page_create_entry(&page, i, i, i, i, data, strlen(data) + 1);
    }

    page_remove_after(&page, prev_index + 501);


    assert(page_term(&page) == RS_OK);

    rc = page_init(&page, "/tmp/store/page.resql", -1, 0);
    assert(rc == RS_OK);
    assert(page_entry_count(&page) == 501);

    for (uint64_t i = 0; i < 501; i++) {
        entry = page_entry_at(&page, prev_index + 1 + i);
        assert(entry != NULL);
        assert(entry_term(entry) == i);
        assert(entry_flags(entry) == i);
        assert(entry_cid(entry) == i);
        assert(entry_seq(entry) == i);
        assert(strcmp(entry_data(entry), data) == 0);
    }

    assert(page_term(&page) == RS_OK);
}

int main(void)
{
    test_execute(page_open_test);
    test_execute(page_reopen_test);
    test_execute(page_remove_after_test);
    test_execute(page_expand_test);

    return 0;
}
