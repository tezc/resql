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

#include "node.h"

#include "rs.h"
#include "server.h"

#include "sc/sc_array.h"
#include "sc/sc_queue.h"
#include "sc/sc_str.h"
#include "sc/sc_uri.h"

struct node *node_create(const char *name, struct server *server, bool connect)
{
	struct node *n;

	n = rs_calloc(1, sizeof(*n));

	n->poll = &server->poll;
	n->timer = &server->timer;
	n->name = sc_str_create(name);
	n->next = 1;
	n->round = 0;
	n->match = 0;
	n->conn_timer = SC_TIMER_INVALID;
	n->interval = 32;
	n->ss_index = 0;
	n->ss_pos = 0;

	sc_list_init(&n->list);
	sc_queue_create(n->uris, 2);
	conn_init(&n->conn, server);
	sc_buf_init(&n->info, 1024);

	if (connect) {
		n->conn_timer =
			sc_timer_add(n->timer, 0, SERVER_TIMER_CONNECT, n);
	}

	return n;
}

void node_destroy(struct node *n)
{
	struct sc_uri *uri;

	sc_list_del(NULL, &n->list);
	conn_term(&n->conn);
	sc_str_destroy(&n->name);
	sc_timer_cancel(n->timer, &n->conn_timer);

	sc_queue_foreach (n->uris, uri) {
		sc_uri_destroy(&uri);
	}
	sc_queue_destroy(n->uris);

	sc_buf_term(&n->info);
	rs_free(n);
}

void node_disconnect(struct node *n)
{
	sc_list_del(NULL, &n->list);
	conn_disconnect(&n->conn);
	node_clear_indexes(n, n->match);
}

void node_update_indexes(struct node *n, uint64_t round, uint64_t match)
{
	n->round = round;
	n->match = match;
}

void node_clear_indexes(struct node *n, uint64_t match)
{
	n->next = match + 1;
	n->match = match;
	n->round = 0;
	n->ss_index = 0;
	n->ss_pos = 0;
	n->msg_inflight = 0;
	n->out_timestamp = 0;
	n->in_timestamp = 0;
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
	n->conn_timer =
		sc_timer_add(n->timer, timeout, SERVER_TIMER_CONNECT, n);

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
	node_clear_indexes(n, n->match);
}
