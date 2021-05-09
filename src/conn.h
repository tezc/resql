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

#ifndef RESQL_CONN_H
#define RESQL_CONN_H

#include "msg.h"

#include "sc/sc_buf.h"
#include "sc/sc_list.h"
#include "sc/sc_sock.h"

#include <stdint.h>

struct sc_uri;
struct server;

enum conn_state
{
	CONN_DISCONNECTED,
	CONN_TCP_ATTEMPT,
	CONN_CONNECTED,
};

struct conn {
	struct sc_list list;
	enum conn_state state;
	struct sc_sock sock;
	struct sc_buf out;
	struct sc_buf in;
	struct msg msg;
	struct server *server;
	uint64_t timer_id;

	char local[64];
	char remote[64];
};

struct conn *conn_create(struct server *server, struct sc_sock *s);
void conn_destroy(struct conn *c);

void conn_init(struct conn *c, struct server *s);
void conn_term(struct conn *c);

void conn_clear_buf(struct conn *c);
struct sc_buf *conn_out(struct conn *c);
int conn_set(struct conn *c, struct conn *src);
void conn_set_type(struct conn *c, int type);

void conn_schedule(struct conn *c, uint32_t type, uint32_t timeout);

int conn_try_connect(struct conn *c, struct sc_uri *uri);
int conn_on_out_connected(struct conn *c);

int conn_on_writable(struct conn *c);
int conn_on_readable(struct conn *c);
int conn_register(struct conn *c, bool read, bool write);
int conn_unregister(struct conn *c, bool read, bool write);

int conn_flush(struct conn *c);

#endif
