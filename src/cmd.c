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

void cmd_encode_init(struct sc_buf *b, unsigned char rand[256])
{
	sc_buf_put_64(b, sc_time_ms());
	sc_buf_put_64(b, sc_time_mono_ms());
	sc_buf_put_raw(b, rand, 256);
}

struct cmd_init cmd_decode_init(struct sc_buf *b)
{
	struct cmd_init init;

	init.realtime = sc_buf_get_64(b);
	init.monotonic = sc_buf_get_64(b);
	sc_buf_get_data(b, init.rand, sizeof(init.rand));

	return init;
}

void cmd_encode_term(struct sc_buf *b)
{
	sc_buf_put_64(b, sc_time_ms());
	sc_buf_put_64(b, sc_time_mono_ms());
}

struct cmd_term cmd_decode_term(struct sc_buf *b)
{
	struct cmd_term start;

	start.realtime = sc_buf_get_64(b);
	start.monotonic = sc_buf_get_64(b);

	return start;
}

void cmd_encode_meta(struct sc_buf *b, struct meta *meta)
{
	meta_encode(meta, b);
}

struct cmd_meta cmd_decode_meta(struct sc_buf *b)
{
	struct cmd_meta cmd;

	meta_init(&cmd.meta, "");
	meta_decode(&cmd.meta, b);

	return cmd;
}

void cmd_encode_connect(struct sc_buf *b, const char *name,
			       const char *local, const char *remote)
{
	sc_buf_put_str(b, name);
	sc_buf_put_str(b, local);
	sc_buf_put_str(b, remote);
}

struct cmd_connect cmd_decode_connect(struct sc_buf *buf)
{
	struct cmd_connect cmd;

	cmd.name = sc_buf_get_str(buf);
	cmd.local = sc_buf_get_str(buf);
	cmd.remote = sc_buf_get_str(buf);

	return cmd;
}

void cmd_encode_disconnect(struct sc_buf *b, const char *name, bool clean)
{
	sc_buf_put_str(b, name);
	sc_buf_put_bool(b, clean);
}

struct cmd_disconnect cmd_decode_disconnect(struct sc_buf *b)
{
	struct cmd_disconnect cmd;

	cmd.name = sc_buf_get_str(b);
	cmd.clean = sc_buf_get_bool(b);

	return cmd;
}

void cmd_encode_timestamp(struct sc_buf *b)
{
	sc_buf_put_64(b, sc_time_ms());
	sc_buf_put_64(b, sc_time_mono_ms());
}

struct cmd_timestamp cmd_decode_timestamp(struct sc_buf *b)
{
	struct cmd_timestamp time;

	time.realtime = sc_buf_get_64(b);
	time.monotonic = sc_buf_get_64(b);

	return time;
}

void cmd_encode_log(struct sc_buf *b, const char *level, const char *log)
{
	sc_buf_put_str(b, level);
	sc_buf_put_str(b, log);
}

struct cmd_log cmd_decode_log(struct sc_buf *b)
{
	struct cmd_log log;

	log.level = sc_buf_get_str(b);
	log.log = sc_buf_get_str(b);

	return log;
}
