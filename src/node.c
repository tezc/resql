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
	n->status = "offline";

	sc_list_init(&n->list);
	sc_queue_init(&n->uris);
	conn_init(&n->conn, server);
	sc_buf_init(&n->info, 1024);

	if (connect) {
		n->conn_timer = sc_timer_add(n->timer, 0, SERVER_TIMER_CONNECT, n);
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

	sc_queue_foreach (&n->uris, uri) {
		sc_uri_destroy(&uri);
	}
	sc_queue_term(&n->uris);

	sc_buf_term(&n->info);
	rs_free(n);
}

void node_disconnect(struct node *n)
{
	sc_list_del(NULL, &n->list);
	conn_term(&n->conn);
	node_clear_indexes(n, n->match);
	n->status = "offline";
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

void node_add_uris(struct node *n, struct sc_array_ptr *uris)
{
	struct sc_uri *uri;

	sc_array_foreach (uris, uri) {
		sc_queue_add_last(&n->uris, sc_uri_create(uri->str));
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

	uri = sc_queue_del_first(&n->uris);
	sc_queue_add_last(&n->uris, uri);

	return conn_try_connect(&n->conn, uri);
}

int node_set_conn(struct node *n, struct conn *conn)
{
	int rc;

	conn_term(&n->conn);

	rc = conn_set(&n->conn, conn);
	if (rc != RS_OK) {
		return rc;
	}

	conn_set_type(&n->conn, SERVER_FD_NODE_RECV);
	node_clear_indexes(n, n->match);
	n->status = "connected";

	return RS_OK;
}

bool node_connected(struct node *n)
{
	return n->conn.state == CONN_CONNECTED;
}
