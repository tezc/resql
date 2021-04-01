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
#ifndef RESQL_INFO_H
#define RESQL_INFO_H

#include "sc/sc_buf.h"

struct info
{
    char *name;
    char *urls;
    char *connected;
    char *role;

    struct sc_buf stats;
};

struct info *info_create(const char *name);
void info_destroy(struct info *info);

void info_set_connected(struct info *info, bool connected);
void info_set_role(struct info *info, const char *role);
void info_set_urls(struct info *info, const char *urls);
void info_set_stats(struct info *info, struct sc_buf *buf);


#endif
