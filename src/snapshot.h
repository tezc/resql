/*
 * BSD-3-Clause
 *
 * Copyright 2021 Ozan Tezcan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
