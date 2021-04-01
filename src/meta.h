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

#ifndef RESQL_META_H
#define RESQL_META_H

#include "sc/sc_buf.h"

#include <stdint.h>

enum meta_role
{
    META_LEADER,
    META_FOLLOWER,
};

extern const char *meta_role_str[];

struct meta_node
{
    char *name;
    bool connected;
    enum meta_role role;
    struct sc_uri **uris;
};

struct meta
{
    char *name;
    char *uris;
    uint64_t term;
    uint64_t index;
    uint32_t voter;
    struct meta_node *nodes;

    struct meta *prev;
};

void meta_init(struct meta *m, const char *cluster_name);
void meta_term(struct meta *m);

void meta_copy(struct meta *m, struct meta *src);
void meta_encode(struct meta *m, struct sc_buf *buf);
void meta_decode(struct meta *m, struct sc_buf *buf);
bool meta_add(struct meta *meta, struct sc_uri *uri);
bool meta_remove(struct meta *meta, const char *name);
bool meta_exists(struct meta *m, const char *name);
void meta_remove_prev(struct meta *m);
void meta_rollback(struct meta *m, uint64_t index);
void meta_replace(struct meta *m, void *data, uint32_t len);

void meta_set_connected(struct meta *m, const char *name);
void meta_set_disconnected(struct meta *m, const char *name);
void meta_clear_connection(struct meta *m);
void meta_set_leader(struct meta *m, const char *name);
bool meta_parse_uris(struct meta *m, char *addrs);

void meta_print(struct meta *m, struct sc_buf *buf);

#endif
