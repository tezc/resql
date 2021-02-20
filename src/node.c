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


#include "msg.h"
#include "node.h"
#include "server.h"

#include "sc/sc.h"
#include "sc/sc_queue.h"
#include "sc/sc_uri.h"

struct node *node_create(const char *name, struct server *server, bool connect)
{
    struct node *n;

    n = rs_malloc(sizeof(*n));
    *n = (struct node){0};

    n->loop = &server->loop;
    n->timer = &server->timer;
    n->name = sc_str_create(name);
    n->next_index = 1;
    n->round_index = 0;
    n->match_index = 0;
    n->conn_timer = SC_TIMER_INVALID;
    n->interval = 64;

    sc_queue_create(n->uris, 2);
    conn_init(&n->conn, n->loop, n->timer);
    sc_buf_init(&n->info, 1024);

    if (connect) {
        n->conn_timer = sc_timer_add(n->timer, 0, SERVER_TIMER_CONNECT, n);
    }

    return n;
}

void node_destroy(struct node *n)
{
    struct sc_uri *uri;

    conn_term(&n->conn);
    sc_str_destroy(n->name);
    sc_timer_cancel(n->timer, &n->conn_timer);

    sc_queue_foreach (n->uris, uri) {
        sc_uri_destroy(uri);
    }
    sc_queue_destroy(n->uris);

    sc_buf_term(&n->info);
    rs_free(n);
}

void node_disconnect(struct node *n)
{
    conn_disconnect(&n->conn);
    node_clear_indexes(n);
}

void node_update_indexes(struct node *n, uint64_t round, uint64_t match)
{
    n->round_index = round;
    n->match_index = match;
}

void node_clear_indexes(struct node *n)
{
    n->next_index = 1;
    n->match_index = 0;
    n->round_index = 0;
    n->append_inflight = 0;
    n->ss_index = 0;
    n->ss_pos = 0;
    n->ss_inflight = 0;
}

void node_add_uris(struct node *n, struct sc_uri **uris)
{
    struct sc_uri *uri;

    sc_array_foreach (uris, uri) {
        sc_queue_add_last(n->uris, sc_uri_create(uri->str));
    }
}

int node_try_connect(struct node *n)
{
    uint64_t timeout;
    struct sc_uri *uri;

    if (n->conn.state != CONN_CONNECTED) {
        n->interval = sc_min(32 * 1024, n->interval * 2);
    }

    timeout = (rs_rand() % 256) + n->interval;
    n->conn_timer = sc_timer_add(n->timer, timeout, SERVER_TIMER_CONNECT, n);

    if (n->conn.state == CONN_CONNECTED) {
        n->interval = 64;
        return RS_EXISTS;
    }

    uri = sc_queue_del_first(n->uris);
    sc_queue_add_last(n->uris, uri);

    return conn_try_connect(&n->conn, uri);
}

void node_set_conn(struct node *n, struct conn *conn)
{
    conn_disconnect(&n->conn);
    conn_clear_events(conn);
    conn_term(&n->conn);
    conn_move(&n->conn, conn);
    conn_set_type(&n->conn, SERVER_FD_NODE_RECV);
    conn_allow_read(&n->conn);
    node_clear_indexes(n);
}
