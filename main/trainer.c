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

#include "c/resql.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    int rc;
    int ops =0 ;
    resql *c;
    resql_result *rs;

    struct resql_config config = {
            .uris = "tcp://127.0.0.1:8080",
            .timeout = 10000,
            .client_name = "cli",
            .cluster_name = "cluster",
    };

    rc = resql_create(&c, &config);
    if (rc != RESQL_OK) {
        printf("Failed to connect to server.");
        exit(EXIT_FAILURE);
    }

    resql_put_sql(c, "DROP TABLE IF EXISTS test;");
    resql_put_sql(c, "CREATE TABLE test (key INTEGER, value TEXT);");
    rc = resql_exec(c, false, &rs);
    if (rc != RESQL_OK) {
        printf("Failed : %s ", resql_errstr(c));
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < 1000; i++) {
        ops++;
        resql_put_sql(c, "INSERT INTO test VALUES (:key,:value);");
        resql_bind_param(c, ":key", "%d", i);
        resql_bind_param(c, ":value", "%s", "data");

        rc = resql_exec(c, false, &rs);
        if (rc != RESQL_OK) {
            printf("Failed : %s ", resql_errstr(c));
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < 10000; i++) {
        ops++;
        resql_put_sql(c, "SELECT * FROM test WHERE key = ?;");
        resql_bind_index(c, 0, "%d", i % 1000);
        rc = resql_exec(c, true, &rs);
        if (rc != RESQL_OK) {
            printf("Failed : %s ", resql_errstr(c));
            exit(EXIT_FAILURE);
        }
    }

    resql_put_sql(c, "SELECT resql('shutdown', '*');");
    rc = resql_exec(c, false, &rs);
    if (rc != RESQL_OK) {
        printf("Failed : %s ", resql_errstr(c));
        exit(EXIT_FAILURE);
    }

    printf("trainer done(%d).\n", ops);

    return 0;
}
