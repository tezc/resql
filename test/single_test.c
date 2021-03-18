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

#include "resql.h"
#include "test_util.h"

#include <assert.h>

void test_one()
{
    int rc;
    resql *c;

    test_server_create(0, 1);

    rc = resql_create(&c, &(struct resql_config){0});
    assert(rc == RESQL_OK);

    resql_shutdown(c);
    test_server_stop(0);
}

void test_sizes()
{
    int rc;
    resql *c;

    for (int i = 1; i <= 9; i++) {
        for (int j = 0; j < i; j++) {
            test_server_create(j, i);
        }

        rc = resql_create(&c, &(struct resql_config){0});
        assert(rc == RESQL_OK);
        resql_shutdown(c);

        for (int j = 0; j < i; j++) {
            test_server_stop(j);
        }
    }
}

void test_client()
{
    int rc;
    resql *c;
    resql_result *rs;

    test_server_create(0, 1);

    rc = resql_create(&c, &(struct resql_config){0});
    assert(rc == RESQL_OK);

    resql_put_sql(c, "SELECT * FROM resql_sessions;");
    rc = resql_exec(c, true, &rs);
    client_assert(c, rc == RESQL_OK);

    resql_shutdown(c);
    test_server_stop(0);
}

int main()
{
    test_execute(test_client);
    test_execute(test_one);
    test_execute(test_sizes);
    return 0;
}
