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

#include "server.h"
#include "rs.h"
#include "test_util.h"

#include "c/resql.h"
#include "sc/sc_log.h"

#include <unistd.h>

struct resql *create_client(const char *name)
{
    const char *uris =
            "tcp://127.0.0.1:7600 tcp://127.0.0.1:7601 tcp://127.0.0.1:7602";

    struct resql_config settings = {.cluster_name = "cluster",
            .client_name = name,
            .timeout = 10000,
            .uris = uris};

    struct resql *client;

    int rc = resql_create(&client, &settings);
    if (rc != RS_OK) {
        printf("Failed to connect to server \n");
        return NULL;
    } else {
        printf("Connected to server \n");
    }

    return client;
}

struct server *create_node_0()
{
    char *options[] = {""};

    struct settings settings;

    settings_init(&settings);
    settings_read_cmdline(&settings, sizeof(options) / sizeof(char *), options);

    sc_str_set(&settings.node.log_level, "DEBUG");
    sc_str_set(&settings.node.name, "node0");
    sc_str_set(&settings.node.bind_uri,
               "tcp://node0@127.0.0.1:7600 unix:///tmp/var");
    sc_str_set(&settings.node.ad_uri, "tcp://node0@127.0.0.1:7600");
    sc_str_set(&settings.cluster.nodes, "tcp://node0@127.0.0.1:7600");
    sc_str_set(&settings.node.dir, "/tmp/node0");
    settings.node.in_memory = true;

    struct server *server = server_create(&settings);

    int rc = server_start(server, true);
    if (rc != RS_OK) {
        abort();
    }

    return server;
}

static void restart_simple()
{
    file_clear_dir("/tmp/node0", ".resql");

    int rc;
    struct resql_column* row;
    struct server *s1 = create_node_0();
    struct resql *client = create_client("clientim");

    if (!s1 || !client) {
        rs_abort("");
    }

    struct resql_result *rs = NULL;

    for (int i = 0; i < 100; i++) {

        resql_put_sql(client, "Select 'resql'");

        rc = resql_exec(client, true, &rs);
        if (rc != RESQL_OK) {
            rs_abort("");
        }

        do {
            assert(resql_column_count(rs) == 1);
            row = resql_row(rs);

            while (resql_row(rs)) {
                assert(row[0].type == RESQL_TEXT);
                assert(strcmp("resql", row[0].text) == 0);
            }
        } while (resql_next(rs));
    }

    rc = server_stop(s1);
    if (rc != RS_OK) {
        abort();
    }

    s1 = create_node_0();
    if (!s1) {
        rs_abort("");
    }

    for (int i = 0; i < 100; i++) {
        resql_put_sql(client, "Select 'resql'");

        rc = resql_exec(client, false, &rs);
        if (rc != RESQL_OK) {
            rs_abort("");
        }

        do {
            assert(resql_column_count(rs) == 1);
            row = resql_row(rs);

            while (resql_row(rs)) {
                assert(row[0].type == RESQL_TEXT);
                assert(strcmp("resql", row[0].text) == 0);
            }
        } while (resql_next(rs));
    }

    rc = resql_destroy(client);
    if (rc != RS_OK) {
        abort();
    }

    rc = server_stop(s1);
    if (rc != RS_OK) {
        abort();
    }
}

static void restart_simple2()
{
    file_clear_dir("/tmp/node0", ".resql");

    int rc;
    struct resql_column* row;
    struct server *s1 = create_node_0();
    struct resql *client = create_client("clientim");

    if (!s1 || !client) {
        rs_abort("");
    }

    struct resql_result *rs = NULL;

    for (int i = 0; i < 100; i++) {

        resql_put_sql(client, "Select 'resql'");

        rc = resql_exec(client, true, &rs);
        if (rc != RESQL_OK) {
            rs_abort("");
        }

        do {
            assert(resql_column_count(rs) == 1);
            row = resql_row(rs);

            while (resql_row(rs)) {
                assert(row[0].type == RESQL_TEXT);
                assert(strcmp("resql", row[0].text) == 0);
            }
        } while (resql_next(rs));
    }

    rc = server_stop(s1);
    if (rc != RS_OK) {
        abort();
    }

    rc = resql_destroy(client);
    if (rc != RS_OK) {
        abort();
    }

    s1 = create_node_0();
    if (!s1) {
        rs_abort("");
    }

    rc = server_stop(s1);
    if (rc != RS_OK) {
        abort();
    }
}

int main(void)
{
    test_execute(restart_simple);
    test_execute(restart_simple2);

    return 0;
}
