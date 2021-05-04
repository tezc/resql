/*
 * BSD-3-Clause
 *
 * Copyright 2021 Ozan Tezcan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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

void cmd_encode_connect(struct sc_buf *b, const char *name, const char *local,
			const char *remote)
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
