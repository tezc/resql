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

#include "server.h"

#include "client.h"
#include "cmd.h"
#include "conf.h"
#include "entry.h"
#include "file.h"
#include "node.h"
#include "session.h"

#include "sc/sc_array.h"
#include "sc/sc_log.h"
#include "sc/sc_queue.h"
#include "sc/sc_signal.h"
#include "sc/sc_time.h"
#include "sc/sc_uri.h"

#include <errno.h>
#include <inttypes.h>

#define MAX_SIZE  (SC_SOCK_BUF_SIZE - 128)
#define META_FILE "meta.resql"
#define META_TMP  "meta.tmp.resql"

static const char *server_role_str[] = {
	"FOLLOWER",
	"CANDIDATE",
	"LEADER",
};

const char *server_add_node(void *arg, const char *node);
const char *server_remove_node(void *arg, const char *node);
const char *server_shutdown(void *arg, const char *node);
static int server_prepare_start(struct server *s);

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

	if (strcmp(uri->scheme, "unix") == 0) {
		family = SC_SOCK_UNIX;
	} else if (*uri->host == '[') {
		family = SC_SOCK_INET6;
	} else {
		family = SC_SOCK_INET;
	}

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

	rc = sc_sock_poll_add(&s->poll, &sock->fdt, SC_SOCK_READ, &sock->fdt);
	if (rc != 0) {
		rs_exit("Poll failed : %s \n", sc_sock_poll_err(&s->poll));
	}

	sc_log_info("Listening at : %s \n", uri->str);
}

void server_init(struct server *s)
{
	int rc;
	char *urls, *save = NULL;
	const char *token;
	const char *dir = s->conf.node.dir;
	struct sc_sock_fd *fdt;
	struct conf *c = &s->conf;

	sc_thread_init(&s->thread);

	rc = sc_log_set_level(c->node.log_level);
	if (rc != 0) {
		rs_exit("Invalid log level : '%s' \n", c->node.log_level);
	}

	rc = strcasecmp(c->node.log_dest, "stdout");
	if (rc != 0) {
		sc_log_set_stdout(false);
		rc = sc_log_set_file("log.prev.txt", "log.current.txt");
		if (rc != 0) {
			rs_exit("log set file : %s \n", strerror(errno));
		}
	}

	rc = rs_rand_init();
	if (rc != RS_OK) {
		rs_exit("rand failed.");
	}

	rc = file_mkdir(c->node.dir);
	if (rc != RS_OK) {
		rs_exit("Failed to create directory : '%s' \n", c->node.dir);
	}

retry_pid:
	rc = rs_write_pid_file(c->node.dir);
	if (rc != RS_OK) {
		if (rc == RS_ERROR) {
			rs_exit("PID file error, shutting down. ");
		}

		if (rc == RS_FULL) {
			sc_log_error("Disk is full. Retry in 5 seconds. \n");
			sc_time_sleep(5000);
			goto retry_pid;
		}
	}

	rc = sc_sock_poll_init(&s->poll);
	if (rc != 0) {
		rs_exit("poll_init : %s \n", sc_sock_poll_err(&s->poll));
	}

	rc = sc_sock_pipe_init(&s->efd, SERVER_FD_TASK);
	if (rc != 0) {
		rs_exit("pipe_init : %s \n", sc_sock_poll_err(&s->poll));
	}

	fdt = &s->efd.fdt;
	rc = sc_sock_poll_add(&s->poll, fdt, SC_SOCK_READ, fdt);
	if (rc != 0) {
		rs_exit("poll_add : %s \n", sc_sock_poll_err(&s->poll));
	}

	rc = sc_sock_pipe_init(&s->sigfd, SERVER_FD_SIGNAL);
	if (rc != 0) {
		rs_exit("pipe_init : %s \n", sc_sock_poll_err(&s->poll));
	}

	sc_signal_shutdown_fd = s->sigfd.fds[1];
	fdt = &s->sigfd.fdt;
	rc = sc_sock_poll_add(&s->poll, fdt, SC_SOCK_READ, fdt);
	if (rc != 0) {
		rs_exit("poll_add : %s \n", sc_sock_poll_err(&s->poll));
	}

	sc_array_create(s->endpoints, 2);

	conf_print(&s->conf);

	urls = s->conf.node.bind_url;
	while ((token = sc_str_token_begin(urls, &save, " ")) != NULL) {
		server_listen(s, token);
	}

	if (sc_array_size(s->endpoints) == 0) {
		rs_exit("Server is not listening on any port. \n");
	}

	meta_init(&s->meta, s->conf.cluster.name);
	sc_buf_init(&s->tmp, 1024);
	sc_timer_init(&s->timer, sc_time_mono_ms());
	sc_array_create(s->term_clients, 4);
	sc_array_create(s->nodes, 4);
	sc_array_create(s->unknown_nodes, 4);
	sc_list_init(&s->pending_conns);
	sc_list_init(&s->connected_nodes);
	sc_list_init(&s->read_reqs);
	sc_map_init_sv(&s->clients, 32, 0);
	sc_map_init_64v(&s->vclients, 32, 0);
	sc_queue_create(s->jobs, 4);
	sc_queue_create(s->cache, 32);

	s->info_timer = SC_TIMER_INVALID;
	s->election_timer = SC_TIMER_INVALID;
	s->full_timer = SC_TIMER_INVALID;

	s->meta_path = sc_str_create_fmt("%s/%s", dir, META_FILE);
	s->meta_tmp_path = sc_str_create_fmt("%s/%s", dir, META_TMP);
}

int server_close(struct server *s)
{
	int rc, ret = RS_OK;
	struct node *node;
	struct conn *conn;
	struct client *client;
	struct sc_list *list, *tmp;
	struct server_job job;

	sc_str_destroy(&s->voted_for);

	sc_array_foreach (s->nodes, node) {
		node_destroy(node);
	}

	sc_array_foreach (s->unknown_nodes, node) {
		node_destroy(node);
	}

	sc_list_foreach_safe (&s->pending_conns, tmp, list) {
		conn = sc_list_entry(list, struct conn, list);
		conn_destroy(conn);
	}

	sc_map_foreach_value (&s->clients, client) {
		client_destroy(client);
	}

	sc_array_foreach (s->term_clients, client) {
		client_destroy(client);
	}

	sc_queue_foreach (s->jobs, job) {
		sc_str_destroy(&job.data);
	}

	rc = snapshot_term(&s->ss);
	if (rc != RS_OK) {
		ret = rc;
		sc_log_error("snapshot_term : %d", rc);
	}

	rc = state_term(&s->state);
	if (rc != RS_OK) {
		ret = rc;
		sc_log_error("state_term : %d", rc);
	}

	store_term(&s->store);

	sc_queue_clear(s->jobs);
	sc_array_clear(s->term_clients);
	sc_array_clear(s->nodes);
	sc_array_clear(s->unknown_nodes);
	sc_list_clear(&s->pending_conns);
	sc_list_clear(&s->connected_nodes);
	sc_list_clear(&s->read_reqs);
	sc_map_clear_sv(&s->clients);
	sc_map_clear_64v(&s->vclients);
	sc_timer_clear(&s->timer);

	s->stop_requested = false;
	s->ss_inprogress = false;
	s->cluster_up = false;
	s->last_ts = 0;
	s->role = SERVER_ROLE_FOLLOWER;

	s->commit = 0;
	s->round = 0;
	s->round_prev = 0;
	s->round_match = 0;

	return ret;
}

void server_term(struct server *s)
{
	int rc;
	struct sc_buf buf;
	struct server_endpoint e;

	server_close(s);

	sc_array_destroy(s->nodes);
	sc_array_destroy(s->unknown_nodes);
	sc_array_destroy(s->term_clients);

	sc_timer_term(&s->timer);
	sc_map_term_sv(&s->clients);
	sc_map_term_64v(&s->vclients);

	sc_queue_foreach (s->cache, buf) {
		sc_buf_term(&buf);
	}

	sc_queue_destroy(s->cache);
	sc_queue_destroy(s->jobs);

	sc_str_destroy(&s->meta_path);
	sc_str_destroy(&s->meta_tmp_path);
	sc_buf_term(&s->tmp);

	sc_array_foreach (s->endpoints, e) {
		if (strcmp(e.uri->scheme, "unix") == 0) {
			file_unlink(e.uri->path);
		}

		sc_uri_destroy(&e.uri);

		rc = sc_sock_term(e.sock);
		if (rc != 0) {
			sc_log_error("sock_term : %s\n", sc_sock_error(e.sock));
		}

		rs_free(e.sock);
	}
	sc_array_destroy(s->endpoints);

	rc = sc_sock_pipe_term(&s->efd);
	if (rc != 0) {
		sc_log_error("pipe_term : %s \n", sc_sock_pipe_err(&s->efd));
	}

	rc = sc_sock_pipe_term(&s->sigfd);
	if (rc != 0) {
		sc_log_error("pipe_term : %s \n", sc_sock_pipe_err(&s->sigfd));
	}

	rc = sc_sock_poll_term(&s->poll);
	if (rc != 0) {
		sc_log_error("poll_term : %s \n", sc_sock_poll_err(&s->poll));
	}

	rs_delete_pid_file(s->conf.node.dir);

	metric_term(&s->metric);
	meta_term(&s->meta);
	conf_term(&s->conf);
}

struct sc_buf server_buf_alloc(struct server *s)
{
	struct sc_buf buf;

	if (sc_queue_size(s->cache) > 0) {
		return sc_queue_del_first(s->cache);
	}

	sc_buf_init(&buf, 4096);
	return buf;
}

void server_buf_free(struct server *s, struct sc_buf buf)
{
	if (sc_queue_size(s->cache) >= 1024) {
		sc_buf_term(&buf);
		return;
	}

	sc_buf_clear(&buf);
	sc_buf_shrink(&buf, 32 * 1024);
	sc_queue_add_first(s->cache, buf);
}

int server_prepare_cluster(struct server *s)
{
	int rc;
	const char *path = s->conf.node.dir;
	struct state_cb cb = {
		.arg = s,
		.add_node = server_add_node,
		.remove_node = server_remove_node,
		.shutdown = server_shutdown,
	};

	s->cluster_up = false;
	s->role = SERVER_ROLE_FOLLOWER;
	s->leader = NULL;
	s->own = node_create(s->conf.node.name, s, false);

	sc_array_add(s->nodes, s->own);

	state_init(&s->state, cb, path, s->conf.cluster.name);

	rc = snapshot_init(&s->ss, s);
	if (rc != RS_OK) {
		return rc;
	}

	return RS_OK;
}

int server_write_meta(struct server *s)
{
	int rc;
	struct file f;

	file_init(&f);

	rc = file_open(&f, s->meta_tmp_path, "w+");
	if (rc != RS_OK) {
		return rc;
	}

	sc_buf_clear(&s->tmp);
	sc_buf_put_str(&s->tmp, s->conf.node.name);
	sc_buf_put_str(&s->tmp, s->voted_for);
	meta_encode(&s->meta, &s->tmp);

	rc = file_write(&f, sc_buf_rbuf(&s->tmp), sc_buf_size(&s->tmp));
	if (rc != RS_OK) {
		goto cleanup_file;
	}

	rc = file_flush(&f);
	if (rc != RS_OK) {
		goto cleanup_file;
	}

	rc = file_term(&f);
	if (rc != RS_OK) {
		goto cleanup_tmp;
	}

	rc = file_rename(s->meta_path, s->meta_tmp_path);
	if (rc != RS_OK) {
		goto cleanup_tmp;
	}

	rc = file_fsync(s->conf.node.dir);
	if (rc != RS_OK) {
		goto cleanup_tmp;
	}

	return RS_OK;

cleanup_file:
	file_term(&f);
cleanup_tmp:
	file_remove_path(s->meta_tmp_path);

	return rc;
}

static int server_create_meta(struct server *s)
{
	bool b;
	int rc;
	const char *urls = s->conf.cluster.nodes;

	meta_term(&s->meta);
	meta_init(&s->meta, s->conf.cluster.name);

	b = meta_parse_uris(&s->meta, urls);
	if (!b) {
		sc_log_error("Failed to parse urls : %s \n", urls);
		return RS_ERROR;
	}

	rc = server_write_meta(s);
	if (rc != RS_OK) {
		return rc;
	}

	return RS_OK;
}

static int server_parse_meta(struct server *s)
{
	int rc;
	ssize_t size;
	struct file f;

	file_init(&f);

	rc = file_open(&f, s->meta_path, "r");
	if (rc != RS_OK) {
		goto cleanup;
	}

	size = file_size(&f);
	if (size < 0) {
		rc = RS_ERROR;
		goto cleanup;
	}

	sc_buf_clear(&s->tmp);
	sc_buf_reserve(&s->tmp, (uint32_t) size);

	rc = file_read(&f, sc_buf_wbuf(&s->tmp), (size_t) size);
	if (rc != RS_OK) {
		goto cleanup;
	}

	sc_buf_mark_write(&s->tmp, (uint32_t) size);
	sc_str_set(&s->conf.node.name, sc_buf_get_str(&s->tmp));
	sc_str_set(&s->voted_for, sc_buf_get_str(&s->tmp));

	meta_term(&s->meta);
	meta_init(&s->meta, s->conf.cluster.name);
	meta_decode(&s->meta, &s->tmp);

	rc = RS_OK;

cleanup:
	file_term(&f);
	return rc;
}

int server_read_meta(struct server *s)
{
	bool exist;
	int rc;
	struct state *state = &s->state;

	rc = file_remove_path(s->meta_tmp_path);
	if (rc != RS_OK) {
		sc_log_error("Failed to remove : %s \n", s->meta_tmp_path);
		return rc;
	}

	exist = file_exists_at(s->meta_path);
	rc = exist ? server_parse_meta(s) : server_create_meta(s);
	if (rc != RS_OK) {
		return rc;
	}

	sc_buf_clear(&s->tmp);
	meta_print(&s->meta, &s->tmp);
	sc_log_info(sc_buf_rbuf(&s->tmp));

	exist = meta_exists(&s->meta, s->conf.node.name);
	if (exist) {
		s->in_cluster = true;
	}

	rc = state_open(&s->state, s->conf.node.in_memory);
	if (rc != RS_OK) {
		return rc;
	}

	rc = store_init(&s->store, s->conf.node.dir, state->term, state->index);
	if (rc != RS_OK) {
		return rc;
	}

	rc = snapshot_open(&s->ss, s->state.ss_path, s->state.term,
			   s->state.index);
	if (rc != RS_OK) {
		return rc;
	}

	s->commit = s->state.index;

	return RS_OK;
}

int server_update_meta(struct server *s, uint64_t term, const char *voted_for)
{
	s->meta.term = term;
	sc_str_set(&s->voted_for, voted_for);

	return server_write_meta(s);
}

struct node* server_find_node(struct server *s, const char* name)
{
	struct node *n;

	sc_array_foreach (s->nodes, n) {
		if (strcmp(n->name, name) == 0) {
			return n;
		}
	}

	return NULL;
}

void server_add_nodes(struct server *s)
{
	bool b;
	struct node *node;
	struct meta_node n;

	sc_array_foreach (s->meta.nodes, n) {
		if (strcmp(n.name, s->conf.node.name) == 0) {
			continue;
		}

		node = server_find_node(s, n.name);
		if (!node) {
			node = node_create(n.name, s, true);
			node_add_uris(node, n.uris);
			node_clear_indexes(node, s->store.last_index);
			node->known = true;
			sc_array_add(s->nodes, node);
		}
	}

	for (size_t i = 0; i < sc_array_size(s->nodes); i++) {
		node = s->nodes[i];
		b = meta_exists(&s->meta, node->name);
		if (!b) {
			sc_array_del(s->nodes, i);
			sc_array_add(s->nodes, node);

		}
		if (meta_exists(&s->meta, node->name))
	}
}

void server_adjust_meta_change(struct server *s)
{
	bool b;
	struct meta_node record;
	struct node *node;
	struct sc_list *elem, *tmp;

	s->in_cluster = meta_exists(&s->meta, s->conf.node.name);

	sc_array_foreach (s->meta.nodes, record) {
		for (int i = 0; i < sc_array_size(s->nodes); i++) {
		}
	}

	sc_list_foreach_safe (&s->connected_nodes, tmp, elem) {
		node = sc_list_entry(elem, struct node, list);
		b = meta_exists(&s->meta, node->name);
		if (!b) {
			sc_list_del(&s->connected_nodes, &node->list);

			for (size_t i = 0; i < sc_array_size(s->nodes); i++) {
				if (s->nodes[i] == node) {
					sc_array_del(s->nodes, i);
					break;
				}
			}

			sc_array_add(s->unknown_nodes, node);
		}
	}
}

static const char *server_msg(struct server *s, struct msg *msg)
{
	sc_buf_clear(&s->tmp);
	sc_buf_put_text(&s->tmp, "Current role[%s] ", server_role_str[s->role]);
	sc_buf_put_text(&s->tmp, "Term [%" PRIu64 "] ", s->meta.term);
	msg_print(msg, &s->tmp);

	return (const char *) s->tmp.mem;
}

static int server_on_incoming_conn(struct server *s, struct sc_sock_fd *fd)
{
	int rc;
	struct sc_sock in;
	struct sc_sock *endp = rs_entry(fd, struct sc_sock, fdt);
	struct conn *c;

	rc = sc_sock_accept(endp, &in);
	if (rc != SC_SOCK_OK) {
		sc_log_error("accept : %s \n", sc_sock_error(endp));
		return RS_OK;
	}

	c = conn_create(s, &in);
	if (!c) {
		return RS_OK;
	}

	conn_schedule(c, SERVER_TIMER_PENDING, 50000);
	sc_list_add_tail(&s->pending_conns, &c->list);

	return RS_OK;
}

static int server_on_task(struct server *s)
{
	char c;
	int size;

	size = sc_sock_pipe_read(&s->efd, &c, sizeof(c));
	if (size != 1) {
		sc_log_error("pipe_read : %d \n", size);
		return RS_ERROR;
	}

	s->stop_requested = true;

	return RS_OK;
}

static int server_on_signal(struct server *s)
{
	int size;
	uint64_t val;

	size = sc_sock_pipe_read(&s->sigfd, &val, sizeof(val));
	if (size != sizeof(val)) {
		return RS_ERROR;
	}

	if (s->conf.cmdline.systemd) {
		sc_sock_notify_systemd("STOPPING=1\n");
	}

	sc_log_info("Received shutdown command, shutting down. \n");
	s->stop_requested = true;

	return RS_OK;
}

static void server_on_pending_disconnect(struct server *s, struct conn *in,
					 enum msg_rc rc)
{
	char str[128];
	struct sc_buf *buf;

	sc_sock_print(&in->sock, str, sizeof(str));

	if (rc != MSG_ERR) {
		buf = conn_out(in);
		msg_create_connect_resp(buf, rc, 0, s->meta.term, s->meta.uris);
		conn_flush(in);
	}

	sc_log_debug("Pending connection %s disconnected \n", str);
	sc_list_del(&s->pending_conns, &in->list);
	conn_destroy(in);
}

static int server_wait_snapshot(struct server *s)
{
	int rc;

	if (!s->ss_inprogress) {
		return RS_NOOP;
	}

	s->ss_inprogress = false;

	rc = snapshot_wait(&s->ss);
	if (rc != RS_OK) {
		metric_snapshot(false, 0, 0);
		return rc;
	}

	store_snapshot_taken(&s->store);
	metric_snapshot(true, s->ss.time, s->ss.size);
	snapshot_replace(&s->ss);

	return RS_OK;
}

static int server_create_entry(struct server *s, bool force, uint64_t seq,
			       uint64_t cid, uint32_t flags, struct sc_buf *buf)
{
	int rc;
	uint32_t size = sc_buf_size(buf);
	void *data = sc_buf_rbuf(buf);

retry:
	rc = store_create_entry(&s->store, s->meta.term, seq, cid, flags, data,
				size);
	if (rc == RS_FULL) {
		rc = server_wait_snapshot(s);
		if (rc == RS_OK) {
			goto retry;
		}

		if (rc == RS_FULL) {
			return rc;
		}

		if (force) {
			rc = store_reserve(&s->store, size);
			if (rc != RS_OK) {
				rs_exit("Out of disk space.. \n");
			}
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

static int server_log(struct server *s, const char *level, const char *fmt, ...)
{
	int rc;
	char tmp[1024];
	va_list va;

	va_start(va, fmt);
	rs_vsnprintf(tmp, sizeof(tmp), fmt, va);
	va_end(va);

	sc_buf_clear(&s->tmp);
	cmd_encode_log(&s->tmp, level, tmp);

	rc = server_create_entry(s, true, 0, 0, CMD_LOG, &s->tmp);
	if (rc != RS_OK) {
		return rc;
	}

	sc_log_info("server_log(%s) %s \n", level, tmp);
}

#define server_ap(fmt, ...) fmt, __VA_ARGS__
#define server_info(s, ...) (server_log(s, "INFO", server_ap(__VA_ARGS__, "")))
#define server_warn(s, ...) (server_log(s, "WARN", server_ap(__VA_ARGS__, "")))
#define server_err(s, ...)  (server_log(s, "ERROR", server_ap(__VA_ARGS__, "")))

static int server_write_init_cmd(struct server *s)
{
	int rc;
	unsigned char rand[256];

	rc = rs_urandom(rand, sizeof(rand));
	if (rc != RS_OK) {
		return rc;
	}

	sc_buf_clear(&s->tmp);
	cmd_encode_init(&s->tmp, rand);

	rc = server_create_entry(s, true, 0, 0, CMD_INIT, &s->tmp);
	if (rc != RS_OK) {
		return rc;
	}

	return RS_OK;
}

static int server_write_meta_cmd(struct server *s)
{
	bool b;
	struct node *node;
	struct sc_list *elem;

	assert(s->leader == s->own);

	meta_clear_connection(&s->meta);

	b = meta_exists(&s->meta, s->conf.node.name);
	if (b) {
		meta_set_leader(&s->meta, s->conf.node.name);
	}

	meta_set_connected(&s->meta, s->conf.node.name);

	sc_list_foreach (&s->connected_nodes, elem) {
		node = sc_list_entry(elem, struct node, list);
		meta_set_connected(&s->meta, node->name);
	}

	sc_buf_clear(&s->tmp);
	cmd_encode_meta(&s->tmp, &s->meta);

	return server_create_entry(s, true, 0, 0, CMD_META, &s->tmp);
}

static int server_on_node_disconnect(struct server *s, struct node *node)
{
	int rc = RS_OK;

	sc_log_info("Node is not connected : %s \n", node->name);

	node_disconnect(node);

	if (s->leader == node) {
		s->leader = NULL;
	}

	if (s->role == SERVER_ROLE_LEADER) {
		rc = server_write_meta_cmd(s);
	}

	return rc;
}

static int server_write_term_start_cmd(struct server *s)
{
	sc_buf_clear(&s->tmp);
	cmd_encode_term(&s->tmp);

	return server_create_entry(s, true, 0, 0, CMD_TERM, &s->tmp);
}

static int server_on_client_disconnect(struct server *s, struct client *c,
				       enum msg_rc msg_rc)
{
	conn_clear_buf(&c->conn);

	c->terminated = true;

	sc_map_del_64v(&s->vclients, c->id, NULL);
	sc_map_del_sv(&s->clients, c->name, NULL);
	sc_array_add(s->term_clients, c);

	sc_log_debug("Client %s disconnected. \n", c->name);

	if (s->role != SERVER_ROLE_LEADER) {
		return RS_OK;
	}

	sc_buf_clear(&s->tmp);
	cmd_encode_disconnect(&s->tmp, c->name, msg_rc == MSG_OK);

	return server_create_entry(s, true, 0, 0, CMD_DISCONNECT, &s->tmp);
}

static int server_finalize_client_connection(struct server *s, struct client *c)
{
	int rc;
	struct sc_buf *b = conn_out(&c->conn);

	msg_create_connect_resp(b, MSG_OK, c->seq, s->meta.term, s->meta.uris);

	rc = conn_flush(&c->conn);
	if (rc != RS_OK) {
		goto err;
	}

	rc = client_processed(c);
	if (rc != RS_OK) {
		goto err;
	}

	sc_map_put_64v(&s->vclients, c->id, c);
	sc_log_debug("Client connected : %s \n", c->name);

	return RS_OK;

err:
	return server_on_client_disconnect(s, c, MSG_ERR);
}

static int server_on_client_connect_req(struct server *s, struct conn *in,
					struct msg_connect_req *msg)
{
	bool found;
	int rc, ret = RS_OK;
	enum msg_rc msg_rc = MSG_ERR;
	struct client *c, *prev;

	if (!s->cluster_up || s->role != SERVER_ROLE_LEADER) {
		msg_rc = MSG_NOT_LEADER;
		goto err;
	}

	if (!msg->name) {
		goto err;
	}

	found = sc_map_get_sv(&s->clients, msg->name, (void **) &prev);
	if (found) {
		if (prev->id == 0) {
			goto err;
		} else {
			rc = server_on_client_disconnect(s, prev, MSG_ERR);
			if (rc != RS_OK) {
				ret = rc;
				goto err;
			}
		}
	}

	sc_list_del(&s->pending_conns, &in->list);

	c = client_create(in, msg->name);
	if (!c) {
		goto err;
	}

	rs_free(in);

	sc_map_put_sv(&s->clients, c->name, c);
	sc_buf_clear(&s->tmp);
	cmd_encode_connect(&s->tmp, c->name, c->conn.local, c->conn.remote);

	return server_create_entry(s, true, 0, 0, CMD_CONNECT, &s->tmp);

err:
	server_on_pending_disconnect(s, in, msg_rc);
	return ret;
}

static int server_on_node_connect_req(struct server *s, struct conn *pending,
				      struct msg_connect_req *msg)
{
	int rc;
	bool found = false;
	struct sc_buf *buf;
	struct node *n = NULL;

	sc_array_foreach (s->nodes, n) {
		if (strcmp(n->name, msg->name) == 0) {
			sc_list_add_tail(&s->connected_nodes, &n->list);
			found = true;
			break;
		}
	}

	if (!found) {
		n = node_create(msg->name, s, false);
		n->known = false;
		sc_array_add(s->unknown_nodes, n);
	}

	sc_list_del(&s->pending_conns, &pending->list);

	rc = node_set_conn(n, pending);
	if (rc != RS_OK) {
		goto err;
	}

	rs_free(pending);

	buf = conn_out(&n->conn);
	msg_create_connect_resp(buf, MSG_OK, 0, s->meta.term, s->meta.uris);

	rc = conn_flush(&n->conn);
	if (rc != RS_OK) {
		node_disconnect(n);
		return RS_OK;
	}

	if (s->role == SERVER_ROLE_LEADER) {
		return server_write_meta_cmd(s);
	}

	return RS_OK;
err:
	server_on_pending_disconnect(s, pending, MSG_ERR);
	return RS_OK;
}

static int server_on_connect_resp(struct server *s, struct sc_sock_fd *fd)
{
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
		return RS_OK;
	}

	if (msg.type != MSG_CONNECT_RESP || msg.connect_resp.rc != MSG_OK) {
		goto disconnect;
	}

	conn_set_type(&node->conn, SERVER_FD_NODE_RECV);
	sc_list_add_tail(&s->connected_nodes, &node->list);

	return RS_OK;

disconnect:
	return server_on_node_disconnect(s, node);
}

static int server_on_outgoing_conn(struct server *s, struct sc_sock_fd *fd)
{
	int rc;
	struct sc_sock *sock = rs_entry(fd, struct sc_sock, fdt);
	struct conn *conn = rs_entry(sock, struct conn, sock);
	struct node *node = rs_entry(conn, struct node, conn);
	struct conf *c = &s->conf;
	struct sc_buf *buf;

	rc = conn_on_out_connected(&node->conn);
	if (rc != RS_OK) {
		node_disconnect(node);
		return RS_OK;
	}

	buf = conn_out(&node->conn);
	msg_create_connect_req(buf, MSG_NODE, c->cluster.name, c->node.name);

	rc = conn_flush(&node->conn);
	if (rc != RS_OK) {
		node_disconnect(node);
	}

	return RS_OK;
}

static void server_schedule_election(struct server *s)
{
	const uint64_t type = SERVER_TIMER_ELECTION;
	uint64_t t = s->conf.advanced.heartbeat + (rs_rand() % 2048);

	s->election_timer = sc_timer_add(&s->timer, t, type, NULL);
}

static int server_become_leader(struct server *s)
{
	int rc;
	struct node *node;

	s->role = SERVER_ROLE_LEADER;
	s->leader = s->own;

	sc_array_foreach (s->nodes, node) {
		node_clear_indexes(node, s->store.last_index);
	}

	sc_array_foreach (s->unknown_nodes, node) {
		node_clear_indexes(node, s->store.last_index);
	}

	if (s->store.last_index == 0) {
		rc = server_write_init_cmd(s);
		if (rc != RS_OK) {
			return rc;
		}
	}

	rc = server_write_meta_cmd(s);
	if (rc != RS_OK) {
		return rc;
	}

	rc = server_write_term_start_cmd(s);
	if (rc != RS_OK) {
		return rc;
	}

	sc_buf_clear(&s->tmp);
	meta_print(&s->meta, &s->tmp);
	sc_log_info(sc_buf_rbuf(&s->tmp));

	return RS_OK;
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

static int server_check_prevote_count(struct server *s)
{
	int rc;
	struct sc_list *n, *tmp;
	struct node *node;
	struct sc_buf *buf;

	if (s->prevote_count >= (s->meta.voter / 2) + 1) {
		rc = server_update_meta(s, s->prevote_term, s->conf.node.name);
		if (rc != RS_OK) {
			return rc;
		}

		s->vote_count = 0;

		if (s->in_cluster) {
			s->vote_count += 1;
		}

		sc_list_foreach_safe (&s->connected_nodes, tmp, n) {
			node = sc_list_entry(n, struct node, list);

			buf = conn_out(&node->conn);
			msg_create_reqvote_req(buf, s->meta.term,
					       s->store.last_index,
					       s->store.last_term);
			rc = conn_flush(&node->conn);
			if (rc != RS_OK) {
				rc = server_on_node_disconnect(s, node);
				if (rc != RS_OK) {
					return rc;
				}
			}
		}

		if (s->vote_count >= (s->meta.voter / 2) + 1) {
			rc = server_become_leader(s);
			if (rc != RS_OK) {
				return rc;
			}
		}
	}

	return RS_OK;
}

static int server_on_election_timeout(struct server *s)
{
	int rc;
	size_t connected;
	struct sc_list *n, *tmp;
	struct node *node = NULL;
	struct sc_buf *buf;
	uint64_t timeout = s->conf.advanced.heartbeat;

	if (s->leader == s->own) {
		return RS_OK;
	}

	if (timeout > s->timestamp - s->vote_timestamp) {
		return RS_OK;
	}

	if (s->leader != NULL &&
	    timeout > s->timestamp - s->leader->in_timestamp) {
		return RS_OK;
	}

	if (!s->in_cluster && s->meta.prev == NULL) {
		return RS_OK;
	}

	connected = sc_list_count(&s->connected_nodes);

	if (s->in_cluster) {
		connected += 1;
	}

	if (connected < (s->meta.voter / 2) + 1) {
		sc_log_info("Cluster nodes = %zu, "
			    "Connected nodes = %zu, "
			    "No election will take place \n",
			    (size_t) s->meta.voter, connected);
		return RS_OK;
	}

	sc_log_info("Starting election, term[%" PRIu64 "]\n", s->meta.term + 1);

	s->role = SERVER_ROLE_CANDIDATE;
	s->prevote_count = 0;

	if (s->in_cluster) {
		s->prevote_count += 1;
	}

	s->prevote_term = s->meta.term + 1;

	sc_list_foreach_safe (&s->connected_nodes, tmp, n) {
		node = sc_list_entry(n, struct node, list);
		buf = conn_out(&node->conn);

		msg_create_prevote_req(buf, s->meta.term + 1,
				       s->store.last_index, s->store.last_term);
		rc = conn_flush(&node->conn);
		if (rc != RS_OK) {
			rc = server_on_node_disconnect(s, node);
			if (rc != RS_OK) {
				return rc;
			}
		}
	}

	return server_check_prevote_count(s);
}

static void server_try_connect(struct server *s, struct node *n)
{
	int rc;
	struct sc_buf *buf;
	const char *node = s->conf.node.name;
	const char *cluster = s->conf.cluster.name;

	rc = node_try_connect(n);
	if (rc == RS_OK) {
		sc_log_debug("Connected to : %s \n", n->conn.remote);

		buf = conn_out(&n->conn);
		msg_create_connect_req(buf, MSG_NODE, cluster, node);

		rc = conn_flush(&n->conn);
		if (rc != RS_OK) {
			conn_term(&n->conn);
		}
	}
}

static int server_on_info_timer(struct server *s)
{
	int rc;
	struct sc_list *l;
	struct node *n;
	struct sc_buf *buf, *info;

	s->info_timer = sc_timer_add(&s->timer, 10000, SERVER_TIMER_INFO, NULL);

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

		return server_create_entry(s, true, 0, 0, CMD_INFO, &s->tmp);
	}

	if (s->leader != NULL) {
		info = &s->own->info;
		buf = conn_out(&s->leader->conn);
		msg_create_info_req(buf, sc_buf_rbuf(info), sc_buf_size(info));

		rc = conn_flush(&s->leader->conn);
		if (rc != RS_OK) {
			return server_on_node_disconnect(s, s->leader);
		}
	}

	return RS_OK;
}

static void server_on_full_disk(struct server *s)
{
	int rc;

	if (s->full) {
		return;
	}

	sc_log_error("Disk is full. \n");

	rc = server_close(s);
	if (rc != RS_OK && rc != RS_FULL) {
		rs_exit("server close : %d", rc);
	}

	s->full = true;
	sc_timer_cancel(&s->timer, &s->full_timer);
	s->full_timer = sc_timer_add(&s->timer, 10000, SERVER_TIMER_FULL, NULL);
}

static void server_on_full_timer(struct server *s)
{
	assert(s->full);

	int rc;
	char tmp[PATH_MAX];
	char req[64];
	char cur[64];

	ssize_t ss_size;
	size_t dir_size;
	size_t limit = 64 * 1024 * 1024;

	dir_size = rs_dir_free(s->conf.node.dir);

	rs_snprintf(tmp, sizeof(tmp), "%s/snapshot.resql", s->conf.node.dir);
	ss_size = file_size_at(tmp);
	if (ss_size > 0) {
		limit += (size_t) ss_size;
	}

	if (dir_size > limit) {
		rc = server_prepare_start(s);
		if (rc == RS_ERROR) {
			rs_exit("prepare_start : %d .", rc);
		}

		if (rc == RS_OK) {
			s->full = false;
			return;
		}
	}

	s->full_timer = sc_timer_add(&s->timer, 10000, SERVER_TIMER_FULL, NULL);

	sc_bytes_to_size(cur, sizeof(cur), dir_size);
	sc_bytes_to_size(req, sizeof(req), limit);

	sc_log_error("Free space : %s, need : %s. Retry in 10 seconds \n", cur,
		     req);
}

void server_timeout(void *arg, uint64_t timeout, uint64_t type, void *data)
{
	(void) timeout;

	int rc = RS_OK;
	struct server *s = arg;

	if (s->timer_rc != RS_OK) {
		return;
	}

	switch (type) {
	case SERVER_TIMER_PENDING:
		server_on_pending_disconnect(s, data, MSG_TIMEOUT);
		break;
	case SERVER_TIMER_CONNECT:
		server_try_connect(s, data);
		break;
	case SERVER_TIMER_ELECTION:
		rc = server_on_election_timeout(s);
		server_schedule_election(s);
		break;
	case SERVER_TIMER_INFO:
		rc = server_on_info_timer(s);
		break;
	case SERVER_TIMER_FULL:
		server_on_full_timer(s);
		break;
	default:
		break;
	}

	s->timer_rc = rc;
}

static int server_on_reqvote_req(struct server *s, struct node *node,
				 struct msg *msg)
{
	bool grant = false;
	int rc;
	uint64_t timeout = s->conf.advanced.heartbeat;
	uint64_t last_index = s->store.last_index;
	struct msg_prevote_req *req = &msg->prevote_req;
	struct sc_buf *buf;

	if (s->leader != NULL &&
	    timeout > s->timestamp - s->leader->in_timestamp) {
		goto out;
	}

	if (req->term == s->meta.term && s->voted_for != NULL) {
		goto out;
	}

	if (req->term > s->meta.term && req->last_log_index >= last_index) {
		grant = true;
		rc = server_update_meta(s, req->term, node->name);
		if (rc != RS_OK) {
			return rc;
		}
	}

out:
	buf = conn_out(&node->conn);
	msg_create_reqvote_resp(buf, req->term, last_index, grant);

	return RS_OK;
}

static int server_on_reqvote_resp(struct server *s, struct node *n,
				  struct msg *msg)
{
	int rc = RS_OK;
	struct msg_reqvote_resp *resp = &msg->reqvote_resp;

	if (s->role != SERVER_ROLE_CANDIDATE || s->meta.term != resp->term) {
		sc_log_debug("Unexpected msg from [%s] : %s \n", n->name,
			     server_msg(s, msg));
		return RS_OK;
	}

	if (resp->term > s->meta.term) {
		sc_log_debug("Recv[reqvoteresp], node[%s] term[%" PRIu64 "], "
			     "node's term [%" PRIu64 "], "
			     "stepping down to follower \n",
			     n->name, resp->term, s->meta.term);

		rc = server_update_meta(s, resp->term, NULL);
		if (rc != RS_OK) {
			return rc;
		}

		s->prevote_count = 0;
		s->role = SERVER_ROLE_FOLLOWER;
		return RS_OK;
	}

	if (!resp->granted) {
		return RS_OK;
	}

	s->vote_count++;
	if (s->vote_count >= s->meta.voter / 2 + 1) {
		rc = server_become_leader(s);
	}

	return rc;
}

static int server_on_prevote_req(struct server *s, struct node *n,
				 struct msg *msg)
{
	bool result = false;
	uint64_t index = s->store.last_index;
	uint64_t timeout = s->conf.advanced.heartbeat;
	struct msg_prevote_req *req = &msg->prevote_req;
	struct sc_buf *buf;

	if (s->leader != NULL &&
	    timeout > s->timestamp - s->leader->in_timestamp) {
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
	buf = conn_out(&n->conn);
	msg_create_prevote_resp(buf, req->term, index, result);

	return RS_OK;
}

static int server_on_prevote_resp(struct server *s, struct node *node,
				  struct msg *msg)
{
	(void) node;

	struct msg_prevote_resp *resp = &msg->prevote_resp;

	if (s->role != SERVER_ROLE_CANDIDATE || s->prevote_term != resp->term) {
		return RS_OK;
	}

	if (resp->term > s->prevote_term) {
		server_update_meta(s, resp->term, NULL);
		s->prevote_count = 0;
		s->role = SERVER_ROLE_FOLLOWER;
		return RS_OK;
	}

	if (!resp->granted) {
		return RS_OK;
	}

	s->prevote_count++;

	return server_check_prevote_count(s);
}

static int server_on_applied_entry(struct server *s, unsigned char *entry,
				   struct session *sess);

static int server_update_commit(struct server *s, uint64_t commit)
{
	int rc;
	unsigned char *entry;
	struct session *sess;

	if (s->commit < commit) {
		uint64_t min = sc_min(commit, s->store.last_index);
		for (uint64_t i = s->commit + 1; i <= min; i++) {
			entry = store_get_entry(&s->store, i);
			rc = state_apply(&s->state, i, entry, &sess);
			if (rc != RS_OK) {
				if (rc == RS_FULL) {
					return rc;
				}

				rs_abort("error : %d \n", rc);
			}

			rc = server_on_applied_entry(s, entry, sess);
			if (rc != RS_OK) {
				return rc;
			}
		}

		s->commit = commit;
	}

	if (!s->ss_inprogress && s->commit >= store_ss_index(&s->store)) {
		s->ss_inprogress = true;
		rc = snapshot_take(&s->ss, store_ss_page(&s->store));
		if (rc != RS_OK) {
			rs_abort("error");
		}
	}

	return RS_OK;
}

static int server_store_entries(struct server *s, uint64_t index,
				unsigned char *buf, uint32_t len)
{
	int rc;
	uint32_t data_len, total_len;
	uint64_t term;
	void *data;

	unsigned char *e, *curr;

	entry_foreach (buf, len, e) {
		term = entry_term(e);
		data = entry_data(e);
		data_len = entry_data_len(e);
		total_len = entry_len(e);

		curr = store_get_entry(&s->store, index);
		if (curr) {
			if (entry_term(curr) != term) {
				store_remove_after(&s->store, index - 1);
				meta_rollback(&s->meta, index - 1);
			} else {
				index++;
				continue;
			}
		}

		if (entry_flags(e) == CMD_META) {
			meta_replace(&s->meta, data, data_len);
		}
retry:
		rc = store_put_entry(&s->store, index, e);
		if (rc == RS_FULL) {
			rc = server_wait_snapshot(s);
			if (rc == RS_OK) {
				goto retry;
			}

			if (rc == RS_FULL) {
				return rc;
			}

			rc = store_reserve(&s->store, total_len);
			if (rc != RS_OK) {
				rs_exit("Out of disk space .. \n");
			}
			goto retry;
		}
		index++;
	}

	if (s->conf.advanced.fsync) {
		store_flush(&s->store);
	}

	return RS_OK;
}

int server_on_append_req(struct server *s, struct node *n, struct msg *msg)
{
	int rc;
	bool success = false;
	uint64_t prev;
	struct msg_append_req *req = &msg->append_req;
	struct sc_buf *buf;

	if (s->meta.term > req->term) {
		goto out;
	}

	if (req->term > s->meta.term ||
	    (s->meta.term == req->term && s->leader == NULL)) {
		server_become_follower(s, n);
		rc = server_update_meta(s, req->term, n->name);
		if (rc != RS_OK) {
			return rc;
		}
	}

	n->in_timestamp = s->timestamp;

	prev = store_prev_term(&s->store, req->prev_log_index);
	if (req->prev_log_index > s->store.last_index ||
	    req->prev_log_term != prev) {
		goto out;
	}

	rc = server_store_entries(s, req->prev_log_index + 1, req->buf,
				  req->len);
	if (rc != RS_OK) {
		return rc;
	}

	server_become_follower(s, n);
	rc = server_update_commit(s, req->leader_commit);
	if (rc != RS_OK) {
		return rc;
	}

	success = true;
out:
	buf = conn_out(&n->conn);
	msg_create_append_resp(buf, s->meta.term, s->store.last_index,
			       success ? req->round : 0, success);
	return RS_OK;
}

int server_on_append_resp(struct server *s, struct node *node, struct msg *msg)
{
	int rc;
	struct msg_append_resp *resp = &msg->append_resp;

	if (s->role != SERVER_ROLE_LEADER) {
		return RS_OK;
	}

	node->msg_inflight--;
	if (resp->success) {
		node_update_indexes(node, resp->round, resp->index);
	}

	if (!resp->success) {
		if (resp->term > s->meta.term) {
			rc = server_update_meta(s, resp->term, NULL);
			if (rc != RS_OK) {
				return rc;
			}

			server_become_follower(s, NULL);
		}

		node->match = resp->index;
		node->next = node->match + 1;
		node->round = 0;
	}

	return RS_OK;
}

const char *server_add_node(void *arg, const char *node)
{
	struct server *s = arg;
	struct server_job job = {
		.type = SERVER_JOB_ADD_NODE,
		.data = sc_str_create(node),
	};

	if (s->state.term == s->meta.term && s->role == SERVER_ROLE_LEADER) {
		sc_queue_add_last(s->jobs, job);
	}

	return "Config change is in progress. Check resql_log table for details";
}

const char *server_remove_node(void *arg, const char *node)
{
	struct server *s = arg;

	if (s->state.term == s->meta.term && s->role == SERVER_ROLE_LEADER) {
		struct server_job job = {
			.type = SERVER_JOB_REMOVE_NODE,
			.data = sc_str_create(node),
		};

		sc_queue_add_last(s->jobs, job);
	}

	return "Config change is in progress. Check resql_log table for details";
}

const char *server_shutdown(void *arg, const char *node)
{
	struct server *s = arg;
	struct server_job job = {
		.type = SERVER_JOB_SHUTDOWN,
		.data = sc_str_create(node),
	};

	if (s->state.term == s->meta.term && s->role == SERVER_ROLE_LEADER) {
		sc_queue_add_last(s->jobs, job);
	}

	return "Shutdown in progress.";
}

static void server_replace_snapshot(struct server *s)
{
	struct state_cb cb = {
		.arg = s,
		.add_node = server_add_node,
		.remove_node = server_remove_node,
		.shutdown = server_shutdown,
	};

	state_term(&s->state);
	store_term(&s->store);

	state_init(&s->state, cb, s->conf.node.dir, s->conf.cluster.name);
	state_open(&s->state, s->conf.node.in_memory);
	store_init(&s->store, s->conf.node.dir, s->state.term, s->state.index);
	snapshot_open(&s->ss, s->state.ss_path, s->state.term, s->state.index);

	s->commit = s->state.index;
}

int server_on_snapshot_req(struct server *s, struct node *n, struct msg *msg)
{
	bool success = true;
	int rc;
	struct msg_snapshot_req *req = &msg->snapshot_req;
	struct sc_buf *buf;

	if (s->meta.term > req->term) {
		success = false;
		goto out;
	}

	if ((s->meta.term == req->term && s->leader == NULL) ||
	    req->term > s->meta.term) {
		server_become_follower(s, n);

		rc = server_update_meta(s, req->term, n->name);
		if (rc != RS_OK) {
			return rc;
		}
	}

	n->in_timestamp = s->timestamp;

	rc = server_wait_snapshot(s);
	if (rc == RS_FULL) {
		return rc;
	}

	rc = snapshot_recv(&s->ss, req->ss_term, req->ss_index, req->done,
			   req->offset, req->buf, req->len);
	if (rc == RS_DONE) {
		server_replace_snapshot(s);
	}

	if (rc == RS_ERROR) {
		success = false;
	}
out:
	buf = conn_out(&n->conn);
	msg_create_snapshot_resp(buf, s->meta.term, success, req->done);

	return RS_OK;
}

int server_on_snapshot_resp(struct server *s, struct node *n, struct msg *msg)
{
	int rc;
	struct msg_snapshot_resp *resp = &msg->snapshot_resp;

	n->msg_inflight--;

	if (resp->term > s->meta.term) {
		rc = server_update_meta(s, resp->term, NULL);
		if (rc != RS_OK) {
			return rc;
		}

		server_become_follower(s, NULL);
		return RS_OK;
	}

	if (resp->done) {
		n->next = s->ss.index + 1;
		rc = server_warn(s, "Snapshot[%" PRIu64 "] sent to : %s",
				 s->ss.index, n->name);
		if (rc != RS_OK) {
			return rc;
		}
	}

	return RS_OK;
}

int server_on_info_req(struct server *s, struct node *n, struct msg *msg)
{
	(void) s;

	sc_buf_clear(&n->info);
	sc_buf_put_raw(&n->info, msg->info_req.buf, msg->info_req.len);

	return RS_OK;
}

int server_on_shutdown_req(struct server *s)
{
	sc_log_info("Received shutdown request.. \n");
	s->stop_requested = true;

	return RS_OK;
}

int server_on_node_recv(struct server *s, struct sc_sock_fd *fd, uint32_t ev)
{
	int rc, ret;
	struct msg msg;

	struct sc_sock *sock = rs_entry(fd, struct sc_sock, fdt);
	struct conn *conn = rs_entry(sock, struct conn, sock);
	struct node *node = rs_entry(conn, struct node, conn);

	if (ev & SC_SOCK_WRITE) {
		rc = conn_on_writable(&node->conn);
		if (rc != RS_OK) {
			goto disconnect;
		}
	}

	if (ev & SC_SOCK_READ) {
		rc = conn_on_readable(&node->conn);
		if (rc == SC_SOCK_ERROR) {
			goto disconnect;
		}

		while ((rc = msg_parse(&node->conn.in, &msg)) == RS_OK) {
			switch (msg.type) {
			case MSG_APPEND_REQ:
				ret = server_on_append_req(s, node, &msg);
				break;
			case MSG_APPEND_RESP:
				ret = server_on_append_resp(s, node, &msg);
				break;
			case MSG_PREVOTE_REQ:
				ret = server_on_prevote_req(s, node, &msg);
				break;
			case MSG_PREVOTE_RESP:
				ret = server_on_prevote_resp(s, node, &msg);
				break;
			case MSG_REQVOTE_REQ:
				ret = server_on_reqvote_req(s, node, &msg);
				break;
			case MSG_REQVOTE_RESP:
				ret = server_on_reqvote_resp(s, node, &msg);
				break;
			case MSG_SNAPSHOT_REQ:
				ret = server_on_snapshot_req(s, node, &msg);
				break;
			case MSG_SNAPSHOT_RESP:
				ret = server_on_snapshot_resp(s, node, &msg);
				break;
			case MSG_INFO_REQ:
				ret = server_on_info_req(s, node, &msg);
				break;
			case MSG_SHUTDOWN_REQ:
				ret = server_on_shutdown_req(s);
				break;
			default:
				return server_on_node_disconnect(s, node);
			}

			if (ret != RS_OK) {
				return ret;
			}
		}

		if (rc == RS_INVALID) {
			sc_log_warn("Invalid message, node :%s \n", node->name);
			goto disconnect;
		}
	}

	rc = conn_flush(&node->conn);
	if (rc != RS_OK) {
		goto disconnect;
	}

	return RS_OK;

disconnect:
	return server_on_node_disconnect(s, node);
}

int server_on_client_recv(struct server *s, struct sc_sock_fd *fd, uint32_t ev)
{
	int rc;
	enum msg_rc ret = MSG_ERR;
	struct sc_sock *sock = rs_entry(fd, struct sc_sock, fdt);
	struct conn *conn = rs_entry(sock, struct conn, sock);
	struct client *c = rs_entry(conn, struct client, conn);
	struct msg_client_req *req;
	struct sc_buf buf;

	if (c->terminated) {
		return RS_OK;
	}

	if (ev & SC_SOCK_WRITE) {
		rc = conn_on_writable(&c->conn);
		if (rc != RS_OK) {
			goto disconnect;
		}
	}

	if (ev & SC_SOCK_READ) {
		if (client_pending(c)) {
			rc = conn_unregister(conn, true, false);
			if (rc != RS_OK) {
				goto disconnect;
			}

			return RS_OK;
		}

		rc = conn_on_readable(&c->conn);
		if (rc != RS_OK) {
			goto disconnect;
		}

		rc = msg_parse(&c->conn.in, &c->msg);
		if (rc == RS_INVALID) {
			goto disconnect;
		}

		if (rc == RS_PARTIAL) {
			return RS_OK;
		}

		if (c->msg.type == MSG_DISCONNECT_REQ) {
			ret = MSG_OK;
			goto disconnect;
		}

		if (c->msg.type != MSG_CLIENT_REQ) {
			goto disconnect;
		}

		c->msg_wait = true;
		req = &c->msg.client_req;

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
						 CMD_REQUEST, &buf);
			if (rc == RS_FULL) {
				return rc;
			}

			if (rc != RS_OK) {
				goto disconnect;
			}

			c->seq = req->seq;
			conn_clear_buf(&c->conn);
		}
	}

	return RS_OK;

disconnect:
	server_on_client_disconnect(s, c, ret);
	return RS_OK;
}

static int server_prepare_start(struct server *s)
{
	int rc;
	struct node *node;
	struct meta_node n;

	rc = server_prepare_cluster(s);
	if (rc != RS_OK) {
		return rc;
	}

	rc = server_read_meta(s);
	if (rc != RS_OK) {
		return rc;
	}

	sc_array_foreach (s->meta.nodes, n) {
		if (strcmp(n.name, s->conf.node.name) == 0) {
			continue;
		}

		node = node_create(n.name, s, true);
		node_add_uris(node, n.uris);
		node_clear_indexes(node, s->store.last_index);
		node->known = true;
		sc_array_add(s->nodes, node);
	}

	server_schedule_election(s);
	s->info_timer = sc_timer_add(&s->timer, 0, SERVER_TIMER_INFO, NULL);

	return RS_OK;
}

static int server_on_meta(struct server *s, struct meta *meta)
{
	if (meta->index < s->meta.index) {
		return RS_OK;
	}

	if (meta->index == s->meta.index && s->meta.prev != NULL) {
		meta_remove_prev(&s->meta);
		sc_buf_clear(&s->tmp);
		meta_print(&s->meta, &s->tmp);
		sc_log_info(sc_buf_rbuf(&s->tmp));
	} else if (meta->index > s->meta.index) {
		meta_copy(&s->meta, meta);
	}

	return server_write_meta(s);
}

static int server_on_term_start(struct server *s, struct meta *m)
{
	if (m->term != s->meta.term) {
		return RS_OK;
	}

	sc_log_info("Term[%" PRIu64 "], leader[%s] \n", m->term,
		    s->leader->name);
	s->cluster_up = true;

	return RS_OK;
}

static int server_on_client_connect_applied(struct server *s,
					    struct session *sess)
{
	bool found;
	struct client *c;

	if (s->role != SERVER_ROLE_LEADER) {
		return RS_OK;
	}

	found = sc_map_get_sv(&s->clients, sess->name, (void **) &c);
	if (!found) {
		return RS_OK;
	}

	c->id = sess->id;
	c->seq = sess->seq;

	return server_finalize_client_connection(s, c);
}

static int server_on_applied_client_req(struct server *s, uint64_t cid,
					void *resp, uint32_t len)
{
	int rc;
	bool found;
	struct client *c;
	struct sc_buf *b;

	if (s->role != SERVER_ROLE_LEADER) {
		return RS_OK;
	}

	found = sc_map_get_64v(&s->vclients, cid, (void **) &c);
	if (!found) {
		return RS_OK;
	}

	b = conn_out(&c->conn);
	sc_buf_put_raw(b, resp, len);

	rc = conn_flush(&c->conn);
	if (rc != RS_OK) {
		goto err;
	}

	rc = client_processed(c);
	if (rc != RS_OK) {
		goto err;
	}

	return RS_OK;
err:
	return server_on_client_disconnect(s, c, MSG_ERR);
}

static int server_on_applied_entry(struct server *s, unsigned char *entry,
				   struct session *sess)
{
	int rc = RS_OK;
	enum cmd_id c = (enum cmd_id) entry_flags(entry);

	switch (c) {
	case CMD_META:
		rc = server_on_meta(s, &s->state.meta);
		break;
	case CMD_TERM:
		rc = server_on_term_start(s, &s->state.meta);
		break;
	case CMD_REQUEST:
		rc = server_on_applied_client_req(s, sess->id,
						  sc_buf_rbuf(&sess->resp),
						  sc_buf_size(&sess->resp));
		break;

	case CMD_CONNECT:
		rc = server_on_client_connect_applied(s, sess);
		break;
	case CMD_DISCONNECT:
	case CMD_INIT:
	case CMD_INFO:
	case CMD_TIMESTAMP:
	case CMD_LOG:
		break;
	default:
		rs_abort("");
	}

	return rc;
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

static int server_process_readonly(struct server *s, struct client *c)
{
	int rc;
	struct sc_buf *buf;

	buf = conn_out(&c->conn);
	rc = state_apply_readonly(&s->state, c->id, c->msg.client_req.buf,
				  c->msg.client_req.len, buf);
	if (rc != RS_OK) {
		/**
		 * readonly requests should not fail. Even it triggers
		 * RS_FULL, it's returned as RS_OK from state as RS_FULL
		 * gives no harm on readonly requests.
		 */
		rs_abort("apply_readonly : %d.", rc);
	}

	rc = client_processed(c);
	if (rc != RS_OK) {
		goto err;
	}

	rc = conn_flush(&c->conn);
	if (rc != RS_OK) {
		goto err;
	}

	return RS_OK;
err:
	return server_on_client_disconnect(s, c, MSG_ERR);
}
static int server_check_commit(struct server *s)
{
	int rc;
	uint64_t match, round_index;
	uint32_t index = s->meta.voter / 2;
	struct client *c;
	struct sc_buf *buf;
	struct sc_list *n, *it;

	sc_array_sort(s->nodes, server_sort_matches);
	match = s->nodes[index]->match;

	rc = server_update_commit(s, match);
	if (rc != RS_OK) {
		return rc;
	}

	if (sc_list_is_empty(&s->read_reqs)) {
		return RS_OK;
	}

	sc_array_sort(s->nodes, server_sort_rounds);
	round_index = s->nodes[index]->round;

	s->round_match = sc_max(round_index, s->round_match);

	sc_list_foreach_safe (&s->read_reqs, n, it) {
		c = sc_list_entry(it, struct client, read);
		if (c->round_index > s->round_match ||
		    c->commit_index > s->commit) {
			break;
		}

		rc = server_process_readonly(s, c);
		if (rc != RS_OK) {
			return rc;
		}
	}

	return RS_OK;
}

static int server_job_add_node(struct server *s, struct server_job *job)
{
	bool b;
	int rc = RS_OK;
	const char *msg;
	struct sc_uri *uri = NULL;

	if (s->meta.prev != NULL) {
		msg = "Rejected config change, there is a pending change in progress.";
		goto err;
	}

	uri = sc_uri_create(job->data);
	if (!uri || *uri->port == '\0' || *uri->userinfo == '\0' ||
	    strcmp(uri->scheme, "tcp") != 0) {
		msg = "Invalid(url format) config change request. ";
		goto err;
	}

	b = meta_exists(&s->meta, uri->userinfo);
	if (b) {
		msg = "Node already exists";
		goto err;
	}

	meta_add(&s->meta, uri);
	rc = server_write_meta_cmd(s);

	goto out;

err:
	rc = server_err(s, "Add node[%s] : %s", job->data, msg);
out:
	sc_uri_destroy(&uri);
	return rc;
}

static int server_job_remove_node(struct server *s, struct server_job *job)
{
	bool b;
	const char *msg;
	const char *name = job->data;

	if (s->meta.prev != NULL) {
		msg = "Rejected config change, there is a pending change in progress.";
		goto err;
	}

	b = meta_exists(&s->meta, name);
	if (!b) {
		msg = "Node does not exists.";
		goto err;
	}

	meta_remove(&s->meta, name);

	return server_write_meta_cmd(s);
err:
	return server_err(s, "Remove node[%s] : %s", job->data, msg);
}

static int server_job_shutdown(struct server *s, struct server_job *job)
{
	int rc;
	struct sc_list *l;
	struct node *n;
	struct sc_buf *buf;
	const char *name = job->data;

	if (*name == '*') {
		sc_list_foreach (&s->connected_nodes, l) {
			n = sc_list_entry(l, struct node, list);

			buf = conn_out(&n->conn);
			msg_create_shutdown_req(buf, true);

			rc = conn_flush(&n->conn);
			if (rc != RS_OK) {
				rc = server_on_node_disconnect(s, n);
				if (rc != RS_OK) {
					return rc;
				}
			}
		}
	}

	if (*name == '*' || strcmp(name, s->conf.node.name) == 0) {
		return server_on_shutdown_req(s);
	}

	return RS_OK;
}

static int server_handle_jobs(struct server *s)
{
	int rc;
	struct server_job job;

	while (sc_queue_size(s->jobs) > 0) {
		job = sc_queue_del_first(s->jobs);

		switch (job.type) {
		case SERVER_JOB_ADD_NODE:
			rc = server_job_add_node(s, &job);
			break;
		case SERVER_JOB_REMOVE_NODE:
			rc = server_job_remove_node(s, &job);
			break;
		case SERVER_JOB_SHUTDOWN:
			rc = server_job_shutdown(s, &job);
			break;
		default:
			break;
		}

		sc_str_destroy(&job.data);

		/**
		 * It's okay to return here, remaining jobs will be destroyed
		 * on top level. If rc is not RS_OK, there is a serious problem
		 * anyway. e.g disk is full.
		 */
		if (rc != RS_OK) {
			return rc;
		}
	}

	return RS_OK;
}

static int server_flush_snapshot(struct server *s, struct node *n)
{
	int rc;
	bool done;
	uint32_t len;
	void *data;
	struct sc_buf *buf;

	if (n->msg_inflight > 0) {
		return RS_OK;
	}

	if (n->ss_index != s->ss.index) {
		n->ss_index = s->ss.index;
		n->ss_pos = 0;

		rc = server_warn(s, "Sending snapshot[%" PRIu64 "] to : %s",
				 n->ss_index, n->name);
		if (rc != RS_OK) {
			return rc;
		}
	}

	len = (uint32_t) sc_min(MAX_SIZE, s->ss.map.len - n->ss_pos);
	data = s->ss.map.ptr + n->ss_pos;
	done = n->ss_pos + len == s->ss.map.len;

	if (len == 0) {
		goto flush;
	}

	buf = conn_out(&n->conn);
	msg_create_snapshot_req(buf, s->meta.term, s->ss.term, s->ss.index,
				n->ss_pos, done, data, len);
	n->ss_pos += len;
	n->msg_inflight++;
	n->out_timestamp = s->timestamp;

flush:
	rc = conn_flush(&n->conn);
	if (rc != RS_OK) {
		return server_on_node_disconnect(s, n);
	}

	return RS_OK;
}

static int server_flush_nodes(struct server *s)
{
	int rc;
	uint32_t size, count;
	uint64_t prev, index;
	uint64_t timeout = s->conf.advanced.heartbeat;
	unsigned char *entries;
	struct sc_list *l, *tmp;
	struct sc_buf *b;
	struct node *n;

	sc_list_foreach_safe (&s->connected_nodes, tmp, l) {
		n = sc_list_entry(l, struct node, list);

		if (n->next <= s->store.ss_index) {
			rc = server_flush_snapshot(s, n);
			if (rc != RS_OK) {
				return rc;
			}

			continue;
		}

		if (n->msg_inflight > 0 || (n->next > s->store.last_index &&
					    s->round == s->round_prev)) {
			goto flush;
		}

		store_entries(&s->store, n->next, MAX_SIZE, &entries, &size,
			      &count);
		prev = store_prev_term(&s->store, n->next - 1);

		b = conn_out(&n->conn);
		msg_create_append_req(b, s->meta.term, n->next - 1, prev,
				      s->commit, s->round, entries, size);
		n->next += count;
		n->msg_inflight++;
		n->out_timestamp = s->timestamp;

flush:
		if (n->msg_inflight == 0 &&
		    s->timestamp - n->out_timestamp > timeout / 2) {
			index = n->next - 1;
			prev = store_prev_term(&s->store, index);

			b = conn_out(&n->conn);
			msg_create_append_req(b, s->meta.term, index, prev,
					      s->commit, s->round, NULL, 0);
			n->msg_inflight++;
			n->out_timestamp = s->timestamp;
		}

		rc = conn_flush(&n->conn);
		if (rc != RS_OK) {
			rc = server_on_node_disconnect(s, n);
			if (rc != RS_OK) {
				return rc;
			}
		}
	}

	return RS_OK;
}

static int server_flush(struct server *s)
{
	int rc;
	struct client *c;

	sc_array_foreach (s->term_clients, c) {
		client_destroy(c);
	}
	sc_array_clear(s->term_clients);

	if (s->role != SERVER_ROLE_LEADER) {
		return RS_OK;
	}

	rc = server_flush_nodes(s);
	if (rc != RS_OK) {
		return rc;
	}

	if (s->own->next <= s->store.last_index) {
		if (s->conf.advanced.fsync) {
			store_flush(&s->store);
		}
		s->own->match = s->store.last_index;
		s->own->next = s->store.last_index + 1;
	}
	s->own->round = s->round;

	rc = server_check_commit(s);
	if (rc != RS_OK) {
		return rc;
	}

	s->round_prev = s->round;

	return server_handle_jobs(s);
}

static int server_on_connect_req(struct server *s, struct sc_sock_fd *fd)
{
	int rc;
	uint32_t type;
	enum msg_rc resp_code;
	struct msg msg;
	struct sc_sock *sock = rs_entry(fd, struct sc_sock, fdt);
	struct conn *pending = rs_entry(sock, struct conn, sock);

	rc = conn_on_readable(pending);
	if (rc == SC_SOCK_ERROR) {
		resp_code = MSG_ERR;
		goto disconnect;
	}

	rc = msg_parse(&pending->in, &msg);
	if (rc == RS_INVALID) {
		resp_code = MSG_CORRUPT;
		goto disconnect;
	} else if (rc == RS_PARTIAL) {
		return RS_OK;
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

	if (s->full) {
		resp_code = MSG_DISK_FULL;
		goto disconnect;
	}

	type = msg.connect_req.flags & MSG_CONNECT_TYPE;

	switch (type) {
	case MSG_CLIENT:
		rc = server_on_client_connect_req(s, pending, &msg.connect_req);
		break;
	case MSG_NODE:
		rc = server_on_node_connect_req(s, pending, &msg.connect_req);
		break;
	default:
		resp_code = MSG_CORRUPT;
		goto disconnect;
	}

	return rc;

disconnect:
	server_on_pending_disconnect(s, pending, resp_code);
	return RS_OK;
}

static void *server_run(void *arg)
{
	int rc;
	int events = 0, retry = 1, timeout;
	uint32_t event;
	struct sc_sock_fd *fd;
	struct server *s = arg;
	struct conf conf = s->conf;

	sc_log_set_thread_name(conf.node.name);

	rc = metric_init(&s->metric, conf.node.dir);
	if (rc != RS_OK) {
		rs_exit("metric init failed.");
	}

	rc = server_prepare_start(s);
	if (rc != RS_OK) {
		if (rc == RS_ERROR) {
			rs_exit("Failed to start server, shutting down. \n");
		}

		if (rc == RS_FULL) {
			server_on_full_disk(s);
		}
	}

	if (conf.cmdline.systemd) {
		rc = sc_sock_notify_systemd("READY=1\n");
		if (rc != 0) {
			rs_exit("systemd failed : %s \n", strerror(errno));
		}
	}

	sc_log_info("Resql[v%s] has been started.. \n", RS_VERSION);

	while (!s->stop_requested) {
		s->timestamp = sc_time_mono_ms();

		timeout = (int) sc_timer_timeout(&s->timer, s->timestamp, s,
						 server_timeout);
		if (s->timer_rc == RS_ERROR) {
			rs_exit("server_timeout failed");
		}

		if (s->timer_rc == RS_FULL) {
			s->timer_rc = RS_OK;
			server_on_full_disk(s);
		}

		retry = retry > 0 ? retry - 1 : 0;

		events = sc_sock_poll_wait(&s->poll, retry > 0 ? 0 : timeout);
		if (events < 0) {
			rs_exit("poll : %s \n", sc_sock_poll_err(&s->poll));
		}

		rc = RS_OK;

		for (int i = 0; i < events; i++) {

			retry = 100;
			fd = sc_sock_poll_data(&s->poll, i);
			event = sc_sock_poll_event(&s->poll, i);

			switch (fd->type) {
			case SERVER_FD_NODE_RECV:
				rc = server_on_node_recv(s, fd, event);
				break;
			case SERVER_FD_CLIENT_RECV:
				rc = server_on_client_recv(s, fd, event);
				break;
			case SERVER_FD_INCOMING_CONN:
				rc = server_on_incoming_conn(s, fd);
				break;
			case SERVER_FD_OUTGOING_CONN:
				rc = server_on_outgoing_conn(s, fd);
				break;
			case SERVER_FD_WAIT_FIRST_REQ:
				rc = server_on_connect_req(s, fd);
				break;
			case SERVER_FD_WAIT_FIRST_RESP:
				rc = server_on_connect_resp(s, fd);
				break;
			case SERVER_FD_TASK:
				rc = server_on_task(s);
				break;
			case SERVER_FD_SIGNAL:
				rc = server_on_signal(s);
				break;
			default:
				rs_abort("fd type : %d \n", fd->type);
			}

			if (rc != RS_OK) {
				break;
			}
		}

		if (rc == RS_FULL) {
			server_on_full_disk(s);
		} else if (rc == RS_ERROR) {
			rs_exit("failed with code : %d ", rc);
		}

		rc = server_flush(s);
		if (rc == RS_FULL) {
			server_on_full_disk(s);
		} else if (rc == RS_ERROR) {
			rs_exit("failed with code : %d ", rc);
		}
	}

	sc_log_info("Resql[%s] is shutting down \n", s->conf.node.name);

	if (s->conf.cmdline.systemd) {
		sc_sock_notify_systemd("STATUS=Resql is shutting down.\n");
	}

	server_term(s);

	return (void *) RS_OK;
}

int server_start_now(struct conf *c)
{
	int rc;
	struct server *s;

	s = rs_calloc(1, sizeof(*s));
	s->conf = *c;
	server_init(s);

	rc = (int) (uintptr_t) server_run(&s->thread);
	rs_free(s);

	return rc;
}

struct server *server_start(struct conf *c)
{
	int rc;

	struct server *s;

	s = rs_calloc(1, sizeof(*s));

	s->conf = *c;
	server_init(s);

	rc = sc_thread_start(&s->thread, server_run, s);
	if (rc != 0) {
		sc_thread_term(&s->thread);
		goto error;
	}

	return s;
error:
	rs_free(s);
	return NULL;
}

int server_stop(struct server *s)
{
	int rc, ret = RS_OK;

	rc = sc_sock_pipe_write(&s->efd, &(char){1}, 1);
	if (rc != 1) {
		sc_log_error("sc_sock_pipe_write : %s \n", strerror(errno));
		ret = RS_ERROR;
	}

	rc = (int) (intptr_t) sc_thread_term(&s->thread);
	if (rc != 0) {
		sc_log_error("thread_term : %s \n", sc_thread_err(&s->thread));
		ret = RS_ERROR;
	}

	rs_free(s);
	return ret;
}
