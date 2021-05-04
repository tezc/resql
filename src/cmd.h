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
struct cmd_config {
	bool add;
	const char *name;
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
		struct cmd_config config;
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

void cmd_encode_connect(struct sc_buf *b, const char *name, const char *local,
			const char *remote);
struct cmd_connect cmd_decode_connect(struct sc_buf *buf);

void cmd_encode_disconnect(struct sc_buf *b, const char *name, bool clean);
struct cmd_disconnect cmd_decode_disconnect(struct sc_buf *b);

void cmd_encode_timestamp(struct sc_buf *b);
struct cmd_timestamp cmd_decode_timestamp(struct sc_buf *b);

void cmd_encode_log(struct sc_buf *b, const char *level, const char *log);
struct cmd_log cmd_decode_log(struct sc_buf *b);

#endif
