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

#ifndef RESQL_PAGE_H
#define RESQL_PAGE_H

#include "sc/sc_buf.h"
#include "sc/sc_mmap.h"

#include <stdint.h>

struct page {
	bool init;
	char *path;
	struct sc_mmap map;

	uint64_t prev_index;
	uint32_t flush_pos;
	uint64_t flush_index;

	struct sc_buf buf;
	unsigned char **entries;
};

int page_init(struct page *p, const char *path, int64_t len,
	      uint64_t prev_index);
void page_term(struct page *p);
int page_reserve(struct page *p, uint32_t size);
bool page_isempty(struct page *p);

void page_clear(struct page *p, uint64_t prev_index);
void page_fsync(struct page *p, uint64_t index);

uint32_t page_entry_count(struct page *p);
uint32_t page_quota(struct page *p);
uint32_t page_cap(struct page *p);

uint64_t page_last_index(struct page *p);
uint64_t page_last_term(struct page *p);
unsigned char *page_last_entry(struct page *p);

void page_create_entry(struct page *p, uint64_t term, uint64_t seq,
		       uint64_t cid, uint32_t flags, void *data, uint32_t len);

void page_put_entry(struct page *p, unsigned char *entry);

unsigned char *page_entry_at(struct page *p, uint64_t index);
void page_remove_after(struct page *p, uint64_t index);

void page_get_entries(struct page *p, uint64_t index, uint32_t limit,
		      unsigned char **entries, uint32_t *size, uint32_t *count);

#endif
