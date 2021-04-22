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

#ifndef RESQL_SNAPSHOT_H
#define RESQL_SNAPSHOT_H

#include "sc/sc_cond.h"
#include "sc/sc_mmap.h"
#include "sc/sc_sock.h"
#include "sc/sc_thread.h"

#include <limits.h>
#include <stdint.h>

#define SNAPSHOT_MAX_PAGE_COUNT 4

struct server;
struct page;

struct snapshot {
	bool init;
	bool open;
	struct server *server;
	struct sc_mmap map;
	char *path;
	char *tmp_path;
	char *copy_path;

	// Current index and size
	uint64_t index;
	uint64_t term;

	// Latest snapshot
	uint64_t time;
	size_t size;
	uint64_t latest_index;
	uint64_t latest_term;

	// Recv
	uint64_t recv_index;
	uint64_t recv_term;
	char *recv_path;
	struct file *tmp;

	struct sc_thread thread;
	struct sc_sock_pipe efd;
	struct sc_cond cond;
	_Atomic bool running;
};

int snapshot_init(struct snapshot *ss, struct server *server);
int snapshot_term(struct snapshot *ss);

int snapshot_open(struct snapshot *ss, const char *path, uint64_t term,
		  uint64_t index);
bool snapshot_running(struct snapshot *ss);
int snapshot_wait(struct snapshot *ss);

int snapshot_replace(struct snapshot *ss);

int snapshot_take(struct snapshot *ss, struct page *page);
int snapshot_recv(struct snapshot *ss, uint64_t term, uint64_t index, bool done,
		  uint64_t offset, void *data, uint64_t len);
void snapshot_clear(struct snapshot *ss);

#endif
