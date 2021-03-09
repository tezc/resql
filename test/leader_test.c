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

#include "test_util.h"

#include "c/resql.h"
#include "sc/sc_uri.h"

#include <conf.h>
#include <server.h>
#include <stdio.h>
#include <unistd.h>

struct server *create_node_0()
{
    char *options[] = {"", "-e"};

    int rc;
    struct conf settings;
    struct server *server;

    conf_init(&settings);
    conf_read_config(&settings, false, sizeof(options) / sizeof(char *), options);

    sc_str_set(&settings.node.log_level, "DEBUG");
    sc_str_set(&settings.node.name, "node0");
    sc_str_set(&settings.node.bind_url, "tcp://node0@127.0.0.1:7600");
    sc_str_set(&settings.node.ad_url, "tcp://node0@127.0.0.1:7600");
    sc_str_set(&settings.cluster.nodes, "tcp://node0@127.0.0.1:7600 tcp://node1@127.0.0.1:7601 tcp://node2@127.0.0.1:7602");
    sc_str_set(&settings.node.dir, "/tmp/node0");
    settings.node.in_memory = true;

    server = server_create(&settings);

    rc = server_start(server, true);
    if (rc != RS_OK) {
        abort();
    }

    return server;
}

struct server *create_node_1()
{
    char *options[] = {"", "-e"};

    int rc;
    struct conf settings;
    struct server *server;

    conf_init(&settings);
    conf_read_config(&settings, false, sizeof(options) / sizeof(char *), options);

    sc_str_set(&settings.node.log_level, "DEBUG");
    sc_str_set(&settings.node.name, "node1");
    sc_str_set(&settings.node.bind_url, "tcp://node1@127.0.0.1:7601");
    sc_str_set(&settings.node.ad_url, "tcp://node1@127.0.0.1:7601");
    sc_str_set(&settings.cluster.nodes, "tcp://node0@127.0.0.1:7600 tcp://node1@127.0.0.1:7601 tcp://node2@127.0.0.1:7602");
    sc_str_set(&settings.node.dir, "/tmp/node1");
    settings.node.in_memory = true;

    server = server_create(&settings);

    rc = server_start(server, true);
    if (rc != RS_OK) {
        abort();
    }

    return server;
}

struct server *create_node_2()
{
    char *options[] = {"", "-e"};

    int rc;
    struct conf settings;
    struct server *server;

    conf_init(&settings);
    conf_read_config(&settings, false, sizeof(options) / sizeof(char *), options);

    sc_str_set(&settings.node.log_level, "DEBUG");
    sc_str_set(&settings.node.name, "node2");
    sc_str_set(&settings.node.bind_url, "tcp://node2@127.0.0.1:7602");
    sc_str_set(&settings.node.ad_url, "tcp://node2@127.0.0.1:7602");
    sc_str_set(&settings.cluster.nodes, "tcp://node0@127.0.0.1:7600 tcp://node1@127.0.0.1:7601 tcp://node2@127.0.0.1:7602");
    sc_str_set(&settings.node.dir, "/tmp/node2");
    settings.node.in_memory = true;

    server = server_create(&settings);

    rc = server_start(server, true);
    if (rc != RS_OK) {
        abort();
    }

    return server;
}

void write_test()
{
    int rc;
    resql *c;
    resql_result *rs;
    const char *uris =
            "tcp://127.0.0.1:7600 tcp://127.0.0.1:7601 tcp://127.0.0.1:7602";

    struct server *s0, *s1, *s2;

    s0 = create_node_0();
    s1 = create_node_1();
    s2 = create_node_2();

    struct resql_config settings = {.cluster_name = "cluster",
            .client_name = "any",
            .timeout_millis = 50000,
            .urls = uris};

    rc = resql_create(&c, &settings);
    assert(rc == RESQL_OK);

    resql_put_sql(c, "DROP TABLE IF EXISTS snapshot;");
    resql_put_sql(c, "CREATE TABLE snapshot (key TEXT, value TEXT);");

    rc = resql_exec(c, false, &rs);
    client_assert(c, rc == RESQL_OK);

    for (int i = 0; i < 1000; i++) {
        char tmp[32];
        snprintf(tmp, sizeof(tmp), "%d", i);

        resql_put_sql(c, "INSERT INTO snapshot VALUES(:key, 'value')");
        resql_bind_param_text(c, ":key", tmp);

        rc = resql_exec(c, false, &rs);
        if (rc != RESQL_OK) {
            rs_abort("");
        }
    }

    //server_stop(s0);

    for (int i = 1000; i < 2000; i++) {
        char tmp[32];
        snprintf(tmp, sizeof(tmp), "%d", i);

        resql_put_sql(c, "INSERT INTO snapshot VALUES(:key, 'value')");
        resql_bind_param_text(c, ":key", tmp);

        rc = resql_exec(c, false, &rs);
        if (rc != RESQL_OK) {
            rs_abort("");
        }
    }


    resql_shutdown(c);
    server_stop(s0);
    server_stop(s1);
    server_stop(s2);
}

int main()
{
    test_execute(write_test);
}
