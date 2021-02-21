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

#include <server.h>
#include <settings.h>
#include <stdio.h>
#include <unistd.h>

static struct server* s[16];

static char *names[] = {
        "node0",
        "node1",
        "node2",
        "node3",
        "node4",
        "node5",
        "node6"
};

static char* binds[] = {
        "tcp://node0@127.0.0.1:7600 unix:///tmp/var0",
        "tcp://node1@127.0.0.1:7601 unix:///tmp/var1",
        "tcp://node2@127.0.0.1:7602 unix:///tmp/var2",
        "tcp://node3@127.0.0.1:8083 unix:///tmp/var3",
        "tcp://node4@127.0.0.1:8084 unix:///tmp/var4",
        "tcp://node5@127.0.0.1:8085 unix:///tmp/var5",
        "tcp://node6@127.0.0.1:8086 unix:///tmp/var6",
};

static char* ads[] = {
        "tcp://node0@127.0.0.1:7600",
        "tcp://node1@127.0.0.1:7601",
        "tcp://node2@127.0.0.1:7602",
        "tcp://node3@127.0.0.1:8083",
        "tcp://node4@127.0.0.1:8084",
        "tcp://node5@127.0.0.1:8085",
        "tcp://node6@127.0.0.1:8086",
};

static char* dirs[] = {
        "/tmp/node0",
        "/tmp/node1",
        "/tmp/node2",
        "/tmp/node3",
        "/tmp/node4",
        "/tmp/node5",
        "/tmp/node6",
};

struct server* servers[7];

struct server* create_server(int id, int init)
{
    char nodes[1024] = {""};
    char *options[] = {"", "-e"};

    struct config config;

    config_init(&config);
    config_read_cmdline(&config, sizeof(options) / sizeof(char *), options);

    sc_str_set(&config.cluster.name, "cluster");
    sc_str_set(&config.node.log_level, "DEBUG");
    config.node.in_memory = true;

    sc_str_set(&config.node.name, names[id]);
    sc_str_set(&config.node.bind_url, binds[id]);
    sc_str_set(&config.node.ad_url, ads[id]);
    sc_str_set(&config.node.dir, dirs[id]);

    if (init == 0) {
        strcat(nodes, ads[id]);
    } else {
        for (int i = 0; i < init; i++) {
            strcat(nodes, ads[i]);
            strcat(nodes, " ");
        }
    }

    sc_str_set(&config.cluster.nodes, nodes);

    struct server *server = server_create(&config);

    int rc = server_start(server, true);
    if (rc != RS_OK) {
        abort();
    }

    servers[id] = server;

    return server;
}

void stop_servers()
{
    for (int i =0 ; i< 7; i++) {
        if (servers[i] != NULL) {
            server_stop(servers[i]);
            server_destroy(servers[i]);
        }
    }
}

struct server* create_servers(int n, int init)
{
    for (int i =0 ; i < n; i++) {
        s[i] = create_server(i, init);
    }

    return (struct server*) s;
}

void test()
{
    int rc;
    resql *c;
    resql_result *rs;
    const char *urls =
            "tcp://127.0.0.1:7600 tcp://127.0.0.1:7601 tcp://127.0.0.1:7602";

    create_servers(2, 2);

    struct resql_config config = {.cluster_name = "cluster",
            .client_name = "any",
            .timeout = 10000,
            .urls = urls};

    resql_create(&c, &config);

    resql_put_statement(c, "SELECT 'multi';");
    rc = resql_exec(c, true, &rs);
    if (rc == RESQL_OK) {
        while (resql_result_next(rs)) {
            while (resql_result_row(rs)) {
                printf("%s \n", resql_result_column_name(rs, 0));
                printf("%s \n", resql_result_text(rs, 0));
            }
        }
    }

    for (int i =0 ;i < 1000; i++) {
        resql_put_statement(c, "SELECT 'multi';");
        rc = resql_exec(c, false, &rs);
        if (rc == RESQL_OK) {

            while (resql_result_next(rs)) {
                while (resql_result_row(rs)) {
                    printf("%s \n", resql_result_column_name(rs, 0));
                    printf("%s \n", resql_result_text(rs, 0));
                }
            }
        }

        sleep(1);
    }

    stop_servers();
}

void write_test()
{
    int rc;
    resql *c;
    resql_result *rs;
    const char *urls =
            "tcp://127.0.0.1:7600 tcp://127.0.0.1:7601 tcp://127.0.0.1:7602";

    create_servers(3, 3);

    sleep(1000000000);


    struct resql_config config = {.cluster_name = "cluster",
            .client_name = "any",            .timeout = 50000,
            .urls = urls};

    resql_create(&c, &config);

    resql_put_statement(c, "CREATE TABLE snapshot (key TEXT, value TEXT);");

    rc = resql_exec(c, false, &rs);
    if (rc != RESQL_OK) {
        rs_abort("");
    }

    for (int i = 0; i < 1000; i++) {
        for (int j = 0; j < 1000; j++) {
            char tmp[32];
            snprintf(tmp, sizeof(tmp), "%d", (i * 1000) + j);

            resql_put_statement(
                    c, "INSERT INTO snapshot VALUES(:key, 'value')");
            resql_bind_param_text(c, ":key", tmp);
        }

        rc = resql_exec(c, false, &rs);
        if (rc != RESQL_OK) {
            rs_abort("");
        }
    }

    sleep(100000000);

    stop_servers();
}

int main()
{
    //test_execute(test);
    test_execute(write_test);
    //test_execute(write_test2);
}
