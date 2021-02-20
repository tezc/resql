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

#ifndef RESQL_SNAPSHOT_H
#define RESQL_SNAPSHOT_H

#include "sc/sc_cond.h"
#include "sc/sc_mmap.h"
#include "sc/sc_sock.h"
#include "sc/sc_thread.h"

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

#define SNAPSHOT_MAX_PAGE_COUNT 4

struct server;
struct page;

struct snapshot
{
    struct server *server;
    struct sc_mmap map;
    char *path;
    char *tmp_path;

    // Current index and size
    uint64_t index;
    uint64_t term;

    // Latest snapshot
    uint64_t time;
    uint64_t size;
    uint64_t latest_index;
    uint64_t latest_term;

    // Recv
    uint64_t recv_index;
    uint64_t recv_term;
    char *tmp_recv_path;
    struct file *tmp;

    struct sc_thread thread;
    struct sc_sock_pipe efd;
    struct sc_cond cond;
};

void snapshot_init(struct snapshot *ss, struct server *server);
void snapshot_term(struct snapshot *ss);

void snapshot_open(struct snapshot *ss, const char *path, uint64_t term,
                   uint64_t index);

void snapshot_replace(struct snapshot *ss);

void snapshot_take(struct snapshot *ss, struct page *page);
int snapshot_recv(struct snapshot *ss, uint64_t term, uint64_t index, bool done,
                  uint64_t offset, void *data, uint64_t len);
void snapshot_clear(struct snapshot *ss);


#endif
