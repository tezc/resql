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


#ifndef RESQL_CONF_H
#define RESQL_CONF_H

#include <stdbool.h>
#include <stdint.h>

struct conf
{
    struct
    {
        char *name;
        char *bind_url;
        char *ad_url;
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
        bool fsync;
        int64_t heartbeat;
    } advanced;

    struct
    {
        bool systemd;
        bool empty;
        bool wipe;
        char *config_file;
    } cmdline;

    char err[128];
};

void conf_init(struct conf *c);
void conf_term(struct conf *c);

void conf_read_config(struct conf *c, bool read_file, int argc, char **argv);
void conf_print(struct conf *c);


#endif
