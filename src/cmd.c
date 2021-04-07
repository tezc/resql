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


#include "cmd.h"

#include "sc/sc_time.h"

void cmd_encode_init(struct sc_buf *buf, unsigned char rand[256])
{
    sc_buf_put_64(buf, sc_time_ms());
    sc_buf_put_64(buf, sc_time_mono_ms());
    sc_buf_put_raw(buf, rand, 256);
}

struct cmd_init cmd_decode_init(struct sc_buf *buf)
{
    struct cmd_init init;

    init.realtime = sc_buf_get_64(buf);
    init.monotonic = sc_buf_get_64(buf);
    sc_buf_get_data(buf, init.rand, sizeof(init.rand));

    return init;
}

void cmd_encode_term_start(struct sc_buf *buf)
{
    sc_buf_put_64(buf, sc_time_ms());
    sc_buf_put_64(buf, sc_time_mono_ms());
}

struct cmd_term_start cmd_decode_term_start(struct sc_buf *buf)
{
    struct cmd_term_start start;

    start.realtime = sc_buf_get_64(buf);
    start.monotonic = sc_buf_get_64(buf);

    return start;
}

void cmd_encode_meta(struct sc_buf *buf, struct meta *meta)
{
    meta_encode(meta, buf);
}

struct cmd_meta cmd_decode_meta(struct sc_buf *buf)
{
    struct cmd_meta cmd;

    meta_init(&cmd.meta, "");
    meta_decode(&cmd.meta, buf);

    return cmd;
}

void cmd_encode_client_connect(struct sc_buf *buf, const char *name,
                               const char *local, const char *remote)
{
    sc_buf_put_str(buf, name);
    sc_buf_put_str(buf, local);
    sc_buf_put_str(buf, remote);
}

struct cmd_client_connect cmd_decode_client_connect(struct sc_buf *buf)
{
    struct cmd_client_connect cmd;

    cmd.name = sc_buf_get_str(buf);
    cmd.local = sc_buf_get_str(buf);
    cmd.remote = sc_buf_get_str(buf);

    return cmd;
}

void cmd_encode_client_disconnect(struct sc_buf *buf, const char *name,
                                  bool clean)
{
    sc_buf_put_str(buf, name);
    sc_buf_put_bool(buf, clean);
}

struct cmd_client_disconnect cmd_decode_client_disconnect(struct sc_buf *buf)
{
    struct cmd_client_disconnect cmd;

    cmd.name = sc_buf_get_str(buf);
    cmd.clean = sc_buf_get_bool(buf);

    return cmd;
}

void cmd_encode_timestamp(struct sc_buf *buf)
{
    sc_buf_put_64(buf, sc_time_ms());
    sc_buf_put_64(buf, sc_time_mono_ms());
}

struct cmd_timestamp cmd_decode_timestamp(struct sc_buf *buf)
{
    struct cmd_timestamp time;

    time.realtime = sc_buf_get_64(buf);
    time.monotonic = sc_buf_get_64(buf);

    return time;
}

void cmd_encode_log(struct sc_buf *buf, const char *level, const char *log)
{
    sc_buf_put_str(buf, level);
    sc_buf_put_str(buf, log);
}

struct cmd_log cmd_decode_log(struct sc_buf *buf)
{
    struct cmd_log log;

    log.level = sc_buf_get_str(buf);
    log.log = sc_buf_get_str(buf);

    return log;
}
