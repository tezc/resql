#include "resql.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

resql *c;
resql *u;

static void assert_client(bool b)
{
    if (!b) {
        printf("Failed %s \n", resql_errstr(c));
        exit(1);
    }
}

static void test_reset()
{
    int rc;
    resql_result *rs;
    struct resql_column *col;

    resql_put_sql(c, "INSERT INTO ctest VALUES(?, ?, ?, ?);");
    resql_bind_index_int(c, 0, -1);
    resql_bind_index_text(c, 1, "jane");
    resql_bind_index_float(c, 2, 3.11);
    resql_bind_index_blob(c, 3, 5, "blob");
    resql_clear(c);

    resql_put_sql(c, "INSERT INTO ctest VALUES(?, ?, ?, ?);");
    resql_bind_index_int(c, 0, -1000);
    resql_bind_index_text(c, 1, "jack");
    resql_bind_index_float(c, 2, 4.11);
    resql_bind_index_blob(c, 3, 5, "test");

    rc = resql_exec(c, false, &rs);
    assert_client(rc == RESQL_OK);

    resql_put_sql(c, "SELECT * FROM ctest;");
    rc = resql_exec(c, true, &rs);
    assert_client(rc == RESQL_OK);

    assert(resql_row_count(rs) == 1);
    assert(resql_column_count(rs) == 4);

    col = resql_row(rs);

    assert(col[0].type == RESQL_INTEGER);
    assert(strcmp(col[0].name, "id") == 0);
    assert(col[0].intval == -1000);

    assert(col[1].type == RESQL_TEXT);
    assert(strcmp(col[1].name, "name") == 0);
    assert(col[1].len == strlen("jack"));
    assert(strcmp(col[1].text, "jack") == 0);

    assert(col[2].type == RESQL_FLOAT);
    assert(strcmp(col[2].name, "points") == 0);
    assert(col[2].floatval == 4.11);

    assert(col[3].type == RESQL_BLOB);
    assert(strcmp(col[3].name, "data") == 0);
    assert(col[3].len == 5);
    assert(strcmp(col[3].blob, "test") == 0);
}

static void test_single_index()
{
    int rc;
    resql_result *rs;
    struct resql_column *col;

    resql_put_sql(c, "INSERT INTO ctest VALUES(?, ?, ?, ?);");
    resql_bind_index_int(c, 0, -1);
    resql_bind_index_text(c, 1, "jane");
    resql_bind_index_float(c, 2, 3.11);
    resql_bind_index_blob(c, 3, 5, "blob");

    rc = resql_exec(c, false, &rs);
    assert_client(rc == RESQL_OK);

    resql_put_sql(c, "SELECT * FROM ctest;");
    rc = resql_exec(c, true, &rs);
    assert_client(rc == RESQL_OK);

    assert(resql_row_count(rs) == 1);
    assert(resql_column_count(rs) == 4);

    col = resql_row(rs);

    assert(col[0].type == RESQL_INTEGER);
    assert(strcmp(col[0].name, "id") == 0);
    assert(col[0].intval == -1);

    assert(col[1].type == RESQL_TEXT);
    assert(strcmp(col[1].name, "name") == 0);
    assert(col[1].len == strlen("jane"));
    assert(strcmp(col[1].text, "jane") == 0);

    assert(col[2].type == RESQL_FLOAT);
    assert(strcmp(col[2].name, "points") == 0);
    assert(col[2].floatval == 3.11);

    assert(col[3].type == RESQL_BLOB);
    assert(strcmp(col[3].name, "data") == 0);
    assert(col[3].len == 5);
    assert(strcmp(col[3].blob, "blob") == 0);
}

static void test_single_param()
{
    int rc;
    resql_result *rs;
    struct resql_column *col;

    resql_put_sql(c, "INSERT INTO ctest VALUES(:id, :name, :points, :data);");
    resql_bind_param_int(c, ":id", -1);
    resql_bind_param_text(c, ":name", "jane");
    resql_bind_param_float(c, ":points", 3.11);
    resql_bind_param_blob(c, ":data", 5, "blob");

    rc = resql_exec(c, false, &rs);
    assert_client(rc == RESQL_OK);

    resql_put_sql(c, "SELECT * FROM ctest;");
    rc = resql_exec(c, true, &rs);
    assert_client(rc == RESQL_OK);

    assert(resql_row_count(rs) == 1);
    assert(resql_column_count(rs) == 4);

    col = resql_row(rs);

    assert(col[0].type == RESQL_INTEGER);
    assert(strcmp(col[0].name, "id") == 0);
    assert(col[0].intval == -1);

    assert(col[1].type == RESQL_TEXT);
    assert(strcmp(col[1].name, "name") == 0);
    assert(col[1].len == strlen("jane"));
    assert(strcmp(col[1].text, "jane") == 0);

    assert(col[2].type == RESQL_FLOAT);
    assert(strcmp(col[2].name, "points") == 0);
    assert(col[2].floatval == 3.11);

    assert(col[3].type == RESQL_BLOB);
    assert(strcmp(col[3].name, "data") == 0);
    assert(col[3].len == 5);
    assert(strcmp(col[3].blob, "blob") == 0);
}

static void test_prepared_index()
{
    int rc;
    resql_result *rs;
    resql_stmt stmt;
    struct resql_column *col;

    rc = resql_prepare(c, "INSERT INTO ctest VALUES(?, ?, ?, ?);", &stmt);
    assert_client(rc == RESQL_OK);

    resql_put_prepared(c, &stmt);
    resql_bind_index_int(c, 0, -1);
    resql_bind_index_text(c, 1, "jane");
    resql_bind_index_float(c, 2, 3.11);
    resql_bind_index_blob(c, 3, 5, "blob");

    rc = resql_exec(c, false, &rs);
    assert_client(rc == RESQL_OK);

    resql_put_sql(c, "SELECT * FROM ctest;");
    rc = resql_exec(c, true, &rs);
    assert_client(rc == RESQL_OK);

    assert(resql_row_count(rs) == 1);
    assert(resql_column_count(rs) == 4);

    col = resql_row(rs);

    assert(col[0].type == RESQL_INTEGER);
    assert(strcmp(col[0].name, "id") == 0);
    assert(col[0].intval == -1);

    assert(col[1].type == RESQL_TEXT);
    assert(strcmp(col[1].name, "name") == 0);
    assert(col[1].len == strlen("jane"));
    assert(strcmp(col[1].text, "jane") == 0);

    assert(col[2].type == RESQL_FLOAT);
    assert(strcmp(col[2].name, "points") == 0);
    assert(col[2].floatval == 3.11);

    assert(col[3].type == RESQL_BLOB);
    assert(strcmp(col[3].name, "data") == 0);
    assert(col[3].len == 5);
    assert(strcmp(col[3].blob, "blob") == 0);

    rc = resql_del_prepared(c, &stmt);
    assert_client(rc == RESQL_OK);
}

static void test_prepared_param()
{
    int rc;
    resql_result *rs;
    resql_stmt stmt;
    struct resql_column *col;

    rc = resql_prepare(
            c, "INSERT INTO ctest VALUES(:id, :name, :points, :data);", &stmt);
    assert_client(rc == RESQL_OK);

    resql_put_prepared(c, &stmt);
    resql_bind_param_int(c, ":id", -1);
    resql_bind_param_text(c, ":name", "jane");
    resql_bind_param_float(c, ":points", 3.11);
    resql_bind_param_blob(c, ":data", 5, "blob");

    rc = resql_exec(c, false, &rs);
    assert_client(rc == RESQL_OK);
    assert(resql_changes(rs) == 1);

    resql_put_sql(c, "SELECT * FROM ctest;");
    rc = resql_exec(c, true, &rs);
    assert_client(rc == RESQL_OK);

    assert(resql_row_count(rs) == 1);
    assert(resql_column_count(rs) == 4);

    col = resql_row(rs);

    assert(col[0].type == RESQL_INTEGER);
    assert(strcmp(col[0].name, "id") == 0);
    assert(col[0].intval == -1);

    assert(col[1].type == RESQL_TEXT);
    assert(strcmp(col[1].name, "name") == 0);
    assert(col[1].len == strlen("jane"));
    assert(strcmp(col[1].text, "jane") == 0);

    assert(col[2].type == RESQL_FLOAT);
    assert(strcmp(col[2].name, "points") == 0);
    assert(col[2].floatval == 3.11);

    assert(col[3].type == RESQL_BLOB);
    assert(strcmp(col[3].name, "data") == 0);
    assert(col[3].len == 5);
    assert(strcmp(col[3].blob, "blob") == 0);

    rc = resql_del_prepared(c, &stmt);
    assert_client(rc == RESQL_OK);
}

static void test_prepared_param_many()
{
    int rc;
    resql_result *rs;
    resql_stmt stmt;
    struct resql_column *col;

    rc = resql_prepare(
            c, "INSERT INTO ctest VALUES(:id, :name, :points, :data);", &stmt);
    assert_client(rc == RESQL_OK);

    for (int i = 0; i < 10000; i++) {
        resql_put_prepared(c, &stmt);
        resql_bind_param_int(c, ":id", i);
        resql_bind_param_text(c, ":name", "jane");
        resql_bind_param_float(c, ":points", 3.11);
        resql_bind_param_blob(c, ":data", 5, "blob");
    }

    rc = resql_exec(c, false, &rs);
    assert_client(rc == RESQL_OK);

    for (int i = 0; i < 9999; i++) {
        assert(resql_changes(rs) == 1);
        assert(resql_next(rs) == true);
    }

    resql_put_sql(c, "SELECT * FROM ctest;");
    rc = resql_exec(c, true, &rs);
    assert_client(rc == RESQL_OK);

    assert(resql_row_count(rs) == 10000);
    assert(resql_column_count(rs) == 4);

    for (int i = 0; i < 10000; i++) {
        col = resql_row(rs);

        assert(col[0].type == RESQL_INTEGER);
        assert(strcmp(col[0].name, "id") == 0);
        assert(col[0].intval == i);

        assert(col[1].type == RESQL_TEXT);
        assert(strcmp(col[1].name, "name") == 0);
        assert(col[1].len == strlen("jane"));
        assert(strcmp(col[1].text, "jane") == 0);

        assert(col[2].type == RESQL_FLOAT);
        assert(strcmp(col[2].name, "points") == 0);
        assert(col[2].floatval == 3.11);

        assert(col[3].type == RESQL_BLOB);
        assert(strcmp(col[3].name, "data") == 0);
        assert(col[3].len == 5);
        assert(strcmp(col[3].blob, "blob") == 0);
    }

    resql_reset_rows(rs);

    assert(resql_row_count(rs) == 10000);
    assert(resql_column_count(rs) == 4);

    for (int i = 0; i < 10000; i++) {
        col = resql_row(rs);

        assert(col[0].type == RESQL_INTEGER);
        assert(strcmp(col[0].name, "id") == 0);
        assert(col[0].intval == i);

        assert(col[1].type == RESQL_TEXT);
        assert(strcmp(col[1].name, "name") == 0);
        assert(col[1].len == strlen("jane"));
        assert(strcmp(col[1].text, "jane") == 0);

        assert(col[2].type == RESQL_FLOAT);
        assert(strcmp(col[2].name, "points") == 0);
        assert(col[2].floatval == 3.11);

        assert(col[3].type == RESQL_BLOB);
        assert(strcmp(col[3].name, "data") == 0);
        assert(col[3].len == 5);
        assert(strcmp(col[3].blob, "blob") == 0);
    }

    rc = resql_del_prepared(c, &stmt);
    assert_client(rc == RESQL_OK);
}

static void test_fail()
{
    int rc;
    struct resql_column *col;
    resql_result *rs;
    resql_stmt stmt;

    resql_bind_index_int(c, 0, -1);
    rc = resql_exec(c, false, &rs);
    assert_client(rc != RESQL_OK);

    resql_bind_index_text(c, 0, "");
    rc = resql_exec(c, false, &rs);
    assert_client(rc != RESQL_OK);
    assert(*resql_errstr(c) != '\0');

    resql_bind_index_float(c, 0, 3.1);
    rc = resql_exec(c, false, &rs);
    assert_client(rc != RESQL_OK);

    resql_bind_index_blob(c, 0, 5, "test");
    rc = resql_exec(c, false, &rs);
    assert_client(rc != RESQL_OK);

    resql_bind_index_null(c, 0);
    rc = resql_exec(c, false, &rs);
    assert_client(rc != RESQL_OK);

    resql_bind_param_int(c, 0, -1);
    rc = resql_exec(c, false, &rs);
    assert_client(rc != RESQL_OK);

    resql_bind_param_text(c, 0, "");
    rc = resql_exec(c, false, &rs);
    assert_client(rc != RESQL_OK);

    resql_bind_param_float(c, 0, 3.1);
    rc = resql_exec(c, false, &rs);
    assert_client(rc != RESQL_OK);

    resql_bind_param_blob(c, 0, 5, "test");
    rc = resql_exec(c, false, &rs);
    assert_client(rc != RESQL_OK);

    resql_bind_param_null(c, 0);
    rc = resql_exec(c, false, &rs);
    assert_client(rc != RESQL_OK);

    resql_put_sql(c, "INSERT INTO ctest VALUES(?, ?, ?, ?);");
    resql_bind_index_null(c, 0);
    resql_bind_index_null(c, 1);
    resql_bind_index_null(c, 2);
    resql_bind_index_null(c, 3);
    rc = resql_exec(c, false, &rs);
    assert_client(rc == RESQL_OK);

    resql_put_sql(c, "SELECT * FROM ctest;");
    rc = resql_exec(c, true, &rs);
    assert_client(rc == RESQL_OK);

    assert(resql_row_count(rs) == 1);
    assert(resql_column_count(rs) == 4);

    col = resql_row(rs);

    assert(col[0].type == RESQL_NULL);
    assert(col[1].type == RESQL_NULL);
    assert(col[2].type == RESQL_NULL);
    assert(col[3].type == RESQL_NULL);

    resql_put_sql(c, "DELETE FROM ctest;");
    rc = resql_exec(c, false, &rs);
    assert_client(rc == RESQL_OK);

    resql_put_sql(c, "INSERT INTO ctest VALUES(:id, :name, :points, :data);");
    resql_bind_param_null(c, ":id");
    resql_bind_param_null(c, ":name");
    resql_bind_param_null(c, ":points");
    resql_bind_param_null(c, ":data");
    rc = resql_exec(c, false, &rs);
    assert_client(rc == RESQL_OK);

    resql_put_sql(c, "SELECT * FROM ctest;");
    rc = resql_exec(c, true, &rs);
    assert_client(rc == RESQL_OK);

    assert(resql_row_count(rs) == 1);
    assert(resql_column_count(rs) == 4);

    col = resql_row(rs);

    assert(col[0].type == RESQL_NULL);
    assert(col[1].type == RESQL_NULL);
    assert(col[2].type == RESQL_NULL);
    assert(col[3].type == RESQL_NULL);

    resql_put_sql(c, "INSERT INTO ctest VALUES(?, ?, ?, ?);");
    rc = resql_prepare(
            c, "INSERT INTO ctest VALUES(:id, :name, :points, :data);", &stmt);
    assert_client(rc == RESQL_SQL_ERROR);

    rc = resql_prepare(
            c, "INSERT INTO ctest VALUES(:id, :name, :points, :data);", &stmt);
    assert_client(rc == RESQL_OK);

    resql_put_sql(c, "INSERT INTO ctest VALUES(?, ?, ?, ?);");
    rc = resql_del_prepared(c, &stmt);
    assert_client(rc == RESQL_SQL_ERROR);

    rc = resql_del_prepared(c, &stmt);
    assert_client(rc == RESQL_OK);
}

static void test_connect()
{
    int rc;
    resql *client;

    rc = resql_create(&client,
                      &(struct resql_config){.client_name = "conn-client",
                                             .cluster_name = "cluster",
                                             .timeout = 2000,
                                             .uris = "tcp://1271.0.0.1:7600"});
    assert(rc != RESQL_OK);
    assert(resql_destroy(client) == RESQL_OK);

    rc = resql_create(&client,
                      &(struct resql_config){.client_name = "conn-client",
                                             .cluster_name = "cluster2",
                                             .timeout = 2000,
                                             .uris = "tcp://127.0.0.1:7600"});
    assert(rc != RESQL_OK);
    assert(resql_destroy(client) == RESQL_OK);

    rc = resql_create(&client,
                      &(struct resql_config){.client_name = "conn-client",
                              .cluster_name = "cluster2",
                              .timeout = 2000,
                              .uris = "tcp://127.0.0.1:769900"});
    assert(rc != RESQL_OK);
    assert(resql_destroy(client) == RESQL_OK);

    rc = resql_create(&client,
                      &(struct resql_config){
                              .cluster_name = "cluster",
                              .timeout = 2000,
                              .uris = "tcp://127.0.0.1:7600"});
    assert(rc == RESQL_OK);
    assert(resql_destroy(client) == RESQL_OK);

    rc = resql_create(&client,
                      &(struct resql_config){
                              .cluster_name = "cluster",
                              .timeout = 2000,
                              .uris = "tcp://127.0.0.1:7600"});
    assert(rc == RESQL_OK);
    assert(resql_destroy(client) == RESQL_OK);

    rc = resql_create(&client,
                      &(struct resql_config){.client_name = "conn-client",
                              .cluster_name = "cluster2",
                              .timeout = 1,
                              .uris = "tcp://127.0.0.1:7600"});
    assert(rc != RESQL_OK);
    assert(resql_destroy(client) == RESQL_OK);

    rc = resql_create(&client,
                      &(struct resql_config){.uris = "tcp://127.0.0.1:7600"});
    assert(rc != RESQL_OK);
    assert(resql_destroy(client) == RESQL_OK);

    rc = resql_create(&client,
                      &(struct resql_config){0});
    assert(rc != RESQL_OK);
    assert(resql_destroy(client) == RESQL_OK);

    rc = resql_create(&client,
                      &(struct resql_config){.client_name = "conn-client",
                              .timeout = 2000,
                              .uris = "tcp://127.0.0.1:7600"});
    assert(rc != RESQL_OK);
    assert(resql_destroy(client) == RESQL_OK);

    rc = resql_create(&client,
                      &(struct resql_config){.client_name = "conn-client",
                              .timeout = 2000,
                              .cluster_name = "cluster"});
    assert(rc != RESQL_OK);
    assert(resql_destroy(client) == RESQL_OK);

    rc = resql_create(&client,
                      &(struct resql_config){
                              .cluster_name = "cluster",
                              .timeout = 2000,
                              .uris = "tcp://127.0.0.1:7600"});
    assert(rc == RESQL_OK);

    rc = resql_destroy(client);
    assert(rc == RESQL_OK);

    rc = resql_create(&client,
                      &(struct resql_config){
                              .cluster_name = "cluster",
                              .source_addr = "127.0.0.1",
                              .source_port = "7689",
                              .timeout = 2000,
                              .uris = "tcp://127.0.0.1:7600"});
    if (rc != RESQL_OK && client != NULL) {
        printf("Failed %s \n", resql_errstr(c));
    }

    assert(rc == RESQL_OK);

    rc = resql_destroy(client);
    assert(rc == RESQL_OK);
}

void run(void (*fn)())
{
    int rc;
    resql *client, *unixclient;
    resql_result *rs;

    struct resql_config conf = {
            .client_name = "c-client",
            .cluster_name = "cluster",
            .timeout = 4000,
            .uris = "tcp://127.0.0.1:7600",
    };

    rc = resql_create(&client, &conf);
    if (rc != RESQL_OK) {
        printf("Failed to create client \n");
        exit(1);
    }

    resql_put_sql(client, "DROP TABLE IF EXISTS ctest;");
    resql_put_sql(client, "CREATE TABLE ctest (id INTEGER, "
                          "name TEXT, points FLOAT, data BLOB);");
    rc = resql_exec(client, false, &rs);
    if (rc != RESQL_OK) {
        printf("Failed : %s \n", resql_errstr(client));
        exit(1);
    }

    c = client;

    struct resql_config conf2 = {
            .client_name = "c-client-unix",
            .cluster_name = "cluster",
            .timeout = 4000,
            .uris = "unix:///tmp/resql",
    };

    rc = resql_create(&unixclient, &conf2);
    if (rc != RESQL_OK) {
        printf("Failed to create client \n");
        exit(1);
    }

    u = unixclient;

    fn();

    resql_put_sql(client, "DROP TABLE ctest;");
    rc = resql_exec(client, false, &rs);
    if (rc != RESQL_OK) {
        printf("Failed : %s \n", resql_errstr(client));
        exit(1);
    }

    rc = resql_destroy(client);
    if (rc != RESQL_OK) {
        printf("Failed to destroy client \n");
        exit(1);
    }

    rc = resql_destroy(unixclient);
    if (rc != RESQL_OK) {
        printf("Failed to destroy client \n");
        exit(1);
    }
}


int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    resql_init();

    run(test_reset);
    run(test_single_index);
    run(test_single_param);
    run(test_prepared_index);
    run(test_prepared_param);
    run(test_prepared_param_many);
    run(test_fail);
    run(test_connect);

    resql_term();

    return 0;
}
