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

#include "session.h"

#include "rs.h"
#include "state.h"

#include "sc/sc_str.h"

#include <time.h>

struct session *session_create(struct state *state, const char *name,
			       uint64_t id)
{
	struct session *s;

	s = rs_calloc(1, sizeof(*s));

	s->state = state;
	s->name = sc_str_create(name);
	s->local = sc_str_create("");
	s->remote = sc_str_create("");
	s->id = id;
	s->seq = 0;
	s->connect_time = 0;

	sc_map_init_64v(&s->stmts, 0, 0);
	sc_buf_init(&s->resp, 64);
	sc_list_init(&s->list);

	return s;
}

void session_destroy(struct session *s)
{
	sqlite3_stmt *stmt;

	sc_list_del(NULL, &s->list);
	sc_buf_term(&s->resp);
	sc_str_destroy(&s->name);
	sc_str_destroy(&s->local);
	sc_str_destroy(&s->remote);
	sc_str_destroy(&s->connect_time);

	sc_map_foreach_value (&s->stmts, stmt) {
		sqlite3_finalize(stmt);
	}
	sc_map_term_64v(&s->stmts);

	rs_free(s);
}

void session_connected(struct session *s, const char *local, const char *remote,
		       uint64_t ts)
{
	size_t n;
	char tmp[32] = {0};
	struct tm tm, *p;
	time_t t = (time_t) ts / 1000;

	s->disconnect_time = 0;
	sc_list_del(NULL, &s->list);
	sc_str_set(&s->local, local);
	sc_str_set(&s->remote, remote);

	// Just return on error, this is only informative data.
	p = localtime_r(&t, &tm);
	if (!p) {
		return;
	}

	n = strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", p);
	if (n == 0) {
		return;
	}

	sc_str_set(&s->connect_time, tmp);
}

void session_disconnected(struct session *s, uint64_t timestamp)
{
	s->disconnect_time = timestamp;
	sc_str_set(&s->local, "");
	sc_str_set(&s->remote, "");
}

uint64_t session_sql_to_id(struct session *s, const char *sql)
{
	uint64_t prev_id;
	sqlite3_stmt *prev;

	sc_map_foreach (&s->stmts, prev_id, prev) {
		if (strcmp(sql, sqlite3_sql(prev)) == 0) {
			return prev_id;
		}
	}

	return 0;
}

void session_add_stmt(struct session *s, uint64_t id, void *stmt)
{
	sc_map_put_64v(&s->stmts, id, stmt);
}

int session_del_stmt(struct session *s, uint64_t id)
{
	sqlite3_stmt *stmt;

	stmt = sc_map_del_64v(&s->stmts, id);
	if (!sc_map_found(&s->stmts)) {
		return RS_ERROR;
	}

	sqlite3_finalize(stmt);

	return RS_OK;
}

void *session_get_stmt(struct session *s, uint64_t id)
{
	sqlite3_stmt *stmt;

	stmt = sc_map_get_64v(&s->stmts, id);

	return sc_map_found(&s->stmts) ? stmt : NULL;
}
