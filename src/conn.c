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

#include "conn.h"

#include "metric.h"
#include "rs.h"
#include "server.h"

#include "sc/sc_uri.h"

#include <sc/sc_log.h>

struct conn *conn_create(struct server *server, struct sc_sock *s)
{
	int rc;
	struct conn *c;
	struct sc_sock_poll *p = &server->poll;

	c = rs_malloc(sizeof(*c));
	conn_init(c, server);

	c->sock = *s;
	c->state = CONN_CONNECTED;
	c->sock.fdt.type = SERVER_FD_WAIT_FIRST_REQ;

	rc = sc_sock_poll_add(p, &c->sock.fdt, SC_SOCK_READ, &c->sock.fdt);
	if (rc != 0) {
		goto poll_err;
	}

	sc_sock_local_str(s, c->local, sizeof(c->local));
	sc_sock_remote_str(s, c->remote, sizeof(c->remote));

	return c;

poll_err:
	sc_log_error("sc_sock_poll_add : %s \n", sc_sock_poll_err(p));
	sc_sock_term(s);
	rs_free(c);

	return NULL;
}

void conn_destroy(struct conn *c)
{
	conn_term(c);
	rs_free(c);
}

void conn_init(struct conn *c, struct server *s)
{
	c->server = s;
	c->state = CONN_DISCONNECTED;
	c->timer_id = SC_TIMER_INVALID;

	sc_list_init(&c->list);
	sc_sock_init(&c->sock, 0, 0, 0);

	c->in = (struct sc_buf){0};
	c->out = (struct sc_buf){0};
}

void conn_clear_inbuf(struct conn *c)
{
	assert(sc_buf_cap(&c->in) != 0);
	server_buf_free(c->server, c->in);
	c->in = (struct sc_buf){0};
}

void conn_clear_bufs(struct conn *c)
{
	if (sc_buf_cap(&c->in) != 0 && sc_buf_size(&c->in) == 0) {
		server_buf_free(c->server, c->in);
		c->in = (struct sc_buf){0};
	}

	if (sc_buf_cap(&c->out) != 0 && sc_buf_size(&c->out) == 0) {
		server_buf_free(c->server, c->out);
		c->out = (struct sc_buf){0};
	}
}

struct sc_buf *conn_outbuf(struct conn *c)
{
	if (sc_buf_cap(&c->out) == 0) {
		c->out = server_buf_alloc(c->server);
	}

	return &c->out;
}

static void conn_cleanup_sock(struct conn *c)
{
	int rc;
	struct sc_sock *s = &c->sock;
	struct sc_sock_poll *p = &c->server->poll;

	rc = sc_sock_poll_del(p, &s->fdt, SC_SOCK_READ | SC_SOCK_WRITE, NULL);
	if (rc != 0) {
		sc_log_error("conn_term : %s \n", sc_sock_poll_err(p));
	}

	rc = sc_sock_term(s);
	if (rc != 0) {
		sc_log_error("conn_term : %s \n", sc_sock_error(s));
	}

	c->state = CONN_DISCONNECTED;
}

void conn_term(struct conn *c)
{
	sc_timer_cancel(&c->server->timer, &c->timer_id);
	conn_cleanup_sock(c);

	if (sc_buf_cap(&c->in) != 0) {
		server_buf_free(c->server, c->in);
		c->in = (struct sc_buf){0};
	}

	if (sc_buf_cap(&c->out) != 0) {
		server_buf_free(c->server, c->out);
		c->out = (struct sc_buf){0};
	}
}

void conn_move(struct conn *c, struct conn *src)
{
	*c = *src;
	sc_list_init(&c->list);
}

void conn_schedule(struct conn *c, uint32_t type, uint32_t timeout)
{
	struct sc_timer *t = &c->server->timer;

	sc_timer_cancel(t, &c->timer_id);
	c->timer_id = sc_timer_add(t, timeout, type, c);
}

void conn_clear_timer(struct conn *c)
{
	struct sc_timer *t = &c->server->timer;

	sc_timer_cancel(t, &c->timer_id);
}

int conn_try_connect(struct conn *c, struct sc_uri *uri)
{
	int rc;
	struct sc_sock_poll *p = &c->server->poll;

	if (c->state == CONN_CONNECTED) {
		return RS_EXISTS;
	}

	if (c->state == CONN_TCP_ATTEMPT) {
		conn_cleanup_sock(c);
	}

	c->state = CONN_TCP_ATTEMPT;
	sc_sock_init(&c->sock, SERVER_FD_OUTGOING_CONN, false, SC_SOCK_INET);

	rc = sc_sock_connect(&c->sock, uri->host, uri->port, NULL, NULL);

	switch (rc) {
	case SC_SOCK_OK:
		c->state = CONN_CONNECTED;
		c->sock.fdt.type = SERVER_FD_WAIT_FIRST_RESP;
		sc_sock_poll_add(p, &c->sock.fdt, SC_SOCK_READ, &c->sock.fdt);

		sc_sock_local_str(&c->sock, c->local, sizeof(c->local));
		sc_sock_remote_str(&c->sock, c->remote, sizeof(c->remote));
		rc = RS_OK;
		break;
	case SC_SOCK_WANT_WRITE:
		sc_sock_poll_add(p, &c->sock.fdt, SC_SOCK_WRITE, &c->sock.fdt);
		rc = RS_INPROGRESS;
		break;
	default:
		rc = RS_ERROR;
		break;
	}

	return rc;
}

int conn_on_out_connected(struct conn *c)
{
	int rc;
	struct sc_sock_poll *p = &c->server->poll;

	sc_timer_cancel(&c->server->timer, &c->timer_id);

	rc = sc_sock_finish_connect(&c->sock);
	if (rc != SC_SOCK_OK) {
		return RS_ERROR;
	}

	conn_clear_events(c);

	rc = sc_sock_poll_add(p, &c->sock.fdt, SC_SOCK_READ, &c->sock.fdt);
	if (rc != 0) {
		return RS_ERROR;
	}

	c->state = CONN_CONNECTED;
	c->sock.fdt.type = SERVER_FD_WAIT_FIRST_RESP;

	sc_sock_local_str(&c->sock, c->local, sizeof(c->local));
	sc_sock_remote_str(&c->sock, c->remote, sizeof(c->remote));

	return RS_OK;
}

void conn_disconnect(struct conn *c)
{
	sc_timer_cancel(&c->server->timer, &c->timer_id);

	if (c->state != CONN_DISCONNECTED) {
		conn_cleanup_sock(c);
	}

	sc_buf_clear(&c->in);
	sc_buf_clear(&c->out);

	conn_clear_bufs(c);
}

int conn_on_writable(struct conn *c)
{
	int rc;
	struct sc_sock_fd *fdt = &c->sock.fdt;
	struct sc_sock_poll *p = &c->server->poll;

	rc = sc_sock_poll_del(p, fdt, SC_SOCK_WRITE, fdt);
	if (rc != 0) {
		sc_log_error("sc_sock_poll_del : %s \n", sc_sock_poll_err(p));
	}

	return conn_flush(c);
}

int conn_on_readable(struct conn *c)
{
	bool success;
	int rc, cap;
	void *buf;

	if (sc_buf_cap(&c->in) == 0) {
		c->in = server_buf_alloc(c->server);
	}

	sc_buf_compact(&c->in);

retry:
	buf = sc_buf_wbuf(&c->in);
	cap = sc_buf_quota(&c->in);

	if (cap == 0) {
		success = sc_buf_reserve(&c->in, c->in.cap * 2);
		if (!success) {
			return RS_ERROR;
		}
		goto retry;
	}

	rc = sc_sock_recv(&c->sock, buf, cap, 0);
	if (rc > 0) {
		sc_buf_mark_write(&c->in, (uint32_t) rc);
		metric_recv(rc);
	}

	return rc != SC_SOCK_ERROR ? RS_OK : RS_ERROR;
}

void conn_allow_read(struct conn *c)
{
	int rc;
	struct sc_sock_poll *p = &c->server->poll;

	rc = sc_sock_poll_add(p, &c->sock.fdt, SC_SOCK_READ, &c->sock.fdt);
	if (rc != 0) {
		sc_log_error("sc_sock_poll_add : %s \n", sc_sock_poll_err(p));
	}
}

void conn_disallow_read(struct conn *c)
{
	int rc;
	struct sc_sock_fd *fdt = &c->sock.fdt;
	struct sc_sock_poll *p = &c->server->poll;

	rc = sc_sock_poll_del(p, fdt, SC_SOCK_READ, fdt);
	if (rc != 0) {
		sc_log_error("sc_sock_poll_del : %s \n", sc_sock_poll_err(p));
	}
}

void conn_set_type(struct conn *c, int type)
{
	c->sock.fdt.type = type;
}

void conn_clear_events(struct conn *c)
{
	int rc;
	struct sc_sock_fd *fdt = &c->sock.fdt;
	struct sc_sock_poll *p = &c->server->poll;

	rc = sc_sock_poll_del(p, fdt, fdt->op, fdt);
	if (rc != 0) {
		sc_log_error("sc_sock_poll_del : %s \n", sc_sock_poll_err(p));
	}
}

int conn_flush(struct conn *c)
{
	int rc, len;
	void *buf;
	struct sc_sock_poll *p = &c->server->poll;

	if (!sc_buf_valid(&c->out)) {
		return RS_ERROR;
	}

retry:
	len = sc_buf_size(&c->out);
	if (len == 0) {
		goto out;
	}

	buf = sc_buf_rbuf(&c->out);

	rc = sc_sock_send(&c->sock, buf, len, 0);
	if (rc == SC_SOCK_ERROR) {
		return RS_ERROR;
	}

	if (rc == SC_SOCK_WANT_WRITE) {
		rc = sc_sock_poll_add(p, &c->sock.fdt, SC_SOCK_WRITE,
				      &c->sock.fdt);
		if (rc != 0) {
			sc_log_error("poll_add %s \n", sc_sock_poll_err(p));
			return RS_ERROR;
		}

		goto out;
	}

	metric_send(rc);
	sc_buf_mark_read(&c->out, (uint32_t) rc);

	if (sc_buf_size(&c->out) > 0) {
		goto retry;
	}

out:
	conn_clear_bufs(c);

	return RS_OK;
}
