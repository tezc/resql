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

#include "entry.h"
#include "file.h"
#include "page.h"
#include "rs.h"
#include "test_util.h"

#include <string.h>

static void page_open_test(void)
{
	size_t page_size;
	struct page page;

	page_init(&page, test_tmp_page0, -1, 5000);

	page_size = page.map.len;
	page_fsync(&page, page_last_index(&page));
	page_term(&page);

	page_init(&page, test_tmp_page0, -1, 0);
	rs_assert(page.map.len == page_size);
	rs_assert(page.prev_index == 5000);
	page_term(&page);
}

static void page_reopen_test(void)
{
	const int prev_index = 5000;
	char *data = "data";
	unsigned char *entry;
	struct page page;

	page_init(&page, test_tmp_page0, -1, prev_index);

	for (int i = 0; i < 1000; i++) {
		page_create_entry(&page, i, i, i, i, data, strlen(data) + 1);

		if (i % 2 == 0) {
			page_fsync(&page, 5000 + 500);
		}
	}

	page_term(&page);
	page_init(&page, test_tmp_page0, -1, 0);

	for (uint64_t i = 0; i < 1000; i++) {
		entry = page_entry_at(&page, prev_index + 1 + i);
		rs_assert(entry != NULL);
		rs_assert(entry_term(entry) == i);
		rs_assert(entry_flags(entry) == i);
		rs_assert(entry_cid(entry) == i);
		rs_assert(entry_seq(entry) == i);
		rs_assert(strcmp(entry_data(entry), data) == 0);
	}

	page_term(&page);
}

static void page_expand_test(void)
{
	const int prev_index = 5000;
	char *data = "data";
	unsigned char *entry;
	struct page page;
	uint32_t size;

	page_init(&page, test_tmp_page0, -1, prev_index);

	for (int i = 0; i < 4000; i++) {
		page_create_entry(&page, i, i, i, i, data, strlen(data) + 1);

		if (i % 2 == 0) {
			page_fsync(&page, 5000 + 500);
		}
	}

	size = page_quota(&page);
	rs_assert(size < 50000000);

	rs_assert(page_reserve(&page, 50000000) == RS_OK);
	rs_assert(page_quota(&page) >= 50000000);

	rs_assert(page_reserve(&page, 2 *1024 *1024 * 1024ull) != RS_OK);

	for (uint64_t i = 0; i < 4000; i++) {
		entry = page_entry_at(&page, prev_index + 1 + i);
		rs_assert(entry != NULL);
		rs_assert(entry_term(entry) == i);
		rs_assert(entry_flags(entry) == i);
		rs_assert(entry_cid(entry) == i);
		rs_assert(entry_seq(entry) == i);
		rs_assert(strcmp(entry_data(entry), data) == 0);
	}

	page_term(&page);
}

static void page_remove_after_test(void)
{
	const int prev_index = 5000;
	char *data = "data";
	unsigned char *entry;
	struct page page;

	page_init(&page, test_tmp_page0, -1, prev_index);

	for (int i = 0; i < 1000; i++) {
		page_create_entry(&page, i, i, i, i, data, strlen(data) + 1);
	}

	page_remove_after(&page, prev_index + 501);

	page_term(&page);

	page_init(&page, test_tmp_page0, -1, 0);
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

	page_term(&page);
}

int main(void)
{
	test_execute(page_open_test);
	test_execute(page_reopen_test);
	test_execute(page_remove_after_test);
	test_execute(page_expand_test);

	return 0;
}
