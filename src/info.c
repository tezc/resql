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
    struct info *info;

    info = rs_calloc(1, sizeof(*info));

    info->name = sc_str_create(name);
    sc_buf_init(&info->stats, 1024);

    return info;
}

void info_destroy(struct info *info)
{
    sc_str_destroy(info->name);
    sc_str_destroy(info->urls);
    sc_str_destroy(info->connected);
    sc_str_destroy(info->role);
    sc_buf_term(&info->stats);
    rs_free(info);
}

void info_set_connected(struct info *info, bool connected)
{
    sc_str_set(&info->connected, connected ? "true" : "false");
}

void info_set_role(struct info *info, const char *role)
{
    sc_str_set(&info->role, role);
}

void info_set_urls(struct info *info, const char *urls)
{
    sc_str_set(&info->urls, urls);
}

void info_set_stats(struct info *info, struct sc_buf *stats)
{
    sc_buf_clear(&info->stats);
    sc_buf_put_raw(&info->stats, sc_buf_rbuf(stats), sc_buf_size(stats));
}
