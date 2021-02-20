/*
 *  Copyright (C) 2020 Resql Authors
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "server.h"
#include "rs.h"
#include "test_util.h"

#include "c/resql.h"

#include <unistd.h>


struct server *create_node_0()
{
    char *options[] = {"", "-e"};

    struct settings settings;

    settings_init(&settings);
    settings_read_cmdline(&settings, sizeof(options) / sizeof(char *), options);

    sc_str_set(&settings.node.log_level, "DEBUG");
    sc_str_set(&settings.node.name, "node0");
    sc_str_set(&settings.node.bind_uri,
               "tcp://node0@127.0.0.1:8080 unix:///tmp/var");
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

static void single()
{
    struct server *s1 = create_node_0();

    if (!s1) {
        rs_abort("");
    }

    pause();

    int rc = server_stop(s1);
    if (rc != RS_OK) {
        abort();
    }
}



int main(void)
{
    test_execute(single);

    return 0;
}
