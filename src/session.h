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

#ifndef RESQL_SESSION_H
#define RESQL_SESSION_H

#include "sc/sc_buf.h"
#include "sc/sc_list.h"
#include "sc/sc_map.h"

#include <stdint.h>

struct session {
	struct state *state;
	struct sc_list list;
	char *name;
	char *local;
	char *remote;
	char *connect_time;
	uint64_t id;
	uint64_t seq;
	uint64_t disconnect_time;

	struct sc_buf resp;
	struct sc_map_64v stmts;
};

struct session *session_create(struct state *st, const char *name, uint64_t id);
void session_destroy(struct session *s);

void session_connected(struct session *s, const char *local, const char *remote,
		       uint64_t ts);
void session_disconnected(struct session *s, uint64_t timestamp);

uint64_t session_sql_to_id(struct session *s, const char *sql);

void session_add_stmt(struct session *s, uint64_t id, void *stmt);
void *session_get_stmt(struct session *s, uint64_t id);
int session_del_stmt(struct session *s, uint64_t id);

#endif
