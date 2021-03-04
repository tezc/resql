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

#include <sc/sc_log.h>
#include <unistd.h>

struct resql *create_client(const char *name)
{
    const char *urls =
            "tcp://127.0.0.1:7600 tcp://127.0.0.1:7601 tcp://127.0.0.1:7602";

    struct resql_config settings = {.cluster_name = "cluster",
                                      .client_name = name,
                                      .timeout_millis = 120000,
                                      .urls = urls};

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

static void snapshot_simple()
{
    file_clear_dir("/tmp/node0", ".resql");

    int rc;
    struct resql_column *row;
    struct server *s1 = create_node_0();
    struct resql *c = create_client("clientim");

    if (!s1 || !c) {
        rs_abort("");
    }

    struct resql_result *rs = NULL;

    resql_put_sql(c, "CREATE TABLE snapshot (key TEXT, value TEXT);");

    rc = resql_exec(c, false, &rs);
    if (rc != RESQL_OK) {
        rs_abort("");
    }

    for (int i = 0; i < 1000; i++) {
        for (int j = 0; j < 1000; j++) {
            char tmp[32];
            snprintf(tmp, sizeof(tmp), "%d", (i * 1000) + j);

            resql_put_sql(c, "INSERT INTO snapshot VALUES(:key, 'value')");
            resql_bind_param_text(c, ":key", tmp);
        }

        rc = resql_exec(c, false, &rs);
        if (rc != RESQL_OK) {
            rs_abort("");
        }
    }

    rc = server_stop(s1);
    if (rc != RS_OK) {
        abort();
    }

    s1 = create_node_0();
    if (!s1) {
        rs_abort("");
    }

    resql_put_sql(c, "Select count(*) from snapshot;");
    rc = resql_exec(c, false, &rs);
    client_assert(c, rc == RESQL_OK);

    assert(resql_row_count(rs) == 1);
    assert(resql_row(rs)[0].intval == 1000000);

    resql_put_sql(c, "Select * from snapshot;");
    rc = resql_exec(c, false, &rs);
    client_assert(c, rc == RESQL_OK);

    int x = 0;
    assert(resql_row_count(rs) == 1000000);

    while ((row = resql_row(rs)) != NULL) {
        char tmp[32];
        snprintf(tmp, sizeof(tmp), "%d", x++);

        assert(row[0].type == RESQL_TEXT);
        assert(strcmp(tmp, row[0].text) == 0);
    }

    assert(resql_next(rs) == false);

    rc = resql_shutdown(c);
    if (rc != RS_OK) {
        abort();
    }

    rc = server_stop(s1);
    if (rc != RS_OK) {
        abort();
    }
}

int main(void)
{
    test_execute(snapshot_simple);

    return 0;
}
