/*
 *  reSQL
 *
 *  Copyright (C) 2021 reSQL Authors
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


#ifndef RESQL_STATE_H
#define RESQL_STATE_H

#include "aux.h"
#include "cmd.h"
#include "entry.h"
#include "file.h"
#include "meta.h"

#include "sc/sc_array.h"
#include "sc/sc_map.h"
#include "sc/sc.h"

struct state_cb
{
    void* arg;
    const char* (*add_node)(void* arg, const char* node);
    const char* (*remove_node)(void* arg, const char* node);
    const char* (*shutdown)(void* arg, const char* node);
};

struct state
{
    struct state_cb cb;
    char *path;
    char *ss_path;
    char *ss_tmp_path;
    char *last_err;

    bool closed;
    bool client;
    bool readonly;

    struct aux aux;
    struct meta meta;
    uint64_t term;
    uint64_t index;

    // time
    uint64_t timestamp;
    uint64_t monotonic;
    uint64_t realtime;

    char *auth;
    uint64_t max_page;

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

struct session *state_apply(struct state *st, uint64_t index, char *entry);

#endif
