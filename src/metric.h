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

#ifndef RESQL_METRIC_H
#define RESQL_METRIC_H

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/utsname.h>

struct sc_buf;

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
    size_t ss_size;
    bool ss_success;

    char dir[PATH_MAX];
};


void metric_init(struct metric *m, const char *dir);
void metric_term(struct metric *m);
int metric_encode(struct metric *m, struct sc_buf *buf);
void metric_recv(int64_t val);
void metric_send(int64_t val);
void metric_fsync(uint64_t val);
void metric_snapshot(bool success, uint64_t time, size_t size);

#endif
