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

#include "test_util.h"

#include "resql.h"
#include "sc/sc_uri.h"

#include <conf.h>
#include <server.h>
#include <stdio.h>

void write_test()
{
    int rc;
    char tmp[32];
    resql *c;
    resql_result *rs;

    test_server_create(0, 3);
    test_server_create(1, 3);
    test_server_create(2, 3);

    c = test_client_create();

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
}

int main()
{
    test_execute(write_test);
}
