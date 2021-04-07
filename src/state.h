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
void state_term(struct state *st);

void state_config(sqlite3_context *ctx, int argc, sqlite3_value **argv);
int state_randomness(sqlite3_vfs *vfs, int size, char *out);
int state_currenttime(sqlite3_vfs *vfs, sqlite3_int64 *val);

int state_read_snapshot(struct state *st, bool in_memory);
int state_read_for_snapshot(struct state *st);

void state_open(struct state *st, bool in_memory);
void state_close(struct state *st);

void state_initial_snapshot(struct state *st);
int state_apply_readonly(struct state *st, uint64_t cid, unsigned char *buf,
			 uint32_t len, struct sc_buf *resp);

struct session *state_apply(struct state *st, uint64_t index,
			    unsigned char *entry);

#endif
