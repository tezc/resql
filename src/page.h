/*
 *  Resql
 *
 *  Copyright (C) 2021 Ozan Tezcan
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#ifndef RESQL_PAGE_H
#define RESQL_PAGE_H

#include "sc/sc_buf.h"
#include "sc/sc_list.h"
#include "sc/sc_mmap.h"

#include <stdint.h>

struct page
{
    char *path;
    struct sc_mmap map;

    uint64_t prev_index;
    uint32_t flush_pos;
    uint32_t flush_index;

    struct sc_buf buf;
    char **entries;
};

int page_init(struct page *p, const char *path, int64_t len, uint64_t prev_index);
int page_term(struct page *p);
int page_expand(struct page *p);
bool page_isempty(struct page *p);
uint64_t page_next_index(struct page *p);
void page_clear(struct page *p, uint64_t prev_index);
int page_fsync(struct page *p, uint64_t index);

uint32_t page_entry_count(struct page *p);
uint32_t page_quota(struct page *p);

uint64_t page_last_index(struct page *p);
uint64_t page_last_term(struct page *p);
char *page_last_entry(struct page *p);

void page_create_entry(struct page *p, uint64_t term, uint64_t seq,
                       uint64_t cid, uint32_t flags, void *data, uint32_t len);

void page_put_entry(struct page *p, char *entry);

char *page_entry_at(struct page *p, uint64_t index);
void page_remove_after(struct page *p, uint64_t index);

void page_get_entries(struct page *p, uint64_t index, uint32_t limit,
                      char **entries, uint32_t *size, uint32_t *count);

#endif
