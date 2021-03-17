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

#include "resql.h"

#include <unistd.h>


struct server *create_node_0()
{
    char *options[] = {"", "-e"};

    struct conf conf;

    conf_init(&conf);
    conf_read_config(&conf, false, sizeof(options) / sizeof(char *), options);

    sc_str_set(&conf.node.log_level, "DEBUG");
    sc_str_set(&conf.node.name, "node0");
    sc_str_set(&conf.node.bind_url,
               "tcp://node0@127.0.0.1:7600 unix:///tmp/var");
    sc_str_set(&conf.node.ad_url, "tcp://node0@127.0.0.1:7600");
    sc_str_set(&conf.cluster.nodes, "tcp://node0@127.0.0.1:7600");
    sc_str_set(&conf.node.dir, "/tmp/node0");
    conf.node.in_memory = true;

    struct server *server = server_create(&conf);

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
