#include "resql.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

resql* c;
resql* u;

static void assert_client(bool b)
{
    if (!b) {
        printf("Failed %s \n", resql_errstr(c));
        exit(1);
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
    assert(col[2].floatval ==  3.11);

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
    assert(col[2].floatval ==  3.11);

    assert(col[3].type == RESQL_BLOB);
    assert(strcmp(col[3].name, "data") == 0);
    assert(col[3].len == 5);
    assert(strcmp(col[3].blob, "blob") == 0);
}

void run(void (*fn)())
{
    int rc;
    resql* client, *unixclient;
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

    run(test_single_index);
    run(test_single_param);

    resql_term();

    return 0;
}
