#include "resql.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_util.h"

#define test_client_execute(A) (run(A, #A))

resql *c;

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

    rs_assert(resql_row_count(rs) == 1);
    rs_assert(resql_column_count(rs) == 4);

    col = resql_row(rs);

    rs_assert(col[0].type == RESQL_INTEGER);
    rs_assert(strcmp(col[0].name, "id") == 0);
    rs_assert(col[0].intval == -1000);

    rs_assert(col[1].type == RESQL_TEXT);
    rs_assert(strcmp(col[1].name, "name") == 0);
    rs_assert(col[1].len == strlen("jack"));
    rs_assert(strcmp(col[1].text, "jack") == 0);

    rs_assert(col[2].type == RESQL_FLOAT);
    rs_assert(strcmp(col[2].name, "points") == 0);
    rs_assert(col[2].floatval == 4.11);

    rs_assert(col[3].type == RESQL_BLOB);
    rs_assert(strcmp(col[3].name, "data") == 0);
    rs_assert(col[3].len == 5);
    rs_assert(strcmp(col[3].blob, "test") == 0);
}

static void test_last_insert_id()
{
    int rc;
    resql_result *rs;

    for (int i = 0; i < 10; i++) {
        resql_put_sql(c, "INSERT INTO ctest VALUES(?, ?, ?, ?);");
        resql_bind_index_int(c, 0, i);
        resql_bind_index_text(c, 1, "jane");
        resql_bind_index_float(c, 2, 3.11);
        resql_bind_index_blob(c, 3, 5, "blob");

        rc = resql_exec(c, false, &rs);
        assert_client(rc == RESQL_OK);
        rs_assert(resql_changes(rs) == 1);
        rs_assert(resql_last_row_id(rs) == i + 1);
    }

    for (int i = 10; i < 20; i++) {
        resql_put_sql(c, "INSERT INTO ctest VALUES(?, ?, ?, ?);");
        resql_bind_index_int(c, 0, i);
        resql_bind_index_text(c, 1, "jane");
        resql_bind_index_float(c, 2, 3.11);
        resql_bind_index_blob(c, 3, 5, "blob");

        rc = resql_exec(c, false, &rs);
        assert_client(rc == RESQL_OK);
        rs_assert(resql_changes(rs) == 1);
        rs_assert(resql_last_row_id(rs) == i + 1);
    }
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

    rs_assert(resql_row_count(rs) == 1);
    rs_assert(resql_column_count(rs) == 4);

    col = resql_row(rs);

    rs_assert(col[0].type == RESQL_INTEGER);
    rs_assert(strcmp(col[0].name, "id") == 0);
    rs_assert(col[0].intval == -1);

    rs_assert(col[1].type == RESQL_TEXT);
    rs_assert(strcmp(col[1].name, "name") == 0);
    rs_assert(col[1].len == strlen("jane"));
    rs_assert(strcmp(col[1].text, "jane") == 0);

    rs_assert(col[2].type == RESQL_FLOAT);
    rs_assert(strcmp(col[2].name, "points") == 0);
    rs_assert(col[2].floatval == 3.11);

    rs_assert(col[3].type == RESQL_BLOB);
    rs_assert(strcmp(col[3].name, "data") == 0);
    rs_assert(col[3].len == 5);
    rs_assert(strcmp(col[3].blob, "blob") == 0);
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

    rs_assert(resql_row_count(rs) == 1);
    rs_assert(resql_column_count(rs) == 4);

    col = resql_row(rs);

    rs_assert(col[0].type == RESQL_INTEGER);
    rs_assert(strcmp(col[0].name, "id") == 0);
    rs_assert(col[0].intval == -1);

    rs_assert(col[1].type == RESQL_TEXT);
    rs_assert(strcmp(col[1].name, "name") == 0);
    rs_assert(col[1].len == strlen("jane"));
    rs_assert(strcmp(col[1].text, "jane") == 0);

    rs_assert(col[2].type == RESQL_FLOAT);
    rs_assert(strcmp(col[2].name, "points") == 0);
    rs_assert(col[2].floatval == 3.11);

    rs_assert(col[3].type == RESQL_BLOB);
    rs_assert(strcmp(col[3].name, "data") == 0);
    rs_assert(col[3].len == 5);
    rs_assert(strcmp(col[3].blob, "blob") == 0);
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

    rs_assert(resql_row_count(rs) == 1);
    rs_assert(resql_column_count(rs) == 4);

    col = resql_row(rs);

    rs_assert(col[0].type == RESQL_INTEGER);
    rs_assert(strcmp(col[0].name, "id") == 0);
    rs_assert(col[0].intval == -1);

    rs_assert(col[1].type == RESQL_TEXT);
    rs_assert(strcmp(col[1].name, "name") == 0);
    rs_assert(col[1].len == strlen("jane"));
    rs_assert(strcmp(col[1].text, "jane") == 0);

    rs_assert(col[2].type == RESQL_FLOAT);
    rs_assert(strcmp(col[2].name, "points") == 0);
    rs_assert(col[2].floatval == 3.11);

    rs_assert(col[3].type == RESQL_BLOB);
    rs_assert(strcmp(col[3].name, "data") == 0);
    rs_assert(col[3].len == 5);
    rs_assert(strcmp(col[3].blob, "blob") == 0);

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
    rs_assert(resql_changes(rs) == 1);

    resql_put_sql(c, "SELECT * FROM ctest;");
    rc = resql_exec(c, true, &rs);
    assert_client(rc == RESQL_OK);

    rs_assert(resql_row_count(rs) == 1);
    rs_assert(resql_column_count(rs) == 4);

    col = resql_row(rs);

    rs_assert(col[0].type == RESQL_INTEGER);
    rs_assert(strcmp(col[0].name, "id") == 0);
    rs_assert(col[0].intval == -1);

    rs_assert(col[1].type == RESQL_TEXT);
    rs_assert(strcmp(col[1].name, "name") == 0);
    rs_assert(col[1].len == strlen("jane"));
    rs_assert(strcmp(col[1].text, "jane") == 0);

    rs_assert(col[2].type == RESQL_FLOAT);
    rs_assert(strcmp(col[2].name, "points") == 0);
    rs_assert(col[2].floatval == 3.11);

    rs_assert(col[3].type == RESQL_BLOB);
    rs_assert(strcmp(col[3].name, "data") == 0);
    rs_assert(col[3].len == 5);
    rs_assert(strcmp(col[3].blob, "blob") == 0);

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
        rs_assert(resql_changes(rs) == 1);
        rs_assert(resql_next(rs) == true);
    }

    resql_put_sql(c, "SELECT * FROM ctest;");
    rc = resql_exec(c, true, &rs);
    assert_client(rc == RESQL_OK);

    rs_assert(resql_row_count(rs) == 10000);
    rs_assert(resql_column_count(rs) == 4);

    for (int i = 0; i < 10000; i++) {
        col = resql_row(rs);

        rs_assert(col[0].type == RESQL_INTEGER);
        rs_assert(strcmp(col[0].name, "id") == 0);
        rs_assert(col[0].intval == i);

        rs_assert(col[1].type == RESQL_TEXT);
        rs_assert(strcmp(col[1].name, "name") == 0);
        rs_assert(col[1].len == strlen("jane"));
        rs_assert(strcmp(col[1].text, "jane") == 0);

        rs_assert(col[2].type == RESQL_FLOAT);
        rs_assert(strcmp(col[2].name, "points") == 0);
        rs_assert(col[2].floatval == 3.11);

        rs_assert(col[3].type == RESQL_BLOB);
        rs_assert(strcmp(col[3].name, "data") == 0);
        rs_assert(col[3].len == 5);
        rs_assert(strcmp(col[3].blob, "blob") == 0);
    }

    resql_reset_rows(rs);

    rs_assert(resql_row_count(rs) == 10000);
    rs_assert(resql_column_count(rs) == 4);

    for (int i = 0; i < 10000; i++) {
        col = resql_row(rs);

        rs_assert(col[0].type == RESQL_INTEGER);
        rs_assert(strcmp(col[0].name, "id") == 0);
        rs_assert(col[0].intval == i);

        rs_assert(col[1].type == RESQL_TEXT);
        rs_assert(strcmp(col[1].name, "name") == 0);
        rs_assert(col[1].len == strlen("jane"));
        rs_assert(strcmp(col[1].text, "jane") == 0);

        rs_assert(col[2].type == RESQL_FLOAT);
        rs_assert(strcmp(col[2].name, "points") == 0);
        rs_assert(col[2].floatval == 3.11);

        rs_assert(col[3].type == RESQL_BLOB);
        rs_assert(strcmp(col[3].name, "data") == 0);
        rs_assert(col[3].len == 5);
        rs_assert(strcmp(col[3].blob, "blob") == 0);
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
    rs_assert(*resql_errstr(c) != '\0');

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

    rs_assert(resql_row_count(rs) == 1);
    rs_assert(resql_column_count(rs) == 4);

    col = resql_row(rs);

    rs_assert(col[0].type == RESQL_NULL);
    rs_assert(col[1].type == RESQL_NULL);
    rs_assert(col[2].type == RESQL_NULL);
    rs_assert(col[3].type == RESQL_NULL);

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

    rs_assert(resql_row_count(rs) == 1);
    rs_assert(resql_column_count(rs) == 4);

    col = resql_row(rs);

    rs_assert(col[0].type == RESQL_NULL);
    rs_assert(col[1].type == RESQL_NULL);
    rs_assert(col[2].type == RESQL_NULL);
    rs_assert(col[3].type == RESQL_NULL);

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
                                             .timeout_millis = 2000,
                                             .urls = "tcp://1271.0.0.1:7600"});
    rs_assert(rc != RESQL_OK);
    rs_assert(resql_shutdown(client) == RESQL_OK);

    rc = resql_create(&client,
                      &(struct resql_config){.client_name = "conn-client",
                                             .cluster_name = "cluster2",
                                             .timeout_millis = 2000,
                                             .urls = "tcp://127.0.0.1:7600"});
    rs_assert(rc != RESQL_OK);
    rs_assert(resql_shutdown(client) == RESQL_OK);

    rc = resql_create(&client,
                      &(struct resql_config){.client_name = "conn-client",
                              .cluster_name = "cluster2",
                              .timeout_millis = 2000,
                              .urls = "tcp://127.0.0.1:769900"});
    rs_assert(rc != RESQL_OK);
    rs_assert(resql_shutdown(client) == RESQL_OK);

    rc = resql_create(&client,
                      &(struct resql_config){
                              .cluster_name = "cluster",
                              .timeout_millis = 2000,
                              .urls = "tcp://127.0.0.1:7600"});
    rs_assert(rc == RESQL_OK);
    rs_assert(resql_shutdown(client) == RESQL_OK);

    rc = resql_create(&client,
                      &(struct resql_config){
                              .cluster_name = "cluster",
                              .timeout_millis = 2000,
                              .urls = "tcp://127.0.0.1:7600"});
    rs_assert(rc == RESQL_OK);
    rs_assert(resql_shutdown(client) == RESQL_OK);

    rc = resql_create(&client,
                      &(struct resql_config){.client_name = "conn-client",
                              .cluster_name = "cluster2",
                              .timeout_millis = 1,
                              .urls = "tcp://127.0.0.1:7600"});
    rs_assert(rc != RESQL_OK);
    rs_assert(resql_shutdown(client) == RESQL_OK);

    rc = resql_create(&client,
                      &(struct resql_config){.urls = "tcp://127.0.0.1:7600"});
    rs_assert(rc == RESQL_OK);
    rs_assert(resql_shutdown(client) == RESQL_OK);

    rc = resql_create(&client,
                      &(struct resql_config){0});
    rs_assert(rc == RESQL_OK);
    rs_assert(resql_shutdown(client) == RESQL_OK);

    rc = resql_create(&client,
                      &(struct resql_config){.client_name = "conn-client",
                              .timeout_millis = 2000,
                              .urls = "tcp://127.0.0.1:7600"});
    rs_assert(rc == RESQL_OK);
    rs_assert(resql_shutdown(client) == RESQL_OK);

    rc = resql_create(&client,
                      &(struct resql_config){.client_name = "conn-client",
                              .timeout_millis = 2000,
                              .cluster_name = "cluster"});
    rs_assert(rc == RESQL_OK);
    rs_assert(resql_shutdown(client) == RESQL_OK);

    rc = resql_create(&client,
                      &(struct resql_config){
                              .cluster_name = "cluster",
                              .timeout_millis = 2000,
                              .urls = "tcp://127.0.0.1:7600"});
    rs_assert(rc == RESQL_OK);

    rc = resql_shutdown(client);
    rs_assert(rc == RESQL_OK);

    rc = resql_create(&client,
                      &(struct resql_config){
                              .cluster_name = "cluster",
                              .outgoing_addr = "127.0.0.1",
                              .outgoing_port = "8689",
                              .timeout_millis = 2000,
                              .urls = "tcp://127.0.0.1:7600"});
    if (rc != RESQL_OK && client != NULL) {
        printf("Failed %s \n", resql_errstr(c));
    }

    rs_assert(rc == RESQL_OK);

    rc = resql_shutdown(client);
    rs_assert(rc == RESQL_OK);
}

static void example1()
{
    resql *client;
    struct resql_result *rs;
    struct resql_column *row;

    resql_create(&client, &(struct resql_config){0});

    resql_put_sql(client, "SELECT 'Hello World!';");
    resql_exec(client, true, &rs);
    row = resql_row(rs);

    printf("%s \n", row[0].text);

    resql_shutdown(client);
}

static void example2()
{
    int rc;
    resql *client;
    struct resql_result *rs;
    struct resql_column *row;

    struct resql_config conf = {
            .client_name = "testclient",
            .cluster_name = "cluster",
            .timeout_millis = 4000,
            .urls = "tcp://127.0.0.1:7600",
    };

    rc = resql_create(&client, &conf);
    if (rc != RESQL_OK) {
        printf("Failed to create client \n");
        exit(1);
    }

    resql_put_sql(client, "SELECT 'Hello World!';");
    resql_exec(client, true, &rs);

    row = resql_row(rs);
    printf("%s \n", row[0].text);

    rc = resql_shutdown(client);
    if (rc != RESQL_OK) {
        printf("Failed to create client \n");
        exit(1);
    }
}

static void example3()
{
    int rc;
    resql *client;
    struct resql_result *rs;

    rc = resql_create(&client, &(struct resql_config){0});
    if (rc != RESQL_OK) {
        printf("Failed to create client \n");
        exit(1);
    }

    resql_put_sql(client, "CREATE TABLE test (key TEXT, value TEXT);");
    rc = resql_exec(client, false, &rs);
    if (rc != RESQL_OK) {
        printf("Operation failed : %s \n", resql_errstr(client));
        exit(1);
    }

    // Clean-up
    resql_put_sql(client, "DROP TABLE test;");
    rc = resql_exec(client, false, &rs);
    if (rc != RESQL_OK) {
        printf("Operation failed : %s \n", resql_errstr(client));
        exit(1);
    }

    resql_shutdown(client);
}

static void example4()
{
    int rc;
    resql *client;
    struct resql_result *rs;

    rc = resql_create(&client, &(struct resql_config){0});
    if (rc != RESQL_OK) {
        printf("Failed to create client \n");
        exit(1);
    }

    resql_put_sql(client, "CREATE TABLE test (key TEXT, value TEXT);");
    rc = resql_exec(client, false, &rs);
    if (rc != RESQL_OK) {
        printf("Operation failed : %s \n", resql_errstr(client));
        exit(1);
    }

    // Option-1, with parameter name
    resql_put_sql(client, "INSERT INTO test VALUES(:name,:lastname);");
    resql_bind_param_text(client, ":name", "jane");
    resql_bind_param_text(client, ":lastname", "doe");
    rc = resql_exec(client, false, &rs);
    if (rc != RESQL_OK) {
        printf("Operation failed : %s \n", resql_errstr(client));
        exit(1);
    }

    // Option-2, with parameter index
    resql_put_sql(client, "INSERT INTO test VALUES(?, ?);");
    resql_bind_index_text(client, 0, "jane");
    resql_bind_index_text(client, 1, "doe");
    rc = resql_exec(client, false, &rs);
    if (rc != RESQL_OK) {
        printf("Operation failed : %s \n", resql_errstr(client));
        exit(1);
    }

    // Clean-up
    resql_put_sql(client, "DROP TABLE test;");
    rc = resql_exec(client, false, &rs);
    if (rc != RESQL_OK) {
        printf("Operation failed : %s \n", resql_errstr(client));
        exit(1);
    }

    resql_shutdown(client);
}

static void example5()
{
    int rc;
    resql *client;
    struct resql_result *rs;

    rc = resql_create(&client, &(struct resql_config){0});
    if (rc != RESQL_OK) {
        printf("Failed to create client \n");
        exit(1);
    }

    resql_put_sql(client, "CREATE TABLE test (key TEXT, value TEXT);");
    rc = resql_exec(client, false, &rs);
    if (rc != RESQL_OK) {
        printf("Operation failed : %s \n", resql_errstr(client));
        exit(1);
    }

    resql_put_sql(client, "INSERT INTO test VALUES('jane','doe');");
    resql_put_sql(client, "INSERT INTO test VALUES('jack','doe');");
    resql_put_sql(client, "INSERT INTO test VALUES('joe','doe');");
    rc = resql_exec(client, false, &rs);
    if (rc != RESQL_OK) {
        printf("Operation failed : %s \n", resql_errstr(client));
        exit(1);
    }

    resql_put_sql(client, "SELECT * FROM test;");
    rc = resql_exec(client, true, &rs);
    if (rc != RESQL_OK) {
        printf("Operation failed : %s \n", resql_errstr(client));
        exit(1);
    }

    struct resql_column* row;

    while ((row = resql_row(rs)) != NULL) {
        printf("name : %s, lastname : %s \n", row[0].text, row[1].text);
    }

    // Clean-up
    resql_put_sql(client, "DROP TABLE test;");
    rc = resql_exec(client, false, &rs);
    if (rc != RESQL_OK) {
        printf("Operation failed : %s \n", resql_errstr(client));
        exit(1);
    }

    resql_shutdown(client);
}

static void example6()
{
    int rc;
    resql *client;
    resql_stmt stmt;
    struct resql_result *rs;


    rc = resql_create(&client, &(struct resql_config){0});
    if (rc != RESQL_OK) {
        printf("Failed to create client \n");
        exit(1);
    }

    resql_put_sql(client, "CREATE TABLE test (key TEXT, value TEXT);");
    rc = resql_exec(client, false, &rs);
    if (rc != RESQL_OK) {
        printf("Operation failed : %s \n", resql_errstr(client));
        exit(1);
    }

    // Option-1, indexes
    rc = resql_prepare(client, "INSERT INTO test VALUES(?, ?);", &stmt);
    if (rc != RESQL_OK) {
        printf("Operation failed : %s \n", resql_errstr(client));
        exit(1);
    }

    resql_put_prepared(client, &stmt);
    resql_bind_index_text(client, 0, "jane");
    resql_bind_index_text(client, 1, "doe");
    rc = resql_exec(client, false, &rs);
    if (rc != RESQL_OK) {
        printf("Operation failed : %s \n", resql_errstr(client));
        exit(1);
    }

    // Clean-up if you're not going to use prepared statement again.
    rc = resql_del_prepared(client, &stmt);
    if (rc != RESQL_OK) {
        printf("Operation failed : %s \n", resql_errstr(client));
        exit(1);
    }

    // Option-2, parameter names

    rc = resql_prepare(client, "INSERT INTO test VALUES(:name, :lastname);", &stmt);
    if (rc != RESQL_OK) {
        printf("Operation failed : %s \n", resql_errstr(client));
        exit(1);
    }

    resql_put_prepared(client, &stmt);
    resql_bind_param_text(client, ":name", "jane");
    resql_bind_param_text(client, ":lastname", "doe");
    rc = resql_exec(client, false, &rs);
    if (rc != RESQL_OK) {
        printf("Operation failed : %s \n", resql_errstr(client));
        exit(1);
    }

    // Clean-up if you're not going to use prepared statement again.
    rc = resql_del_prepared(client, &stmt);
    if (rc != RESQL_OK) {
        printf("Operation failed : %s \n", resql_errstr(client));
        exit(1);
    }

    // Clean-up
    resql_put_sql(client, "DROP TABLE test;");
    rc = resql_exec(client, false, &rs);
    if (rc != RESQL_OK) {
        printf("Operation failed : %s \n", resql_errstr(client));
        exit(1);
    }

    resql_shutdown(client);
}

void run(void (*fn)(), const char* name)
{
    int rc;
    resql *client;
    resql_result *rs;

    printf("[ Running ] %s \n", name);

    struct resql_config conf = {
            .client_name = "c-client",
            .cluster_name = "cluster",
            .timeout_millis = 30000,
            .urls = "tcp://127.0.0.1:7600",
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

    fn();

    resql_put_sql(client, "DROP TABLE ctest;");
    rc = resql_exec(client, false, &rs);
    if (rc != RESQL_OK) {
        printf("Failed : %s \n", resql_errstr(client));
        exit(1);
    }

    rc = resql_shutdown(client);
    if (rc != RESQL_OK) {
        printf("Failed to destroy client \n");
        exit(1);
    }

    printf("[ Passed  ] %s  \n", name);
}

static void example7()
{
    int rc;
    resql *r;
    struct resql_result *rs;

    rc = resql_create(&r, &(struct resql_config){0});
    if (rc != RESQL_OK) {
        printf("Failed to create client \n");
        exit(1);
    }

    resql_put_sql(r, "CREATE TABLE test (key TEXT, value TEXT);");
    resql_put_sql(r, "INSERT INTO test VALUES('mykey', 1000);");
    rc = resql_exec(r, false, &rs);
    if (rc != RESQL_OK) {
        printf("Operation failed : %s \n", resql_errstr(r));
        exit(1);
    }

    // Demo for getAndIncrement atomically.
    resql_put_sql(r, "SELECT * FROM test WHERE key = 'mykey';");
    resql_put_sql(r, "UPDATE test SET value = value + 1 WHERE key = 'mykey'");
    resql_put_sql(r, "SELECT * FROM test WHERE key = 'mykey';");
    rc = resql_exec(r, false, &rs);
    if (rc != RESQL_OK) {
        printf("Operation failed : %s \n", resql_errstr(r));
        exit(1);
    }

    // rs has three result sets, each corresponds to operations
    // that we added into the batch.

    // First operation was SELECT
    struct resql_column *row = resql_row(rs);
    printf("Value was : %s \n", row[1].text);

    // Advance to the next result set which is for UPDATE.
    resql_next(rs);
    printf("Line changed : %d \n", resql_changes(rs));

    // Advance to the next result set which is for SELECT again.
    resql_next(rs);
    row = resql_row(rs);
    printf("Value is now : %s \n", row[1].text);

    // Clean-up
    resql_put_sql(r, "DROP TABLE test;");
    rc = resql_exec(r, false, &rs);
    if (rc != RESQL_OK) {
        printf("Operation failed : %s \n", resql_errstr(r));
        exit(1);
    }

    resql_shutdown(r);
}


int main(int argc, char *argv[])
{
    (void) argc;
    (void) argv;

    resql_init();
    init_all();

    test_server_create(0, 3);
    test_server_create(1, 3);
    test_server_create(2, 3);

    test_client_execute(test_last_insert_id);
    test_client_execute(test_reset);
    test_client_execute(test_single_index);
    test_client_execute(test_single_param);
    test_client_execute(test_prepared_index);
    test_client_execute(test_prepared_param);
    test_client_execute(test_prepared_param_many);
    test_client_execute(test_fail);
    test_client_execute(test_connect);
    test_client_execute(example1);
    test_client_execute(example2);
    test_client_execute(example3);
    test_client_execute(example4);
    test_client_execute(example5);
    test_client_execute(example6);
    test_client_execute(example7);

    resql_term();

    test_client_destroy_all();
    test_server_destroy_all();

    return 0;
}
