/*
 *  reSQL
 *
 *  Copyright (C) 2021 reSQL Authors
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

#ifndef RESQL_LIB_METRIC_H
#define RESQL_LIB_METRIC_H

#include "sc/sc_buf.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/utsname.h>

struct metric
{
    uint64_t total_memory;
    struct utsname utsname;
    int64_t pid;
    uint64_t start_time;
    char start[64];
    uint64_t bytes_recv;
    uint64_t bytes_sent;

    uint64_t fsync_total;
    uint64_t fsync_count;
    uint64_t fsync_max;

    uint64_t ss_total;
    uint64_t ss_count;
    uint64_t ss_max;
    uint64_t ss_size;
    bool ss_success;

    char dir[PATH_MAX];
};



void metric_init(struct metric *m, const char *dir);
void metric_term(struct metric *m);
int metric_encode(struct metric *m, struct sc_buf *buf);
void metric_recv(uint64_t val);
void metric_send(uint64_t val);
void metric_fsync(uint64_t val);
void metric_snapshot(bool success, uint64_t time, uint64_t size);

#endif
