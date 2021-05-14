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

#ifndef RESQL_PAGE_H
#define RESQL_PAGE_H

#include "sc/sc_array.h"
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
	struct sc_array_ustr entries;
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
bool page_last_part(struct page *p);

uint64_t page_prev_index(struct page *p);
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
