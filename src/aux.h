/*
 *  Resql
 *
 *  Copyright (C) 2021 Resql Authors
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef RESQL_AUX_H
#define RESQL_AUX_H

#include "info.h"
#include "meta.h"
#include "session.h"

#include "sqlite/sqlite3.h"

struct aux
{
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
};

int aux_init(struct aux *aux, const char *path, int mode);
int aux_term(struct aux *aux);

int aux_load_to_memory(struct aux *aux, const char *from);
int aux_prepare(struct aux *aux);

int aux_clear_info(struct aux *aux);
int aux_write_info(struct aux *aux, struct info *info);
int aux_read_info(struct aux *aux, struct info *info, sqlite3_stmt *stmt);

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
