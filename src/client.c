/*
 *  reSQL
 *
 *  Copyright (C) 2021 reSQL Authors
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
#include "rs.h"
#include "server.h"

#include "sc/sc_log.h"

struct client *client_create(struct conn *conn, const char *name)
{
    struct client *client;

    client = rs_calloc(1, sizeof(*client));

    client->name = sc_str_create(name);
    client->msg_wait = false;

    conn_move(&client->conn, conn);
    sc_list_init(&client->list);
    sc_list_init(&client->read);

    conn_set_type(&client->conn, SERVER_FD_CLIENT_RECV);
    conn_disallow_read(&client->conn);

    return client;
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
    sc_list_clear(&c->read);
    conn_allow_read(&c->conn);
}

void client_print(struct client *c, char *buf, int len)
{
    int rc;

    rc = snprintf(buf, len, "Client : %s, ", c->name);
    if (rc < 0 || rc >= len) {
        *buf = '\0';
        return;
    }

    sc_sock_print(&c->conn.sock, buf + rc, len - rc);
}

bool client_pending(struct client *c)
{
    return c->msg_wait || sc_buf_size(&c->conn.out);
}
