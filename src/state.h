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

#ifndef RESQL_STATE_H
#define RESQL_STATE_H

#include "aux.h"
#include "meta.h"
#include "rs.h"

#include "sc/sc.h"
#include "sc/sc_list.h"
#include "sc/sc_map.h"

struct state_cb {
	void *arg;
	const char *(*add_node)(void *arg, const char *node);
	const char *(*remove_node)(void *arg, const char *node);
	const char *(*shutdown)(void *arg, const char *node);
};

struct state {
	struct state_cb cb;
	char *path;
	char *ss_path;
	char *ss_tmp_path;
	char *last_err;
	struct session *session; // current session

	bool closed;

	// operation flags
	bool client;
	bool readonly;
	bool full;
	int64_t max_page;

	struct aux aux;
	struct meta meta;
	uint64_t term;
	uint64_t index;

	// time
	uint64_t timestamp;
	uint64_t monotonic;
	uint64_t realtime;

	struct sc_list disconnects;

	struct sc_map_sv nodes;
	struct sc_map_sv names;
	struct sc_map_64v ids;
	struct sc_buf tmp;

	struct sc_rand rrand;
	struct sc_rand wrand;
	char err[128];
};

int state_global_init();
int state_global_shutdown();

void state_init(struct state *st, struct state_cb cb, const char *path,
		const char *name);
int state_term(struct state *st);

void state_config(sqlite3_context *ctx, int argc, sqlite3_value **argv);
int state_randomness(sqlite3_vfs *vfs, int size, char *out);
int state_currenttime(sqlite3_vfs *vfs, sqlite3_int64 *val);

int state_read_snapshot(struct state *st, bool in_memory);
int state_read_for_snapshot(struct state *st);

int state_open(struct state *st, bool in_memory);
int state_close(struct state *st);

int state_initial_snapshot(struct state *st);
int state_apply_readonly(struct state *st, uint64_t cid, unsigned char *buf,
			 uint32_t len, struct sc_buf *resp);

int state_apply(struct state *st, uint64_t index, unsigned char *e,
		struct session **s);

#endif
