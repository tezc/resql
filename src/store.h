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

#ifndef RESQL_STORE_H
#define RESQL_STORE_H

#include "page.h"

#include <stddef.h>
#include <stdint.h>

struct store {
	char *path;

	uint64_t ss_index;
	uint64_t ss_term;
	bool ss_inprogress;

	struct page *pages[2];
	struct page *curr;

	uint64_t last_term;
	uint64_t last_index;
};

int store_init(struct store *s, const char *path, uint64_t ss_term,
	       uint64_t ss_index);
int store_term(struct store *s);

void store_flush(struct store *s);
void store_snapshot_taken(struct store *s);

uint64_t store_ss_index(struct store *s);
struct page *store_ss_page(struct store *s);
int store_expand(struct store *s);

int store_create_entry(struct store *s, uint64_t term, uint64_t seq,
		       uint64_t cid, uint32_t flags, void *data, uint32_t len);

int store_put_entry(struct store *store, uint64_t index, unsigned char *entry);

unsigned char *store_get_entry(struct store *s, uint64_t index);
uint64_t store_prev_term_of(struct store *s, uint64_t index);

void store_entries(struct store *s, uint64_t index, uint32_t limit,
		   unsigned char **entries, uint32_t *size, uint32_t *count);

void store_remove_after(struct store *s, uint64_t index);

#endif
