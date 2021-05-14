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

#include "info.h"

#include "rs.h"

#include "sc/sc_str.h"

struct info *info_create(const char *name)
{
	struct info *n;

	n = rs_calloc(1, sizeof(*n));

	n->name = sc_str_create(name);
	sc_buf_init(&n->stats, 1024);

	return n;
}

void info_destroy(struct info *n)
{
	sc_str_destroy(&n->name);
	sc_str_destroy(&n->urls);
	sc_str_destroy(&n->connected);
	sc_str_destroy(&n->role);
	sc_buf_term(&n->stats);
	rs_free(n);
}

void info_set_connected(struct info *n, bool connected)
{
	sc_str_set(&n->connected, connected ? "true" : "false");
}

void info_set_role(struct info *n, const char *role)
{
	sc_str_set(&n->role, role);
}

void info_set_urls(struct info *n, const char *urls)
{
	sc_str_set(&n->urls, urls);
}

void info_set_stats(struct info *n, void *data, uint32_t len)
{
	sc_buf_clear(&n->stats);
	sc_buf_put_raw(&n->stats, data, len);
}
