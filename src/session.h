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


#ifndef RESQL_SESSION_H
#define RESQL_SESSION_H

#include "sc/sc_buf.h"
#include "sc/sc_list.h"
#include "sc/sc_map.h"

#include <stdint.h>


struct session
{
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

uint64_t session_create_stmt(struct session *s, uint64_t id, const char *sql,
                             int len, const char **err);

void *session_get_stmt(struct session *s, uint64_t id);
int session_del_stmt(struct session *s, uint64_t id);


#endif
