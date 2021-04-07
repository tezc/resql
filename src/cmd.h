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

struct cmd_log
{
    const char *level;
    const char *log;
};


void cmd_encode_init(struct sc_buf *buf, unsigned char rand[256]);
struct cmd_init cmd_decode_init(struct sc_buf *buf);

void cmd_encode_meta(struct sc_buf *buf, struct meta *meta);
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

void cmd_encode_log(struct sc_buf *buf, const char *level, const char *log);
struct cmd_log cmd_decode_log(struct sc_buf *buf);

#endif
