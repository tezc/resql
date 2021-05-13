/*
 * BSD-3-Clause
 *
 * Copyright 2021 Ozan Tezcan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "entry.h"
#include "file.h"
#include "page.h"
#include "rs.h"
#include "test_util.h"

#include "sc/sc_log.h"

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
	sc_log_info("Page init - 1 \n");

	for (int i = 0; i < 4000; i++) {
		page_create_entry(&page, i, i, i, i, data, strlen(data) + 1);
		sc_log_info("Page create entry - 1 \n");

		if (i % 2 == 0) {
			page_fsync(&page, 5000 + 500);
		}
		sc_log_info("Page fsync - 1 \n");
	}

	sc_log_info("Page quota \n");
	size = page_quota(&page);
	rs_assert(size < 50000000);

	rs_assert(page_reserve(&page, 50000000) == RS_OK);
	rs_assert(page_quota(&page) >= 50000000);
	sc_log_info("Page reserve \n");
	rs_assert(page_reserve(&page, 2 * 1024 * 1024 * 1024ull) != RS_OK);
	sc_log_info("Page reserve 2\n");

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
