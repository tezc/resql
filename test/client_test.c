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
    const char *urls =
            "tcp://127.0.0.1:7600 tcp://127.0.0.1:7601 tcp://127.0.0.1:7602";

    struct resql_config config = {.cluster_name = "cluster",
                                      .client_name = name,
                                      .timeout_millis = 10000,
                                      .urls = urls};

    struct resql *client;

    int rc = resql_create(&client, &config);
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
    char *options[] = {"", "-e"};

    struct conf settings;

    conf_init(&settings);

    sc_str_set(&settings.node.log_level, "DEBUG");
    sc_str_set(&settings.node.name, "node0");
    sc_str_set(&settings.node.bind_url,
               "tcp://node0@127.0.0.1:7600 unix:///tmp/var");
    sc_str_set(&settings.node.ad_url, "tcp://node0@127.0.0.1:7600");
    sc_str_set(&settings.cluster.nodes, "tcp://node0@127.0.0.1:7600");
    sc_str_set(&settings.node.dir, "/tmp/node0");
    settings.node.in_memory = true;
    conf_read_config(&settings, false, sizeof(options) / sizeof(char *), options);

    struct server *server = server_create(&settings);

    int rc = server_start(server, true);
    if (rc != RS_OK) {
        abort();
    }

    return server;
}

static void client_simple()
{
    struct resql_column *row;
    struct server *s1 = create_node_0();
    struct resql *client = create_client("clientim");

    if (!s1 || !client) {
        rs_abort("");
    }

    struct resql_result *rs = NULL;

    for (int i = 0; i < 100; i++) {

        resql_put_sql(client, "Select 'resql'");

        int rc = resql_exec(client, true, &rs);
        if (rc != RESQL_OK) {
            rs_abort("");
        }

        do {
            assert(resql_column_count(rs) == 1);

            while ((row = resql_row(rs)) != NULL) {
                assert(row[0].type == RESQL_TEXT);
                assert(strcmp("resql", row[0].text) == 0);
            }

        } while (resql_next(rs));
    }


    for (int i = 0; i < 100; i++) {
        resql_put_sql(client, "Select 'resql'");

        int rc = resql_exec(client, false, &rs);
        if (rc != RESQL_OK) {
            rs_abort("");
        }

        do {
            assert(resql_column_count(rs) == 1);

            while ((row = resql_row(rs)) != NULL) {

                assert(row[0].type == RESQL_TEXT);
                assert(strcmp("resql", row[0].text) == 0);
            }
        } while (resql_next(rs));
    }

    int rc = resql_shutdown(client);
    if (rc != RS_OK) {
        abort();
    }

    rc = server_stop(s1);
    if (rc != RS_OK) {
        abort();
    }
}

static void client_prepared()
{
    int rc;
    struct resql_column* row;
    resql_result  *rs = NULL;
    resql_stmt stmt = 0;
    struct server *s1 = create_node_0();
    resql *client = create_client("clientim");

    if (!s1 || !client) {
        rs_abort("");
    }

    rc = resql_del_prepared(client, &stmt);
    client_assert(client, rc == RESQL_SQL_ERROR);

    resql_put_sql(
            client,
            "CREATE TABLE test (a INTEGER PRIMARY KEY, b TEXT, c FLOAT, d BLOB);");
    rc = resql_exec(client, false, &rs);
    client_assert(client, rc == RESQL_OK);

    rc = resql_prepare(client, "INSERT INTO test VALUES (:key1, :key2, :key3, :key4);", &stmt);
    client_assert(client, rc == RESQL_OK);

    for (int i = 0; i < 100; i++) {
        resql_put_prepared(client, &stmt);
        resql_bind_param_int(client, ":key1", i);
        resql_bind_param_text(client, ":key2", "test");
        resql_bind_param_float(client, ":key3", 3.11);
        resql_bind_param_blob(client, ":key4", 5, "blob");

        rc = resql_exec(client, false, &rs);
        if (rc != RESQL_OK) {
            printf("Err :%s \n", resql_errstr(client));
            rs_abort("");
        }

        do {
            while (resql_row(rs)) {
                assert(true);
            }
        } while (resql_next(rs));
    }

    rc = resql_del_prepared(client, &stmt);
    client_assert(client, rc == RESQL_OK);

    resql_put_sql(client, "SELECT * FROM test");
    rc = resql_exec(client, false, &rs);
    client_assert(client, rc == RESQL_OK);

    int x = 0;
    do {
        assert(resql_column_count(rs) == 4);

        while ((row = resql_row(rs)) != NULL) {
            assert(row[0].type  == RESQL_INTEGER);
            assert(row[1].type  == RESQL_TEXT);
            assert(row[2].type == RESQL_FLOAT);
            assert(row[3].type  == RESQL_BLOB);
            assert(strcmp("a", row[0].name) == 0);
            assert(strcmp("b", row[1].name) == 0);
            assert(strcmp("c", row[2].name) == 0);
            assert(strcmp("d", row[3].name) == 0);

            assert(row[0].intval == x++);
            assert(strcmp(row[1].text, "test") == 0);
            assert(row[2].floatval == 3.11);
            assert(row[3].len == 5);
            assert(strcmp(row[3].blob, "blob") == 0);
        }
    } while (resql_next(rs));


    rc = resql_shutdown(client);
    if (rc != RS_OK) {
        abort();
    }

    rc = server_stop(s1);
    if (rc != RS_OK) {
        abort();
    }
}

static void client_error()
{
    int rc;
    struct resql_result *rs = NULL;
    struct resql_column* row;
    resql_stmt stmt = 0;
    struct server *s1 = create_node_0();
    struct resql *client = create_client("clientim");

    if (!s1 || !client) {
        rs_abort("");
    }

    rc = resql_del_prepared(client, &stmt);
    assert(rc == RESQL_SQL_ERROR);

    resql_put_sql(
            client,
            "CREATE TABLE test (a INTEGER PRIMARY KEY, b TEXT, c FLOAT, d BLOB);");
    rc = resql_exec(client, false, &rs);
    assert(rc == RESQL_OK);

    resql_put_sql(client,
                      "INSERT INTO test VALUES (:key1, :key2, :key3, :key4);");
    resql_bind_param_int(client, ":key1", 1);
    resql_bind_param_text(client, ":key2", "test");
    resql_bind_param_float(client, ":key3", 3.11);
    resql_bind_param_blob(client, ":key4", 5, "blob");

    resql_put_sql(client,
                      "INSERT INTO test VALUES (:key1, :key2, :key3, :key4);");
    resql_bind_param_int(client, ":key1", 1);
    resql_bind_param_text(client, ":key2", "test");
    resql_bind_param_float(client, ":key3", 3.11);
    resql_bind_param_blob(client, ":key4", 5, "blob");

    rc = resql_exec(client, false, &rs);
    assert(rc == RESQL_SQL_ERROR);

    resql_put_sql(client, "SELECT * FROM test");
    rc = resql_exec(client, false, &rs);
    if (rc != RESQL_OK) {
        printf("Err :%s \n", resql_errstr(client));
        rs_abort("");
    }

    while (resql_next(rs)) {
        assert(true);
    }

    resql_put_sql(client,
                      "INSERT INTO test VALUES (:key1, :key2, :key3, :key4);");
    resql_bind_param_int(client, ":key1", 0);
    resql_bind_param_text(client, ":key2", "test");
    resql_bind_param_float(client, ":key3", 3.11);
    resql_bind_param_blob(client, ":key4", 5, "blob");

    resql_put_sql(client,
                      "INSERT INTO test VALUES (:key1, :key2, :key3, :key4);");
    resql_bind_param_int(client, ":key1", 1);
    resql_bind_param_text(client, ":key2", "test");
    resql_bind_param_float(client, ":key3", 3.11);
    resql_bind_param_blob(client, ":key4", 5, "blob");

    rc = resql_exec(client, false, &rs);
    if (rc != RESQL_OK) {
        printf("Err :%s \n", resql_errstr(client));
        rs_abort("");
    }

    int k = 0;
    do {
        k++;
    } while (resql_next(rs));

    assert(k == 2);

    resql_put_sql(client, "SELECT * FROM test");
    rc = resql_exec(client, false, &rs);
    if (rc != RESQL_OK) {
        printf("Err :%s \n", resql_errstr(client));
        rs_abort("");
    }


    int x = 0;
    do {
        assert(resql_column_count(rs) == 4);

        while ((row = resql_row(rs)) != NULL) {

            assert(row[0].type == RESQL_INTEGER);
            assert(row[1].type == RESQL_TEXT);
            assert(row[2].type == RESQL_FLOAT);
            assert(row[3].type == RESQL_BLOB);
            assert(strcmp("a", row[0].name) == 0);
            assert(strcmp("b", row[1].name) == 0);
            assert(strcmp("c", row[2].name) == 0);
            assert(strcmp("d", row[3].name) == 0);

            assert(row[0].intval == x++);
            assert(strcmp(row[1].text, "test") == 0);
            assert(row[2].floatval == 3.11);
            assert(row[3].len == 5);
            assert(strcmp(row[3].blob, "blob") == 0);
        }
    } while (resql_next(rs));

    rc = resql_prepare(
            client, "INSERT INTO test VALUES (:key1, :key2, :key3, :key4);",
            &stmt);
    assert(rc == RESQL_OK);


    rc = resql_shutdown(client);
    if (rc != RS_OK) {
        abort();
    }

    rc = server_stop(s1);
    if (rc != RS_OK) {
        abort();
    }
}

static void client_many()
{
    int rc;
    struct resql_result *rs = NULL;
    resql_stmt stmt = 0;
    struct server *s1 = create_node_0();
    struct resql *client = create_client("clientim");

    if (!s1 || !client) {
        rs_abort("");
    }

    rc = resql_del_prepared(client, &stmt);
    assert(rc == RESQL_SQL_ERROR);

    resql_put_sql(
            client,
            "CREATE TABLE test (a INTEGER PRIMARY KEY, b TEXT, c FLOAT, d BLOB);");
    rc = resql_exec(client, false, &rs);
    assert(rc == RESQL_OK);

    for (int i = 0; i < 1000; i++) {
        resql_put_sql(
                client,
                "INSERT INTO test VALUES (:key1, :key2, :key3, :key4);");
        resql_bind_param_int(client, ":key1", i);
        resql_bind_param_text(client, ":key2", "test");
        resql_bind_param_float(client, ":key3", 3.11);
        resql_bind_param_blob(client, ":key4", 5, "blob");

        rc = resql_exec(client, false, &rs);
        if (rc != RESQL_OK) {
            printf("Err :%s \n", resql_errstr(client));
            rs_abort("");
        }


        do {
            while (resql_row(rs) != NULL) {
                assert(true);
            }
        } while (resql_next(rs));
    }

    rc = resql_shutdown(client);
    if (rc != RS_OK) {
        abort();
    }

    rc = server_stop(s1);
    if (rc != RS_OK) {
        abort();
    }
}


static void client_big()
{
    int rc;
    struct resql_result *rs = NULL;
    resql_stmt stmt = 0;
    struct server *s1 = create_node_0();
    struct resql *client = create_client("clientim");

    if (!s1 || !client) {
        rs_abort("");
    }

    rc = resql_del_prepared(client, &stmt);
    assert(rc == RESQL_SQL_ERROR);

    resql_put_sql(
            client,
            "CREATE TABLE test (a INTEGER PRIMARY KEY, b TEXT, c FLOAT, d BLOB);");
    rc = resql_exec(client, false, &rs);
    assert(rc == RESQL_OK);

    char *p = calloc(1, 1024 * 1024);
    memset(p, 0, 1024 * 1024);

    for (int i = 0; i < 1000; i++) {
        resql_put_sql(
                client,
                "INSERT INTO test VALUES (:key1, :key2, :key3, :key4);");
        resql_bind_param_int(client, ":key1", i);
        resql_bind_param_text(client, ":key2", "test");
        resql_bind_param_float(client, ":key3", 3.11);
        resql_bind_param_blob(client, ":key4", 1 * 1024, p);

        rc = resql_exec(client, false, &rs);
        if (rc != RESQL_OK) {
            printf("Err :%s \n", resql_errstr(client));
            rs_abort("");
        }

        do {
            while (resql_row(rs)) {
                assert(true);
            }
        } while (resql_next(rs));
    }

    free(p);

    rc = resql_shutdown(client);
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
    test_execute(client_big);
    test_execute(client_many);
    test_execute(client_simple);
    test_execute(client_prepared);
    test_execute(client_error);

    return 0;
}
