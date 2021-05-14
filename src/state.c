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

#include "state.h"

#include "cmd.h"
#include "entry.h"
#include "file.h"
#include "info.h"
#include "metric.h"
#include "msg.h"
#include "session.h"

#include "sc/sc_array.h"
#include "sc/sc_log.h"
#include "sc/sc_str.h"
#include "sc/sc_uri.h"

#include <errno.h>
#include <inttypes.h>

#define STATE_FILE	   "state.resql"
#define STATE_TMP_FILE	   "state.tmp.resql"
#define STATE_SS_FILE	   "snapshot.resql"
#define STATE_SS_TMP_FILE  "snapshot.tmp.resql"

thread_local struct state *t_state;
static sqlite3_vfs ext;

static const char *usage_default =
	"usage : SELECT resql('config-name', 'param');";
static const char *usage_add_node =
	"usage : SELECT resql('add-node', 'tcp://name@127.0.0.1:8085');";
static const char *usage_remove_node =
	"usage : SELECT resql('remove-node', 'node0');";
static const char *usage_max_size =
	"usage : SELECT resql('max-size', 5000000);";

void state_config(sqlite3_context *ctx, int argc, sqlite3_value **argv)
{
	int64_t val;
	const unsigned char *cmd;
	const unsigned char *value;
	const char *ret;
	struct state *state = t_state;

	if (argc <= 0) {
		sqlite3_result_error(ctx, usage_default, -1);
		return;
	}

	cmd = sqlite3_value_text(argv[0]);
	if (strcmp((char *) cmd, "add-node") == 0) {
		if (argc != 2) {
			sqlite3_result_error(ctx, usage_add_node, -1);
			return;
		}

		if (state->cb.add_node) {
			value = sqlite3_value_text(argv[1]);
			ret = state->cb.add_node(state->cb.arg,
						 (const char *) value);
			sqlite3_result_text(ctx, ret, -1, NULL);
		}
	} else if (strcmp((char *) cmd, "remove-node") == 0) {
		if (argc != 2) {
			sqlite3_result_error(ctx, usage_remove_node, -1);
			return;
		}

		if (state->cb.remove_node) {
			value = sqlite3_value_text(argv[1]);
			ret = state->cb.remove_node(state->cb.arg,
						    (const char *) value);
			sqlite3_result_text(ctx, ret, -1, NULL);
		}
	} else if (strcmp((char *) cmd, "shutdown") == 0) {
		if (argc != 2) {
			sqlite3_result_error(ctx, usage_remove_node, -1);
			return;
		}

		if (state->cb.shutdown) {
			value = sqlite3_value_text(argv[1]);
			ret = state->cb.shutdown(state->cb.arg,
						 (const char *) value);
			sqlite3_result_text(ctx, ret, -1, NULL);
		}
	} else if (strcmp((char *) cmd, "max-size") == 0) {
		if (argc > 2) {
			sqlite3_result_error(ctx, usage_max_size, -1);
			return;
		}

		if (argc == 2) {
			val = sqlite3_value_int64(argv[1]);
			if (val < 0) {
				sqlite3_result_error(
					ctx, "Max size cannot be negative", -1);
				return;
			}

			state->max_page = val / 4096;
		}

		sqlite3_result_int64(ctx, state->max_page * 4096);
	} else {
		sqlite3_result_error(ctx, "Unknown command", -1);
	}
}

int state_randomness(sqlite3_vfs *vfs, int size, char *out)
{
	(void) vfs;

	struct sc_rand *r;
	struct state *st = t_state;

	r = st->readonly ? &st->rrand : &st->wrand;
	sc_rand_read(r, out, size);

	return size;
}

int state_currenttime(sqlite3_vfs *vfs, sqlite3_int64 *val)
{
	(void) vfs;

	const uint64_t unix_epoch = 24405875 * (uint64_t) 8640000;

	*val = (int64_t) (unix_epoch + t_state->realtime);
	return SQLITE_OK;
}

int state_max_page(unsigned int max, unsigned int curr)
{
	(void) max;

	struct state *s = t_state;

	if (s->client && curr > s->max_page) {
		s->full = true;
		return -1;
	}

	return 0;
}

int state_global_init()
{
	int rc;
	sqlite3_vfs *orig;

	rc = sqlite3_initialize();
	if (rc != SQLITE_OK) {
		sc_log_error("sqlite3_initialize %s \n", sqlite3_errstr(rc));
		return RS_ERROR;
	}

	orig = sqlite3_vfs_find(NULL);
	if (!orig) {
		sc_log_error("sqlite3_vfs_find : missing");
		return RS_ERROR;
	}

	ext = *orig;
	ext.zName = "resql_vfs";
	ext.xRandomness = state_randomness;
	ext.xCurrentTimeInt64 = state_currenttime;

	rc = sqlite3_vfs_register(&ext, 1);
	if (rc != SQLITE_OK) {
		sc_log_error("sqlite3_vfs_register %s \n", sqlite3_errstr(rc));
		return RS_ERROR;
	}

	sqlite3_set_max_page_cb(state_max_page);

	return RS_OK;
}

int state_global_shutdown()
{
	int rc;

	rc = sqlite3_shutdown();
	if (rc != SQLITE_OK) {
		sc_log_info("sqlite3_shutdown : %s \n", sqlite3_errstr(rc));
		return RS_ERROR;
	}

	return RS_OK;
}

void state_init(struct state *st, struct state_cb cb, const char *path,
		const char *name)
{
	memset(st, 0, sizeof *st);

	st->cb = cb;
	st->path = sc_str_create_fmt("%s/%s", path, STATE_FILE);
	st->tmp_path = sc_str_create_fmt("%s/%s", path, STATE_TMP_FILE);
	st->ss_path = sc_str_create_fmt("%s/%s", path, STATE_SS_FILE);
	st->ss_tmp_path = sc_str_create_fmt("%s/%s", path, STATE_SS_TMP_FILE);
	st->max_page = UINT_MAX;

	sc_buf_init(&st->tmp, 1024);
	meta_init(&st->meta, name);
	sc_map_init_sv(&st->nodes, 16, 0);
	sc_map_init_sv(&st->names, 16, 0);
	sc_map_init_64v(&st->ids, 16, 0);
	sc_list_init(&st->disconnects);

	t_state = st;
}

int state_term(struct state *st)
{
	int rc;

	rc = state_close(st);

	sc_buf_term(&st->tmp);
	sc_str_destroy(&st->path);
	sc_str_destroy(&st->tmp_path);
	sc_str_destroy(&st->ss_path);
	sc_str_destroy(&st->ss_tmp_path);

	return rc;
}

int state_authorizer(void *user, int action, const char *arg0, const char *arg1,
		     const char *arg2, const char *arg3)
{
	(void) arg2;
	(void) arg3;

	size_t len;
	struct state *st = user;

	if (!st->client) {
		return SQLITE_OK;
	}

	switch (action) {
	case SQLITE_READ:
		len = strlen("resql_clients");
		if (strncmp("resql_clients", arg0, len) == 0 &&
		    strncmp("resp", arg1, strlen("resp")) == 0) {
			return SQLITE_IGNORE;
		}
		// fall through
	case SQLITE_ALTER_TABLE:
		arg0 = arg1;
		// fall through
	case SQLITE_CREATE_TABLE:
	case SQLITE_DELETE:
	case SQLITE_DROP_TABLE:
	case SQLITE_INSERT:
	case SQLITE_UPDATE:
	case SQLITE_DROP_VTABLE:
		if (strncmp("resql", arg0, strlen("resql")) == 0) {
			return SQLITE_DENY;
		}

		break;
	default:
		break;
	}

	return SQLITE_OK;
}

int state_write_infos(struct state *st, struct aux *aux)
{
	int rc;
	struct info *info;

	rc = aux_clear_nodes(aux);
	if (rc != RS_OK) {
		return rc;
	}

	sc_map_foreach_value (&st->nodes, info) {
		rc = aux_write_node(aux, info);
		if (rc != RS_OK) {
			return rc;
		}
	}

	return RS_OK;
}

int state_write_vars(struct state *st, struct aux *aux)
{
	int rc;

	sc_buf_clear(&st->tmp);

	sc_buf_put_64(&st->tmp, st->term);
	sc_buf_put_64(&st->tmp, st->index);
	sc_buf_put_64(&st->tmp, st->ss_term);
	sc_buf_put_64(&st->tmp, st->ss_index);
	meta_encode(&st->meta, &st->tmp);

	sc_buf_put_64(&st->tmp, st->timestamp);
	sc_buf_put_64(&st->tmp, st->realtime);
	sc_buf_put_64(&st->tmp, st->monotonic);
	sc_buf_put_8(&st->tmp, st->wrand.i);
	sc_buf_put_8(&st->tmp, st->wrand.j);
	sc_buf_put_raw(&st->tmp, st->wrand.init, sizeof(st->wrand.init));

	rc = aux_write_kv(aux, "var", &st->tmp);
	if (rc != RS_OK) {
		return rc;
	}

	sc_buf_clear(&st->tmp);
	sc_buf_put_raw(&st->tmp, st->meta.name,
		       (uint32_t) sc_str_len(st->meta.name) + 1);

	return aux_write_kv(aux, "cluster_name", &st->tmp);
}

int state_read_vars(struct state *st, struct aux *aux)
{
	int rc;
	const char *sql;
	sqlite3_stmt *sess, *stmt;
	struct session *s;

	sc_buf_clear(&st->tmp);

	rc = aux_read_kv(aux, "var", &st->tmp);
	if (rc != RS_OK) {
		return rc;
	}

	st->term = sc_buf_get_64(&st->tmp);
	st->index = sc_buf_get_64(&st->tmp);
	st->ss_term = sc_buf_get_64(&st->tmp);
	st->ss_index = sc_buf_get_64(&st->tmp);
	meta_decode(&st->meta, &st->tmp);

	st->timestamp = sc_buf_get_64(&st->tmp);
	st->realtime = sc_buf_get_64(&st->tmp);
	st->monotonic = sc_buf_get_64(&st->tmp);
	st->wrand.i = sc_buf_get_8(&st->tmp);
	st->wrand.j = sc_buf_get_8(&st->tmp);

	sc_buf_get_data(&st->tmp, st->wrand.init, sizeof(st->wrand.init));
	sc_rand_init(&st->rrand, st->wrand.init);

	sql = "SELECT * FROM resql_clients";
	rc = sqlite3_prepare(st->aux.db, sql, -1, &sess, 0);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	sql = "SELECT * FROM resql_statements WHERE client_id = (?)";
	rc = sqlite3_prepare(st->aux.db, sql, -1, &stmt, 0);
	if (rc != SQLITE_OK) {
		return RS_ERROR;
	}

	while ((rc = sqlite3_step(sess)) == SQLITE_ROW) {
		s = session_create(st, "", 0);

		rc = aux_read_session(&st->aux, s, sess, stmt);
		if (rc == RS_ERROR) {
			rs_abort("db");
		}

		sc_map_put_64v(&st->ids, s->id, s);
		sc_map_put_sv(&st->names, s->name, s);
	}

	if (rc != SQLITE_DONE) {
		rs_abort("db");
	}

	sqlite3_finalize(sess);
	sqlite3_finalize(stmt);

	return RS_OK;
}

int state_read_snapshot(struct state *st, bool in_memory)
{
	int rc;
	bool b;

	b = file_exists_at(st->ss_path);
	if (!b) {
		return RS_ENOENT;
	}

	if (in_memory) {
		rc = aux_init(&st->aux, "", SQLITE_OPEN_MEMORY);
		if (rc != RS_OK) {
			return rc;
		}

		rc = aux_load_to_memory(&st->aux, st->ss_path);
		if (rc != RS_OK) {
			goto cleanup_aux;
		}
	} else {
		b = file_exists_at(st->path);
		sc_log_info("Found state file : %s \n", b ? "yes" : " no");

		if (!b) {
			rc = file_copy(st->path, st->ss_path);
			if (rc != RS_OK) {
				return rc;
			}
		}

		rc = file_rename(st->tmp_path, st->path);
		if (rc != RS_OK) {
			return rc;
		}

		rc = aux_init(&st->aux, st->tmp_path, 0);
		if (rc != RS_OK) {
			return rc;
		}
	}

	rc = state_read_vars(st, &st->aux);
	if (rc != RS_OK) {
		return rc;
	}

	sqlite3_set_authorizer(st->aux.db, state_authorizer, st);

	return RS_OK;

cleanup_aux:
	aux_term(&st->aux);
	return rc;
}

int state_read_for_snapshot(struct state *st)
{
	bool b;
	int rc;

	st->snapshot = true;
	st->in_memory = false;

	b = file_exists_at(st->ss_path);
	if (!b) {
		rs_abort("Cannot find snapshot file at %s \n", st->ss_path);
	}

	rc = file_copy(st->ss_tmp_path, st->ss_path);
	if (rc != RS_OK) {
		return rc;
	}

	rc = aux_init(&st->aux, st->ss_tmp_path, 0);
	if (rc != RS_OK) {
		rs_abort("db");
	}

	rc = state_read_vars(st, &st->aux);
	if (rc != RS_OK) {
		rs_abort("db");
	}

	sqlite3_set_authorizer(st->aux.db, state_authorizer, st);

	return RS_OK;
}

int state_open(struct state *st, bool in_memory)
{
	int rc;

	st->in_memory = in_memory;
	st->snapshot = false;

	file_remove_path(st->tmp_path);

	rc = file_remove_path(st->ss_tmp_path);
	if (rc != RS_OK) {
		return rc;
	}

	rc = state_read_snapshot(st, in_memory);
	if (rc == RS_ERROR || rc == RS_FULL) {
		return rc;
	}

	if (rc == RS_ENOENT) {
		sc_log_warn("Cannot find snapshot, creating one \n");

		rc = state_initial_snapshot(st);
		if (rc != RS_OK) {
			return rc;
		}

		rc = state_read_snapshot(st, in_memory);
		if (rc != RS_OK) {
			return rc;
		}

		metric_snapshot(true, 1000, (size_t) file_size_at(st->ss_path));
	}

	sc_log_info("Opened snapshot at index [%" PRIu64 "] \n", st->index);
	return rc;
}

int state_close(struct state *st)
{
	int rc;
	int ret = RS_OK;
	struct session *s;
	struct info *info;

	if (!st->closed) {
		st->closed = true;
		st->client = false;
		st->readonly = false;

		rc = state_write_vars(st, &st->aux);
		if (rc != RS_OK) {
			ret = rc;
		}

		rc = aux_clear_sessions(&st->aux);
		if (rc != RS_OK) {
			ret = rc;
		}

		sc_map_foreach_value (&st->names, s) {
			sc_list_del(NULL, &s->list);
		}

		sc_map_foreach_value (&st->names, s) {
			rc = aux_write_session(&st->aux, s);
			if (rc != RS_OK) {
				ret = rc;
			}
			session_destroy(s);
		}

		sc_map_term_sv(&st->names);
		sc_map_term_64v(&st->ids);

		sc_map_foreach_value (&st->nodes, info) {
			info_destroy(info);
		}
		sc_map_term_sv(&st->nodes);

		meta_term(&st->meta);

		rc = aux_term(&st->aux);
		if (rc != RS_OK) {
			ret = rc;
		}

		if (!st->snapshot && !st->in_memory) {
			rc = file_rename(st->path, st->tmp_path);
			if (rc != RS_OK) {
				ret = rc;
			}
		}
	}

	return ret;
}

int state_initial_snapshot(struct state *st)
{
	int rc;
	struct aux aux;

	rc = aux_init(&aux, st->ss_tmp_path, SQLITE_OPEN_CREATE);
	if (rc != RS_OK) {
		return rc;
	}

	rc = state_write_vars(st, &aux);
	if (rc != RS_OK) {
		goto cleanup_aux;
	}

	rc = aux_term(&aux);
	if (rc != RS_OK) {
		return rc;
	}

	rc = rename(st->ss_tmp_path, st->ss_path);
	if (rc != 0) {
		sc_log_error("rename : %s to : %s failed : %s", st->ss_tmp_path,
			     st->ss_path, strerror(errno));
		rc = RS_ERROR;
	}

cleanup_aux:
	aux_term(&aux);
	return rc;
}

int state_on_client_connect(struct state *st, const char *name,
			    const char *local, const char *remote,
			    struct session **s)
{
	int rc;
	struct session *sess;

	sess = sc_map_get_sv(&st->names, name);
	if (!sc_map_found(&st->names)) {
		sess = session_create(st, name, st->index);
		sc_map_put_sv(&st->names, sess->name, sess);
		sc_map_put_64v(&st->ids, sess->id, sess);
	}

	session_connected(sess, local, remote, st->realtime);
	rc = aux_write_session(&st->aux, sess);

	*s = sess;

	return rc;
}

int state_on_client_disconnect(struct state *st, const char *name, bool clean)
{
	int rc;
	struct session *s;

	s = sc_map_get_sv(&st->names, name);
	if (!sc_map_found(&st->names)) {
		return RS_OK;
	}

	session_disconnected(s, st->timestamp);

	if (clean) {
		rc = aux_del_session(&st->aux, s);
		if (rc != RS_OK) {
			return rc;
		}

		sc_map_del_sv(&st->names, name);
		sc_map_del_64v(&st->ids, s->id);

		session_destroy(s);
	} else {
		rc = aux_write_session(&st->aux, s);
		if (rc != RS_OK) {
			return rc;
		}
		sc_list_add_tail(&st->disconnects, &s->list);
	}

	return RS_OK;
}

static void state_update_info(struct state *st)
{
	struct info *in;
	struct sc_uri *uri;
	struct meta_node n;

	sc_array_foreach (&st->meta.nodes, n) {
		in = sc_map_get_sv(&st->nodes, n.name);
		if (!sc_map_found(&st->nodes)) {
			in = info_create(n.name);
			sc_map_put_sv(&st->nodes, in->name, in);
		}

		info_set_role(in, meta_role_str[n.role]);

		sc_buf_clear(&st->tmp);

		for (size_t i = 0; i < sc_array_size(&n.uris); i++) {
			uri = sc_array_at(&n.uris, i);
			sc_buf_put_text(&st->tmp, uri->str);
		}
		info_set_urls(in, (char *) st->tmp.mem);
	}
}

int state_on_meta(struct state *st, uint64_t index, struct meta *meta)
{
	bool found;
	int rc;
	struct meta_node n;
	struct info *info;
	const char *name, *role;

	meta_copy(&st->meta, meta);
	meta_term(meta);

	sc_map_foreach (&st->nodes, name, info) {
		found = false;

		sc_array_foreach (&st->meta.nodes, n) {
			if (strcmp(n.name, name) == 0) {
				found = true;
				break;
			}
		}

		if (!found) {
			sc_map_del_sv(&st->nodes, name);
			info_destroy(info);
			break;
		}
	}

	state_update_info(st);

	rc = state_write_infos(st, &st->aux);
	if (rc != RS_OK) {
		return rc;
	}

	rc = state_write_vars(st, &st->aux);
	if (rc != RS_OK) {
		return rc;
	}

	sc_buf_clear(&st->tmp);
	sc_buf_put_text(&st->tmp, "Term[%" PRIu64 "] : ", st->meta.term);

	sc_array_foreach (&st->meta.nodes, n) {
		role = meta_role_str[n.role];
		sc_buf_put_text(&st->tmp, "[%s:%s] ", n.name, role);
	}

	rc = aux_add_log(&st->aux, index, "INFO", (char *) st->tmp.mem);

	return rc;
}

int state_on_term_start(struct state *st, uint64_t index, uint64_t realtime,
			uint64_t monotonic)
{
	int rc;
	const char *name;

	st->realtime = st->realtime > realtime ? st->realtime : realtime;
	st->monotonic = monotonic;

	sc_map_foreach_key (&st->names, name) {
		rc = state_on_client_disconnect(st, name, false);
		if (rc != RS_OK) {
			return rc;
		}
	}

	rc = aux_add_log(&st->aux, index, "INFO", "Term start");

	return rc;
}

int state_on_info(struct state *st, struct sc_buf *buf)
{
	bool connected;
	uint32_t len;
	void *ptr;
	const char *name;
	struct info *n;

	while (sc_buf_size(buf) != 0) {
		name = sc_buf_get_str(buf);
		connected = sc_buf_get_bool(buf);
		len = sc_buf_get_32(buf);
		ptr = sc_buf_get_blob(buf, len);

		n = sc_map_get_sv(&st->nodes, name);
		if (sc_map_found(&st->nodes)) {
			info_set_connected(n, connected);
			if (len != 0) {
				info_set_stats(n, ptr, len);
			}
		}
	}

	return state_write_infos(st, &st->aux);
}

int state_on_timestamp(struct state *st, uint64_t realtime, uint64_t monotonic)
{
	int rc;
	struct session *s;
	struct sc_list *tmp, *it;

	assert(st->monotonic <= monotonic);

	st->timestamp += monotonic - st->monotonic;
	st->monotonic = monotonic;
	st->realtime = realtime;

	sc_list_foreach_safe (&st->disconnects, tmp, it) {
		s = sc_list_entry(it, struct session, list);

		if (st->timestamp - s->disconnect_time > 60000) {
			rc = aux_del_session(&st->aux, s);
			if (rc != RS_OK) {
				return rc;
			}

			sc_map_del_sv(&st->names, s->name);
			sc_map_del_64v(&st->ids, s->id);
			session_destroy(s);
		}
	}

	return RS_OK;
}

static const char *state_errstr(struct state *st)
{
	const char *err = NULL;

	if (st->last_err != NULL) {
		err = st->last_err;
		st->last_err = NULL;
	} else {
		err = sqlite3_errmsg(st->aux.db);
	}

	return err;
}

static int state_bind_params(struct state *st, struct sc_buf *req,
			     sqlite3_stmt *stmt)
{
	int rc, type, idx;
	uint32_t size;
	int64_t t;
	double f;
	const char *param, *val;
	void *data;

	while ((type = sc_buf_get_8(req)) != MSG_BIND_END) {
		switch (type) {
		case MSG_BIND_INDEX:
			/**
			 * Sqlite index starts from '1', to make it consistent
			 * with sqlite_column_xxx api which starts from '0', add
			 * 1 to index so users can work starting from '0'.
			 */
			idx = (int) sc_buf_get_32(req) + 1;
			break;
		case MSG_BIND_NAME:
			param = sc_buf_get_str(req);

			idx = sqlite3_bind_parameter_index(stmt, param);
			if (idx == 0) {
				st->last_err = "Invalid parameter name";
				return RS_ERROR;
			}
			break;
		default:
			goto invalid;
		}

		type = sc_buf_get_8(req);

		switch (type) {
		case MSG_PARAM_INTEGER:
			t = sc_buf_get_64(req);
			rc = sqlite3_bind_int64(stmt, idx, t);
			break;
		case MSG_PARAM_FLOAT:
			f = sc_buf_get_double(req);
			rc = sqlite3_bind_double(stmt, idx, f);
			break;
		case MSG_PARAM_TEXT:
			size = sc_buf_peek_32(req);
			val = sc_buf_get_str(req);
			rc = sqlite3_bind_text(stmt, idx, val, size,
					       SQLITE_STATIC);
			break;
		case MSG_PARAM_BLOB:
			size = sc_buf_get_32(req);
			data = sc_buf_get_blob(req, size);
			rc = sqlite3_bind_blob(stmt, idx, data, size,
					       SQLITE_STATIC);
			break;
		case MSG_PARAM_NULL:
			rc = sqlite3_bind_null(stmt, idx);
			break;
		default:
			goto invalid;
		}

		if (rc != SQLITE_OK) {
			return aux_rc(rc);
		}
	}

	return RS_OK;

invalid:
	st->last_err = "Invalid message";
	return RS_ERROR;
}

static void state_encode_row(struct state *st, int col, sqlite3_stmt *stmt,
			     struct sc_buf *resp)
{
	(void) st;

	int type, len;
	int64_t val;
	double d;
	const char *data;

	for (int i = 0; i < col; i++) {
		type = sqlite3_column_type(stmt, i);

		switch (type)
		case SQLITE_INTEGER: {
			val = sqlite3_column_int64(stmt, i);
			sc_buf_put_8(resp, MSG_PARAM_INTEGER);
			sc_buf_put_64(resp, (uint64_t) val);
			break;
		case SQLITE_FLOAT:
			d = sqlite3_column_double(stmt, i);
			sc_buf_put_8(resp, MSG_PARAM_FLOAT);
			sc_buf_put_double(resp, d);
			break;
		case SQLITE_TEXT:
			len = sqlite3_column_bytes(stmt, i);
			data = (const char *) sqlite3_column_text(stmt, i);
			sc_buf_put_8(resp, MSG_PARAM_TEXT);
			sc_buf_put_str_len(resp, data, len);
			break;
		case SQLITE_BLOB:
			len = sqlite3_column_bytes(stmt, i);
			data = sqlite3_column_blob(stmt, i);
			sc_buf_put_8(resp, MSG_PARAM_BLOB);
			sc_buf_put_blob(resp, data, (uint32_t) len);
			break;
		case SQLITE_NULL:
			sc_buf_put_8(resp, MSG_PARAM_NULL);
			break;
		default:
			break;
		}
	}
}

static int state_step(struct state *st, sqlite3_stmt *stmt, struct sc_buf *resp)
{
	int rc, col;
	uint32_t row = 0, row_pos;
	const char *name;

	rc = sqlite3_step(stmt);

	sc_buf_put_32(resp, (uint32_t) sqlite3_changes(st->aux.db));
	sc_buf_put_64(resp, (uint64_t) sqlite3_last_insert_rowid(st->aux.db));

	if (rc == SQLITE_ROW) {
		sc_buf_put_8(resp, MSG_FLAG_ROW);

		col = sqlite3_column_count(stmt);
		sc_buf_put_32(resp, (uint32_t) col);

		for (int i = 0; i < col; i++) {
			name = sqlite3_column_name(stmt, i);
			sc_buf_put_str(resp, name);
		}

		row_pos = sc_buf_wpos(resp);
		sc_buf_put_32(resp, row);

		do {
			row++;
			state_encode_row(st, col, stmt, resp);
		} while ((rc = sqlite3_step(stmt)) == SQLITE_ROW);

		sc_buf_set_32_at(resp, row_pos, row);
	}

	if (rc != SQLITE_DONE) {
		return aux_rc(rc);
	}

	return RS_OK;
}

static int state_exec_prepared_statement(struct state *st, sqlite3_stmt *stmt,
					 bool readonly, struct sc_buf *req,
					 struct sc_buf *resp)
{
	int rc;

	if (readonly && sqlite3_stmt_readonly(stmt) == 0) {
		st->last_err = "Operation is not readonly.";
		return RS_ERROR;
	}

	rc = state_bind_params(st, req, stmt);
	if (rc != RS_OK) {
		return rc;
	}

	return state_step(st, stmt, resp);
}

static int state_exec_stmt(struct state *st, bool readonly, struct sc_buf *req,
			   struct sc_buf *resp)
{
	int rc;
	uint32_t len = sc_buf_peek_32(req);
	const char *str = sc_buf_get_str(req);
	sqlite3_stmt *stmt = NULL;

	rc = sqlite3_prepare_v3(st->aux.db, str, len, 0, &stmt, NULL);
	if (rc != SQLITE_OK) {
		return aux_rc(rc);
	}

	rc = state_exec_prepared_statement(st, stmt, readonly, req, resp);

	sqlite3_finalize(stmt);

	return rc;
}

static int state_prepare_stmt(struct state *st, struct session *s,
			      uint64_t index, struct sc_buf *req,
			      struct sc_buf *resp)
{
	const int pre = SQLITE_PREPARE_PERSISTENT;

	int rc;
	uint64_t id;
	sqlite3_stmt *stmt;
	const int len = sc_buf_peek_32(req);
	const char *str = sc_buf_get_str(req);

	if (!sc_buf_valid(req)) {
		st->last_err = "Corrupt message";
		return RS_ERROR;
	}

	id = session_sql_to_id(s, str);
	if (id == 0) {
		rc = sqlite3_prepare_v3(st->aux.db, str, len, pre, &stmt, NULL);
		if (rc != SQLITE_OK) {
			return aux_rc(rc);
		}

		session_add_stmt(s, index, stmt);
		id = index;
	}

	st->client = false;
	rc = aux_add_stmt(&st->aux, s->name, s->id, id, str);
	st->client = true;

	sc_buf_put_64(resp, id);

	return rc;
}

static int state_del_prepared(struct state *st, struct session *s,
			      struct sc_buf *req)
{
	int rc;
	uint64_t id;

	id = sc_buf_get_64(req);

	if (!sc_buf_valid(req)) {
		st->last_err = "Corrupt message";
		return RS_ERROR;
	}

	rc = session_del_stmt(s, id);
	if (rc != RS_OK) {
		st->last_err = "Prepared statement does not exist.";
		return RS_ERROR;
	}

	st->client = false;
	rc = aux_rm_stmt(&st->aux, id);
	st->client = true;

	return rc;
}

static int state_exec_stmt_id(struct state *st, struct session *sess,
			      bool readonly, struct sc_buf *req,
			      struct sc_buf *resp)
{
	int rc;
	uint64_t id;
	sqlite3_stmt *stmt;

	id = sc_buf_get_64(req);

	if (!sc_buf_valid(req)) {
		st->last_err = "Invalid message";
		return RS_ERROR;
	}

	stmt = session_get_stmt(sess, id);
	if (stmt == NULL) {
		st->last_err = "Prepared statement does not exist.";
		return RS_ERROR;
	}

	rc = state_exec_prepared_statement(st, stmt, readonly, req, resp);

	aux_clear(stmt);

	return rc;
}

static void state_encode_error(struct state *st, struct sc_buf *resp)
{
	sc_buf_clear(resp);
	msg_create_client_resp_header(resp);
	sc_buf_put_8(resp, MSG_FLAG_ERROR);
	sc_buf_put_str(resp, state_errstr(st));
	sc_buf_put_8(resp, MSG_FLAG_MSG_END);
	msg_finalize_client_resp(resp);
}

int state_exec_request(struct state *st, struct session *s, uint64_t index,
		       bool readonly, struct sc_buf *req, struct sc_buf *resp)
{
	assert(sc_buf_size(req) < UINT32_MAX);

	int rc, rv;
	uint32_t pos, result_len;
	enum msg_flag flag;

	sc_buf_clear(resp);
	msg_create_client_resp_header(resp);
	sc_buf_put_8(resp, MSG_FLAG_OK);

	rc = sqlite3_step(st->aux.begin);
	if (rc != SQLITE_DONE) {
		return aux_rc(rc);
	}

	while ((flag = (enum msg_flag) sc_buf_get_8(req)) == MSG_FLAG_OP) {
		sc_buf_put_8(resp, MSG_FLAG_OP);

		pos = sc_buf_wpos(resp);
		sc_buf_put_32(resp, 0);

		flag = (enum msg_flag) sc_buf_get_8(req);

		if (readonly && (flag == MSG_FLAG_STMT_PREPARE ||
				 flag == MSG_FLAG_STMT_DEL_PREPARED)) {
			st->last_err = "Not a readonly operation";
			goto error;
		}

		switch (flag) {
		case MSG_FLAG_STMT:
			rc = state_exec_stmt(st, readonly, req, resp);
			break;
		case MSG_FLAG_STMT_ID:
			rc = state_exec_stmt_id(st, s, readonly, req, resp);
			break;
		case MSG_FLAG_STMT_PREPARE:
			rc = state_prepare_stmt(st, s, index, req, resp);
			break;
		case MSG_FLAG_STMT_DEL_PREPARED:
			rc = state_del_prepared(st, s, req);
			break;
		default:
			st->last_err = "Invalid message";
			goto error;
		}

		flag = (enum msg_flag) sc_buf_get_8(req);

		if (rc != RS_OK || flag != MSG_FLAG_OP_END) {
			goto error;
		}

		sc_buf_put_8(resp, MSG_FLAG_OP_END);

		result_len = sc_buf_wpos(resp) - pos;
		sc_buf_set_32_at(resp, pos, result_len);
	}

	if (flag != MSG_FLAG_MSG_END || !sc_buf_valid(req)) {
		st->last_err = "Invalid message";
		goto error;
	}

	rc = sqlite3_step(st->aux.commit);
	if (rc != SQLITE_DONE) {
		rc = aux_rc(rc);
		goto error;
	}

	sc_buf_put_8(resp, MSG_FLAG_MSG_END);
	msg_finalize_client_resp(resp);

	if (!sc_buf_valid(resp)) {
		st->last_err = (resp->err == SC_BUF_OOM) ?
					     "Response is too big." :
					     "Internal error.";
		goto error;
	}

	return RS_OK;

error:
	state_encode_error(st, resp);

	st->client = false;
	rv = sqlite3_step(st->aux.rollback);
	if (rv != SQLITE_DONE) {
		if (aux_rc(rv) != RS_ERROR) {
			sc_log_error("Rollback : %s \n",
				     sqlite3_errmsg(st->aux.db));
		}
	}

	if (rc != RS_FATAL) {
		rc = RS_OK;
	}

	return rc;
}

int state_on_client_request(struct state *st, uint64_t index, unsigned char *e,
			    struct session **s)
{
	int rc;
	uint32_t len = entry_data_len(e);
	uint64_t client_id = entry_cid(e);
	uint64_t seq = entry_seq(e);
	void *data = entry_data(e);
	struct sc_buf req = sc_buf_wrap(data, len, SC_BUF_READ);
	struct session *sess;

	st->last_err = NULL;

	sess = sc_map_get_64v(&st->ids, client_id);
	if (!sc_map_found(&st->ids)) {
		rs_abort("Session does not exist : %" PRIu64 "\n", client_id);
	}

	*s = sess;

	if (seq == sess->seq) {
		sc_buf_set_rpos(&sess->resp, 0);
		return RS_OK;
	}

	rc = state_exec_request(st, sess, index, false, &req, &sess->resp);
	if (rc == RS_FULL && st->full) {
		// max-page-size config has been reached, it's safe to continue
		rc = RS_OK;
	}

	sc_buf_shrink(&sess->resp, 32 * 1024);

	sess->seq = seq;

	return rc;
}

int state_apply_readonly(struct state *st, uint64_t cid, unsigned char *buf,
			 uint32_t len, struct sc_buf *resp)
{
	int rc;
	struct session *s;
	struct sc_buf req = sc_buf_wrap(buf, len, SC_BUF_READ);

	st->last_err = NULL;
	st->client = true;
	st->readonly = true;
	st->full = false;

	s = sc_map_get_64v(&st->ids, cid);
	if (!sc_map_found(&st->ids)) {
		st->last_err = "Session does not exist.";
		goto error;
	}

	rc = state_exec_request(st, s, 0, true, &req, resp);
	if (rc == RS_FULL) {
		/*
		 * RS_FULL is benign on readonly requests, it occurs when temp
		 * store is full.
		 */
		rc = RS_OK;
	}

	return rc;

error:
	state_encode_error(st, resp);
	return RS_OK;
}

static int state_on_init(struct state *st, uint64_t realtime,
			 uint64_t monotonic, const unsigned char *rand)
{
	st->realtime = realtime;
	st->monotonic = monotonic;

	sc_rand_init(&st->wrand, rand);
	sc_rand_init(&st->rrand, rand);

	return aux_add_log(&st->aux, 0, "INFO", "Cluster init.");
}

int state_apply(struct state *st, uint64_t index, unsigned char *e,
		struct session **s)
{
	int rc;
	enum cmd_id type;
	struct cmd cmd;
	struct sc_buf buf;

	if (index != st->index + 1) {
		sc_log_error("index : %" PRIu64 " state : %" PRIu64 " \n",
			     index, st->index);
	}
	assert(e != NULL);
	assert(st->index + 1 == index);

	*s = NULL;
	st->client = false;
	st->readonly = false;
	st->full = false;
	st->term = entry_term(e);
	st->index = index;

	buf = sc_buf_wrap(entry_data(e), entry_data_len(e), SC_BUF_READ);
	type = (enum cmd_id) entry_flags(e);

	switch (type) {
	case CMD_INIT:
		cmd.init = cmd_decode_init(&buf);
		rc = state_on_init(st, cmd.init.realtime, cmd.init.monotonic,
				   cmd.init.rand);
		break;
	case CMD_META:
		cmd.meta = cmd_decode_meta(&buf);
		rc = state_on_meta(st, index, &cmd.meta.meta);
		break;
	case CMD_TERM:
		cmd.term = cmd_decode_term(&buf);
		rc = state_on_term_start(st, index, cmd.term.realtime,
					 cmd.term.monotonic);
		break;
	case CMD_REQUEST:
		st->client = true;
		rc = state_on_client_request(st, index, e, s);
		break;
	case CMD_CONNECT:
		cmd.connect = cmd_decode_connect(&buf);
		rc = state_on_client_connect(st, cmd.connect.name,
					     cmd.connect.local,
					     cmd.connect.remote, s);
		break;
	case CMD_DISCONNECT:
		cmd.disconnect = cmd_decode_disconnect(&buf);
		rc = state_on_client_disconnect(st, cmd.disconnect.name,
						cmd.disconnect.clean);
		break;
	case CMD_TIMESTAMP:
		cmd.timestamp = cmd_decode_timestamp(&buf);
		rc = state_on_timestamp(st, cmd.timestamp.realtime,
					cmd.timestamp.monotonic);
		break;
	case CMD_INFO:
		rc = state_on_info(st, &buf);
		break;
	case CMD_LOG:
		cmd.log = cmd_decode_log(&buf);
		rc = aux_add_log(&st->aux, index, cmd.log.level, cmd.log.log);
		break;
	default:
		rs_abort("Unknown operation : %d", type);
	}

	return rc;
}
