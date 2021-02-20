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

#ifndef RESQL_CONN_H
#define RESQL_CONN_H

#include "msg.h"
#include "rs.h"
#include "server.h"

#include "sc/sc_buf.h"
#include "sc/sc_list.h"
#include "sc/sc_map.h"
#include "sc/sc_sock.h"
#include "sc/sc_uri.h"

enum conn_state
{
    CONN_DISCONNECTED,
    CONN_TCP_ATTEMPT,
    CONN_CONNECTED,
};

struct conn
{
    struct sc_list list;
    enum conn_state state;
    struct sc_sock sock;
    struct sc_buf out;
    struct sc_buf in;
    struct msg msg;
    struct sc_sock_poll *poll;
    struct sc_timer *timer;
    uint64_t timer_id;

    char local[64];
    char remote[64];
};


struct conn *conn_create(struct sc_sock_poll *loop, struct sc_timer *timer,
                         struct sc_sock *sock);
void conn_destroy(struct conn *conn);

void conn_move(struct conn *c, struct conn *src);
void conn_init(struct conn *c, struct sc_sock_poll *poll,
               struct sc_timer *timer);
void conn_term(struct conn *c);

void conn_schedule(struct conn *c, uint32_t type, uint32_t timeout);
void conn_clear_timer(struct conn *c);

int conn_try_connect(struct conn *c, struct sc_uri *uri);
void conn_disconnect(struct conn *c);
int conn_on_out_connected(struct conn *c);

int conn_on_writable(struct conn *c);
int conn_on_readable(struct conn *c);
void conn_allow_read(struct conn *c);
void conn_set_type(struct conn *c, enum server_fd_type type);
void conn_disallow_read(struct conn *c);
void conn_clear_events(struct conn *c);
int conn_flush(struct conn *c);

#endif
