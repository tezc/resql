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

#include "client.h"
#include "cmd.h"
#include "conf.h"
#include "conn.h"
#include "file.h"
#include "metric.h"
#include "msg.h"
#include "node.h"
#include "rs.h"
#include "server.h"
#include "session.h"
#include "state.h"

#include "sc/sc.h"
#include "sc/sc_array.h"
#include "sc/sc_buf.h"
#include "sc/sc_crc32.h"
#include "sc/sc_log.h"
#include "sc/sc_signal.h"
#include "sc/sc_thread.h"
#include "sc/sc_time.h"
#include "sc/sc_timer.h"
#include "sc/sc_uri.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

#define BATCH_SIZE    (SC_SOCK_BUF_SIZE - 128)
#define DEF_META_FILE "meta.resql"
#define DEF_META_TMP  "meta.tmp.resql"

const char *server_add_node(void *arg, const char *node);
const char *server_remove_node(void *arg, const char *node);
const char *server_shutdown(void *arg, const char *node);

void server_global_init()
{
    srand(sc_time_mono_ns());
    state_global_init();
    sc_signal_init();
    sc_crc32_init();
    sc_log_init();
    sc_sock_startup();
}

void server_global_shutdown()
{
    sc_sock_cleanup();
    state_global_shutdown();
}

struct server *server_create(struct conf *conf)
{
    int rc;
    struct server *s;

    if (conf->cmdline.wipe) {
        rc = file_clear_dir(conf->node.dir, RS_STORE_EXTENSION);
        if (rc != RS_OK) {
            rs_abort("file_clear_dir failed. \n");
        }

        sc_log_info("Cleared database. \n");
        exit(EXIT_SUCCESS);
    }

    rc = sc_log_set_level(conf->node.log_level);
    if (rc != 0) {
        rs_exit("Invalid log level : '%s' \n", conf->node.log_level);
    }

    rc = strcasecmp(conf->node.log_dest, "stdout");
    if (rc != 0) {
        sc_log_set_stdout(false);
        sc_log_set_file("log.prev.txt", "log.current.txt");
    }

    s = rs_calloc(1, sizeof(*s));
    s->conf = *conf;

    rc = file_mkdir(conf->node.dir);
    if (rc != RS_OK) {
        rs_exit("Failed to create dir : '%s' \n", conf->node.dir);
    }

    rs_write_pid_file(conf->node.dir);
    sc_thread_init(&s->thread);

    meta_init(&s->meta, s->conf.cluster.name);
    sc_sock_poll_init(&s->loop);
    sc_sock_pipe_init(&s->efd, SERVER_FD_TASK);
    sc_sock_poll_add(&s->loop, &s->efd.fdt, SC_SOCK_READ, &s->efd.fdt);

    sc_sock_pipe_init(&s->sigfd, SERVER_FD_SIGNAL);
    sc_signal_shutdown_fd = s->sigfd.fds[1];
    sc_sock_poll_add(&s->loop, &s->sigfd.fdt, SC_SOCK_READ, &s->sigfd.fdt);
    sc_buf_init(&s->tmp, 1024);
    sc_timer_init(&s->timer, sc_time_mono_ms());
    sc_array_create(s->endpoints, 2);
    sc_list_init(&s->pending_conns);
    sc_map_init_sv(&s->clients, 32, 0);
    sc_map_init_64v(&s->vclients, 32, 0);
    sc_array_create(s->term_clients, 4);

    sc_array_create(s->nodes, 4);
    sc_list_init(&s->connected_nodes);
    sc_array_create(s->unknown_nodes, 4);
    sc_queue_create(s->jobs, 4);
    sc_list_init(&s->read_reqs);

    s->stop_requested = false;
    s->ss_inprogress = false;
    s->commit = 0;
    s->round = 0;
    s->round_prev = 0;
    s->round_match = 0;
    s->voted_for = NULL;

    return s;
}

void server_destroy(struct server *s)
{
    int rc;
    struct node *node;
    struct client *client;
    struct server_endpoint endp;
    struct server_job job;

    snapshot_term(&s->ss);

    sc_str_destroy(s->voted_for);
    sc_str_destroy(s->passwd);

    sc_array_foreach (s->nodes, node) {
        node_destroy(node);
    }
    sc_array_destroy(s->nodes);
    sc_array_destroy(s->unknown_nodes);

    state_term(&s->state);
    store_term(&s->store);
    meta_term(&s->meta);

    sc_sock_poll_term(&s->loop);
    sc_timer_term(&s->timer);

    sc_map_foreach_value (&s->clients, client) {
        client_destroy(client);
    }
    sc_map_term_sv(&s->clients);
    sc_map_term_64v(&s->vclients);

    sc_array_foreach (s->endpoints, endp) {
        if (strcmp(endp.uri->scheme, "unix") == 0) {
            rc = unlink(endp.uri->path);
            if (rc != 0) {
                sc_log_error("Failed to unlink : %s \n", endp.uri->path);
            }
        }

        sc_uri_destroy(endp.uri);
        sc_sock_term(endp.sock);
        rs_free(endp.sock);
    }
    sc_array_destroy(s->endpoints);



    sc_queue_foreach (s->jobs, job) {
        rs_free(job.data);
    }
    sc_queue_destroy(s->jobs);

    sc_array_foreach (s->term_clients, client) {
        client_destroy(client);
    }
    sc_array_destroy(s->term_clients);

    sc_thread_term(&s->thread);
    sc_str_destroy(s->meta_path);
    sc_str_destroy(s->meta_tmp_path);
    sc_buf_term(&s->tmp);
    rs_delete_pid_file(s->conf.node.dir);
    conf_term(&s->conf);

    sc_sock_pipe_term(&s->efd);
    sc_sock_pipe_term(&s->sigfd);

    rs_free(s);
}

void server_prepare_cluster(struct server *s)
{
    int rc;
    const char *path = s->conf.node.dir;
    struct state_cb cb = {.arg = s,
                          .add_node = server_add_node,
                          .remove_node = server_remove_node,
                          .shutdown = server_shutdown};

    s->meta_path = sc_str_create_fmt("%s/%s", path, DEF_META_FILE);
    s->meta_tmp_path = sc_str_create_fmt("%s/%s", path, DEF_META_TMP);
    s->cluster_up = false;
    s->role = SERVER_ROLE_FOLLOWER;
    s->leader = NULL;
    s->own = node_create(s->conf.node.name, s, false);

    sc_array_add(s->nodes, s->own);

    state_init(&s->state, cb, path, s->conf.cluster.name);
    snapshot_init(&s->ss, s);

    rc = file_remove_if_exists(s->meta_tmp_path);
    if (rc != RS_OK) {
        rs_abort("remove");
    }

    if (s->conf.cmdline.empty) {
        rc = file_clear_dir(s->conf.node.dir, RS_STORE_EXTENSION);
        if (rc != RS_OK) {
            rs_abort("cleardir");
        }
    }
}

void server_write_meta(struct server *s)
{
    int rc;
    struct file file;

    file_init(&file);

    rc = file_open(&file, s->meta_tmp_path, "w+");
    if (rc != RS_OK) {
        rs_abort("Failed to create file at %s \n", s->meta_tmp_path);
    }

    sc_buf_clear(&s->tmp);
    sc_buf_put_str(&s->tmp, s->conf.node.name);
    sc_buf_put_str(&s->tmp, s->voted_for);
    meta_encode(&s->meta, &s->tmp);

    file_write(&file, sc_buf_rbuf(&s->tmp), sc_buf_size(&s->tmp));
    file_term(&file);

    rc = rename(s->meta_tmp_path, s->meta_path);
    if (rc != 0) {
        rs_abort("rename : %s to %s failed \n", s->meta_tmp_path, s->meta_path);
    }
}

void server_read_meta(struct server *s)
{
    int rc;
    ssize_t size;
    struct file file;

    rc = file_remove_if_exists(s->meta_tmp_path);
    if (rc != RS_OK) {
        rs_abort("remove");
    }

    if (!file_exists_at(s->meta_path)) {
        meta_parse_uris(&s->meta, s->conf.cluster.nodes);
        server_write_meta(s);
    } else {
        file_init(&file);

        rc = file_open(&file, s->meta_path, "r");
        if (rc != RS_OK) {
            sc_log_error("Cannot open meta file at %s, err : %s \n",
                         s->meta_path, strerror(errno));
            file_term(&file);
            rs_abort("open");
        }

        size = file_size(&file);
        sc_buf_clear(&s->tmp);
        sc_buf_reserve(&s->tmp, size);

        file_read(&file, sc_buf_wbuf(&s->tmp), size);
        file_term(&file);

        sc_buf_mark_write(&s->tmp, size);
        sc_str_set(&s->conf.node.name, sc_buf_get_str(&s->tmp));
        sc_str_set(&s->voted_for, sc_buf_get_str(&s->tmp));
        meta_decode(&s->meta, &s->tmp);
    }

    sc_buf_clear(&s->tmp);
    meta_print(&s->meta, &s->tmp);
    sc_log_info(sc_buf_rbuf(&s->tmp));

    state_open(&s->state, s->conf.node.in_memory);
    store_init(&s->store, s->conf.node.dir, s->state.term, s->state.index);
    snapshot_open(&s->ss, s->state.ss_path, s->state.term, s->state.index);

    s->commit = s->state.index;
}

void server_update_meta(struct server *s, uint64_t term, const char *voted_for)
{
    s->meta.term = term;
    sc_str_set(&s->voted_for, voted_for);
    server_write_meta(s);
}

static void server_listen(struct server *s, const char *addr)
{
    int family, rc;
    const char *host;
    struct sc_uri *uri;
    struct sc_sock *sock;
    struct server_endpoint e;

    uri = sc_uri_create(addr);
    if (!uri) {
        rs_exit("Failed to parse bind url : %s \n", addr);
    }

    family = strcmp(uri->scheme, "unix") == 0 ? SC_SOCK_UNIX :
             *uri->host == '['                ? SC_SOCK_INET6 :
                                                SC_SOCK_INET;
    sock = rs_malloc(sizeof(*sock));
    sc_sock_init(sock, SERVER_FD_INCOMING_CONN, false, family);

    host = (family == SC_SOCK_UNIX) ? uri->path : uri->host;
    rc = sc_sock_listen(sock, host, uri->port);
    if (rc != RS_OK) {
        rs_exit("Failed to listen at : %s, reason : %s \n", uri->str,
                sc_sock_error(sock));
    }

    e.uri = uri;
    e.sock = sock;

    sc_array_add(s->endpoints, e);
    sc_sock_poll_add(&s->loop, &sock->fdt, SC_SOCK_READ, &sock->fdt);

    sc_log_info("Listening at : %s \n", uri->str);
}

static void server_on_incoming_conn(struct server *s, struct sc_sock_fd *fd,
                                    uint32_t event)
{
    int rc;
    struct sc_sock *endpoint = rs_entry(fd, struct sc_sock, fdt);
    struct sc_sock in;
    struct conn *conn;

    (void) event;

    rc = sc_sock_accept(endpoint, &in);
    if (rc != SC_SOCK_OK) {
        return;
    }

    conn = conn_create(&s->loop, &s->timer, &in);
    conn_schedule(conn, SERVER_TIMER_PENDING, 50000);

    sc_list_add_tail(&s->pending_conns, &conn->list);
}

static void server_on_task(struct server *s, struct sc_sock_fd *fd, uint32_t ev)
{
    (void) ev;
    (void) fd;

    char c;
    size_t size;

    size = sc_sock_pipe_read(&s->efd, &c, sizeof(c));
    assert(size == 1);

    s->stop_requested = true;
}

static void server_on_signal(struct server *s, struct sc_sock_fd *fd)
{
    (void) fd;

    uint64_t val;

    sc_sock_pipe_read(&s->sigfd, &val, sizeof(val));

    if (s->conf.cmdline.systemd) {
        sc_sock_notify_systemd("STOPPING=1\n");
    }

    sc_log_info("Received shutdown command, shutting down. \n");
    s->stop_requested = true;
}

static void server_on_pending_disconnect(struct server *s, struct conn *in,
                                         enum msg_rc rc)
{
    char str[128];

    sc_sock_print(&in->sock, str, sizeof(str));

    if (rc != MSG_ERR) {
        msg_create_connect_resp(&in->out, rc, 0, s->meta.term, s->meta.uris);
        conn_flush(in);
    }

    sc_log_debug("Pending connection %s disconnected \n", str);
    sc_list_del(&s->pending_conns, &in->list);
    conn_destroy(in);
}

static void server_on_node_disconnect(struct server *s, struct node *node);

static int server_create_entry(struct server *s, bool force, uint64_t seq,
                               uint64_t cid, uint32_t flags, struct sc_buf *buf)
{
    int rc;

retry:
    rc = store_create_entry(&s->store, s->meta.term, seq, cid, flags,
                            sc_buf_rbuf(buf), sc_buf_size(buf));
    if (rc == RS_FULL) {
        if (s->ss_inprogress) {
            s->ss_inprogress = false;

            rc = (int) (uintptr_t) sc_cond_wait(&s->ss.cond);
            if (rc == RS_OK) {
                store_snapshot_taken(&s->store);
                metric_snapshot(true, s->ss.time, s->ss.size);
                snapshot_replace(&s->ss);

                goto retry;
            }

            metric_snapshot(false, 0, 0);
        }

        if (force) {
            store_expand(&s->store);
            goto retry;
        }

        return RS_FULL;
    }

    if (s->timestamp - s->last_ts > 10000) {
        s->last_ts = s->timestamp;

        sc_buf_clear(&s->tmp);
        cmd_encode_timestamp(&s->tmp);
        server_create_entry(s, true, 0, 0, CMD_TIMESTAMP, &s->tmp);
    }


    return RS_OK;
}

static void server_write_init_cmd(struct server *s)
{
    char rand[256];

    file_random(rand, sizeof(rand));

    sc_buf_clear(&s->tmp);
    cmd_encode_init(&s->tmp, rand);

    server_create_entry(s, true, 0, 0, CMD_INIT, &s->tmp);
}

static void server_write_meta_cmd(struct server *s)
{
    struct node *node;
    struct sc_list *elem;

    assert(s->leader == s->own);

    meta_clear_connection(&s->meta);
    meta_set_leader(&s->meta, s->conf.node.name);
    meta_set_connected(&s->meta, s->conf.node.name);

    sc_list_foreach (&s->connected_nodes, elem) {
        node = sc_list_entry(elem, struct node, list);
        meta_set_connected(&s->meta, node->name);
    }

    sc_buf_clear(&s->tmp);
    cmd_encode_meta(&s->tmp, &s->meta);
    server_create_entry(s, true, 0, 0, CMD_META, &s->tmp);
}

static void server_write_term_start_cmd(struct server *s)
{
    sc_buf_clear(&s->tmp);
    cmd_encode_term_start(&s->tmp);
    server_create_entry(s, true, 0, 0, CMD_TERM_START, &s->tmp);
}

static void server_on_node_disconnect(struct server *s, struct node *node)
{
    sc_log_info("Node disconnected : %s \n", node->name);
    node_disconnect(node);

    if (s->leader == node) {
        s->leader = NULL;
    }

    if (s->role == SERVER_ROLE_LEADER) {
        server_write_meta_cmd(s);
    }
}

static void server_on_client_disconnect(struct server *s, struct client *client,
                                        enum msg_rc rc)
{
    if (s->role == SERVER_ROLE_LEADER) {
        sc_buf_clear(&s->tmp);
        cmd_encode_client_disconnect(&s->tmp, client->name, rc == MSG_OK);
        server_create_entry(s, true, 0, 0, CMD_CLIENT_DISCONNECT, &s->tmp);
    }

    client->terminated = true;
    sc_log_debug("Client %s disconnected. \n", client->name);
    sc_map_del_64v(&s->vclients, client->id, NULL);
    sc_map_del_sv(&s->clients, client->name, NULL);
    sc_array_add(s->term_clients, client);
}

static void server_finalize_client_connection(struct server *s,
                                              struct client *client)
{
    int rc;

    conn_allow_read(&client->conn);

    msg_create_connect_resp(&client->conn.out, MSG_OK, client->seq,
                            s->meta.term, s->meta.uris);
    rc = conn_flush(&client->conn);
    if (rc != RS_OK) {
        server_on_client_disconnect(s, client, MSG_ERR);
        return;
    }

    sc_map_put_64v(&s->vclients, client->id, client);
}

static void server_on_client_connect_req(struct server *s, struct conn *in,
                                         struct msg *msg)
{
    bool found;
    struct client *client, *prev;

    if (!s->cluster_up || s->role != SERVER_ROLE_LEADER) {
        server_on_pending_disconnect(s, in, MSG_NOT_LEADER);
        return;
    }

    if (!msg->connect_req.name) {
        server_on_pending_disconnect(s, in, MSG_ERR);
        return;
    }

    found = sc_map_get_sv(&s->clients, msg->connect_req.name, (void **) &prev);
    if (found) {
        if (prev->id == 0) {
            server_on_pending_disconnect(s, in, MSG_ERR);
            return;
        } else {
            server_on_client_disconnect(s, prev, MSG_ERR);
        }
    }

    sc_list_del(&s->pending_conns, &in->list);
    conn_clear_events(in);
    conn_clear_timer(in);
    client = client_create(in, msg->connect_req.name);
    rs_free(in);

    sc_buf_clear(&s->tmp);
    cmd_encode_client_connect(&s->tmp, msg->connect_req.name,
                              client->conn.local, client->conn.remote);

    server_create_entry(s, true, 0, 0, CMD_CLIENT_CONNECT, &s->tmp);
    sc_map_put_sv(&s->clients, client->name, client);
}

static void server_on_node_connect_req(struct server *s, struct conn *pending,
                                       struct msg *msg)
{
    int rc;
    bool found = false;
    const char *name = msg->connect_req.name;
    struct node *n = NULL;

    sc_list_del(&s->pending_conns, &pending->list);
    conn_clear_events(pending);
    conn_clear_timer(pending);

    sc_array_foreach (s->nodes, n) {
        if (strcmp(n->name, name) == 0) {
            sc_list_add_tail(&s->connected_nodes, &n->list);
            found = true;
            break;
        }
    }

    if (!found) {
        n = node_create(name, s, false);
        n->known = false;
        sc_array_add(s->unknown_nodes, n);
    }

    node_set_conn(n, pending);
    rs_free(pending);
    msg_create_connect_resp(&n->conn.out, MSG_OK, 0, s->meta.term,
                            s->meta.uris);
    rc = conn_flush(&n->conn);
    if (rc != RS_OK) {
        server_on_pending_disconnect(s, pending, MSG_ERR);
    }
}

static void server_on_connect_resp(struct server *s, struct sc_sock_fd *fd,
                                   uint32_t event)
{
    (void) event;

    int rc;
    struct sc_sock *sock = rs_entry(fd, struct sc_sock, fdt);
    struct conn *conn = rs_entry(sock, struct conn, sock);
    struct node *node = rs_entry(conn, struct node, conn);
    struct msg msg;

    rc = conn_on_readable(&node->conn);
    if (rc == SC_SOCK_ERROR) {
        goto disconnect;
    }

    rc = msg_parse(&node->conn.in, &msg);
    if (rc == RS_INVALID || rc == RS_ERROR) {
        goto disconnect;
    } else if (rc == RS_PARTIAL) {
        return;
    }

    if (msg.type != MSG_CONNECT_RESP || msg.connect_resp.rc != MSG_OK) {
        goto disconnect;
    }

    conn_set_type(&node->conn, SERVER_FD_NODE_RECV);
    sc_list_add_tail(&s->connected_nodes, &node->list);

    return;

disconnect:
    server_on_node_disconnect(s, node);
}

static void server_on_outgoing_conn(struct server *s, struct sc_sock_fd *fd,
                                    uint32_t event)
{
    (void) event;

    int rc;
    struct sc_sock *sock = rs_entry(fd, struct sc_sock, fdt);
    struct conn *conn = rs_entry(sock, struct conn, sock);
    struct node *node = rs_entry(conn, struct node, conn);

    rc = conn_on_out_connected(&node->conn);
    if (rc != RS_OK) {
        server_on_node_disconnect(s, node);
        return;
    }

    msg_create_connect_req(&node->conn.out, MSG_NODE, s->conf.cluster.name,
                           s->conf.node.name);

    rc = conn_flush(&node->conn);
    if (rc != RS_OK) {
        server_on_node_disconnect(s, node);
    }
}

static void server_schedule_election(struct server *s)
{
    uint32_t t = s->conf.advanced.heartbeat + (rs_rand() % 1024);
    s->election_timer = sc_timer_add(&s->timer, t, SERVER_TIMER_ELECTION, NULL);
}

static void server_become_leader(struct server *s)
{
    struct node *node;

    s->role = SERVER_ROLE_LEADER;
    s->leader = s->own;

    sc_array_foreach (s->nodes, node) {
        node_clear_indexes(node);
    }

    sc_array_foreach (s->unknown_nodes, node) {
        node_clear_indexes(node);
    }

    if (s->store.last_index == 0) {
        server_write_init_cmd(s);
    }

    server_write_meta_cmd(s);
    server_write_term_start_cmd(s);

    sc_buf_clear(&s->tmp);
    meta_print(&s->meta, &s->tmp);
    sc_log_info(sc_buf_rbuf(&s->tmp));
}

static void server_become_follower(struct server *s, struct node *leader)
{
    if (s->role == SERVER_ROLE_FOLLOWER && s->leader == leader) {
        return;
    }

    s->role = SERVER_ROLE_FOLLOWER;
    s->leader = leader;
    snapshot_clear(&s->ss);
    if (leader != NULL) {
        meta_set_leader(&s->meta, leader->name);
    }
}

static void server_check_prevote_count(struct server *s)
{
    int rc;
    struct sc_list *n, *tmp;
    struct node *node;

    if (s->prevote_count >= s->meta.voter / 2 + 1) {
        server_update_meta(s, s->prevote_term, s->conf.node.name);
        s->vote_count = 1;

        sc_list_foreach_safe (&s->connected_nodes, tmp, n) {
            node = sc_list_entry(n, struct node, list);

            msg_create_reqvote_req(&node->conn.out, s->meta.term,
                                   s->store.last_index, s->store.last_term);
            rc = conn_flush(&node->conn);
            if (rc != RS_OK) {
                server_on_node_disconnect(s, node);
            }
        }

        if (s->vote_count >= s->meta.voter / 2 + 1) {
            server_become_leader(s);
        }
    }
}

static void server_on_election_timeout(struct server *s)
{
    int rc;
    size_t connected;
    struct sc_list *n, *tmp;
    struct node *node = NULL;
    uint64_t timeout = s->conf.advanced.heartbeat;

    if (s->leader == s->own) {
        return;
    }

    if (timeout > s->timestamp - s->vote_timestamp) {
        return;
    }

    if (s->leader != NULL && timeout > s->timestamp - s->leader->in_timestamp) {
        return;
    }

    if (!meta_exists(&s->meta, s->conf.node.name)) {
        return;
    }

    connected = sc_list_count(&s->connected_nodes) + 1;

    if (connected < (s->meta.voter / 2) + 1) {
        sc_log_info("Cluster nodes = %lu, "
                    "Connected nodes = %lu, "
                    "No election will take place \n",
                    s->meta.voter, connected);
        return;
    }

    sc_log_info("Starting election for term %lu \n", s->meta.term + 1);

    s->role = SERVER_ROLE_CANDIDATE;
    s->prevote_count = 1;
    s->prevote_term = s->meta.term + 1;

    sc_list_foreach_safe (&s->connected_nodes, tmp, n) {
        node = sc_list_entry(n, struct node, list);
        msg_create_prevote_req(&node->conn.out, s->meta.term + 1,
                               s->store.last_index, s->store.last_term);
        rc = conn_flush(&node->conn);
        if (rc != RS_OK) {
            server_on_node_disconnect(s, node);
        }
    }

    server_check_prevote_count(s);
}

void server_on_timeout(void *arg, uint64_t timeout, uint64_t type, void *data)
{
    (void) timeout;

    struct server *s = arg;

    switch (type) {
    case SERVER_TIMER_PENDING: {
        server_on_pending_disconnect(s, data, MSG_TIMEOUT);
    } break;

    case SERVER_TIMER_CONNECT: {
        int rc;
        struct node *node = data;

        rc = node_try_connect(node);
        if (rc == RS_OK) {
            msg_create_connect_req(&node->conn.out, MSG_NODE,
                                   s->conf.cluster.name, s->conf.node.name);
            rc = conn_flush(&node->conn);
            if (rc != RS_OK) {
                server_on_node_disconnect(s, node);
            }
        }
    } break;

    case SERVER_TIMER_ELECTION: {
        server_on_election_timeout(s);
        server_schedule_election(s);
    } break;

    case SERVER_TIMER_INFO: {
        struct sc_list *l;
        struct node *n;

        sc_buf_clear(&s->own->info);
        metric_encode(&s->metric, &s->own->info);

        if (s->role == SERVER_ROLE_LEADER) {
            sc_buf_clear(&s->tmp);
            sc_buf_put_str(&s->tmp, s->conf.node.name);
            sc_buf_put_blob(&s->tmp, sc_buf_rbuf(&s->own->info),
                            sc_buf_size(&s->own->info));

            sc_list_foreach (&s->connected_nodes, l) {
                n = sc_list_entry(l, struct node, list);
                sc_buf_put_str(&s->tmp, n->name);
                sc_buf_put_blob(&s->tmp, sc_buf_rbuf(&n->info),
                                sc_buf_size(&n->info));
            }

            server_create_entry(s, true, 0, 0, CMD_INFO, &s->tmp);
        } else {
            if (s->leader != NULL) {
                msg_create_info_req(&s->leader->conn.out,
                                    sc_buf_rbuf(&s->own->info),
                                    sc_buf_size(&s->own->info));
                conn_flush(&s->leader->conn);
            }
        }

        s->info_timer = sc_timer_add(&s->timer, 10000, SERVER_TIMER_INFO, NULL);
    } break;

    default:
        break;
    }
}

int server_on_client_req(struct server *s, struct client *c, struct msg *msg)
{
    int rc;
    struct sc_buf buf;
    struct msg_client_req *req = &msg->client_req;

    c->msg_wait = true;

    switch (msg->type) {
    case MSG_CLIENT_REQ:
        if (req->readonly) {
            if (s->round_prev == s->round) {
                s->round++;
            }

            c->round_index = s->round;
            c->commit_index = s->store.last_index;
            sc_list_add_tail(&s->read_reqs, &c->read);
        } else {
            buf = sc_buf_wrap(req->buf, req->len, SC_BUF_READ);
            rc = server_create_entry(s, false, req->seq, c->id,
                                     CMD_CLIENT_REQUEST, &buf);
            if (rc == RS_FULL) {
                server_on_client_disconnect(s, c, MSG_OK);
                return RS_ERROR;
            }
            c->seq = req->seq;
        }
        break;
    case MSG_DISCONNECT_REQ:
        server_on_client_disconnect(s, c, MSG_OK);
    default:
        return RS_ERROR;
    }

    return RS_OK;
}

static void server_on_reqvote_req(struct server *s, struct node *node,
                                  struct msg *msg)
{
    int rc;
    bool grant = false;
    uint64_t last_index = s->store.last_index;
    struct msg_prevote_req *req = &msg->prevote_req;

    if (req->term == s->meta.term && s->voted_for != NULL) {
        goto out;
    }

    if (req->term > s->meta.term && req->last_log_index >= last_index) {
        grant = true;
        server_update_meta(s, req->term, node->name);
    }

out:
    msg_create_reqvote_resp(&node->conn.out, req->term, last_index, grant);

    rc = conn_flush(&node->conn);
    if (rc != RS_OK) {
        server_on_node_disconnect(s, node);
    }
}

static void server_on_reqvote_resp(struct server *s, struct node *node,
                                   struct msg *msg)
{
    struct msg_reqvote_resp *resp = &msg->reqvote_resp;

    if (s->role != SERVER_ROLE_CANDIDATE || s->meta.term != resp->term) {
        sc_log_warn("Unexpected message from %s \n", node->name);
        return;
    }

    if (resp->term > s->meta.term) {
        server_update_meta(s, resp->term, NULL);
        s->prevote_count = 0;
        s->role = SERVER_ROLE_FOLLOWER;
        return;
    }

    if (!resp->granted) {
        return;
    }

    s->vote_count++;
    if (s->vote_count >= s->meta.voter / 2 + 1) {
        server_become_leader(s);
    }
}

static void server_on_prevote_req(struct server *s, struct node *node,
                                  struct msg *msg)
{
    int rc;
    bool result = false;
    uint64_t index = s->store.last_index;
    uint64_t timeout = s->conf.advanced.heartbeat;
    struct msg_prevote_req *req = &msg->prevote_req;

    if (s->leader != NULL && timeout > s->timestamp - s->leader->in_timestamp) {
        goto out;
    }

    if (req->term == s->meta.term && s->voted_for != NULL) {
        goto out;
    }

    if (req->term > s->meta.term && req->last_log_index >= index) {
        result = true;
    }

    s->vote_timestamp = s->timestamp;

out:
    msg_create_prevote_resp(&node->conn.out, req->term, index, result);

    rc = conn_flush(&node->conn);
    if (rc != RS_OK) {
        server_on_node_disconnect(s, node);
    }
}

static void server_on_prevote_resp(struct server *s, struct node *node,
                                   struct msg *msg)
{
    struct msg_prevote_resp *resp = &msg->prevote_resp;

    if (s->role != SERVER_ROLE_CANDIDATE || s->prevote_term != resp->term) {
        sc_log_warn("Unexpected message from %s \n", node->name);
        return;
    }

    if (resp->term > s->prevote_term) {
        server_update_meta(s, resp->term, NULL);
        s->prevote_count = 0;
        s->role = SERVER_ROLE_FOLLOWER;
        return;
    }

    if (!resp->granted) {
        return;
    }

    s->prevote_count++;
    server_check_prevote_count(s);
}

static void server_on_applied_entry(struct server *s, uint64_t index,
                                    char *entry, struct session *session);

void server_on_append_req(struct server *s, struct node *node, struct msg *msg)
{
    int rc;
    char *entry, *curr;
    uint64_t index, prev_term, term;
    struct session *session;
    struct msg_append_req *req = &msg->append_req;

    if (s->meta.term > req->term) {
        msg_create_append_resp(&node->conn.out, s->meta.term,
                               s->store.last_index, req->round, false);

        rc = conn_flush(&node->conn);
        if (rc != RS_OK) {
            server_on_node_disconnect(s, node);
        }

        return;
    }

    if ((s->meta.term == req->term && s->leader == NULL) ||
        req->term > s->meta.term) {

        server_become_follower(s, node);
        server_update_meta(s, req->term, node->name);
    }

    node->in_timestamp = s->timestamp;

    if (req->prev_log_index > s->store.last_index ||
        req->prev_log_term !=
                store_prev_term_of(&s->store, req->prev_log_index)) {

        msg_create_append_resp(&node->conn.out, s->meta.term,
                               req->prev_log_index, req->round, false);

        rc = conn_flush(&node->conn);
        if (rc != RS_OK) {
            server_on_node_disconnect(s, node);
        }

        return;
    }

    index = req->prev_log_index + 1;

    entry_foreach (req->buf, req->len, entry) {
        curr = store_get_entry(&s->store, index);
        if (curr) {
            if (entry_term(curr) != entry_term(entry)) {
                store_remove_after(&s->store, index - 1);
            } else {
                index++;
                continue;
            }
        }

        if (entry_flags(entry) == CMD_META) {
            meta_term(&s->meta);
            meta_init(&s->meta, "");
            cmd_decode_meta_to(entry_data(entry), entry_data_len(entry),
                               &s->meta);
        }
retry:
        rc = store_put_entry(&s->store, index, entry);
        if (rc == RS_FULL) {
            if (s->ss_inprogress) {
                s->ss_inprogress = false;

                rc = (int) (uintptr_t) sc_cond_wait(&s->ss.cond);
                if (rc == RS_OK) {
                    store_snapshot_taken(&s->store);
                    metric_snapshot(true, s->ss.time, s->ss.size);
                    snapshot_replace(&s->ss);

                    goto retry;
                }

                metric_snapshot(false, 0, 0);
            }

            store_expand(&s->store);
            goto retry;

            rs_abort("Failed to create entry \n");
        }
        index++;
    }

    if (s->conf.advanced.fsync) {
        store_flush(&s->store);
    }

    msg_create_append_resp(&node->conn.out, s->meta.term, s->store.last_index,
                           req->round, true);
    rc = conn_flush(&node->conn);
    if (rc != RS_OK) {
        server_on_node_disconnect(s, node);
        return;
    }

    server_become_follower(s, node);

    if (s->commit < req->leader_commit) {
        uint64_t commit = sc_min(req->leader_commit, s->store.last_index);
        for (uint64_t i = s->commit + 1; i <= commit; i++) {
            entry = store_get_entry(&s->store, i);
            session = state_apply(&s->state, i, entry);
            server_on_applied_entry(s, i, entry, session);
        }

        s->commit = commit;
    }

    if (!s->ss_inprogress && s->commit >= store_ss_index(&s->store)) {
        s->ss_inprogress = true;
        snapshot_take(&s->ss, store_ss_page(&s->store));
    }
}

void server_on_append_resp(struct server *s, struct node *node, struct msg *msg)
{
    struct msg_append_resp *resp = &msg->append_resp;

    node->msg_inflight--;

    if (resp->success) {
        node_update_indexes(node, resp->round, resp->index);
    } else {
        if (resp->term > s->meta.term) {
            server_update_meta(s, resp->term, "");
            server_become_follower(s, NULL);
        }
    }
}

const char *server_add_node(void *arg, const char *node)
{
    struct sc_uri *uri;
    struct server *server = arg;
    struct server_job job;

    if (server->state.term != server->meta.term ||
        server->role != SERVER_ROLE_LEADER) {
        return "";
    }

    if (server->conf_change) {
        return "There is already a pending change";
    }

    uri = sc_uri_create(node);
    if (!uri) {
        return "Invalid uri";
    }

    job.type = SERVER_JOB_ADD_NODE;
    job.data = uri;
    sc_queue_add_last(server->jobs, job);
    server->conf_change = true;

    return "inprogress";
}

const char *server_remove_node(void *arg, const char *node)
{
    struct server *server = arg;
    struct server_job job;

    if (server->state.term != server->meta.term ||
        server->role != SERVER_ROLE_LEADER) {
        return "";
    }

    if (server->conf_change) {
        return "There is already a pending change";
    }
    server->conf_change = true;

    if (meta_exists(&server->meta, node) != true) {
        return "Node does not exist.";
    }

    job.type = SERVER_JOB_REMOVE_NODE;
    job.data = sc_str_create(node);
    sc_queue_add_last(server->jobs, job);

    return "inprogress";
}

const char *server_shutdown(void *arg, const char *node)
{
    struct server *server = arg;
    struct server_job job;

    if (server->state.term != server->meta.term ||
        server->role != SERVER_ROLE_LEADER) {
        return "";
    }

    job.type = SERVER_JOB_SHUTDOWN;
    job.data = sc_str_create(node);
    sc_queue_add_last(server->jobs, job);

    return "inprogress";
}

void server_on_snapshot_req(struct server *s, struct node *node,
                            struct msg *msg)
{
    bool success = true;
    int rc;
    struct msg_snapshot_req *req = &msg->snapshot_req;

    if (s->meta.term > req->term) {
        goto fail;
    }

    if ((s->meta.term == req->term && s->leader == NULL) ||
        req->term > s->meta.term) {

        server_become_follower(s, node);
        server_update_meta(s, req->term, node->name);
    }

    node->in_timestamp = s->timestamp;

    if (s->ss_inprogress) {
        sc_cond_wait(&s->ss.cond);
    }

    rc = snapshot_recv(&s->ss, req->ss_term, req->ss_index, req->done,
                       req->offset, req->buf, req->len);
    if (rc == RS_DONE) {
        struct state_cb cb = {.arg = s,
                              .add_node = server_add_node,
                              .remove_node = server_remove_node,
                              .shutdown = server_shutdown};

        state_term(&s->state);
        store_term(&s->store);

        state_init(&s->state, cb, s->conf.node.dir, s->conf.cluster.name);
        state_open(&s->state, s->conf.node.in_memory);
        store_init(&s->store, s->conf.node.dir, s->state.term, s->state.index);
        snapshot_open(&s->ss, s->state.ss_path, s->state.term, s->state.index);
        s->commit = s->state.index;
    }

    if (rc == RS_ERROR) {
        goto fail;
    }

    goto out;

fail:
    success = false;
out:
    msg_create_snapshot_resp(&node->conn.out, s->meta.term, success, req->done);

    rc = conn_flush(&node->conn);
    if (rc != RS_OK) {
        server_on_node_disconnect(s, node);
    }
}

void server_on_snapshot_resp(struct server *s, struct node *node,
                             struct msg *msg)
{
    struct msg_snapshot_resp *resp = &msg->snapshot_resp;

    node->msg_inflight--;

    if (resp->term > s->meta.term) {
        server_update_meta(s, resp->term, "");
        server_become_follower(s, NULL);
        return;
    }

    if (resp->done) {
        node->next = s->ss.index + 1;
    }
}

void server_on_info_req(struct server *s, struct node *node, struct msg *msg)
{
    (void) s;

    sc_buf_clear(&node->info);
    sc_buf_put_raw(&node->info, msg->info_req.buf, msg->info_req.len);
}

void server_on_shutdown_req(struct server *s, struct node *node,
                            struct msg *msg)
{
    (void) node;
    (void) msg;

    sc_log_info("Received shutdown request.. \n");
    s->stop_requested = true;
}

void server_on_node_recv(struct server *s, struct sc_sock_fd *fd,
                         uint32_t event)
{
    int rc;
    struct msg msg;

    struct sc_sock *sock = rs_entry(fd, struct sc_sock, fdt);
    struct conn *conn = rs_entry(sock, struct conn, sock);
    struct node *node = rs_entry(conn, struct node, conn);

    if (event & SC_SOCK_WRITE) {
        rc = conn_on_writable(&node->conn);
        if (rc != RS_OK) {
            goto disconnect;
        }
    }

    if (event & SC_SOCK_READ) {
        rc = conn_on_readable(&node->conn);
        if (rc == SC_SOCK_ERROR) {
            goto disconnect;
        }

        while ((rc = msg_parse(&node->conn.in, &msg)) == RS_OK) {
            switch (msg.type) {
            case MSG_APPEND_REQ:
                server_on_append_req(s, node, &msg);
                break;
            case MSG_APPEND_RESP:
                server_on_append_resp(s, node, &msg);
                break;
            case MSG_PREVOTE_REQ:
                server_on_prevote_req(s, node, &msg);
                break;
            case MSG_PREVOTE_RESP:
                server_on_prevote_resp(s, node, &msg);
                break;
            case MSG_REQVOTE_REQ:
                server_on_reqvote_req(s, node, &msg);
                break;
            case MSG_REQVOTE_RESP:
                server_on_reqvote_resp(s, node, &msg);
                break;
            case MSG_SNAPSHOT_REQ:
                server_on_snapshot_req(s, node, &msg);
                break;
            case MSG_SNAPSHOT_RESP:
                server_on_snapshot_resp(s, node, &msg);
                break;
            case MSG_INFO_REQ:
                server_on_info_req(s, node, &msg);
                break;
            case MSG_SHUTDOWN_REQ:
                server_on_shutdown_req(s, node, &msg);
            default:
                server_on_node_disconnect(s, node);
                return;
            }
        }

        if (rc == RS_INVALID) {
            sc_log_warn("Recv invalid message from client %s \n", node->name);
            goto disconnect;
        }
    }

    rc = conn_flush(&node->conn);
    if (rc == RS_ERROR) {
        goto disconnect;
    }

    return;

disconnect:
    server_on_node_disconnect(s, node);
}

void server_on_client_recv(struct server *s, struct sc_sock_fd *fd, uint32_t ev)
{
    int rc;
    struct sc_sock *sock = rs_entry(fd, struct sc_sock, fdt);
    struct conn *conn = rs_entry(sock, struct conn, sock);
    struct client *client = rs_entry(conn, struct client, conn);

    if (client->terminated) {
        return;
    }

    if (ev & SC_SOCK_WRITE) {
        rc = conn_on_writable(&client->conn);
        if (rc != RS_OK) {
            goto disconnect;
        }
    }

    if (ev & SC_SOCK_READ) {
        if (client_pending(client)) {
            conn_disallow_read(conn);
            return;
        }

        rc = conn_on_readable(&client->conn);
        if (rc != RS_OK) {
            goto disconnect;
        }

        rc = msg_parse(&client->conn.in, &client->msg);
        if (rc == RS_INVALID) {
            goto disconnect;
        }

        if (rc == RS_PARTIAL) {
            return;
        }

        server_on_client_req(s, client, &client->msg);
    }

    return;

disconnect:
    server_on_client_disconnect(s, client, MSG_ERR);
}


static void server_prepare_start(struct server *s)
{
    char *save = NULL, *uris;
    const char *token;
    struct node *node;
    struct meta_node n;

    server_prepare_cluster(s);
    server_read_meta(s);
    conf_print(&s->conf);

    uris = s->conf.node.bind_url;
    while ((token = sc_str_token_begin(uris, &save, " ")) != NULL) {
        server_listen(s, token);
    }

    if (sc_array_size(s->endpoints) == 0) {
        rs_exit("Server is not listening on any port. \n");
    }

    sc_array_foreach (s->meta.nodes, n) {
        if (strcmp(n.name, s->conf.node.name) == 0) {
            continue;
        }

        node = node_create(n.name, s, true);
        node_add_uris(node, n.uris);
        node_clear_indexes(node);
        node->known = true;
        sc_array_add(s->nodes, node);
    }

    server_schedule_election(s);
    s->info_timer = sc_timer_add(&s->timer, 0, SERVER_TIMER_INFO, NULL);
}

static void server_on_meta(struct server *s, struct meta *meta)
{
    if (meta->index < s->meta.index) {
        return;
    }

    if (meta->index == s->meta.index && meta->prev != NULL) {
        meta_remove_prev(&s->meta);
        s->conf_change = false;
        sc_buf_clear(&s->tmp);
        meta_print(&s->meta, &s->tmp);
        sc_log_info(sc_buf_rbuf(&s->tmp));
    } else if (meta->index > s->meta.index) {
        meta_copy(&s->meta, meta);
    }

    server_write_meta(s);
}

static void server_on_term_start(struct server *s, struct meta *meta)
{
    if (meta->term != s->meta.term) {
        return;
    }

    sc_log_info("Term[%llu], leader[%s] \n", meta->term, s->leader->name);
    s->cluster_up = true;
}

static void server_on_client_connect_applied(struct server *s,
                                             struct session *sess)
{
    bool found;
    struct client *client;

    if (s->role == SERVER_ROLE_LEADER) {
        found = sc_map_get_sv(&s->clients, sess->name, (void **) &client);
        if (found) {
            client->id = sess->id;
            client->seq = sess->seq;
            server_finalize_client_connection(s, client);
        }
    }
}

static void server_on_client_disconnect_applied(struct server *s,
                                                struct session *sess)
{
    (void) s;
    (void) sess;
}

static void server_on_applied_client_req(struct server *s, uint64_t cid,
                                         void *resp, uint32_t len)
{
    int rc;
    bool found;
    struct client *client;

    if (s->role != SERVER_ROLE_LEADER) {
        return;
    }

    found = sc_map_get_64v(&s->vclients, cid, (void **) &client);
    if (found) {
        client_processed(client);
        sc_buf_put_raw(&client->conn.out, resp, len);
        rc = conn_flush(&client->conn);
        if (rc != RS_OK) {
            server_on_client_disconnect(s, client, MSG_ERR);
            return;
        }
    }
}

static void server_on_applied_entry(struct server *s, uint64_t index,
                                    char *entry, struct session *sess)
{
    (void) index;

    enum cmd_id type = entry_flags(entry);

    switch (type) {
    case CMD_INIT:
        break;
    case CMD_META:
        server_on_meta(s, &s->state.meta);
        break;
    case CMD_TERM_START:
        server_on_term_start(s, &s->state.meta);
        break;
    case CMD_CLIENT_REQUEST:
        server_on_applied_client_req(s, sess->id, sc_buf_rbuf(&sess->resp),
                                     sc_buf_size(&sess->resp));
        break;

    case CMD_CLIENT_CONNECT:
        server_on_client_connect_applied(s, sess);
        break;
    case CMD_CLIENT_DISCONNECT:
        server_on_client_disconnect_applied(s, sess);
        break;
    case CMD_INFO:
        break;
    case CMD_TIMESTAMP:
        break;
    default:
        rs_abort("");
    }
}

static int server_sort_matches(const void *n1, const void *n2)
{
    struct node *node1 = *(struct node **) n1;
    struct node *node2 = *(struct node **) n2;

    return node1->match > node2->match ? -1 : 1;
}

static int server_sort_rounds(const void *n1, const void *n2)
{
    struct node *node1 = *(struct node **) n1;
    struct node *node2 = *(struct node **) n2;

    return node1->round > node2->round ? -1 : 1;
}

static void server_check_commit(struct server *s)
{
    int rc;
    uint64_t match, round_index;
    uint32_t index = s->meta.voter / 2;
    char *entry;
    struct client *c;
    struct sc_list *n, *it;
    struct session *session;

    sc_array_sort(s->nodes, server_sort_matches);
    match = s->nodes[index]->match;

    if (s->commit < match) {
        for (uint64_t i = s->commit + 1; i <= match; i++) {
            entry = store_get_entry(&s->store, i);
            session = state_apply(&s->state, i, entry);
            server_on_applied_entry(s, i, entry, session);
        }

        s->commit = match;
    }

    if (!s->ss_inprogress && s->commit >= store_ss_index(&s->store)) {
        s->ss_inprogress = true;
        snapshot_take(&s->ss, store_ss_page(&s->store));
    }

    if (sc_list_is_empty(&s->read_reqs)) {
        return;
    }

    sc_array_sort(s->nodes, server_sort_rounds);
    round_index = s->nodes[index]->round;

    s->round_match = sc_max(round_index, s->round_match);

    sc_list_foreach_safe (&s->read_reqs, n, it) {
        c = sc_list_entry(it, struct client, read);
        if (c->round_index > s->round_match || c->commit_index > s->commit) {
            break;
        }

        rc = state_apply_readonly(&s->state, c->id, c->msg.client_req.buf,
                                  c->msg.client_req.len, &c->conn.out);
        client_processed(c);

        if (rc != RS_OK || conn_flush(&c->conn) != RS_OK) {
            server_on_client_disconnect(s, c, MSG_ERR);
        }
    }
}

static void server_handle_jobs(struct server *s)
{
    struct server_job job;

    sc_queue_foreach (s->jobs, job) {
        switch (job.type) {
        case SERVER_JOB_ADD_NODE: {
            struct sc_uri *uri = job.data;
            if (meta_exists(&s->meta, uri->userinfo)) {
                s->conf_change = false;
                sc_log_error(
                        "Config change is invalid, node already exists : %s \n",
                        uri->str);
                continue;
            }

            meta_add(&s->meta, uri);
            sc_uri_destroy(uri);
            server_write_meta_cmd(s);
        } break;
        case SERVER_JOB_REMOVE_NODE: {
            char *name = job.data;
            if (!meta_exists(&s->meta, name)) {
                sc_log_error(
                        "Config change is invalid, node does not exists: %s \n",
                        name);
                continue;
            }

            meta_remove(&s->meta, name);
            sc_str_destroy(name);
            server_write_meta_cmd(s);
        } break;
        case SERVER_JOB_DISCONNECT_CLIENT:
            break;
        case SERVER_JOB_SHUTDOWN: {
            struct sc_list *l;
            struct node *node;
            char *name = job.data;

            if (*name == '*') {
                sc_list_foreach (&s->connected_nodes, l) {
                    node = sc_list_entry(l, struct node, list);
                    msg_create_shutdown_req(&node->conn.out, true);
                    conn_flush(&node->conn);
                }
            }

            if (*name == '*' || strcmp(name, s->conf.node.name) == 0) {
                server_on_shutdown_req(s, NULL, NULL);
            }
        } break;

        default:
            break;
        }
    }

    sc_queue_clear(s->jobs);
}

static void server_flush_snapshot(struct server *s, struct node *n)
{
    bool done;
    int rc;
    uint32_t len;
    void *data;

    if (n->msg_inflight > 0) {
        return;
    }

    if (n->ss_index != s->ss.index) {
        n->ss_index = s->ss.index;
        n->ss_pos = 0;
    }

    len = sc_min(4096, s->ss.map.len - n->ss_pos);
    data = s->ss.map.ptr + n->ss_pos;
    done = n->ss_pos + len == s->ss.map.len;

    if (len == 0) {
        goto flush;
    }

    msg_create_snapshot_req(&n->conn.out, s->meta.term, s->ss.term, s->ss.index,
                            n->ss_pos, done, data, len);
    n->ss_pos += len;
    n->msg_inflight++;

flush:
    rc = conn_flush(&n->conn);
    if (rc != RS_OK) {
        server_on_node_disconnect(s, n);
    }
}

static void server_flush(struct server *s)
{
    bool done;
    int rc;
    char *entries;
    uint32_t size, count, len;
    uint64_t prev;
    uint64_t timeout = s->conf.advanced.heartbeat;
    void *data;
    struct sc_list *l, *tmp;
    struct node *n;
    struct client *client;

    sc_array_foreach (s->term_clients, client) {
        client_destroy(client);
    }
    sc_array_clear(s->term_clients);

    if (s->role != SERVER_ROLE_LEADER) {
        return;
    }

    sc_list_foreach_safe (&s->connected_nodes, tmp, l) {
        n = sc_list_entry(l, struct node, list);

        if (n->next <= s->store.ss_index) {
            server_flush_snapshot(s, n);
            continue;
        }

        if ((n->next > s->store.last_index && s->round == s->round_prev) ||
            n->msg_inflight >= 2) {
            goto flush;
        }

        store_entries(&s->store, n->next, BATCH_SIZE, &entries, &size, &count);
        prev = store_prev_term_of(&s->store, n->next - 1);

        msg_create_append_req(&n->conn.out, s->meta.term, n->next - 1, prev,
                              s->commit, s->round, entries, size);
        n->next += count;
        n->msg_inflight++;
flush:
        rc = conn_flush(&n->conn);
        if (rc != RS_OK) {
            server_on_node_disconnect(s, n);
        }
    }

    if (s->own->next <= s->store.last_index) {
        if (s->conf.advanced.fsync) {
            store_flush(&s->store);
        }
        s->own->match = s->store.last_index;
        s->own->next = s->store.last_index + 1;
    }
    s->own->round = s->round;

    server_check_commit(s);

    s->round_prev = s->round;
    server_handle_jobs(s);
}

static void server_on_connect_req(struct server *s, struct sc_sock_fd *fd,
                                  uint32_t event)
{
    (void) event;

    int rc;
    enum msg_rc resp_code = MSG_OK;
    struct msg msg;
    struct sc_sock *sock = rs_entry(fd, struct sc_sock, fdt);
    struct conn *pending = rs_entry(sock, struct conn, sock);

    rc = conn_on_readable(pending);
    if (rc == SC_SOCK_ERROR) {
        goto disconnect;
    }

    rc = msg_parse(&pending->in, &msg);
    if (rc == RS_INVALID) {
        resp_code = MSG_CORRUPT;
        goto disconnect;
    } else if (rc == RS_PARTIAL) {
        return;
    }

    if (msg.type != MSG_CONNECT_REQ) {
        resp_code = MSG_UNEXPECTED;
        goto disconnect;
    }

    rc = strcmp(msg.connect_req.cluster_name, s->conf.cluster.name);
    if (rc != 0) {
        resp_code = MSG_CLUSTER_NAME_MISMATCH;
        goto disconnect;
    }

    switch (msg.connect_req.remote) {
    case MSG_CLIENT:
        server_on_client_connect_req(s, pending, &msg);
        break;
    case MSG_NODE:
        server_on_node_connect_req(s, pending, &msg);
        break;
    }

    return;

disconnect:
    server_on_pending_disconnect(s, pending, resp_code);
}

static void *server_run(void *arg)
{
    int rc;
    int events = 0, retry = 1;
    uint32_t timeo, event;
    struct sc_sock_fd *fd;
    struct server *s = arg;
    struct sc_sock_poll *loop = &s->loop;

    metric_init(&s->metric, s->conf.node.dir);
    rs_rand_init();

    server_prepare_start(s);
    sc_log_set_thread_name(s->conf.node.name);

    if (s->conf.cmdline.systemd) {
        rc = sc_sock_notify_systemd("READY=1\n");
        if (rc != 0) {
            sc_log_error("systemd failed : %s \n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    sc_log_info("Resql[v%s] has been started.. \n", RS_VERSION);

    while (!s->stop_requested) {
        s->timestamp = sc_time_mono_ms();

        timeo = sc_timer_timeout(&s->timer, s->timestamp, s, server_on_timeout);
        retry--;
        events = sc_sock_poll_wait(loop, retry > 0 ? 0 : (int) timeo);

        for (int i = 0; i < events; i++) {
            retry = 100;
            fd = sc_sock_poll_data(loop, i);
            event = sc_sock_poll_event(loop, i);

            switch (fd->type) {
            case SERVER_FD_NODE_RECV:
                server_on_node_recv(s, fd, event);
                break;
            case SERVER_FD_CLIENT_RECV:
                server_on_client_recv(s, fd, event);
                break;
            case SERVER_FD_INCOMING_CONN:
                server_on_incoming_conn(s, fd, event);
                break;
            case SERVER_FD_OUTGOING_CONN:
                server_on_outgoing_conn(s, fd, event);
                break;
            case SERVER_FD_WAIT_FIRST_REQ:
                server_on_connect_req(s, fd, event);
                break;
            case SERVER_FD_WAIT_FIRST_RESP:
                server_on_connect_resp(s, fd, event);
                break;
            case SERVER_FD_TASK:
                server_on_task(s, fd, event);
                break;
            case SERVER_FD_SIGNAL:
                server_on_signal(s, fd);
                break;
            default:
                rs_abort("Unexpected fd type : %d \n", fd->type);
            }
        }

        server_flush(s);
    }

    sc_log_info("Node[%s] is shutting down \n", s->conf.node.name);
    state_close(&s->state);

    if (s->conf.cmdline.systemd) {
        sc_sock_notify_systemd("STATUS=Shutdown task has been completed!\n");
    }

    return (void *) RS_OK;
}

int server_start(struct server *s, bool new_thread)
{
    int rc;

    if (!new_thread) {
        return (int) (uintptr_t) server_run(&s->thread);
    } else {
        rc = sc_thread_start(&s->thread, server_run, s);
        if (rc != 0) {
            sc_thread_term(&s->thread);
            return RS_ERROR;
        }

        return RS_OK;
    }
}

int server_stop(struct server *s)
{
    int rc;

    sc_sock_pipe_write(&s->efd, &(char){1}, 1);
    rc = (intptr_t) sc_thread_term(&s->thread);

    server_destroy(s);
    return rc;
}
