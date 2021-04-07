/*
 * MIT License
 *
 * Copyright (c) 2021 Ozan Tezcan
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "test_util.h"

#include "resql.h"
#include "sc/sc_uri.h"

#include <conf.h>
#include <server.h>
#include <stdio.h>

void write_test()
{
    int rc;
    int count;
    char tmp[32];
    resql *c;
    resql_result *rs;
    resql_stmt stmt1, stmt2;
    struct resql_column* row;

    test_server_create(0, 3);
    test_server_create(1, 3);
    test_server_create(2, 3);

    c = test_client_create();

    rc = resql_prepare(c, "SELECT 1", &stmt1);
    client_assert(c, rc == RESQL_OK);

    rc = resql_prepare(c, "SELECT 1", &stmt2);
    client_assert(c, rc == RESQL_OK);

    test_destroy_leader();
    test_destroy_leader();
    test_server_create_auto(3);

    resql_put_sql(c, "DROP TABLE IF EXISTS snapshot;");
    resql_put_sql(c, "CREATE TABLE snapshot (key TEXT, value TEXT);");

    rc = resql_exec(c, false, &rs);
    client_assert(c, rc == RESQL_OK);

    for (int i = 0; i < 1000; i++) {
        snprintf(tmp, sizeof(tmp), "%d", i);

        resql_put_sql(c, "INSERT INTO snapshot VALUES(:key, 'value')");
        resql_bind_param_text(c, ":key", tmp);

        rc = resql_exec(c, false, &rs);
        client_assert(c, rc == RESQL_OK);
    }

    for (int i = 1000; i < 2000; i++) {
        snprintf(tmp, sizeof(tmp), "%d", i);

        resql_put_sql(c, "INSERT INTO snapshot VALUES(:key, 'value')");
        resql_bind_param_text(c, ":key", tmp);

        rc = resql_exec(c, false, &rs);
        client_assert(c, rc == RESQL_OK);
    }

    resql_put_sql(c, "SELECT * FROM snapshot;");
    rc = resql_exec(c, true, &rs);
    client_assert(c, rc == RESQL_OK);

    count = resql_row_count(rs);
    rs_assert(count == 2000);

    for (int i = 0; i < count; i++) {
        snprintf(tmp, sizeof(tmp), "%d", i);
        row = resql_row(rs);
        rs_assert(strcmp(row[0].text, tmp) == 0);
        rs_assert(strcmp(row[1].text, "value") == 0);
    }
}

void restart_test()
{
    int rc;
    int count;
    char tmp[32];
    resql *c;
    resql_result *rs;
    resql_stmt stmt1, stmt2;
    struct resql_column* row;

    test_server_create(0, 3);
    test_server_create(1, 3);
    test_server_create(2, 3);

    c = test_client_create();

    rc = resql_prepare(c, "SELECT 1", &stmt1);
    client_assert(c, rc == RESQL_OK);

    rc = resql_prepare(c, "SELECT 1", &stmt2);
    client_assert(c, rc == RESQL_OK);

    test_server_destroy(0);
    test_server_destroy(1);
    test_server_start(0, 3);
    test_server_start(1, 3);

    resql_put_sql(c, "DROP TABLE IF EXISTS snapshot;");
    resql_put_sql(c, "CREATE TABLE snapshot (key TEXT, value TEXT);");

    rc = resql_exec(c, false, &rs);
    client_assert(c, rc == RESQL_OK);

    for (int i = 0; i < 1000; i++) {
        snprintf(tmp, sizeof(tmp), "%d", i);

        resql_put_sql(c, "INSERT INTO snapshot VALUES(:key, 'value')");
        resql_bind_param_text(c, ":key", tmp);

        rc = resql_exec(c, false, &rs);
        client_assert(c, rc == RESQL_OK);
    }

    for (int i = 1000; i < 2000; i++) {
        snprintf(tmp, sizeof(tmp), "%d", i);

        resql_put_sql(c, "INSERT INTO snapshot VALUES(:key, 'value')");
        resql_bind_param_text(c, ":key", tmp);

        rc = resql_exec(c, false, &rs);
        client_assert(c, rc == RESQL_OK);
    }

    resql_put_sql(c, "SELECT * FROM snapshot;");
    rc = resql_exec(c, true, &rs);
    client_assert(c, rc == RESQL_OK);

    count = resql_row_count(rs);
    rs_assert(count == 2000);

    for (int i = 0; i < count; i++) {
        snprintf(tmp, sizeof(tmp), "%d", i);
        row = resql_row(rs);
        rs_assert(strcmp(row[0].text, tmp) == 0);
        rs_assert(strcmp(row[1].text, "value") == 0);
    }
}

int main()
{
    test_execute(write_test);
    test_execute(restart_test);
}
