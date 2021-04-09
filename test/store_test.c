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
#include "file.h"
#include "store.h"
#include "test_util.h"

#include <string.h>

void store_open_test()
{
	struct store s1, s2;

	store_init(&s1, test_tmp_dir, 0, 0);

	for (int i = 0; i < 10000; i++) {
		store_create_entry(&s1, i, i, i, i, "test", strlen("test") + 1);
	}

	store_term(&s1);

	store_init(&s2, test_tmp_dir, 0, 0);

	rs_assert(s2.last_index == 10000);
	store_term(&s2);
}

void store_many()
{
	const uint64_t prev_term = 1000;
	const uint64_t prev_index = 5000;

	int rc;
	uint64_t i;
	struct store s1, s2;

	store_init(&s1, test_tmp_dir, prev_term, prev_index);

	for (i = prev_index + 1; i < 1800000; i++) {
		rc = store_create_entry(&s1, i, i, i, i, "test", strlen("test") + 1);
		if (rc == RS_FULL) {
			break;
		}
	}

	rs_assert(s1.last_index == i - 1);
	rs_assert(s1.last_term == i - 1);

	store_term(&s1);

	store_init(&s2, test_tmp_dir, prev_term, prev_index);
	rs_assert(s2.last_index == i - 1);
	rs_assert(s2.last_term == i - 1);

	store_remove_after(&s2, 1233);
	rs_assert(s2.last_index == prev_index);
	rs_assert(s2.last_term == prev_term);
	store_term(&s2);
}

void store_remove()
{
	const uint64_t prev_term = 1000;
	const uint64_t prev_index = 5000;

	int rc;
	uint64_t i;
	struct store s1, s2;

	store_init(&s1, test_tmp_dir, prev_term, prev_index);

	for (i = prev_index + 1; i < 1800000; i++) {
		rc = store_create_entry(&s1, i, i, i, i, "test", strlen("test") + 1);
		if (rc == RS_FULL) {
			break;
		}
	}

	rs_assert(s1.last_index == i - 1);
	rs_assert(s1.last_term == i - 1);

	store_term(&s1);

	store_init(&s2, test_tmp_dir, prev_term, prev_index);
	rs_assert(s2.last_index == i - 1);
	rs_assert(s2.last_term == i - 1);

	store_remove_after(&s2, 100000);
	rs_assert(s2.last_index == 100000);
	rs_assert(s2.last_term == 100000);
	store_term(&s2);

	store_init(&s2, test_tmp_dir, prev_term, prev_index);
	rs_assert(s2.last_index == 100000);
	rs_assert(s2.last_term == 100000);
	store_term(&s2);
}

void store_remove_second_page()
{
	const uint64_t prev_term = 1000;
	const uint64_t prev_index = 5000;

	int rc;
	uint64_t i;
	struct store s1, s2;

	store_init(&s1, test_tmp_dir, prev_term, prev_index);

	for (i = prev_index + 1; i < 1800000; i++) {
		rc = store_create_entry(&s1, i, i, i, i, "test", strlen("test") + 1);
		if (rc == RS_FULL) {
			break;
		}
	}

	rs_assert(s1.last_index == i - 1);
	rs_assert(s1.last_term == i - 1);

	store_term(&s1);

	store_init(&s2, test_tmp_dir, prev_term, prev_index);
	rs_assert(s2.last_index == i - 1);
	rs_assert(s2.last_term == i - 1);

	store_remove_after(&s2, 1600000);
	rs_assert(s2.last_index == 1600000);
	rs_assert(s2.last_term == 1600000);
	store_term(&s2);

	store_init(&s2, test_tmp_dir, prev_term, prev_index);
	rs_assert(s2.last_index == 1600000);
	rs_assert(s2.last_term == 1600000);
	store_term(&s2);
}

static void store_expand_test()
{
	struct store s1, s2;
	const int size = 32 * 1024 * 1024;

	store_init(&s1, test_tmp_dir, 0, 0);

	char *p = calloc(1, size);

	for (int i = 0; i < 2; i++) {
		store_create_entry(&s1, i, i, i, i, p, size - 313);
	}

	free(p);

	store_term(&s1);

	store_init(&s2, test_tmp_dir, 0, 0);
	rs_assert(s2.last_index == 2);

	store_term(&s2);
}

void store_put_test()
{
	const uint64_t prev_term = 1000;
	const uint64_t prev_index = 5000;

	int rc;
	uint64_t i;
	unsigned char* e;
	struct store s1, s2;

	store_init(&s1, test_tmp_dir, prev_term, prev_index);
	store_init(&s2, test_tmp_dir2, prev_term, prev_index);

	for (i = prev_index + 1; i < 1800000; i++) {
		rc = store_create_entry(&s1, i, i, i, i, "test", strlen("test") + 1);
		if (rc == RS_FULL) {
			break;
		}

		e = store_get_entry(&s1, i);
		store_put_entry(&s2, i, e);
	}

	rs_assert(s1.last_index == i - 1);
	rs_assert(s1.last_term == i - 1);
	rs_assert(s2.last_index == i - 1);
	rs_assert(s2.last_term == i - 1);

	store_term(&s1);
	store_term(&s2);

	store_init(&s2, test_tmp_dir2, prev_term, prev_index);
	rs_assert(s2.last_index == i - 1);
	rs_assert(s2.last_term == i - 1);

	store_remove_after(&s2, 1600000);
	rs_assert(s2.last_index == 1600000);
	rs_assert(s2.last_term == 1600000);
	store_term(&s2);

	store_init(&s2, test_tmp_dir2, prev_term, prev_index);
	rs_assert(s2.last_index == 1600000);
	rs_assert(s2.last_term == 1600000);
	store_term(&s2);
}

int main(void)
{
	test_execute(store_open_test);
	test_execute(store_many);
	test_execute(store_remove);
	test_execute(store_remove_second_page);
	test_execute(store_expand_test);
	test_execute(store_put_test);

	return 0;
}
