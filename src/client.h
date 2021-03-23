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


#ifndef RESQL_CLIENT_H
#define RESQL_CLIENT_H

#include "conn.h"
#include "msg.h"

#include "sc/sc_buf.h"
#include "sc/sc_sock.h"
#include "sc/sc_str.h"

struct client
{
    struct conn conn;
    struct sc_list list;

    char *name;
    uint64_t ts;
    uint64_t id;
    uint64_t seq;

    struct msg msg;
    uint64_t commit_index;
    uint64_t round_index;
    struct sc_list read;
    bool msg_wait;
    bool terminated;
};

struct client *client_create(struct conn *conn, const char *name);
void client_destroy(struct client *c);
void client_print(struct client *c, char *buf, int len);
void client_processed(struct client *c);
bool client_pending(struct client *c);

#endif
