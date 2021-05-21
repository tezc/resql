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

#ifndef RESQL_CLIENT_H
#define RESQL_CLIENT_H

#include "conn.h"
#include "msg.h"

#include "sc/sc_buf.h"
#include "sc/sc_sock.h"
#include "sc/sc_str.h"

struct client {
	bool msg_wait;	 // msg in-progress
	bool terminated; // waiting to be deallocated

	char *name;
	uint64_t id;
	uint64_t seq;	       // current sequence
	uint64_t commit_index; // round index for read request
	uint64_t round_index;  // round index for read request

	struct conn conn;
	struct sc_list read; // read request list
	struct msg msg;	     // current msg
};

struct client *client_create(struct conn *conn, const char *name);
void client_destroy(struct client *c);
void client_print(struct client *c, char *buf, size_t len);
int client_processed(struct client *c);
bool client_pending(struct client *c);

/**
 * Mark client terminated for lazy destroy.
 *
 * When looping on epoll_wait, an event may cause a client to disconnect but
 * we may have collected that client socket's event but not yet processed.
 * This will trigger use-after-free undefined behavior. Rather than destroying
 * client, mark it terminated and call client_destroy() when you processed all
 * events from epoll_wait.
 */
void client_set_terminated(struct client *c);

#endif
