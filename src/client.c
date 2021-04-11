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

#include "client.h"

#include "rs.h"
#include "server.h"

struct client *client_create(struct conn *conn, const char *name)
{
	struct client *c;

	c = rs_calloc(1, sizeof(*c));

	c->name = sc_str_create(name);
	c->msg_wait = false;

	conn_move(&c->conn, conn);
	sc_list_init(&c->list);
	sc_list_init(&c->read);

	conn_set_type(&c->conn, SERVER_FD_CLIENT_RECV);
	conn_disallow_read(&c->conn);

	return c;
}

void client_destroy(struct client *c)
{
	sc_list_del(NULL, &c->read);
	sc_str_destroy(c->name);
	conn_term(&c->conn);
	rs_free(c);
}

void client_processed(struct client *c)
{
	c->msg_wait = false;
	sc_list_del(NULL, &c->read);
	conn_allow_read(&c->conn);
	conn_clear_bufs(&c->conn);
}

void client_print(struct client *c, char *buf, size_t len)
{
	int rc;

	rc = rs_snprintf(buf, len, "Client : %s, ", c->name);
	sc_sock_print(&c->conn.sock, buf + rc, len - rc);
}

bool client_pending(struct client *c)
{
	return c->msg_wait || sc_buf_size(&c->conn.out);
}
