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
