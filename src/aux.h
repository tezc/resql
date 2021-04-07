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
int aux_read_info(struct aux *aux, struct info *info, sqlite3_stmt *stmt);
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

#endif
