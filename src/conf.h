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
        uint64_t heartbeat;
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
