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
