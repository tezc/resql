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


#ifndef RESQL_SERVER_H
#define RESQL_SERVER_H

#include "conf.h"
#include "metric.h"
#include "msg.h"
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
    SERVER_JOB_DISCONNECT_CLIENT,
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

struct server_job
{
    enum server_job_type type;
    void* data;
};

struct server_endpoint
{
    struct sc_uri *uri;
    struct sc_sock *sock;
};

struct server
{
    struct sc_thread thread;
    struct conf conf;
    struct metric metric;
    struct sc_sock_poll loop;
    struct sc_sock_pipe efd;
    struct sc_sock_pipe sigfd;
    struct sc_timer timer;
    struct server_endpoint *endpoints;
    struct sc_list pending_conns;
    struct sc_buf buf;
    struct sc_map_sv clients;
    struct sc_map_64v vclients;
    struct node **nodes;
    struct node **connected_nodes;
    struct node **unknown_nodes;
    struct client **term_clients;
    uint64_t election_timer;
    uint64_t info_timer;
    uint64_t last_ts;

    bool ss_inprogress;
    bool stop_requested;
    bool cluster_up;
    bool conf_change;

    char* passwd;

    // Cluster managemenent
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
    uint64_t round_prev_index;
    uint64_t round_index;
    uint64_t commit_index;
    struct sc_list read_reqs;
    struct sc_buf tmp;
    struct server_job *jobs;
};

void server_global_init();
void server_global_shutdown();

struct server *server_create(struct conf *conf);
void server_destroy(struct server *server);

int server_start(struct server *server, bool new_thread);
int server_stop(struct server *server);

#endif
