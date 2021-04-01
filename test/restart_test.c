/*
 *  Resql
 *
 *  Copyright (C) 2021 Ozan Tezcan
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
#include "file.h"
#include "test_util.h"

#include "resql.h"
#include "sc/sc_log.h"

#include <unistd.h>

static void restart_simple()
{
    file_clear_dir("/tmp/node0", ".resql");

    int rc;
    resql* c;
    struct resql_column* row;
    struct resql_result *rs = NULL;

    test_server_create(0, 1);
    c = test_client_create();

    for (int i = 0; i < 100; i++) {

        resql_put_sql(c, "Select 'resql'");

        rc = resql_exec(c, true, &rs);
        client_assert(c, rc == RESQL_OK);

        do {
            rs_assert(resql_column_count(rs) == 1);
            row = resql_row(rs);

            while (resql_row(rs)) {
                rs_assert(row[0].type == RESQL_TEXT);
                rs_assert(strcmp("resql", row[0].text) == 0);
            }
        } while (resql_next(rs));
    }

    test_server_destroy_all();
    test_server_start(0, 1);

    for (int i = 0; i < 100; i++) {
        resql_put_sql(c, "Select 'resql'");

        rc = resql_exec(c, false, &rs);
        client_assert(c, rc == RESQL_OK);

        do {
            rs_assert(resql_column_count(rs) == 1);
            row = resql_row(rs);

            while (resql_row(rs)) {
                rs_assert(row[0].type == RESQL_TEXT);
                rs_assert(strcmp("resql", row[0].text) == 0);
            }
        } while (resql_next(rs));
    }
}

static void restart_simple2()
{
    file_clear_dir("/tmp/node0", ".resql");

    int rc;
    resql* c;
    struct resql_column* row;
    struct resql_result *rs = NULL;

    test_server_create(0, 1);
    c = test_client_create();

    for (int i = 0; i < 100; i++) {

        resql_put_sql(c, "Select 'resql'");

        rc = resql_exec(c, true, &rs);
        client_assert(c, rc == RESQL_OK);

        do {
            rs_assert(resql_column_count(rs) == 1);
            row = resql_row(rs);

            while (resql_row(rs)) {
                rs_assert(row[0].type == RESQL_TEXT);
                rs_assert(strcmp("resql", row[0].text) == 0);
            }
        } while (resql_next(rs));
    }

    test_server_destroy_all();

    test_server_start(0, 1);
    test_server_destroy_all();

    test_client_destroy_all();
    test_server_start(0, 1);
    test_client_create();
}

int main(void)
{
    test_execute(restart_simple);
    test_execute(restart_simple2);

    return 0;
}
