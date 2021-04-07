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
