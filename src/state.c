/*
 *  Resql
 *
 *  Copyright (C) 2021 Ozan Tezcan
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


#include "aux.h"
#include "cmd.h"
#include "file.h"
#include "info.h"
#include "metric.h"
#include "msg.h"
#include "session.h"
#include "state.h"

#include "sc/sc_buf.h"
#include "sc/sc_log.h"
#include "sc/sc_uri.h"
#include "sqlite/sqlite3.h"

#include <assert.h>
#include <unistd.h>

#define STATE_FILE        "state.resql"
#define STATE_SS_FILE     "snapshot.resql"
#define STATE_SS_TMP_FILE "snapshot.tmp.resql"

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
            ret = state->cb.add_node(state->cb.arg, (const char *) value);
            sqlite3_result_text(ctx, ret, -1, NULL);
        }
    } else if (strcmp((char *) cmd, "remove-node") == 0) {
        if (argc != 2) {
            sqlite3_result_error(ctx, usage_remove_node, -1);
            return;
        }

        if (state->cb.remove_node) {
            value = sqlite3_value_text(argv[1]);
            ret = state->cb.remove_node(state->cb.arg, (const char *) value);
            sqlite3_result_text(ctx, ret, -1, NULL);
        }
    } else if (strcmp((char *) cmd, "shutdown") == 0) {
        if (argc != 2) {
            sqlite3_result_error(ctx, usage_remove_node, -1);
            return;
        }

        if (state->cb.shutdown) {
            value = sqlite3_value_text(argv[1]);
            ret = state->cb.shutdown(state->cb.arg, (const char *) value);
            sqlite3_result_text(ctx, ret, -1, NULL);
        }
    } else if (strcmp((char *) cmd, "max-size") == 0) {
        if (argc < 1 || argc > 2) {
            sqlite3_result_error(ctx, usage_max_size, -1);
            return;
        }

        if (argc == 2) {
            state->max_page = sqlite3_value_int64(argv[1]) / 4096;
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

    const uint64_t unixEpoch = 24405875 * (uint64_t) 8640000;

    *val = unixEpoch + t_state->realtime;
    return SQLITE_OK;
}

int state_max_page(uint32_t max, uint32_t curr)
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
        rs_abort("db");
    }

    orig = sqlite3_vfs_find(NULL);
    if (!orig) {
        rs_abort("db");
    }

    ext = *orig;
    ext.zName = "resql_vfs";
    ext.xRandomness = state_randomness;
    ext.xCurrentTimeInt64 = state_currenttime;

    rc = sqlite3_vfs_register(&ext, 1);
    if (rc != SQLITE_OK) {
        rs_abort("fail %s \n", sqlite3_errstr(rc));
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
    *st = (struct state){{0}};

    st->cb = cb;
    st->path = sc_str_create_fmt("%s/%s", path, STATE_FILE);
    st->ss_path = sc_str_create_fmt("%s/%s", path, STATE_SS_FILE);
    st->ss_tmp_path = sc_str_create_fmt("%s/%s", path, STATE_SS_TMP_FILE);
    st->max_page = UINT64_MAX;

    sc_buf_init(&st->tmp, 1024);
    meta_init(&st->meta, name);
    sc_map_init_sv(&st->nodes, 16, 0);
    sc_map_init_sv(&st->names, 16, 0);
    sc_map_init_64v(&st->ids, 16, 0);
    sc_list_init(&st->disconnects);

    t_state = st;
}

void state_term(struct state *st)
{
    state_close(st);

    sc_buf_term(&st->tmp);
    sc_str_destroy(st->path);
    sc_str_destroy(st->ss_path);
    sc_str_destroy(st->ss_tmp_path);
}

int state_authorizer(void *user, int action, const char *arg0, const char *arg1,
                     const char *arg2, const char *arg3)
{
    (void) arg2;
    (void) arg3;

    uint32_t len;
    struct state *st = user;

    if (!st->client) {
        return SQLITE_OK;
    }

    switch (action) {
    case SQLITE_READ:
        len = strlen("resql_sessions");
        if (strncmp("resql_sessions", arg0, len) == 0 &&
            strncmp("resp", arg1, strlen("resp")) == 0) {
            return SQLITE_IGNORE;
        }
    case SQLITE_ALTER_TABLE:
        arg0 = arg1;
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

void state_abort(struct state *st, int rc)
{

}

int state_check_err(struct state *st, int rc)
{
    switch (rc) {
    case SQLITE_OK:         /* Successful result */
    case SQLITE_ERROR:      /* Generic error */
    case SQLITE_TOOBIG:     /* String or BLOB exceeds size limit */
    case SQLITE_CONSTRAINT: /* Abort due to constraint violation */
    case SQLITE_MISMATCH:   /* Data type mismatch */
    case SQLITE_AUTH:       /* Authorization denied */
    case SQLITE_RANGE:      /* 2nd parameter to sqlite3_bind out of range */
    case SQLITE_ROW:        /* sqlite3_step() has another row ready */
    case SQLITE_DONE:       /* sqlite3_step() has finished executing */
        break;

    case SQLITE_FULL:
        if (st->readonly || st->full) {
            break;
        }
    default:
        rs_abort("sqlite : rc = (%d)(%s). \n", rc, sqlite3_errstr(rc));
    }

    return rc;
}

int state_write_infos(struct state *st, struct aux *aux)
{
    struct info *info;

    aux_clear_info(aux);

    sc_map_foreach_value (&st->nodes, info) {
        aux_write_info(aux, info);
    }

    return RS_OK;
}

int state_write_vars(struct state *st, struct aux *aux)
{
    int rc;

    sc_buf_clear(&st->tmp);

    sc_buf_put_64(&st->tmp, st->term);
    sc_buf_put_64(&st->tmp, st->index);
    meta_encode(&st->meta, &st->tmp);

    sc_buf_put_64(&st->tmp, st->timestamp);
    sc_buf_put_64(&st->tmp, st->realtime);
    sc_buf_put_64(&st->tmp, st->monotonic);
    sc_buf_put_64(&st->tmp, st->wrand.i);
    sc_buf_put_64(&st->tmp, st->wrand.j);
    sc_buf_put_raw(&st->tmp, st->wrand.init, sizeof(st->wrand.init));

    rc = aux_write_kv(aux, "var", &st->tmp);
    if (rc != RS_OK) {
        return rc;
    }

    sc_buf_clear(&st->tmp);
    sc_buf_put_raw(&st->tmp, st->meta.name, sc_str_len(st->meta.name) + 1);

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
    meta_decode(&st->meta, &st->tmp);

    st->timestamp = sc_buf_get_64(&st->tmp);
    st->realtime = sc_buf_get_64(&st->tmp);
    st->monotonic = sc_buf_get_64(&st->tmp);
    st->wrand.i = sc_buf_get_64(&st->tmp);
    st->wrand.j = sc_buf_get_64(&st->tmp);
    sc_buf_get_data(&st->tmp, st->wrand.init, sizeof(st->wrand.init));
    sc_rand_init(&st->rrand, st->wrand.init);

    sql = "SELECT * FROM resql_sessions";
    rc = sqlite3_prepare(st->aux.db, sql, -1, &sess, 0);
    if (rc != SQLITE_OK) {
        return RS_ERROR;
    }

    sql = "SELECT * FROM resql_statements WHERE client_name = (?)";
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

    if (sqlite3_finalize(sess) != SQLITE_OK ||
        sqlite3_finalize(stmt) != SQLITE_OK) {
        rs_abort("db");
    }

    return RS_OK;
}

int state_read_snapshot(struct state *st, bool in_memory)
{
    int rc;
    bool b;

    b = file_exists_at(st->ss_path);
    if (!b) {
        return RS_ERROR;
    }

    if (in_memory) {
        rc = aux_init(&st->aux, "", SQLITE_OPEN_MEMORY);
        if (rc != RS_OK) {
            rs_abort("db");
        }

        rc = aux_load_to_memory(&st->aux, st->ss_path);
        if (rc != RS_OK) {
            rs_abort("db");
        }
    } else {
        rc = file_copy(st->path, st->ss_path);
        if (rc != RS_OK) {
            rs_abort("db");
        }

        rc = aux_init(&st->aux, st->path, 0);
        if (rc != RS_OK) {
            rs_abort("db");
        }
    }

    rc = state_read_vars(st, &st->aux);
    if (rc != RS_OK) {
        rs_abort("db");
    }

    sqlite3_set_authorizer(st->aux.db, state_authorizer, st);

    return RS_OK;
}

int state_read_for_snapshot(struct state *st)
{
    bool b;
    int rc;

    b = file_exists_at(st->ss_path);
    if (!b) {
        sc_log_warn("Cannot find snapshot file at %s \n", st->ss_path);
        return RS_ERROR;
    }

    rc = file_copy(st->ss_tmp_path, st->ss_path);
    if (rc != RS_OK) {
        rs_abort("db");
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

void state_open(struct state *st, bool in_memory)
{
    int rc;

    rc = file_remove_if_exists(st->path);
    if (rc != RS_OK) {
        rs_abort("db");
    }

    rc = file_remove_if_exists(st->ss_tmp_path);
    if (rc != RS_OK) {
        rs_abort("db");
    }

    rc = state_read_snapshot(st, in_memory);
    if (rc != RS_OK) {
        sc_log_warn("Cannot find snapshot, creating one \n");

        state_initial_snapshot(st);

        rc = state_read_snapshot(st, in_memory);
        if (rc != RS_OK) {
            rs_abort("db");
        }

        metric_snapshot(true, 1000, file_size_at(st->ss_path));
    }

    sc_log_info("Opened snapshot at index [%llu] \n", st->index);
}

void state_close(struct state *st)
{
    int rc;
    struct session *s;
    struct info *info;

    if (!st->closed) {
        st->closed = true;
        st->client = false;
        st->readonly = false;

        rc = state_write_vars(st, &st->aux);
        if (rc != RS_OK) {
            rs_abort("db");
        }

        rc = aux_clear_sessions(&st->aux);
        if (rc != RS_OK) {
            rs_abort("db");
        }

        sc_map_foreach_value (&st->names, s) {
            sc_list_del(NULL, &s->list);
        }

        sc_map_foreach_value (&st->names, s) {
            aux_write_session(&st->aux, s);
            session_destroy(s);
        }

        sc_map_term_sv(&st->names);
        sc_map_term_64v(&st->ids);

        sc_map_foreach_value (&st->nodes, info) {
            info_destroy(info);
        }
        sc_map_term_sv(&st->nodes);

        meta_term(&st->meta);
        aux_term(&st->aux);
    }
}

void state_initial_snapshot(struct state *st)
{
    int rc;
    struct aux aux;

    rc = aux_init(&aux, st->ss_tmp_path, SQLITE_OPEN_CREATE);
    if (rc != RS_OK) {
        rs_abort("db");
    }

    rc = state_write_vars(st, &aux);
    if (rc != RS_OK) {
        rs_abort("db");
    }

    rc = aux_term(&aux);
    if (rc != RS_OK) {
        rs_abort("db");
    }

    rc = rename(st->ss_tmp_path, st->ss_path);
    if (rc != 0) {
        rs_abort("db");
    }
}

struct session *state_on_client_connect(struct state *st, const char *name,
                                        const char *local, const char *remote)
{
    bool found;
    struct session *sess;

    found = sc_map_get_sv(&st->names, name, (void **) &sess);
    if (!found) {
        sess = session_create(st, name, st->index);
        sc_map_put_sv(&st->names, sess->name, sess);
        sc_map_put_64v(&st->ids, sess->id, sess);
    }

    session_connected(sess, local, remote, st->realtime);
    aux_write_session(&st->aux, sess);

    return sess;
}

struct session *state_on_client_disconnect(struct state *st, const char *name,
                                           bool clean)
{
    bool found;
    struct session *sess;

    found = sc_map_get_sv(&st->names, name, (void **) &sess);
    if (!found) {
        return NULL;
    }

    session_disconnected(sess, st->timestamp);

    if (clean) {
        aux_del_session(&st->aux, sess);
        sc_map_del_sv(&st->names, name, NULL);
        sc_map_del_64v(&st->ids, sess->id, NULL);
        session_destroy(sess);
    } else {
        aux_write_session(&st->aux, sess);
        sc_list_add_tail(&st->disconnects, &sess->list);
    }

    return sess;
}

void state_disconnect_sessions(struct state *st)
{
    const char *name;

    sc_map_foreach_key (&st->names, name) {
        state_on_client_disconnect(st, name, false);
    }
}

void state_on_meta(struct state *st, struct cmd_meta *cmd)
{
    bool found;
    struct meta_node node;
    struct info *info;
    const char *name;

    meta_copy(&st->meta, &cmd->meta);
    meta_term(&cmd->meta);

    sc_map_foreach (&st->nodes, name, info) {
        found = false;

        sc_array_foreach (st->meta.nodes, node) {
            if (strcmp(node.name, name) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            sc_map_del_sv(&st->nodes, name, NULL);
            info_destroy(info);
            break;
        }
    }

    sc_array_foreach (st->meta.nodes, node) {
        found = sc_map_get_sv(&st->nodes, node.name, (void **) &info);
        if (!found) {
            info = info_create(node.name);
            sc_map_put_sv(&st->nodes, info->name, info);
        }

        info_set_connected(info, node.connected);
        info_set_role(info, meta_role_str[node.role]);

        sc_buf_clear(&st->tmp);

        for (size_t i = 0; i < sc_array_size(node.uris); i++) {
            sc_buf_put_text(&st->tmp, node.uris[i]->str);
        }
        info_set_urls(info, (char *) st->tmp.mem);
    }

    state_write_infos(st, &st->aux);
    state_write_vars(st, &st->aux);
}

void state_on_term_start(struct state *st, struct cmd_start_term *start)
{
    st->realtime = start->realtime;
    st->monotonic = start->monotonic;

    state_disconnect_sessions(st);
}

void state_on_info(struct state *st, struct sc_buf *buf)
{
    bool found;
    uint32_t len;
    void *ptr;
    const char *name;
    struct sc_buf tmp;
    struct info *info;

    while (sc_buf_size(buf) != 0) {
        name = sc_buf_get_str(buf);
        len = sc_buf_get_32(buf);
        ptr = sc_buf_get_blob(buf, len);

        found = sc_map_get_sv(&st->nodes, name, (void **) &info);
        if (found && len != 0) {
            tmp = sc_buf_wrap(ptr, len, SC_BUF_READ);
            info_set_stats(info, &tmp);
        }
    }

    state_write_infos(st, &st->aux);
}

void state_on_timestamp(struct state *st, uint64_t realtime, uint64_t monotonic)
{
    struct session *sess;
    struct sc_list *tmp, *it;

    assert(st->monotonic <= monotonic);

    st->timestamp += monotonic - st->monotonic;
    st->monotonic = monotonic;
    st->realtime = realtime;

    sc_list_foreach_safe (&st->disconnects, tmp, it) {
        sess = sc_list_entry(it, struct session, list);
        if (st->timestamp - sess->disconnect_time > 60000) {
            aux_del_session(&st->aux, sess);
            sc_map_del_sv(&st->names, sess->name, NULL);
            sc_map_del_64v(&st->ids, sess->id, NULL);
            session_destroy(sess);
        }
    }
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

static int state_exec_prepared_statement(struct state *st, sqlite3_stmt *stmt,
                                         bool readonly, struct sc_buf *req,
                                         struct sc_buf *resp)
{
    int rc, type, idx;
    const char *param;

    if (readonly && sqlite3_stmt_readonly(stmt) == 0) {
        st->last_err = "Operation is not readonly.";
        return RS_ERROR;
    }

    while ((type = sc_buf_get_8(req)) != TASK_FLAG_END) {
        switch (type) {
        case TASK_PARAM_INDEX:
            /**
             * Sqlite index starts from '1', to make it consistent with
             * sqlite_column_xxx api which starts from '0', add 1 to
             * index so users can work starting from '0'.
             */
            idx = sc_buf_get_32(req);
            idx++;
            break;
        case TASK_PARAM_NAME:
            param = sc_buf_get_str(req);

            idx = sqlite3_bind_parameter_index(stmt, param);
            if (idx == 0) {
                st->last_err = "Invalid parameter name";
                return RS_ERROR;
            }
            break;
        default:
            st->last_err = "Invalid message";
            return RS_ERROR;
        }

        type = sc_buf_get_8(req);

        switch (type) {
        case TASK_PARAM_INTEGER: {
            int64_t t = sc_buf_get_64(req);

            rc = sqlite3_bind_int64(stmt, idx, t);
            if (rc != SQLITE_OK) {
                return RS_ERROR;
            }
        } break;

        case TASK_PARAM_FLOAT: {
            double f = sc_buf_get_double(req);

            rc = sqlite3_bind_double(stmt, idx, f);
            if (rc != SQLITE_OK) {
                return RS_ERROR;
            }
        } break;

        case TASK_PARAM_TEXT: {
            uint32_t size = sc_buf_peek_32(req);
            const char *val = sc_buf_get_str(req);

            rc = sqlite3_bind_text(stmt, idx, val, size, SQLITE_STATIC);
            if (rc != SQLITE_OK) {
                return RS_ERROR;
            }
        } break;

        case TASK_PARAM_BLOB: {
            uint32_t blen = sc_buf_get_32(req);
            void *data = sc_buf_get_blob(req, blen);

            rc = sqlite3_bind_blob(stmt, idx, data, blen, SQLITE_STATIC);
            if (rc != SQLITE_OK) {
                return RS_ERROR;
            }
        } break;

        case TASK_PARAM_NULL: {
            rc = sqlite3_bind_null(stmt, idx);
            if (rc != SQLITE_OK) {
                return RS_ERROR;
            }
        } break;

        default:
            st->last_err = "Invalid message";
            return RS_ERROR;
        }
    }

    rc = sqlite3_step(stmt);

    sc_buf_put_32(resp, sqlite3_changes(st->aux.db));
    sc_buf_put_64(resp, sqlite3_last_insert_rowid(st->aux.db));

    if (rc == SQLITE_ROW) {
        sc_buf_put_8(resp, TASK_FLAG_ROW);

        int column_count = sqlite3_column_count(stmt);
        sc_buf_put_32(resp, column_count);

        for (int i = 0; i < column_count; i++) {
            const char *name = sqlite3_column_name(stmt, i);
            sc_buf_put_str(resp, name);
        }

        uint32_t row = 0;
        uint32_t row_pos = sc_buf_wpos(resp);
        sc_buf_put_32(resp, row);

        do {
            row++;

            for (int i = 0; i < column_count; i++) {
                int t = sqlite3_column_type(stmt, i);

                switch (t) {
                case SQLITE_INTEGER: {
                    int64_t val = sqlite3_column_int64(stmt, i);
                    sc_buf_put_8(resp, TASK_PARAM_INTEGER);
                    sc_buf_put_64(resp, val);
                } break;

                case SQLITE_FLOAT: {
                    double val = sqlite3_column_double(stmt, i);
                    sc_buf_put_8(resp, TASK_PARAM_FLOAT);
                    sc_buf_put_double(resp, val);
                } break;

                case SQLITE_TEXT: {
                    int len;
                    const char *data;

                    len = sqlite3_column_bytes(stmt, i);
                    data = (const char *) sqlite3_column_text(stmt, i);

                    sc_buf_put_8(resp, TASK_PARAM_TEXT);
                    sc_buf_put_str_len(resp, data, len);
                } break;

                case SQLITE_BLOB: {
                    const void *data = sqlite3_column_blob(stmt, i);
                    int len = sqlite3_column_bytes(stmt, i);

                    sc_buf_put_8(resp, TASK_PARAM_BLOB);
                    sc_buf_put_blob(resp, data, (uint32_t) len);
                } break;

                case SQLITE_NULL: {
                    sc_buf_put_8(resp, TASK_PARAM_NULL);
                } break;

                default:
                    break;
                }
            }
        } while ((rc = sqlite3_step(stmt)) == SQLITE_ROW);

        sc_buf_set_32_at(resp, row_pos, row);
    }

    if (rc != SQLITE_DONE) {
        state_check_err(st, rc);
        return RS_ERROR;
    }

    sc_buf_put_8(resp, TASK_FLAG_DONE);

    return RS_OK;
}

static int state_exec_stmt(struct state *st, bool readonly, struct sc_buf *req,
                           struct sc_buf *resp)
{
    int rc;
    sqlite3_stmt *stmt = NULL;
    uint32_t len = sc_buf_peek_32(req);
    const char *str = sc_buf_get_str(req);

    rc = sqlite3_prepare_v3(st->aux.db, str, len, 0, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return RS_ERROR;
    }

    rc = state_exec_prepared_statement(st, stmt, readonly, req, resp);

    sqlite3_finalize(stmt);

    return rc;
}

static int state_prepare_stmt(struct state *state, struct session *sess,
                              uint64_t index, struct sc_buf *req,
                              struct sc_buf *resp)
{
    uint64_t id;
    const char *err;
    int len = sc_buf_peek_32(req);
    const char *str = sc_buf_get_str(req);

    if (!sc_buf_valid(req)) {
        state->last_err = "Corrupt message";
        return RS_ERROR;
    }

    id = session_create_stmt(sess, index, str, len, &err);
    if (id == 0) {
        return RS_ERROR;
    }

    state->client = false;
    aux_add_stmt(&state->aux, sess->name, sess->id, id, str);
    state->client = true;

    sc_buf_put_64(resp, id);

    return RS_OK;
}

static int state_del_prepared(struct state *state, struct session *sess,
                              uint64_t index, struct sc_buf *req,
                              struct sc_buf *resp)
{
    (void) resp;
    (void) index;

    int rc;
    uint64_t id;

    id = sc_buf_get_64(req);

    if (!sc_buf_valid(req)) {
        state->last_err = "Corrupt message";
        return RS_ERROR;
    }

    rc = session_del_stmt(sess, id);
    if (rc != RS_OK) {
        state->last_err = "Prepared statement does not exist.";
        return RS_ERROR;
    }

    state->client = false;
    aux_rm_stmt(&state->aux, id);
    state->client = true;

    return RS_OK;
}

static int state_exec_stmt_id(struct state *st, struct session *sess,
                              bool readonly, struct sc_buf *req,
                              struct sc_buf *resp)
{
    int rc, ret;
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

    ret = sqlite3_clear_bindings(stmt);
    if (ret != SQLITE_OK) {
        return RS_ERROR;
    }

    ret = sqlite3_reset(stmt);
    if (ret != SQLITE_OK) {
        return RS_ERROR;
    }

    return rc;
}

static int state_exec_non_stmt_task(struct state *st, struct session *sess,
                                    uint64_t index, enum task_flag flag,
                                    struct sc_buf *req, struct sc_buf *resp)
{
    int rc;

    switch (flag) {
    case TASK_FLAG_STMT_PREPARE:
        rc = state_prepare_stmt(st, sess, index, req, resp);
        break;
    case TASK_FLAG_STMT_DEL_PREPARED:
        rc = state_del_prepared(st, sess, index, req, resp);
        break;
    default:
        return RS_NOOP;
    }

    if (rc == RS_ERROR) {
        goto error;
    }

    sc_buf_put_8(resp, TASK_FLAG_DONE);
    msg_finalize_client_resp(resp);

    return RS_OK;

error:
    sc_buf_clear(resp);
    msg_create_client_resp_header(resp);
    sc_buf_put_8(resp, TASK_FLAG_ERROR);
    sc_buf_put_str(resp, state_errstr(st));
    sc_buf_put_8(resp, TASK_FLAG_END);

    msg_finalize_client_resp(resp);

    return RS_ERROR;
}

int state_exec_request(struct state *st, struct session *sess, uint64_t index,
                       void *data, uint32_t len)
{
    assert(len < UINT32_MAX);

    int rc;
    uint32_t pos, result_len;
    enum task_flag flag;
    struct sc_buf *resp = &sess->resp;
    struct sc_buf req = sc_buf_wrap(data, len, SC_BUF_READ);

    sc_buf_clear(&sess->resp);
    msg_create_client_resp_header(resp);

    sc_buf_put_8(resp, TASK_FLAG_OK);

    flag = sc_buf_get_8(&req);
    rc = state_exec_non_stmt_task(st, sess, index, flag, &req, resp);
    if (rc != RS_NOOP) {
        return RS_OK;
    }

    rc = sqlite3_step(st->aux.begin);
    if (rc != SQLITE_DONE) {
        goto error;
    }

    do {
        sc_buf_put_8(resp, TASK_FLAG_STMT);

        pos = sc_buf_wpos(resp);
        sc_buf_put_32(resp, 0);

        switch (flag) {
        case TASK_FLAG_STMT:
            rc = state_exec_stmt(st, false, &req, resp);
            break;
        case TASK_FLAG_STMT_ID:
            rc = state_exec_stmt_id(st, sess, false, &req, resp);
            break;
        default:
            rc = RS_ERROR;
            st->last_err = "Invalid message";
            break;
        }

        if (rc != RS_OK) {
            goto error;
        }

        sc_buf_put_8(resp, TASK_FLAG_END);

        result_len = sc_buf_wpos(resp) - pos;
        sc_buf_set_32_at(resp, pos, result_len);

        if (sc_buf_size(&req) == 0) {
            break;
        }

        flag = sc_buf_get_8(&req);

    } while (true);

    if (!sc_buf_valid(&req)) {
        goto error;
    }

    rc = sqlite3_step(st->aux.commit);
    if (rc != SQLITE_DONE) {
        goto error;
    }

    sc_buf_put_8(resp, TASK_FLAG_DONE);
    msg_finalize_client_resp(resp);

    return RS_OK;

error:
    sc_buf_clear(&sess->resp);
    msg_create_client_resp_header(resp);
    sc_buf_put_8(resp, TASK_FLAG_ERROR);
    sc_buf_put_str(resp, state_errstr(st));
    sc_buf_put_8(resp, TASK_FLAG_END);

    st->client = false;
    rc = sqlite3_step(st->aux.rollback);
    if (rc != SQLITE_DONE) {
        sc_log_error("Rollback failure : %s \n", sqlite3_errmsg(st->aux.db));
    }

    msg_finalize_client_resp(resp);

    return RS_OK;
}

struct session *state_on_client_request(struct state *st, uint64_t index,
                                        char *entry)
{
    bool found;
    uint32_t len;
    uint64_t id = entry_cid(entry);
    void *data;
    struct session *sess;

    st->last_err = NULL;

    found = sc_map_get_64v(&st->ids, id, (void **) &sess);
    if (!found) {
        return NULL;
    }

    data = entry_data(entry);
    len = entry_data_len(entry);

    if (entry_seq(entry) == sess->seq) {
        sc_buf_set_rpos(&sess->resp, 0);
        return sess;
    }

    state_exec_request(st, sess, index, data, len);
    sess->seq = entry_seq(entry);

    return sess;
}

int state_apply_readonly(struct state *st, uint64_t cid, unsigned char *buf,
                         uint32_t len, struct sc_buf *resp)
{
    bool found;
    int rc;
    uint32_t head, pos;
    enum task_flag flag;
    struct session *sess;
    struct sc_buf req = sc_buf_wrap(buf, len, SC_BUF_READ);

    msg_create_client_resp_header(resp);
    head = sc_buf_wpos(resp);

    st->last_err = NULL;
    st->client = true;
    st->readonly = true;
    st->full = false;

    found = sc_map_get_64v(&st->ids, cid, (void **) &sess);
    if (!found) {
        st->last_err = "Session does not exist.";
        goto error;
    }

    sc_buf_put_8(resp, TASK_FLAG_OK);

    rc = sqlite3_step(st->aux.begin);
    if (rc != SQLITE_DONE) {
        state_check_err(st, rc);
        goto error;
    }

    flag = sc_buf_get_8(&req);

    do {
        sc_buf_put_8(resp, TASK_FLAG_STMT);

        pos = sc_buf_wpos(resp);
        sc_buf_put_32(resp, 0);

        switch (flag) {
        case TASK_FLAG_STMT:
            rc = state_exec_stmt(st, true, &req, resp);
            break;
        case TASK_FLAG_STMT_ID:
            rc = state_exec_stmt_id(st, sess, true, &req, resp);
            break;
        default:
            rc = RS_ERROR;
            st->last_err = "Invalid message";
            break;
        }

        if (rc != RS_OK) {
            goto error;
        }

        sc_buf_put_8(resp, TASK_FLAG_END);
        sc_buf_set_32_at(resp, pos, sc_buf_wpos(resp) - pos);

        if (sc_buf_size(&req) == 0) {
            break;
        }

        flag = sc_buf_get_8(&req);

    } while (true);

    if (!sc_buf_valid(&req)) {
        goto error;
    }

    rc = sqlite3_step(st->aux.commit);
    if (rc != SQLITE_DONE) {
        state_check_err(st, rc);
        goto error;
    }

    sc_buf_put_8(resp, TASK_FLAG_DONE);
    msg_finalize_client_resp(resp);

    return RS_OK;

error:
    sc_buf_set_wpos(resp, head);
    sc_buf_put_8(resp, TASK_FLAG_ERROR);
    sc_buf_put_str(resp, state_errstr(st));
    sc_buf_put_8(resp, TASK_FLAG_END);

    rc = sqlite3_step(st->aux.rollback);
    if (rc != SQLITE_DONE) {
        state_check_err(st, rc);
        sc_log_error("Rollback failure : %s \n", sqlite3_errmsg(st->aux.db));
    }

    msg_finalize_client_resp(resp);

    return RS_OK;
}

struct session *state_apply(struct state *st, uint64_t index, char *entry)
{
    struct sc_buf cmd;
    enum cmd_id type;

    assert(entry != NULL);
    assert(st->index + 1 == index);

    st->client = false;
    st->readonly = false;
    st->full = false;
    st->term = entry_term(entry);
    st->index = index;

    cmd = sc_buf_wrap(entry_data(entry), entry_data_len(entry), SC_BUF_READ);
    type = entry_flags(entry);

    switch (type) {
    case CMD_INIT: {
        struct cmd_init c;

        c = cmd_decode_init(&cmd);
        st->realtime = c.realtime;
        st->monotonic = c.monotonic;

        sc_rand_init(&st->wrand, c.rand);
        sc_rand_init(&st->rrand, c.rand);

        aux_add_log(&st->aux, index, "INFO", "Cluster initialized");

        return NULL;
    }

    case CMD_META: {
        struct cmd_meta m;

        m = cmd_decode_meta(&cmd);
        state_on_meta(st, &m);
    } break;

    case CMD_TERM_START: {
        struct cmd_start_term s;

        s = cmd_decode_term_start(&cmd);
        state_on_term_start(st, &s);
        aux_add_log(&st->aux, index, "INFO", "Term start");
        return NULL;
    }

    case CMD_CLIENT_REQUEST:
        st->client = true;
        return state_on_client_request(st, index, entry);

    case CMD_CLIENT_CONNECT: {
        struct cmd_client_connect c;

        c = cmd_decode_client_connect(&cmd);
        return state_on_client_connect(st, c.name, c.local, c.remote);
    }

    case CMD_CLIENT_DISCONNECT: {
        struct cmd_client_disconnect c;

        c = cmd_decode_client_disconnect(&cmd);
        return state_on_client_disconnect(st, c.name, c.clean);
    }

    case CMD_TIMESTAMP: {
        struct cmd_timestamp t;

        t = cmd_decode_timestamp(&cmd);
        state_on_timestamp(st, t.realtime, t.monotonic);
    } break;

    case CMD_INFO: {
        state_on_info(st, &cmd);
        return NULL;
    }

    default:
        rs_abort("");
    }

    return NULL;
}
