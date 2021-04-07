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

static void store_read(struct store *s)
{
	int rc;
	char buf[PATH_MAX];
	size_t c;

	rs_snprintf(buf, sizeof(buf), "%s/%s", s->path, DEF_PAGE_FILE_0);
	rc = page_init(s->pages[0], buf, -1, s->ss_index);
	if (rc != RS_OK) {
		rs_abort("page_init failed");
	}

	rs_snprintf(buf, sizeof(buf), "%s/%s", s->path, DEF_PAGE_FILE_1);
	rc = page_init(s->pages[1], buf, -1, s->ss_index);
	if (rc != RS_OK) {
		rs_abort("page_init failed");
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

	c = sc_array_size(s->pages[1]->entries);
	s->curr = (c > 0) ? s->pages[1] : s->pages[0];

	s->last_index = page_last_index(s->curr);
}

int store_init(struct store *s, const char *path, uint64_t ss_term,
	       uint64_t ss_index)
{
	s->path = sc_str_create(path);
	s->last_index = 0;
	s->last_term = 0;
	s->ss_term = ss_term;
	s->ss_index = ss_index;
	s->pages[0] = rs_malloc(sizeof(*s->pages[0]));
	s->pages[1] = rs_malloc(sizeof(*s->pages[0]));

	store_read(s);

	return RS_OK;
}

int store_term(struct store *s)
{
	page_term(s->pages[0]);
	page_term(s->pages[1]);

	rs_free(s->pages[0]);
	rs_free(s->pages[1]);

	sc_str_destroy(s->path);

	return RS_OK;
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
	uint32_t size;

retry:
	size = entry_encoded_len(len);
	assert(size < STORE_MAX_ENTRY_SIZE);

	if (size > page_quota(s->curr)) {
		if (s->curr != s->pages[1]) {
			s->curr = s->pages[1];
			while (size > page_quota(s->curr)) {
				page_expand(s->curr);
			}
			page_clear(s->curr, s->last_index);
			goto retry;
		}

		return RS_FULL;
	}

	page_create_entry(s->curr, term, seq, cid, flags, data, len);
	s->last_index++;
	s->last_term = term;

	return RS_OK;
}

int store_expand(struct store *s)
{
	rs_assert(s->curr == s->pages[1]);
	page_expand(s->pages[1]);
	return RS_OK;
}

int store_put_entry(struct store *s, uint64_t index, unsigned char *entry)
{
	rs_assert(index == s->last_index + 1);
	rs_assert(s->last_term <= entry_term(entry));

	uint32_t size = entry_len(entry);

retry:
	if (size > page_quota(s->curr)) {
		if (s->curr != s->pages[1]) {
			s->curr = s->pages[1];
			page_clear(s->curr, s->last_index);
			goto retry;
		}

		if (page_entry_count(s->pages[1]) == 0) {
			page_expand(s->pages[1]);
			s->curr = s->pages[1];
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

uint64_t store_prev_term_of(struct store *s, uint64_t index)
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
