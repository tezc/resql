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

#include "info.h"
#include "rs.h"

#include "sc/sc_str.h"

struct info *info_create(const char* name)
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

void info_set_role(struct info *info, const char* role)
{
    sc_str_set(&info->role, role);
}

void info_set_urls(struct info *info, const char* urls)
{
    sc_str_set(&info->urls, urls);
}

void info_set_stats(struct info *info, struct sc_buf* stats)
{
    sc_buf_clear(&info->stats);
    sc_buf_put_raw(&info->stats, sc_buf_rbuf(stats), sc_buf_size(stats));
}
