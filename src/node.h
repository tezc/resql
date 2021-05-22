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

#ifndef RESQL_NODE_H
#define RESQL_NODE_H

#include "conn.h"
#include "meta.h"

#include "sc/sc_list.h"
#include "sc/sc_map.h"
#include "sc/sc_queue.h"

struct node {
	struct server *server;
	struct sc_sock_poll *poll;
	struct sc_timer *timer;
	struct conn conn;

	struct sc_list list;
	struct sc_queue_ptr uris; // URLs
	uint64_t timeout;         // Reconnect timeout
	uint64_t interval;        // Reconnect trial interval
	char *name;               // Node name

	uint64_t next;            // Next entry index
	uint64_t match;           // Entry match index
	uint64_t round;           // Round match index

	uint64_t ss_pos;          // Snapshot offset
	uint64_t ss_index;        // Snapshot index
	uint64_t msg_inflight;    // Number of inflight messages

	int id;                   // Node id
	const char *status;       // Status, e.g online, offline, disk_full

	uint64_t in_timestamp;    // Latest in timestamp
	uint64_t out_timestamp;   // Latest out timestamp
	struct sc_buf info;       // Metrics
};

struct node *node_create(const char *name, struct server *server, bool connect);
void node_destroy(struct node *n);

void node_disconnect(struct node *n);
void node_update_indexes(struct node *n, uint64_t round, uint64_t match);
void node_clear_indexes(struct node *n, uint64_t match);
void node_add_uris(struct node *n, struct sc_array_ptr *uris);
int node_try_connect(struct node *n, unsigned int randtimer);
int node_set_conn(struct node *n, struct conn *conn);
bool node_connected(struct node *n);

#endif
