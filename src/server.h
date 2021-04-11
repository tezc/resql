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

#ifndef RESQL_SERVER_H
#define RESQL_SERVER_H

#include "conf.h"
#include "metric.h"
#include "snapshot.h"
#include "state.h"
#include "store.h"

#include "sc/sc_list.h"
#include "sc/sc_sock.h"
#include "sc/sc_timer.h"

enum server_role
{
	SERVER_ROLE_FOLLOWER,
	SERVER_ROLE_CANDIDATE,
	SERVER_ROLE_LEADER
};

enum server_timer
{
	SERVER_TIMER_PENDING,
	SERVER_TIMER_CONNECT,
	SERVER_TIMER_ELECTION,
	SERVER_TIMER_INFO,
};

enum server_job_type
{
	SERVER_JOB_ADD_NODE,
	SERVER_JOB_REMOVE_NODE,
	SERVER_JOB_SHUTDOWN
};

enum server_fd_type
{
	SERVER_FD_NODE_RECV,
	SERVER_FD_CLIENT_RECV,
	SERVER_FD_INCOMING_CONN,
	SERVER_FD_OUTGOING_CONN,
	SERVER_FD_WAIT_FIRST_REQ,
	SERVER_FD_WAIT_FIRST_RESP,
	SERVER_FD_TASK,
	SERVER_FD_SIGNAL
};

struct server_job {
	enum server_job_type type;
	void *data;
};

struct server_endpoint {
	struct sc_uri *uri;
	struct sc_sock *sock;
};

struct server {
	struct sc_thread thread;
	struct conf conf;
	struct metric metric;
	struct sc_sock_poll poll;
	struct sc_sock_pipe efd;
	struct sc_sock_pipe sigfd;
	struct sc_timer timer;
	struct server_endpoint *endpoints;
	struct sc_list pending_conns;
	struct sc_buf buf;
	struct sc_map_sv clients;
	struct sc_map_64v vclients;
	struct node **nodes;
	struct sc_list connected_nodes;
	struct node **unknown_nodes;
	struct client **term_clients;
	uint64_t election_timer;
	uint64_t info_timer;
	uint64_t last_ts;

	bool ss_inprogress;
	bool stop_requested;
	bool cluster_up;

	char *passwd;

	// Cluster management
	char *meta_path;
	char *meta_tmp_path;

	char *voted_for;
	uint64_t vote_timestamp;
	struct meta meta;

	struct store store;
	struct state state;
	struct snapshot ss;

	struct node *leader;
	struct node *own;
	enum server_role role;

	uint64_t timestamp;
	uint64_t prevote_term;
	unsigned int prevote_count;
	unsigned int vote_count;
	uint64_t round_match;
	uint64_t round_prev;
	uint64_t round;
	uint64_t commit;
	struct sc_list read_reqs;
	struct sc_buf tmp;
	struct server_job *jobs;
	struct sc_buf *cache;
};

void server_global_init();
void server_global_shutdown();

struct server *server_create(struct conf *conf);
void server_destroy(struct server *server);

int server_start(struct server *server, bool new_thread);
int server_stop(struct server *server);

struct sc_buf server_buf_alloc(struct server *s);
void server_buf_free(struct server *s, struct sc_buf buf);

#endif
