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

#ifndef RESQL_CLIENT_H
#define RESQL_CLIENT_H

#include "conn.h"
#include "msg.h"

#include "sc/sc_buf.h"
#include "sc/sc_sock.h"
#include "sc/sc_str.h"

struct client {
	struct conn conn;
	struct sc_list list;

	char *name;
	uint64_t ts;
	uint64_t id;
	uint64_t seq;

	struct msg msg;
	uint64_t commit_index;
	uint64_t round_index;
	struct sc_list read;
	bool msg_wait;
	bool terminated;
};

struct client *client_create(struct conn *conn, const char *name);
void client_destroy(struct client *c);
void client_print(struct client *c, char *buf, size_t len);
void client_processed(struct client *c);
bool client_pending(struct client *c);

#endif
