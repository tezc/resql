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

#include "aux.h"

#include "info.h"
#include "rs.h"
#include "session.h"
#include "state.h"

#include "sc/sc_log.h"
#include "sc/sc_str.h"

int sqlite3_completion_init(sqlite3 *db, char **pzErrMsg,
			    const sqlite3_api_routines *pApi);

void aux_random(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
	(void) argv;

	rs_assert(argc == 0);
	int64_t val;

	state_randomness(NULL, sizeof(val), (char *) &val);
	sqlite3_result_int64(ctx, val);
}

void aux_randomblob(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
	sqlite3_int64 n;
	char *p;

	rs_assert(argc == 1);

	n = sqlite3_value_int64(argv[0]);
	if (n < 1) {
		n = 1;
	}

	p = sqlite3_malloc64((sqlite3_uint64) n);
	if (!p) {
		sqlite3_result_error_nomem(ctx);
		return;
	}

	state_randomness(NULL, (int) n, p);
	sqlite3_result_blob(ctx, p, (int) n, sqlite3_free);
}

void aux_config(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
	state_config(ctx, argc, argv);
}

int aux_init(struct aux *aux, const char *path, int mode)
{
	int rc;

	*aux = (struct aux){0};

	rc = sqlite3_open_v2(path, &aux->db, SQLITE_OPEN_READWRITE | mode,
			     NULL);
	if (rc != SQLITE_OK) {
		goto error;
	}

	rc = sqlite3_create_function(aux->db, "RANDOM", 0, SQLITE_UTF8, NULL,
				     aux_random, NULL, NULL);
	if (rc != SQLITE_OK) {
		goto error;
	}

	rc = sqlite3_create_function(aux->db, "RANDOMBLOB", 1, SQLITE_UTF8,
				     NULL, aux_randomblob, NULL, NULL);
	if (rc != SQLITE_OK) {
		goto error;
	}

	rc = sqlite3_create_function(aux->db, "RESQL", -1, SQLITE_UTF8, NULL,
				     aux_config, NULL, NULL);
	if (rc != SQLITE_OK) {
		goto error;
	}

	return aux_prepare(aux);

error:
	sc_log_error("sqlite3 : %s \n", sqlite3_errstr(rc));
	return RS_ERROR;
}

int aux_term(struct aux *aux)
{
	int rc;

	if (aux->db != NULL) {
		sqlite3_finalize(aux->begin);
		sqlite3_finalize(aux->commit);
		sqlite3_finalize(aux->rollback);
		sqlite3_finalize(aux->add_info);
		sqlite3_finalize(aux->rm_info);
		sqlite3_finalize(aux->add_session);
		sqlite3_finalize(aux->rm_session);
		sqlite3_finalize(aux->add_stmt);
		sqlite3_finalize(aux->rm_stmt);
		sqlite3_finalize(aux->rm_all_stmts);
		sqlite3_finalize(aux->add_log);
		sqlite3_finalize(aux->rotate_log);

		rc = sqlite3_close(aux->db);
		if (rc != SQLITE_OK) {
			sc_log_error("sqlite3_close : %s \n",
				     sqlite3_errmsg(aux->db));
			return RS_ERROR;
		}

		aux->db = NULL;
	}

	return RS_OK;
}

int aux_load_to_memory(struct aux *aux, const char *from)
{
	int rc;
	sqlite3 *file;
	sqlite3_backup *backup;

	rc = sqlite3_open_v2(from, &file, SQLITE_OPEN_READONLY, NULL);
	if (rc != SQLITE_OK) {
		rs_abort("sqlite : %s \n", sqlite3_errstr(rc));
	}

	backup = sqlite3_backup_init(aux->db, "main", file, "main");
	if (backup) {
		rc = sqlite3_backup_step(backup, -1);
		if (rc != SQLITE_DONE) {
			rs_abort("sqlite : %s \n", sqlite3_errstr(rc));
		}

		rc = sqlite3_backup_finish(backup);
		if (rc != SQLITE_OK) {
			rs_abort("sqlite : %s \n", sqlite3_errstr(rc));
		}
	}

	rc = sqlite3_close(file);
	if (rc != SQLITE_OK) {
		rs_abort("sqlite : %s \n", sqlite3_errstr(rc));
	}

	return RS_OK;
}

int aux_prepare(struct aux *aux)
{
	int rc;
	const char *sql;

	rc = sqlite3_completion_init(aux->db, 0, 0);
	if (rc != SQLITE_OK) {
		goto error;
	}

	sql = "CREATE TABLE IF NOT EXISTS resql_log "
	      "(id INTEGER PRIMARY KEY, date TEXT, level TEXT, log TEXT)";
	rc = sqlite3_exec(aux->db, sql, 0, 0, 0);
	if (rc != SQLITE_OK) {
		goto error;
	}

	sql = "INSERT INTO resql_log VALUES (?, datetime(), ?, ?)";
	rc = sqlite3_prepare_v3(aux->db, sql, -1, true, &aux->add_log, NULL);
	if (rc != SQLITE_OK) {
		goto error;
	}

	sql = "DELETE FROM resql_log "
	      "WHERE id = (SELECT id "
	      "            FROM resql_log "
	      "            ORDER BY id DESC "
	      "            LIMIT 1 OFFSET 1000)";
	rc = sqlite3_prepare_v3(aux->db, sql, -1, true, &aux->rotate_log, NULL);
	if (rc != SQLITE_OK) {
		goto error;
	}

	sql = "CREATE TABLE IF NOT EXISTS resql_kv "
	      "(key TEXT PRIMARY KEY, value blob)";
	rc = sqlite3_exec(aux->db, sql, 0, 0, 0);
	if (rc != SQLITE_OK) {
		goto error;
	}

	sql = "CREATE TABLE IF NOT EXISTS resql_info ("
	      "name TEXT PRIMARY KEY,"
	      "connected TEXT,"
	      "role TEXT,"
	      "urls TEXT,"
	      "version TEXT,"
	      "git_branch TEXT,"
	      "git_commit TEXT,"
	      "machine TEXT,"
	      "arch TEXT,"
	      "pid TEXT,"
	      "current_time TEXT,"
	      "start_date TEXT,"
	      "start_time TEXT,"
	      "uptime_seconds TEXT,"
	      "uptime_days TEXT,"
	      "cpu_sys TEXT,"
	      "cpu_user TEXT,"
	      "network_recv_bytes TEXT,"
	      "network_send_bytes TEXT,"
	      "network_recv TEXT,"
	      "network_send TEXT,"
	      "total_memory_bytes TEXT,"
	      "total_memory TEXT,"
	      "used_memory_bytes TEXT,"
	      "used_memory TEXT,"
	      "fsync_max_ms TEXT,"
	      "fsync_average_ms TEXT,"
	      "snapshot_success TEXT,"
	      "snapshot_size_bytes TEXT,"
	      "snapshot_size TEXT,"
	      "snapshot_max_ms TEXT,"
	      "snapshot_average_ms TEXT,"
	      "dir TEXT,"
	      "disk_used_bytes TEXT,"
	      "disk_used TEXT,"
	      "disk_free_bytes TEXT,"
	      "disk_free TEXT);";
	rc = sqlite3_exec(aux->db, sql, 0, 0, 0);
	if (rc != SQLITE_OK) {
		goto error;
	}

	sql = "CREATE TABLE IF NOT EXISTS resql_sessions ("
	      "client_name TEXT PRIMARY KEY, "
	      "client_id INTEGER,"
	      "sequence INTEGER,"
	      "local TEXT,"
	      "remote TEXT,"
	      "connect_time TEXT,"
	      "resp BLOB);";
	rc = sqlite3_exec(aux->db, sql, 0, 0, 0);
	if (rc != SQLITE_OK) {
		goto error;
	}

	sql = "CREATE TABLE IF NOT EXISTS resql_statements "
	      "(id INTEGER PRIMARY KEY, "
	      "client_id INTEGER,"
	      "client_name TEXT,"
	      "sql TEXT);";
	rc = sqlite3_exec(aux->db, sql, 0, 0, 0);
	if (rc != SQLITE_OK) {
		goto error;
	}

	sql = "CREATE INDEX IF NOT EXISTS resql_statements_cid  ON "
	      "resql_statements(client_id);";
	rc = sqlite3_exec(aux->db, sql, 0, 0, 0);
	if (rc != SQLITE_OK) {
		goto error;
	}

	rc = sqlite3_prepare_v3(aux->db, "BEGIN;", -1,
				SQLITE_PREPARE_PERSISTENT, &aux->begin, NULL);
	if (rc != SQLITE_OK) {
		goto error;
	}

	rc = sqlite3_prepare_v3(aux->db, "COMMIT;", -1,
				SQLITE_PREPARE_PERSISTENT, &aux->commit, NULL);
	if (rc != SQLITE_OK) {
		goto error;
	}

	rc = sqlite3_prepare_v3(aux->db, "ROLLBACK;", -1,
				SQLITE_PREPARE_PERSISTENT, &aux->rollback,
				NULL);
	if (rc != SQLITE_OK) {
		goto error;
	}

	sql = "INSERT OR REPLACE INTO resql_info VALUES ("
	      "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?,"
	      "?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
	rc = sqlite3_prepare_v3(aux->db, sql, -1, true, &aux->add_info, NULL);
	if (rc != SQLITE_OK) {
		goto error;
	}

	sql = "DELETE FROM resql_info WHERE name = (?);";
	rc = sqlite3_prepare_v3(aux->db, sql, -1, true, &aux->rm_info, NULL);
	if (rc != SQLITE_OK) {
		goto error;
	}

	sql = "INSERT OR REPLACE INTO resql_sessions VALUES "
	      "(?, ?, ?, ?, ?, ?, ?);";
	rc = sqlite3_prepare_v3(aux->db, sql, -1, true, &aux->add_session,
				NULL);
	if (rc != SQLITE_OK) {
		goto error;
	}

	sql = "DELETE FROM resql_sessions WHERE client_name = (?);";
	rc = sqlite3_prepare_v3(aux->db, sql, -1, true, &aux->rm_session, NULL);
	if (rc != SQLITE_OK) {
		goto error;
	}

	sql = "INSERT OR REPLACE INTO resql_statements VALUES (?, ?, ?, ?);";
	rc = sqlite3_prepare_v3(aux->db, sql, -1, true, &aux->add_stmt, NULL);
	if (rc != SQLITE_OK) {
		goto error;
	}

	sql = "DELETE FROM resql_statements WHERE id = (?);";
	rc = sqlite3_prepare_v3(aux->db, sql, -1, true, &aux->rm_stmt, NULL);
	if (rc != SQLITE_OK) {
		goto error;
	}

	sql = "DELETE FROM resql_statements WHERE client_id = (?);";
	rc = sqlite3_prepare_v3(aux->db, sql, -1, true, &aux->rm_all_stmts,
				NULL);
	if (rc != SQLITE_OK) {
		goto error;
	}

	rc = sqlite3_exec(aux->db, "PRAGMA journal_mode=MEMORY", 0, 0, 0);
	if (rc != SQLITE_OK) {
		goto error;
	}

	rc = sqlite3_exec(aux->db, "PRAGMA locking_mode=EXCLUSIVE", 0, 0, 0);
	if (rc != SQLITE_OK) {
		goto error;
	}

	rc = sqlite3_exec(aux->db, "PRAGMA synchronous=OFF", 0, 0, 0);
	if (rc != SQLITE_OK) {
		goto error;
	}

	rc = sqlite3_exec(aux->db, "PRAGMA temp_store=MEMORY", 0, 0, 0);
	if (rc != SQLITE_OK) {
		goto error;
	}

	return RS_OK;

error:
	sc_log_error("sqlite : %s \n", sqlite3_errmsg(aux->db));
	return RS_ERROR;
}

int aux_clear_info(struct aux *aux)
{
	int rc;

	rc = sqlite3_exec(aux->db, "DELETE FROM resql_info", 0, 0, 0);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	return RS_OK;
}

int aux_write_info(struct aux *aux, struct info *n)
{
	int rc;
	sqlite3_stmt *stmt = aux->add_info;

	rc = sqlite3_clear_bindings(aux->add_info);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	rc = sqlite3_reset(aux->add_info);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	rc |= sqlite3_bind_text(stmt, 1, n->name, -1, NULL);
	rc |= sqlite3_bind_text(stmt, 2, n->connected, -1, NULL);
	rc |= sqlite3_bind_text(stmt, 3, n->role, -1, NULL);
	rc |= sqlite3_bind_text(stmt, 4, n->urls, -1, NULL);

	if (sc_buf_size(&n->stats) == 0) {
		goto out;
	}

	rc |= sqlite3_bind_text(stmt, 5, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 6, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 7, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 8, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 9, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 10, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 11, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 12, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 13, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 14, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 15, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 16, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 17, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 18, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 19, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 20, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 21, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 22, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 23, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 24, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 25, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 26, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 27, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 28, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 29, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 30, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 31, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 32, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 33, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 34, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 35, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 36, sc_buf_get_str(&n->stats), -1, NULL);
	rc |= sqlite3_bind_text(stmt, 37, sc_buf_get_str(&n->stats), -1, NULL);

out:
	rc |= sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		return RS_ERROR;
	}

	return RS_OK;
}

int aux_add_log(struct aux *aux, uint64_t id, const char *level,
		const char *log)
{
	assert(level != NULL);
	assert(log != NULL);

	int rc;

	rc = sqlite3_bind_int64(aux->add_log, 1, (sqlite3_int64) id);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	rc = sqlite3_bind_text(aux->add_log, 2, level, -1, NULL);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	rc = sqlite3_bind_text(aux->add_log, 3, log, -1, NULL);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	rc = sqlite3_step(aux->add_log);
	if (rc != SQLITE_DONE) {
		return RS_ERROR;
	}

	rc = sqlite3_clear_bindings(aux->add_log);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	rc = sqlite3_reset(aux->add_log);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	rc = sqlite3_step(aux->rotate_log);
	if (rc != SQLITE_DONE) {
		return RS_ERROR;
	}

	return RS_OK;
}

int aux_write_session(struct aux *aux, struct session *s)
{
	int rc;
	uint64_t id;
	sqlite3_stmt *stmt;

	rc = sqlite3_bind_text(aux->add_session, 1, s->name,
			       (int) sc_str_len(s->name), NULL);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	rc = sqlite3_bind_int64(aux->add_session, 2, (sqlite3_int64) s->id);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	rc = sqlite3_bind_int64(aux->add_session, 3, (sqlite3_int64) s->seq);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	rc = sqlite3_bind_text(aux->add_session, 4, s->local,
			       (int) sc_str_len(s->local), NULL);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	rc = sqlite3_bind_text(aux->add_session, 5, s->remote,
			       (int) sc_str_len(s->remote), NULL);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	rc = sqlite3_bind_text(aux->add_session, 6, s->connect_time,
			       (int) sc_str_len(s->connect_time), NULL);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	rc = sqlite3_bind_blob(aux->add_session, 7, sc_buf_rbuf(&s->resp),
			       sc_buf_size(&s->resp), NULL);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	rc = sqlite3_step(aux->add_session);
	if (rc != SQLITE_DONE) {
		return RS_ERROR;
	}

	rc = sqlite3_clear_bindings(aux->add_session);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	rc = sqlite3_reset(aux->add_session);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	sc_map_foreach (&s->stmts, id, stmt) {
		rc = sqlite3_bind_int64(aux->add_stmt, 1, (sqlite3_int64) id);
		if (rc != SQLITE_OK) {
			return RS_ERROR;
		}

		rc = sqlite3_bind_int64(aux->add_stmt, 2,
					(sqlite3_int64) s->id);
		if (rc != SQLITE_OK) {
			return RS_ERROR;
		}

		rc = sqlite3_bind_text(aux->add_stmt, 3, s->name,
				       (int) sc_str_len(s->name), NULL);
		if (rc != SQLITE_OK) {
			return RS_ERROR;
		}

		rc = sqlite3_bind_text(aux->add_stmt, 4, sqlite3_sql(stmt), -1,
				       NULL);
		if (rc != SQLITE_OK) {
			return RS_ERROR;
		}

		rc = sqlite3_step(aux->add_stmt);
		if (rc != SQLITE_DONE) {
			return RS_ERROR;
		}

		rc = sqlite3_clear_bindings(aux->add_stmt);
		if (rc != SQLITE_OK) {
			return RS_ERROR;
		}

		rc = sqlite3_reset(aux->add_stmt);
		if (rc != SQLITE_OK) {
			return RS_ERROR;
		}
	}

	return RS_OK;
}

int aux_read_session(struct aux *aux, struct session *s, sqlite3_stmt *sess_tb,
		     sqlite3_stmt *stmt_tb)
{
	int rc;
	uint32_t len;
	const void *p;

	sc_str_set(&s->name, (char *) sqlite3_column_text(sess_tb, 0));
	s->id = (uint64_t) sqlite3_column_int64(sess_tb, 1);
	s->seq = (uint64_t) sqlite3_column_int64(sess_tb, 2);
	sc_str_set(&s->local, (char *) sqlite3_column_text(sess_tb, 3));
	sc_str_set(&s->remote, (char *) sqlite3_column_text(sess_tb, 4));
	sc_str_set(&s->connect_time, (char *) sqlite3_column_text(sess_tb, 5));

	len = (uint32_t) sqlite3_column_bytes(sess_tb, 6);
	p = sqlite3_column_blob(sess_tb, 6);

	sc_buf_clear(&s->resp);
	sc_buf_put_raw(&s->resp, p, len);

	if (!sc_buf_valid(&s->resp)) {
		return RS_ERROR;
	}

	rc = sqlite3_bind_int64(stmt_tb, 1, s->id);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	while (sqlite3_step(stmt_tb) == SQLITE_ROW) {
		sqlite3_stmt *stmt;
		uint64_t id = (uint64_t) sqlite3_column_int64(stmt_tb, 0);
		char *str = (char *) sqlite3_column_text(stmt_tb, 3);

		rc = sqlite3_prepare_v3(aux->db, str, -1,
					SQLITE_PREPARE_PERSISTENT, &stmt, NULL);
		if (rc != RS_OK) {
			sc_log_warn("db_prepare (%s) : %s \n", str,
				    sqlite3_errmsg(aux->db));
		}
		sc_map_put_64v(&s->stmts, id, stmt);
	}

	rc = sqlite3_clear_bindings(stmt_tb);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	rc = sqlite3_reset(stmt_tb);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	return RS_OK;
}

int aux_del_session(struct aux *aux, struct session *s)
{
	int rc;

	rc = sqlite3_reset(aux->rm_session);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	rc = sqlite3_bind_text(aux->rm_session, 1, s->name,
			       (int) sc_str_len(s->name), NULL);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	rc = sqlite3_step(aux->rm_session);
	if (rc != SQLITE_DONE) {
		return RS_ERROR;
	}

	rc = sqlite3_reset(aux->rm_all_stmts);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	rc = sqlite3_bind_int64(aux->rm_all_stmts, 1, (sqlite3_int64) s->id);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	rc = sqlite3_step(aux->rm_all_stmts);
	if (rc != SQLITE_DONE) {
		return RS_ERROR;
	}

	rc = sqlite3_clear_bindings(aux->rm_session);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	rc = sqlite3_clear_bindings(aux->rm_all_stmts);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	return RS_OK;
}

int aux_clear_sessions(struct aux *aux)
{
	int rc;

	rc = sqlite3_exec(aux->db, "DELETE FROM resql_sessions", 0, 0, 0);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	return RS_OK;
}

int aux_write_kv(struct aux *aux, const char *key, struct sc_buf *buf)
{
	int rc, ret = RS_ERROR;
	sqlite3_stmt *stmt;

	rc = sqlite3_prepare_v3(aux->db,
				"INSERT OR REPLACE INTO resql_kv VALUES(?,?);",
				-1, 0, &stmt, NULL);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	rc = sqlite3_bind_text(stmt, 1, key, -1, 0);
	if (rc != SQLITE_OK) {
		goto out;
	}

	rc = sqlite3_bind_blob(stmt, 2, sc_buf_rbuf(buf), sc_buf_size(buf),
			       NULL);
	if (rc != SQLITE_OK) {
		goto out;
	}

	rc = sqlite3_step(stmt);
	if (rc != SQLITE_DONE) {
		goto out;
	}

	ret = RS_OK;

out:
	sqlite3_finalize(stmt);
	return ret;
}

int aux_read_kv(struct aux *aux, const char *key, struct sc_buf *buf)
{
	int rc, ret = RS_ERROR;
	sqlite3_stmt *stmt;

	rc = sqlite3_prepare_v3(aux->db,
				"SELECT value FROM resql_kv where key=?;", -1,
				0, &stmt, NULL);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	rc = sqlite3_bind_text(stmt, 1, key, -1, 0);
	if (rc != SQLITE_OK) {
		goto out;
	}

	rc = sqlite3_step(stmt);
	if (rc != SQLITE_ROW) {
		goto out;
	}

	sc_buf_put_raw(buf, sqlite3_column_blob(stmt, 0),
		       (uint32_t) sqlite3_column_bytes(stmt, 0));

	ret = RS_OK;

out:
	sqlite3_finalize(stmt);
	return ret;
}

int aux_add_stmt(struct aux *aux, const char *client, uint64_t cid, uint64_t id,
		 const char *sql)
{
	int rc = 0;

	rc |= sqlite3_bind_int64(aux->add_stmt, 1, (sqlite3_int64) id);
	rc |= sqlite3_bind_int64(aux->add_stmt, 2, (sqlite3_int64) cid);
	rc |= sqlite3_bind_text(aux->add_stmt, 3, client, -1, NULL);
	rc |= sqlite3_bind_text(aux->add_stmt, 4, sql, -1, NULL);

	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	rc = sqlite3_step(aux->add_stmt);
	if (rc != SQLITE_DONE) {
		return RS_ERROR;
	}

	rc = sqlite3_clear_bindings(aux->add_stmt);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	rc = sqlite3_reset(aux->add_stmt);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	return RS_OK;
}

int aux_rm_stmt(struct aux *aux, uint64_t id)
{
	int rc;

	rc = sqlite3_bind_int64(aux->rm_stmt, 1, (sqlite3_int64) id);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	rc = sqlite3_step(aux->rm_stmt);
	if (rc != SQLITE_DONE) {
		return RS_ERROR;
	}

	rc = sqlite3_clear_bindings(aux->rm_stmt);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	rc = sqlite3_reset(aux->rm_stmt);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	return RS_OK;
}
