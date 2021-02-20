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


#ifndef RESQL_SETTINGS_H
#define RESQL_SETTINGS_H

#include <stdbool.h>
#include <stdint.h>

struct settings
{
    struct
    {
        char *name;
        char *bind_uri;
        char *ad_uri;
        char *source_addr;
        char *source_port;
        char *log_level;
        char *log_dest;
        char *dir;
        bool in_memory;
    } node;

    struct
    {
        char *name;
        char *nodes;
    } cluster;

    struct
    {
        bool empty;
        char *settings_file;
    } cmdline;

    char err[128];
};

void settings_init(struct settings *c);
void settings_term(struct settings *c);

void settings_read_cmdline(struct settings *c, int argc, char *argv[]);
void settings_read_file(struct settings *c, const char *path);
void settings_print(struct settings *c);


#endif
