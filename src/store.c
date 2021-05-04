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

#include "store.h"

#include "entry.h"
#include "page.h"
#include "rs.h"

#include "sc/sc_array.h"
#include "sc/sc_log.h"
#include "sc/sc_str.h"

#include <inttypes.h>
#include <limits.h>

#define STORE_MAX_ENTRY_SIZE (512 * 1024 * 1024)
#define DEF_PAGE_FILE_0	     "page.0.resql"
#define DEF_PAGE_FILE_1	     "page.1.resql"

static void store_swap(struct store *s)
{
	struct page *tmp;

	tmp = s->pages[0];
	s->pages[0] = s->pages[1];
	s->pages[1] = tmp;
}

static int store_read(struct store *s)
{
	int rc;
	size_t c, count;
	char buf[PATH_MAX];

	rs_snprintf(buf, sizeof(buf), "%s/%s", s->path, DEF_PAGE_FILE_0);
	rc = page_init(s->pages[0], buf, -1, s->ss_index);
	if (rc != RS_OK) {
		goto cleanup_first;
	}

	rs_snprintf(buf, sizeof(buf), "%s/%s", s->path, DEF_PAGE_FILE_1);
	rc = page_init(s->pages[1], buf, -1, s->ss_index);
	if (rc != RS_OK) {
		goto cleanup_second;
	}

	if (s->pages[1]->prev_index < s->pages[0]->prev_index) {
		store_swap(s);
	}

	if (s->pages[0]->prev_index != s->ss_index) {
		page_clear(s->pages[0], s->ss_index);
	}

	if (s->pages[1]->prev_index != page_last_index(s->pages[0])) {
		page_clear(s->pages[1], 0);
	}

	if (page_entry_count(s->pages[0]) == 0) {
		store_swap(s);
	}

	sc_log_info("Log page [%s] from (%" PRIu64 ",%" PRIu64 "] \n",
		    s->pages[0]->path, s->pages[0]->prev_index,
		    page_last_index(s->pages[0]));

	sc_log_info("Log page [%s] from (%" PRIu64 ",%" PRIu64 "] \n",
		    s->pages[1]->path, s->pages[1]->prev_index,
		    page_last_index(s->pages[1]));

	c = page_entry_count(s->pages[1]);
	s->curr = (c > 0) ? s->pages[1] : s->pages[0];

	s->last_index = page_last_index(s->curr);

	if (page_entry_count(s->curr)) {
		s->last_term = page_last_term(s->curr);
	} else {
		s->last_term = s->ss_term;
	}

	count = page_entry_count(s->curr);
	s->last_term = count != 0 ? page_last_term(s->curr) : s->ss_term;

	return RS_OK;

cleanup_second:
	page_term(s->pages[1]);
cleanup_first:
	page_term(s->pages[0]);

	return rc;
}

int store_init(struct store *s, const char *path, uint64_t ss_term,
	       uint64_t ss_index)
{
	int rc;

	s->path = sc_str_create(path);
	s->last_index = 0;
	s->last_term = 0;
	s->ss_term = ss_term;
	s->ss_index = ss_index;
	s->pages[0] = rs_malloc(sizeof(*s->pages[0]));
	s->pages[1] = rs_malloc(sizeof(*s->pages[0]));

	rc = store_read(s);
	if (rc != RS_OK) {
		goto cleanup;
	}

	return RS_OK;

cleanup:
	sc_str_destroy(&s->path);
	rs_free(s->pages[0]);
	rs_free(s->pages[1]);
	*s = (struct store){0};

	return rc;
}

void store_term(struct store *s)
{
	page_term(s->pages[0]);
	page_term(s->pages[1]);

	rs_free(s->pages[0]);
	rs_free(s->pages[1]);

	s->pages[0] = NULL;
	s->pages[1] = NULL;

	sc_str_destroy(&s->path);
}

void store_flush(struct store *s)
{
	page_fsync(s->curr, s->last_index);
}

void store_snapshot_taken(struct store *s)
{
	assert(page_isempty(s->pages[0]) == false);

	s->ss_index = page_last_index(s->pages[0]);
	s->ss_term = page_last_term(s->pages[0]);

	page_clear(s->pages[0], 0);
	store_swap(s);
	s->curr = s->pages[0];
}

uint64_t store_ss_index(struct store *s)
{
	if (page_isempty(s->pages[1])) {
		return UINT64_MAX;
	}

	return page_last_index(s->pages[0]);
}

struct page *store_ss_page(struct store *s)
{
	return s->pages[0];
}

int store_create_entry(struct store *s, uint64_t term, uint64_t seq,
		       uint64_t cid, uint32_t flags, void *data, uint32_t len)
{
	int rc;
	uint32_t size;

retry:
	size = entry_encoded_len(len);
	assert(size < STORE_MAX_ENTRY_SIZE);

	if (size > page_quota(s->curr)) {
		if (size > page_cap(s->curr)) {
			rc = page_reserve(s->curr, size);
			if (rc == RS_OK) {
				goto retry;
			}
		}

		if (s->curr != s->pages[1]) {
			s->curr = s->pages[1];
			page_clear(s->curr, s->last_index);
			if (size > page_cap(s->curr)) {
				page_reserve(s->curr, size);
			}
			goto retry;
		}

		return RS_FULL;
	}

	page_create_entry(s->curr, term, seq, cid, flags, data, len);
	s->last_index++;
	s->last_term = term;

	return RS_OK;
}

int store_reserve(struct store *s, uint32_t size)
{
	rs_assert(s->curr == s->pages[1]);
	return page_reserve(s->pages[1], size);
}

int store_put_entry(struct store *s, uint64_t index, unsigned char *entry)
{
	int rc;

	rs_assert(index == s->last_index + 1);
	rs_assert(s->last_term <= entry_term(entry));

	uint32_t size = entry_len(entry);

retry:
	if (size > page_quota(s->curr)) {
		if (size > page_cap(s->curr)) {
			rc = page_reserve(s->curr, size);
			if (rc == RS_OK) {
				goto retry;
			}

			if (rc == RS_ERROR || rc == RS_FULL) {
				return rc;
			}
		}

		if (s->curr != s->pages[1]) {
			store_flush(s);

			s->curr = s->pages[1];
			page_clear(s->curr, s->last_index);

			if (size > page_cap(s->curr)) {
				rc = page_reserve(s->curr, size);
				if (rc == RS_OK) {
					goto retry;
				}

				if (rc == RS_ERROR || rc == RS_FULL) {
					return rc;
				}
			}

			goto retry;
		}

		return RS_FULL;
	}

	page_put_entry(s->curr, entry);

	s->last_index++;
	s->last_term = entry_term(entry);

	return RS_OK;
}

unsigned char *store_get_entry(struct store *s, uint64_t index)
{
	unsigned char *entry;

	entry = page_entry_at(s->pages[0], index);
	if (entry == NULL) {
		entry = page_entry_at(s->pages[1], index);
	}

	return entry;
}

uint64_t store_prev_term(struct store *s, uint64_t index)
{
	unsigned char *entry;

	entry = store_get_entry(s, index);
	return entry != NULL ? entry_term(entry) : s->ss_term;
}

void store_entries(struct store *s, uint64_t index, uint32_t limit,
		   unsigned char **entries, uint32_t *size, uint32_t *count)
{
	page_get_entries(s->pages[0], index, limit, entries, size, count);
	if (*entries == NULL) {
		page_get_entries(s->pages[1], index, limit, entries, size,
				 count);
	}
}

void store_remove_after(struct store *s, uint64_t index)
{
	page_remove_after(s->pages[0], index);
	page_remove_after(s->pages[1], index);

	if (page_isempty(s->pages[1])) {
		s->curr = s->pages[0];
	}

	if (page_isempty(s->curr)) {
		s->last_index = s->ss_index;
		s->last_term = s->ss_term;
		page_clear(s->curr, s->ss_index);
	} else {
		s->last_index = page_last_index(s->curr);
		s->last_term = page_last_term(s->curr);
	}
}
