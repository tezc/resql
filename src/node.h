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

#ifndef RESQL_NODE_H
#define RESQL_NODE_H

#include "conn.h"
#include "meta.h"

#include "sc/sc_list.h"
#include "sc/sc_map.h"

struct node {
	struct server *server;
	struct sc_sock_poll *poll;
	struct sc_timer *timer;
	struct conn conn;

	struct sc_list list;
	struct sc_uri **uris;
	uint64_t conn_timer;
	uint64_t interval;
	char *name;

	uint64_t next;
	uint64_t match;
	uint64_t round;

	uint64_t ss_pos;
	uint64_t ss_index;
	uint64_t msg_inflight;

	int id;
	bool known;
	bool voted;
	enum meta_role role;

	uint64_t in_timestamp;
	uint64_t out_timestamp;
	struct sc_buf info;
};

struct node *node_create(const char *name, struct server *server, bool connect);
void node_destroy(struct node *n);

void node_disconnect(struct node *n);
void node_update_indexes(struct node *n, uint64_t round, uint64_t match);
void node_clear_indexes(struct node *n, uint64_t match);
void node_add_uris(struct node *n, struct sc_uri **uris);
int node_try_connect(struct node *n);
void node_set_conn(struct node *n, struct conn *conn);

#endif
