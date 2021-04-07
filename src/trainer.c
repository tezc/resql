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

#include "resql.h"

#include <stdio.h>
#include <stdlib.h>

void train_single()
{
    int rc;
    int ops = 0;
    resql *c;
    resql_result *rs;
    resql_stmt stmt;

    struct resql_config config = {
            .urls = "tcp://127.0.0.1:9717",
            .timeout_millis = 10000,
            .cluster_name = "cluster",
    };

    rc = resql_create(&c, &config);
    if (rc != RESQL_OK) {
        printf("Failed to connect to server. \n");
        exit(EXIT_FAILURE);
    }

    resql_put_sql(c, "DROP TABLE IF EXISTS test;");
    resql_put_sql(c, "CREATE TABLE test (key INTEGER, value TEXT);");
    rc = resql_exec(c, false, &rs);
    if (rc != RESQL_OK) {
        printf("Failed : %s \n", resql_errstr(c));
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 100; i++) {
        ops++;
        resql_put_sql(c, "INSERT INTO test VALUES (:key,:value);");
        resql_bind_param_int(c, ":key", i);
        resql_bind_param_text(c, ":value", "data");

        rc = resql_exec(c, false, &rs);
        if (rc != RESQL_OK) {
            printf("Failed : %s \n", resql_errstr(c));
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < 100; i++) {
        ops++;
        resql_put_sql(c, "SELECT * FROM test WHERE key = ?;");
        resql_bind_index_int(c, 0, i % 95);
        rc = resql_exec(c, true, &rs);
        if (rc != RESQL_OK) {
            printf("Failed : %s \n", resql_errstr(c));
            exit(EXIT_FAILURE);
        }
    }

    rc = resql_prepare(c, "SELECT * FROM test WHERE key = ?;", &stmt);
    if (rc != RESQL_OK) {
        printf("Failed : %s \n", resql_errstr(c));
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 100; i++) {
        ops++;
        resql_put_prepared(c, &stmt);
        resql_bind_index_int(c, 0, i % 95);
        rc = resql_exec(c, true, &rs);
        if (rc != RESQL_OK) {
            printf("Failed : %s \n", resql_errstr(c));
            exit(EXIT_FAILURE);
        }
    }

    resql_shutdown(c);

    printf("train_single done (%d).\n", ops);
}

void train_multi()
{
    int rc;
    int ops = 0;
    resql *c;
    resql_result *rs;
    resql_stmt stmt;

    struct resql_config config = {
            .urls = "tcp://127.0.0.1:9717",
            .timeout_millis = 10000,
            .cluster_name = "cluster",
    };

    rc = resql_create(&c, &config);
    if (rc != RESQL_OK) {
        printf("Failed to connect to server. \n");
        exit(EXIT_FAILURE);
    }

    resql_put_sql(c, "DROP TABLE IF EXISTS multi;");
    resql_put_sql(
            c,
            "CREATE TABLE multi (id INTEGER PRIMARY KEY, name TEXT, points FLOAT, data BLOB, date TEXT);");
    rc = resql_exec(c, false, &rs);
    if (rc != RESQL_OK) {
        printf("Failed : %s \n", resql_errstr(c));
        exit(EXIT_FAILURE);
    }

    rc = resql_prepare(c, "INSERT INTO multi VALUES(?, ?, ?, ?, ?);", &stmt);
    if (rc != RESQL_OK) {
        printf("Failed : %s \n", resql_errstr(c));
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 100; i++) {
        ops++;
        resql_put_prepared(c, &stmt);
        resql_bind_index_int(c, 0, i);
        resql_bind_index_text(c, 1, "jane");
        resql_bind_index_float(c, 2, 3.22);
        resql_bind_index_blob(c, 3, 5, "blob");
        resql_bind_index_null(c, 4);

        rc = resql_exec(c, false, &rs);
        if (rc != RESQL_OK) {
            printf("Failed : %s \n", resql_errstr(c));
            exit(EXIT_FAILURE);
        }
    }

    rc = resql_prepare(c, "SELECT * FROM multi WHERE id = ?;", &stmt);
    if (rc != RESQL_OK) {
        printf("Failed : %s \n", resql_errstr(c));
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 100; i++) {
        ops++;
        resql_put_prepared(c, &stmt);
        resql_bind_index_int(c, 0, i + 5);
        rc = resql_exec(c, true, &rs);
        if (rc != RESQL_OK) {
            printf("Failed : %s \n", resql_errstr(c));
            exit(EXIT_FAILURE);
        }
    }

    resql_shutdown(c);

    printf("train multi done (%d).\n", ops);
}

int main(int argc, char **argv)
{
    (void) argc;
    (void) argv;

    int rc;
    int ops = 0;
    resql *c;
    resql_result *rs;

    struct resql_config config = {
            .urls = "tcp://127.0.0.1:9717",
            .timeout_millis = 10000,
            .cluster_name = "cluster",
    };

    rc = resql_create(&c, &config);
    if (rc != RESQL_OK) {
        printf("Failed to connect to server. \n");
        exit(EXIT_FAILURE);
    }

    train_single();
    train_multi();

    resql_put_sql(c, "SELECT resql('shutdown', '*');");
    rc = resql_exec(c, false, &rs);
    if (rc != RESQL_OK) {
        printf("Failed : %s \n", resql_errstr(c));
        exit(EXIT_FAILURE);
    }

    printf("trainer done(%d).\n", ops);

    return 0;
}
