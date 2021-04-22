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

#ifndef RESQL_CMD_H
#define RESQL_CMD_H

#include "meta.h"

#include "sc/sc_buf.h"

enum cmd_id
{
	CMD_INIT,
	CMD_META,
	CMD_TERM,
	CMD_REQUEST,
	CMD_CONNECT,
	CMD_DISCONNECT,
	CMD_TIMESTAMP,
	CMD_INFO,
	CMD_LOG
};

struct cmd_init {
	uint64_t monotonic;
	uint64_t realtime;
	unsigned char rand[256];
};

struct cmd_meta {
	struct meta meta;
};

struct cmd_term {
	uint64_t monotonic;
	uint64_t realtime;
};

struct cmd_connect {
	const char *name;
	const char *local;
	const char *remote;
};

struct cmd_disconnect {
	const char *name;
	bool clean;
};

struct cmd_timestamp {
	uint64_t monotonic;
	uint64_t realtime;
};

struct cmd_log {
	const char *level;
	const char *log;
};

struct cmd {
	union {
		struct cmd_init init;
		struct cmd_meta meta;
		struct cmd_term term;
		struct cmd_connect connect;
		struct cmd_disconnect disconnect;
		struct cmd_timestamp timestamp;
		struct cmd_log log;
	};
};

void cmd_encode_init(struct sc_buf *b, unsigned char rand[256]);
struct cmd_init cmd_decode_init(struct sc_buf *b);

void cmd_encode_meta(struct sc_buf *b, struct meta *meta);
struct cmd_meta cmd_decode_meta(struct sc_buf *b);

void cmd_encode_term(struct sc_buf *b);
struct cmd_term cmd_decode_term(struct sc_buf *b);

void cmd_encode_connect(struct sc_buf *b, const char *name,
			       const char *local, const char *remote);
struct cmd_connect cmd_decode_connect(struct sc_buf *buf);

void cmd_encode_disconnect(struct sc_buf *b, const char *name, bool clean);
struct cmd_disconnect cmd_decode_disconnect(struct sc_buf *b);

void cmd_encode_timestamp(struct sc_buf *b);
struct cmd_timestamp cmd_decode_timestamp(struct sc_buf *b);

void cmd_encode_log(struct sc_buf *b, const char *level, const char *log);
struct cmd_log cmd_decode_log(struct sc_buf *b);

#endif
