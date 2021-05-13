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
void store_term(struct store *s);

void store_flush(struct store *s);
void store_snapshot_taken(struct store *s);

uint64_t store_ss_index(struct store *s);
struct page *store_ss_page(struct store *s);
int store_reserve(struct store *s, uint32_t size);
bool store_last_part(struct store *s);

int store_create_entry(struct store *s, uint64_t term, uint64_t seq,
		       uint64_t cid, uint32_t flags, void *data, uint32_t len);

int store_put_entry(struct store *store, uint64_t index, unsigned char *entry);

unsigned char *store_get_entry(struct store *s, uint64_t index);
uint64_t store_prev_term(struct store *s, uint64_t index);

void store_entries(struct store *s, uint64_t index, uint32_t limit,
		   unsigned char **entries, uint32_t *size, uint32_t *count);

void store_remove_after(struct store *s, uint64_t index);

#endif
