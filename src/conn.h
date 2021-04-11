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

void conn_clear_inbuf(struct conn *c);
void conn_clear_outbuf(struct conn *c);
void conn_clear_bufs(struct conn *c);
struct sc_buf *conn_outbuf(struct conn *c);
void conn_move(struct conn *c, struct conn *src);
void conn_init(struct conn *c, struct server *s);
void conn_term(struct conn *c);

void conn_schedule(struct conn *c, uint32_t type, uint32_t timeout);
void conn_clear_timer(struct conn *c);

int conn_try_connect(struct conn *c, struct sc_uri *uri);
void conn_disconnect(struct conn *c);
int conn_on_out_connected(struct conn *c);

int conn_on_writable(struct conn *c);
int conn_on_readable(struct conn *c);
void conn_allow_read(struct conn *c);
void conn_set_type(struct conn *c, int type);
void conn_disallow_read(struct conn *c);
void conn_clear_events(struct conn *c);
int conn_flush(struct conn *c);

#endif
