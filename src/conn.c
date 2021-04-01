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

#include "conn.h"

#include "metric.h"
#include "rs.h"
#include "server.h"

#include "sc/sc_uri.h"

struct conn *conn_create(struct sc_sock_poll *loop, struct sc_timer *timer,
                         struct sc_sock *sock)
{
    struct conn *c;

    c = rs_malloc(sizeof(*c));
    conn_init(c, loop, timer);

    c->sock = *sock;
    c->state = CONN_CONNECTED;
    c->sock.fdt.type = SERVER_FD_WAIT_FIRST_REQ;

    sc_sock_poll_add(loop, &c->sock.fdt, SC_SOCK_READ, &c->sock.fdt);
    sc_sock_local_str(sock, c->local, sizeof(c->local));
    sc_sock_remote_str(sock, c->remote, sizeof(c->remote));

    return c;
}

void conn_destroy(struct conn *c)
{
    conn_term(c);
    rs_free(c);
}

void conn_init(struct conn *c, struct sc_sock_poll *poll,
               struct sc_timer *timer)
{
    c->poll = poll;
    c->timer = timer;
    c->state = CONN_DISCONNECTED;
    c->timer_id = SC_TIMER_INVALID;

    sc_list_init(&c->list);
    sc_buf_init(&c->in, SC_SOCK_BUF_SIZE);
    sc_buf_init(&c->out, SC_SOCK_BUF_SIZE);
    sc_sock_init(&c->sock, 0, 0, 0);
}

void conn_term(struct conn *c)
{
    sc_timer_cancel(c->timer, &c->timer_id);
    sc_sock_poll_del(c->poll, &c->sock.fdt, SC_SOCK_READ | SC_SOCK_WRITE, NULL);
    sc_sock_term(&c->sock);
    sc_buf_term(&c->in);
    sc_buf_term(&c->out);
}

void conn_move(struct conn *c, struct conn *src)
{
    *c = *src;
    sc_list_init(&c->list);
}

void conn_schedule(struct conn *c, uint32_t type, uint32_t timeout)
{
    sc_timer_cancel(c->timer, &c->timer_id);
    c->timer_id = sc_timer_add(c->timer, timeout, type, c);
}

void conn_clear_timer(struct conn *c)
{
    sc_timer_cancel(c->timer, &c->timer_id);
}

int conn_try_connect(struct conn *c, struct sc_uri *uri)
{
    int rc;

    if (c->state == CONN_CONNECTED) {
        return RS_EXISTS;
    }

    if (c->state == CONN_TCP_ATTEMPT) {
        sc_sock_poll_del(c->poll, &c->sock.fdt, c->sock.fdt.op, &c->sock.fdt);
        sc_sock_term(&c->sock);
    }

    c->state = CONN_TCP_ATTEMPT;
    sc_sock_init(&c->sock, SERVER_FD_OUTGOING_CONN, false, SC_SOCK_INET);

    rc = sc_sock_connect(&c->sock, uri->host, uri->port, NULL, NULL);

    switch (rc) {
    case SC_SOCK_OK:
        c->state = CONN_CONNECTED;
        c->sock.fdt.type = SERVER_FD_WAIT_FIRST_RESP;
        sc_sock_poll_add(c->poll, &c->sock.fdt, SC_SOCK_READ, &c->sock.fdt);

        sc_sock_local_str(&c->sock, c->local, sizeof(c->local));
        sc_sock_remote_str(&c->sock, c->remote, sizeof(c->remote));
        rc = RS_OK;

        break;
    case SC_SOCK_WANT_WRITE:
        sc_sock_poll_add(c->poll, &c->sock.fdt, SC_SOCK_WRITE, &c->sock.fdt);
        rc = RS_ERROR;
        break;
    default:
        break;
    }

    return rc;
}

int conn_on_out_connected(struct conn *c)
{
    int rc;

    sc_timer_cancel(c->timer, &c->timer_id);

    rc = sc_sock_finish_connect(&c->sock);
    if (rc == SC_SOCK_ERROR) {
        return RS_ERROR;
    }

    c->state = CONN_CONNECTED;
    c->sock.fdt.type = SERVER_FD_WAIT_FIRST_RESP;

    conn_clear_events(c);
    sc_sock_poll_add(c->poll, &c->sock.fdt, SC_SOCK_READ, &c->sock.fdt);
    sc_sock_local_str(&c->sock, c->local, sizeof(c->local));
    sc_sock_remote_str(&c->sock, c->remote, sizeof(c->remote));

    return RS_OK;
}

void conn_disconnect(struct conn *c)
{
    sc_timer_cancel(c->timer, &c->timer_id);

    if (c->state != CONN_DISCONNECTED) {
        sc_buf_clear(&c->out);
        sc_buf_clear(&c->in);

        sc_sock_poll_del(c->poll, &c->sock.fdt, c->sock.fdt.op, NULL);
        sc_sock_term(&c->sock);
        c->state = CONN_DISCONNECTED;
    }
}

int conn_on_writable(struct conn *c)
{
    sc_sock_poll_del(c->poll, &c->sock.fdt, SC_SOCK_WRITE, &c->sock.fdt);
    return conn_flush(c);
}

int conn_on_readable(struct conn *c)
{
    int rc, cap;
    void *buf;

    sc_buf_compact(&c->in);

retry:
    buf = sc_buf_wbuf(&c->in);
    cap = sc_buf_quota(&c->in);

    if (cap == 0) {
        bool b = sc_buf_reserve(&c->in, c->in.cap * 2);
        if (!b) {
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
    struct sc_sock_fd *fdt = &c->sock.fdt;
    sc_sock_poll_add(c->poll, fdt, SC_SOCK_READ, fdt);
}

void conn_disallow_read(struct conn *c)
{
    struct sc_sock_fd *fdt = &c->sock.fdt;
    sc_sock_poll_del(c->poll, fdt, SC_SOCK_READ, fdt);
}

void conn_set_type(struct conn *c, int type)
{
    c->sock.fdt.type = type;
}

void conn_clear_events(struct conn *c)
{
    struct sc_sock_fd *fdt = &c->sock.fdt;
    sc_sock_poll_del(c->poll, fdt, c->sock.fdt.op, fdt);
}

int conn_flush(struct conn *c)
{
    int rc, len;
    void *buf;
    struct sc_sock_fd *fdt;

    if (!sc_buf_valid(&c->out)) {
        return RS_ERROR;
    }

retry:
    len = sc_buf_size(&c->out);
    buf = sc_buf_rbuf(&c->out);

    if (len == 0) {
        goto out;
    }

    rc = sc_sock_send(&c->sock, buf, len, 0);
    if (rc == SC_SOCK_ERROR) {
        return RS_ERROR;
    }

    if (rc == SC_SOCK_WANT_WRITE) {
        fdt = &c->sock.fdt;
        sc_sock_poll_add(c->poll, fdt, SC_SOCK_WRITE, fdt);
        goto out;
    }

    metric_send(rc);
    sc_buf_mark_read(&c->out, (uint32_t) rc);

    if (sc_buf_size(&c->out) > 0) {
        goto retry;
    }

out:
    sc_buf_compact(&c->out);
    return RS_OK;
}
