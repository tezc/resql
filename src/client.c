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

#include "client.h"

#include "server.h"

struct client *client_create(struct conn *conn, const char *name)
{
	int rc;
	struct client *c;

	c = rs_calloc(1, sizeof(*c));

	rc = conn_set(&c->conn, conn);
	if (rc != RS_OK) {
		goto err;
	}

	conn_set_type(&c->conn, SERVER_FD_CLIENT_RECV);

	c->name = sc_str_create(name);

	/**
	 * This function is called when we receive CONNECT_REQ. So, client is
	 * waiting CONNECT_RESP which will be sent later when new connection is
	 * applied to the state.
	 */
	c->msg_wait = true;

	sc_list_init(&c->read);
	conn_clear_buf(&c->conn);

	return c;
err:
	rs_free(c);
	return NULL;
}

void client_destroy(struct client *c)
{
	sc_list_del(NULL, &c->read);
	sc_str_destroy(&c->name);
	conn_term(&c->conn);
	rs_free(c);
}

int client_processed(struct client *c)
{
	c->msg_wait = false;
	sc_list_del(NULL, &c->read);
	conn_clear_buf(&c->conn);

	return conn_register(&c->conn, true, false);
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

void client_set_terminated(struct client *c)
{
	c->terminated = true;
	conn_clear_buf(&c->conn);
	sc_list_del(NULL, &c->read);
}
