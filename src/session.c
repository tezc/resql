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


#include "rs.h"
#include "session.h"
#include "state.h"

#include "sc/sc_buf.h"
#include "sc/sc_str.h"

#include <string.h>
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

    sc_map_init_64v(&s->stmts, 0, 0);
    sc_buf_init(&s->resp, 64);
    sc_list_init(&s->list);

    return s;
}

void session_destroy(struct session *s)
{
    sqlite3_stmt *stmt;

    sc_buf_term(&s->resp);
    sc_str_destroy(s->name);
    sc_str_destroy(s->local);
    sc_str_destroy(s->remote);
    sc_str_destroy(s->connect_time);
    sc_list_del(NULL, &s->list);

    sc_map_foreach_value (&s->stmts, stmt) {
        sqlite3_finalize(stmt);
    }
    sc_map_term_64v(&s->stmts);

    rs_free(s);
}

void session_connected(struct session *s, const char *local, const char *remote, uint64_t ts)
{
    char tmp[32] = {0};
    struct tm tm, *p;
    time_t t = (time_t) ts / 1000;

    s->disconnect_time = 0;
    sc_list_del(NULL, &s->list);
    sc_str_set(&s->local, local);
    sc_str_set(&s->remote, remote);

    p = localtime_r(&t, &tm);
    if (!p) {
        strcpy(tmp, "localtime_r failed");
    }

    strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", p);
    sc_str_set(&s->connect_time, tmp);
}

void session_disconnected(struct session *s, uint64_t timestamp)
{
    s->disconnect_time = timestamp;
    sc_str_set(&s->local, "");
    sc_str_set(&s->remote, "");
}

uint64_t session_create_stmt(struct session *s, uint64_t id, const char *sql,
                             int len, const char **err)
{
    int rc;
    uint64_t prev_id;
    sqlite3_stmt *prev, *stmt;

    *err = NULL;

    sc_map_foreach (&s->stmts, prev_id, prev) {
        const char *prev_sql = sqlite3_sql(prev);
        if (strcmp(prev_sql, sql) == 0) {
            return (uint64_t) prev_id;
        }
    }

    rc = sqlite3_prepare_v3(s->state->aux.db, sql, len,
                            SQLITE_PREPARE_PERSISTENT, &stmt, NULL);
    if (rc != SQLITE_OK) {
        goto error;
    }

    sc_map_put_64v(&s->stmts, id, stmt);

    return id;

error:
    *err = sqlite3_errmsg(s->state->aux.db);
    return 0;
}

int session_del_stmt(struct session *s, uint64_t id)
{
    bool found;
    int rc;
    sqlite3_stmt *stmt;

    found = sc_map_del_64v(&s->stmts, id, (void **) &stmt);
    if (!found) {
        return RS_ERROR;
    }

    rc = sqlite3_finalize(stmt);
    if (rc != SQLITE_OK) {
        rs_abort("%s \n", sqlite3_errmsg(s->state->aux.db));
    }

    return RS_OK;
}

sqlite3_stmt *session_get_stmt(struct session *s, uint64_t id)
{
    bool found;
    sqlite3_stmt *stmt;

    found = sc_map_get_64v(&s->stmts, id, (void **) &stmt);

    return found ? stmt : NULL;
}
