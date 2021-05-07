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

#include "snapshot.h"

#include "file.h"
#include "page.h"
#include "rs.h"
#include "server.h"

#include "sc/sc_log.h"
#include "sc/sc_str.h"
#include "sc/sc_time.h"

#include <errno.h>
#include <inttypes.h>

#define SS_FILE	     "snapshot.resql"
#define SS_TMP_FILE  "snapshot.tmp.resql"
#define SS_RECV_FILE "snapshot.tmp.recv.resql"
#define SS_COPY_FILE "snapshot.copy.resql"

struct snapshot_task {
	struct page *page;
	bool stop;
};

static void *snapshot_run(void *arg);

int snapshot_init(struct snapshot *ss, struct server *srv)
{
	int rc;
	const char *dir = srv->conf.node.dir;

	*ss = (struct snapshot){0};

	sc_thread_init(&ss->thread);

	rc = sc_sock_pipe_init(&ss->efd, SERVER_FD_TASK);
	if (rc != 0) {
		sc_log_error("pipe : %s \n", sc_sock_pipe_err(&ss->efd));
		return RS_ERROR;
	}

	rc = sc_cond_init(&ss->cond);
	if (rc != 0) {
		sc_log_error("cond : %s \n", strerror(errno));
		goto cleanup_pipe;
	}

	ss->path = sc_str_create_fmt("%s/%s", dir, SS_FILE);
	ss->tmp_path = sc_str_create_fmt("%s/%s", dir, SS_TMP_FILE);
	ss->recv_path = sc_str_create_fmt("%s/%s", dir, SS_RECV_FILE);
	ss->copy_path = sc_str_create_fmt("%s/%s", dir, SS_COPY_FILE);

	ss->server = srv;
	ss->tmp = NULL;
	ss->recv_index = 0;
	ss->recv_term = 0;

	ss->time = 0;
	ss->size = 0;
	ss->running = false;

	rc = sc_thread_start(&ss->thread, snapshot_run, ss);
	if (rc != 0) {
		sc_log_error("thread : %s \n", sc_thread_err(&ss->thread));
		goto cleanup_cond;
	}

	ss->init = true;

	return RS_OK;

cleanup_cond:
	sc_cond_term(&ss->cond);
	sc_str_destroy(&ss->path);
	sc_str_destroy(&ss->tmp_path);
	sc_str_destroy(&ss->recv_path);
	sc_str_destroy(&ss->copy_path);
cleanup_pipe:
	sc_sock_pipe_term(&ss->efd);

	return RS_ERROR;
}

int snapshot_term(struct snapshot *ss)
{
	int rc, ret = RS_OK;
	struct snapshot_task task = {.stop = true};

	snapshot_clear(ss);

	if (!ss->init) {
		return RS_OK;
	}

	rc = sc_sock_pipe_write(&ss->efd, &task, sizeof(task));
	if (rc != sizeof(task)) {
		ret = RS_ERROR;
		sc_log_error("pipe : %s \n", strerror(errno));
	}

	rc = sc_thread_term(&ss->thread);
	if (rc != 0) {
		ret = RS_ERROR;
		sc_log_error("thread : %s \n", sc_thread_err(&ss->thread));
	}

	rc = sc_sock_pipe_term(&ss->efd);
	if (rc != 0) {
		ret = RS_ERROR;
		sc_log_error("pipe : %s \n", sc_sock_pipe_err(&ss->efd));
	}

	rc = sc_cond_term(&ss->cond);
	if (rc != 0) {
		ret = RS_ERROR;
		sc_log_error("cond : %s \n", strerror(errno));
	}

	if (ss->open) {
		ss->open = false;
		rc = sc_mmap_term(&ss->map);
		if (rc != 0) {
			ret = RS_ERROR;
			sc_log_error("mmap : %s \n", sc_mmap_err(&ss->map));
		}
	}

	sc_str_destroy(&ss->path);
	sc_str_destroy(&ss->tmp_path);
	sc_str_destroy(&ss->recv_path);
	sc_str_destroy(&ss->copy_path);

	ss->init = false;

	return ret;
}

int snapshot_open(struct snapshot *ss, const char *path, uint64_t term,
		  uint64_t index)
{
	int rc;
	struct sc_mmap *m = &ss->map;

	rc = sc_mmap_init(m, path, O_RDONLY, PROT_READ, MAP_SHARED, 0, 0);
	if (rc != 0) {
		sc_log_error("mmap init : %s \n", sc_mmap_err(m));
		return RS_ERROR;
	}

	ss->term = term;
	ss->index = index;
	ss->open = true;

	return RS_OK;
}

int snapshot_close(struct snapshot *ss)
{
	int rc;
	struct sc_mmap *m = &ss->map;

	rc = sc_mmap_term(m);
	if (rc != 0) {
		sc_log_error("mmap term : %s \n", sc_mmap_err(m));
		return RS_ERROR;
	}

	return RS_OK;
}

bool snapshot_running(struct snapshot *ss)
{
	return ss->running;
}

int snapshot_wait(struct snapshot *ss)
{
	return (int) (uintptr_t) sc_cond_wait(&ss->cond);
}

int snapshot_replace(struct snapshot *ss)
{
	int rc;

	rc = snapshot_close(ss);
	if (rc != RS_OK) {
		return rc;
	}

	rc = snapshot_open(ss, ss->path, ss->latest_term, ss->latest_index);
	if (rc != RS_OK) {
		sc_log_error("snapshot replace failed. \n");
	}

	return rc;
}

int snapshot_recv(struct snapshot *ss, uint64_t term, uint64_t index, bool done,
		  uint64_t offset, void *data, uint64_t len)
{
	int rc;

	if (ss->recv_term != term || ss->recv_index != index) {
		snapshot_clear(ss);
		ss->recv_term = term;
		ss->recv_index = index;
	}

	if (ss->tmp == NULL) {
		ss->tmp = file_create();
		rc = file_open(ss->tmp, ss->recv_path, "w+");
		if (rc != RS_OK) {
			return rc;
		}
	}

	rc = file_write_at(ss->tmp, offset, data, len);
	if (rc != RS_OK) {
		return rc;
	}

	if (done) {
		rc = file_flush(ss->tmp);
		if (rc != RS_OK) {
			return rc;
		}

		file_destroy(ss->tmp);
		ss->tmp = NULL;

		rc = file_rename(ss->recv_path, ss->path);
		if (rc != RS_OK) {
			return rc;
		}

		ss->term = 0;
		ss->index = 0;

		snapshot_close(ss);

		return RS_SNAPSHOT;
	}

	return RS_OK;
}

void snapshot_clear(struct snapshot *ss)
{
	if (ss->tmp != NULL) {
		file_close(ss->tmp);
		file_remove(ss->tmp);
		file_destroy(ss->tmp);

		ss->tmp = NULL;
		ss->recv_index = 0;
		ss->recv_term = 0;
	}
}

int snapshot_take(struct snapshot *ss, struct page *page)
{
	int rc;

	struct snapshot_task task = {
		.page = page,
		.stop = false,
	};

	rc = sc_sock_pipe_write(&ss->efd, &task, sizeof(task));
	if (rc != sizeof(task)) {
		sc_log_error("pipe_write : %s \n", sc_sock_pipe_err(&ss->efd));
		return RS_ERROR;
	}

	return RS_OK;
}

static void snapshot_compact(struct snapshot *ss, struct page *page)
{
	int rc;
	uint64_t first, last, start;
	struct state state;
	struct session *s;

	ss->running = true;
	start = sc_time_mono_ns();

	first = page->prev_index + 1;
	last = page_last_index(page);

	state_init(&state, (struct state_cb){0}, ss->server->conf.node.dir, "");
	rc = state_read_for_snapshot(&state);
	if (rc != RS_OK) {
		goto error;
	}

	for (uint64_t j = first; j <= last; j++) {
		rc = state_apply(&state, j, page_entry_at(page, j), &s);
		if (rc != RS_OK) {
			goto error;
		}
	}

	state_close(&state);
	file_remove_path(state.ss_path);

	rc = rename(state.ss_tmp_path, state.ss_path);
	if (rc != 0) {
		rs_abort("snapshot");
	}

	ss->latest_term = state.term;
	ss->latest_index = state.index;
	ss->time = (sc_time_mono_ns() - start);
	ss->size = (size_t) file_size_at(state.ss_path);

	state_term(&state);
	sc_cond_signal(&ss->cond, (void *) (uintptr_t) RS_OK);
	ss->running = false;

	sc_log_info("snapshot done in : %" PRIu64 " milliseconds, for [%" PRIu64
		    ",%" PRIu64 "] \n",
		    ss->time / 1000 / 1000, first, last);
	return;

error:
	state_term(&state);
	sc_cond_signal(&ss->cond, (void *) (uintptr_t) rc);
	ss->running = false;
	sc_log_info("snapshot failure in : %" PRIu64
		    " milliseconds, for [%" PRIu64 ",%" PRIu64 "] \n",
		    ss->time / 1000 / 1000, first, last);
}

static void *snapshot_run(void *arg)
{
	int size;
	char buf[128];
	struct snapshot *ss = arg;
	struct snapshot_task task;
	const char *node = ss->server->conf.node.name;

	rs_snprintf(buf, sizeof(buf), "%s-%s", node, "snapshot");
	sc_log_set_thread_name(buf);

	sc_log_info("Snapshot thread has been started. \n");

	while (true) {
		size = sc_sock_pipe_read(&ss->efd, &task, sizeof(task));
		if (size != sizeof(task)) {
			rs_abort("snapshot");
		}

		if (task.stop) {
			sc_log_info("Snapshot thread is shutting down. \n");
			return (void *) RS_OK;
		}

		snapshot_compact(ss, task.page);
	}
}
