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
