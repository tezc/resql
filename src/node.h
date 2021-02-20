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


#ifndef RESQL_NODE_H
#define RESQL_NODE_H

#include "conn.h"
#include "msg.h"

#include "sc/sc_list.h"
#include "sc/sc_map.h"
#include "sc/sc_sock.h"
#include "sc/sc_str.h"
#include "sc/sc_uri.h"

struct node
{
    struct server *server;
    struct sc_sock_poll *loop;
    struct sc_timer *timer;
    struct conn conn;

    struct sc_uri **uris;
    uint64_t conn_timer;
    uint64_t interval;
    char *name;

    uint64_t next_index;
    uint64_t match_index;
    uint64_t round_index;
    uint64_t append_inflight;

    uint64_t ss_pos;
    uint64_t ss_index;
    uint64_t ss_inflight;

    int id;
    bool known;
    bool voted;
    uint64_t vote_term;
    enum meta_role role;

    uint64_t in_timestamp;
    uint64_t out_timestamp;
    struct sc_buf info;
};

struct node *node_create(const char *name, struct server *server, bool connect);
void node_destroy(struct node *n);

void node_disconnect(struct node *n);
void node_update_indexes(struct node *n, uint64_t round, uint64_t match);
void node_clear_indexes(struct node *n);
void node_add_uris(struct node *n, struct sc_uri** uris);
int node_try_connect(struct node *n);
void node_set_conn(struct node *n, struct conn *conn);

#endif
