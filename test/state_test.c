
/*
 *  reSQL
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
#include "test_util.h"

#include "c/resql.h"

struct server *create_node_0()
{
    char *options[] = {"", "-e"};

    struct settings settings;

    settings_init(&settings);
    settings_read_cmdline(&settings, sizeof(options) / sizeof(char *), options);

    sc_str_set(&settings.node.log_level, "DEBUG");
    sc_str_set(&settings.node.name, "node0");
    sc_str_set(&settings.node.bind_uri, "tcp://node0@127.0.0.1:8080");
    sc_str_set(&settings.node.ad_uri, "tcp://node0@127.0.0.1:8080");
    sc_str_set(&settings.cluster.nodes, "tcp://node0@127.0.0.1:8080");
    sc_str_set(&settings.node.dir, "/tmp/node0");
    settings.node.in_memory = true;

    struct server *server = server_create(&settings);

    int rc = server_start(server, true);
    if (rc != RS_OK) {
        abort();
    }

    return server;
}

struct resql *create_client(const char *name)
{
    const char *urls =
            "tcp://127.0.0.1:8080 tcp://127.0.0.1:8081 tcp://127.0.0.1:8082";

    struct resql_config settings = {.cluster_name = "cluster",
            .client_name = name,
            .timeout = 10000,
            .uris = urls};
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


void state_simple()
{
    struct server *server;
    struct resql *client;

    server = create_node_0();
    if (!server) {
        rs_abort("");
    }

    client = create_client("test");
    if (client == NULL) {
        rs_abort("");
    }

    resql_destroy(client);
    server_stop(server);
}

int main(void)
{
    test_execute(state_simple);

    return 0;
}
