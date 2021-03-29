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


#ifndef RESQL_CMD_H
#define RESQL_CMD_H

#include "meta.h"

#include "sc/sc_buf.h"

enum cmd_id
{
    CMD_INIT,
    CMD_META,
    CMD_TERM_START,
    CMD_CLIENT_REQUEST,
    CMD_CLIENT_CONNECT,
    CMD_CLIENT_DISCONNECT,
    CMD_TIMESTAMP,
    CMD_INFO,
    CMD_LOG
};

struct cmd_init
{
    uint64_t monotonic;
    uint64_t realtime;
    unsigned char rand[256];
};

struct cmd_meta
{
    struct meta meta;
};

struct cmd_term_start
{
    uint64_t monotonic;
    uint64_t realtime;
};

struct cmd_client_connect
{
    const char *name;
    const char *local;
    const char *remote;
};

struct cmd_client_disconnect
{
    const char *name;
    bool clean;
};

struct cmd_timestamp
{
    uint64_t monotonic;
    uint64_t realtime;
};

struct cmd_info
{
    struct sc_buf node_info;
};

struct cmd_log
{
    const char *level;
    const char *log;
};


void cmd_encode_init(struct sc_buf *buf, unsigned char rand[256]);
struct cmd_init cmd_decode_init(struct sc_buf *buf);

void cmd_encode_meta(struct sc_buf *buf, struct meta *meta);
void cmd_decode_meta_to(void *data, int len, struct meta *meta);
struct cmd_meta cmd_decode_meta(struct sc_buf *buf);

void cmd_encode_term_start(struct sc_buf *buf);
struct cmd_term_start cmd_decode_term_start(struct sc_buf *buf);

void cmd_encode_client_connect(struct sc_buf *buf, const char *name,
                               const char *local, const char *remote);
struct cmd_client_connect cmd_decode_client_connect(struct sc_buf *buf);

void cmd_encode_client_disconnect(struct sc_buf *buf, const char *name,
                                  bool clean);
struct cmd_client_disconnect cmd_decode_client_disconnect(struct sc_buf *buf);

void cmd_encode_timestamp(struct sc_buf *buf);
struct cmd_timestamp cmd_decode_timestamp(struct sc_buf *buf);

void cmd_encode_log(struct sc_buf *buf, const char* level, const char* log);
struct cmd_log cmd_decode_log(struct sc_buf *buf);

#endif
