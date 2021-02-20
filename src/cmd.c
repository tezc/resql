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


#include "cmd.h"
#include "meta.h"

#include "sc/sc_str.h"
#include "sc/sc_time.h"

void cmd_encode_init(struct sc_buf *buf, char rand[256])
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

struct cmd_start_term cmd_decode_term_start(struct sc_buf *buf)
{
    struct cmd_start_term start;

    start.realtime = sc_buf_get_64(buf);
    start.monotonic = sc_buf_get_64(buf);

    return start;
}

void cmd_encode_meta(struct sc_buf *buf, struct meta *meta)
{
    meta_encode(meta, buf);
}

void cmd_decode_meta_to(void *data, int len, struct meta *meta)
{
    struct sc_buf tmp = sc_buf_wrap(data, len, SC_BUF_READ);
    meta_decode(meta, &tmp);
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
