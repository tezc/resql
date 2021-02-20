/*
 *  Resql
 *
 *  Copyright (C) 2021 Resql Authors
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


#ifndef RESQL_STORE_H
#define RESQL_STORE_H

#include "file.h"
#include "page.h"

#include "sc/sc_buf.h"
#include "sc/sc_list.h"

#include <stddef.h>
#include <stdint.h>

struct store
{
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
int store_put_entry(struct store *store, uint64_t index, char *entry);

char *store_get_entry(struct store *s, uint64_t index);
uint64_t store_prev_term_of(struct store *s, uint64_t index);

void store_get_entries(struct store *s, uint64_t index, uint32_t limit,
                       char **entries, uint32_t *size,
                       uint32_t *count);

void store_remove_after(struct store *s, uint64_t index);

#endif
