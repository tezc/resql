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

#ifndef RESQL_SERVER_H
#define RESQL_SERVER_H

#include "conf.h"
#include "metric.h"
#include "snapshot.h"
#include "state.h"
#include "store.h"

#include "sc/sc_list.h"
#include "sc/sc_queue.h"
#include "sc/sc_sock.h"
#include "sc/sc_timer.h"

sc_array_def(struct server_endpoint, endp);
sc_queue_def(struct server_job, jobs);
sc_queue_def(struct sc_buf, bufs);

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
	SERVER_TIMER_FULL
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
	char *data;
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
	struct sc_rand rand;
	struct sc_buf buf;
	struct sc_map_sv clients;
	struct sc_map_64v vclients;
	struct sc_list pending_conns;
	struct sc_list connected_nodes;
	struct sc_list read_reqs;
	struct sc_buf tmp;
	struct sc_queue_jobs jobs;
	struct sc_queue_bufs cache;
	struct meta meta;
	struct store store;
	struct state state;
	struct snapshot ss;
	struct sc_array_endp endpoints;
	struct sc_array_ptr nodes;
	struct sc_array_ptr unknown_nodes;
	struct sc_array_ptr term_clients;
	struct node *leader;
	struct node *own;

	bool ss_inprogress;
	bool stop_requested;
	bool cluster_up;
	bool full;
	bool in_cluster;
	int timer_rc;

	// Cluster management
	char *meta_path;
	char *meta_tmp_path;
	char *voted_for;

	enum server_role role;
	unsigned int prevote_count;
	unsigned int vote_count;
	uint64_t vote_timestamp;
	uint64_t prevote_term;
	uint64_t round_prev;
	uint64_t round_match;
	uint64_t round;
	uint64_t commit;
	uint64_t timestamp;
	uint64_t election_timer;
	uint64_t info_timer;
	uint64_t full_timer;

	uint64_t last_ts;
	uint64_t last_quorum;
};

struct server *server_start(struct conf *c);
int server_start_now(struct conf *c);
int server_stop(struct server *s);

struct sc_buf server_buf_alloc(struct server *s);
void server_buf_free(struct server *s, struct sc_buf buf);

#endif
