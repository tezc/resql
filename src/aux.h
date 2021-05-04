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

#ifndef RESQL_AUX_H
#define RESQL_AUX_H

#include "sqlite/sqlite3.h"

#include <stdint.h>

struct sc_buf;
struct info;
struct session;

struct aux {
	sqlite3 *db;

	sqlite3_stmt *begin;
	sqlite3_stmt *commit;
	sqlite3_stmt *rollback;
	sqlite3_stmt *add_info;
	sqlite3_stmt *rm_info;
	sqlite3_stmt *add_session;
	sqlite3_stmt *rm_session;
	sqlite3_stmt *add_stmt;
	sqlite3_stmt *rm_stmt;
	sqlite3_stmt *rm_all_stmts;
	sqlite3_stmt *add_log;
	sqlite3_stmt *rotate_log;
};

int aux_init(struct aux *aux, const char *path, int mode);
int aux_term(struct aux *aux);

int aux_load_to_memory(struct aux *aux, const char *from);
int aux_prepare(struct aux *aux);

int aux_clear_info(struct aux *aux);
int aux_write_info(struct aux *aux, struct info *info);

int aux_add_log(struct aux *aux, uint64_t id, const char *level,
		const char *log);

int aux_write_session(struct aux *aux, struct session *s);
int aux_read_session(struct aux *aux, struct session *s, sqlite3_stmt *sess_tb,
		     sqlite3_stmt *stmt_tb);
int aux_del_session(struct aux *aux, struct session *s);
int aux_clear_sessions(struct aux *aux);
int aux_write_kv(struct aux *aux, const char *key, struct sc_buf *buf);
int aux_read_kv(struct aux *aux, const char *key, struct sc_buf *buf);

int aux_add_stmt(struct aux *aux, const char *client, uint64_t cid, uint64_t id,
		 const char *sql);
int aux_rm_stmt(struct aux *aux, uint64_t id);
int aux_rc(int rc);
void aux_clear(sqlite3_stmt *stmt);

#endif
