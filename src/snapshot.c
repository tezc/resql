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


struct snapshot_task
{
    struct page *page;
    bool stop;
};

static void *snapshot_run(void *arg);


void snapshot_init(struct snapshot *ss, struct server *server)
{
    int rc;

    ss->server = server;
    ss->path =
            sc_str_create_fmt("%s/%s", server->conf.node.dir, "snapshot.resql");
    ss->tmp_path = sc_str_create_fmt("%s/%s", server->conf.node.dir,
                                     "snapshot.tmp.resql");
    ss->tmp_recv_path = sc_str_create_fmt("%s/%s", server->conf.node.dir,
                                          "snapshot.tmp.recv.resql");
    ss->tmp = NULL;
    ss->recv_index = 0;
    ss->recv_term = 0;

    ss->time = 0;
    ss->size = 0;

    sc_thread_init(&ss->thread);
    sc_sock_pipe_init(&ss->efd, SERVER_FD_TASK);
    sc_cond_init(&ss->cond);

    rc = sc_thread_start(&ss->thread, snapshot_run, ss);
    if (rc != RS_OK) {
        rs_abort("snapshot : %s \n", ss->thread.err);
    }
}

void snapshot_term(struct snapshot *ss)
{
    int rc;
    struct snapshot_task task = {.stop = true};

    sc_sock_pipe_write(&ss->efd, &task, sizeof(task));

    rc = sc_thread_term(&ss->thread);
    if (rc != RS_OK) {
        rs_abort("snapshot");
    }

    sc_sock_pipe_term(&ss->efd);
    sc_thread_term(&ss->thread);
    sc_cond_term(&ss->cond);
    sc_str_destroy(ss->path);
    sc_str_destroy(ss->tmp_path);
    sc_str_destroy(ss->tmp_recv_path);
    sc_mmap_term(&ss->map);
}

void snapshot_open(struct snapshot *ss, const char *path, uint64_t term,
                   uint64_t index)
{
    int rc;

    rc = sc_mmap_init(&ss->map, path, O_RDONLY, PROT_READ, MAP_SHARED, 0, 0);
    if (rc != 0) {
        rs_abort("snapshot");
    }

    ss->term = term;
    ss->index = index;
}

void snapshot_close(struct snapshot *ss)
{
    int rc;

    rc = sc_mmap_term(&ss->map);
    if (rc != 0) {
        sc_log_error("mmap term : %s \n", sc_mmap_err(&ss->map));
    }
}

void snapshot_replace(struct snapshot *ss)
{
    snapshot_close(ss);
    snapshot_open(ss, ss->path, ss->latest_term, ss->latest_index);
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
        rc = file_open(ss->tmp, ss->tmp_recv_path, "w+");
        if (rc == RS_ERROR) {
            rs_abort("snapshot");
        }
    }

    rc = file_write_at(ss->tmp, offset, data, len);
    if (rc == RS_ERROR) {
        rs_abort("snapshot");
    }

    if (done) {
        file_close(ss->tmp);

        rc = rename(ss->tmp_recv_path, ss->path);
        if (rc != 0) {
            rs_abort("rename : %s \n", strerror(errno));
        }

        ss->term = 0;
        ss->index = 0;

        snapshot_close(ss);

        return RS_DONE;
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

void snapshot_take(struct snapshot *ss, struct page *page)
{
    struct snapshot_task task = {.page = page, .stop = false};

    sc_sock_pipe_write(&ss->efd, &task, sizeof(task));
}

static void snapshot_compact(struct snapshot *ss, struct page *page)
{
    int rc;
    uint64_t first, last;
    struct state state;

    uint64_t start = sc_time_mono_ns();

    state_init(&state, (struct state_cb){0}, ss->server->conf.node.dir, "");

    rc = state_read_for_snapshot(&state);
    if (rc != RS_OK) {
        rs_abort("snapshot");
    }

    first = page->prev_index + 1;
    last = page_last_index(page);

    for (uint64_t j = first; j <= last; j++) {
        state_apply(&state, j, page_entry_at(page, j));
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

    sc_log_info("snapshot done in : " PRIu64 " milliseconds, for [" PRIu64
                "," PRIu64 "] \n",
                ss->time / 1000 / 1000, first, last);
}

void snapshot_set_thread_name(struct snapshot *ss)
{
    char buf[128];
    const char *node = ss->server->conf.node.name;

    rs_snprintf(buf, sizeof(buf), "%s-%s", node, "snapshot");
    sc_log_set_thread_name(buf);
}

static void *snapshot_run(void *arg)
{
    int size;
    struct snapshot *ss = arg;
    struct snapshot_task task;

    snapshot_set_thread_name(ss);
    sc_log_info("Snapshot slave started ... \n");

    while (true) {
        size = sc_sock_pipe_read(&ss->efd, &task, sizeof(task));
        if (size != sizeof(task)) {
            rs_abort("snapshot");
        }

        if (task.stop) {
            return (void *) RS_OK;
        }

        snapshot_compact(ss, task.page);
    }
}
