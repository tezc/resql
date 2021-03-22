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

#include "resql.h"
#include "test_util.h"

void test_one()
{
    test_server_create(0, 1);
    test_client_create();
}

void test_sizes()
{
    for (int i = 1; i <= 9; i++) {
        for (int j = 0; j < i; j++) {
            test_server_create(j, i);
        }

        test_client_create();
        test_client_destroy_all();
        test_server_destroy_all();
    }
}

void test_client()
{
    int rc;
    resql *c;
    resql_result *rs;

    test_server_create(0, 1);
    c = test_client_create();

    resql_put_sql(c, "SELECT * FROM resql_sessions;");
    rc = resql_exec(c, true, &rs);
    client_assert(c, rc == RESQL_OK);
}

int main()
{
    test_execute(test_one);
    test_execute(test_client);
    test_execute(test_sizes);
    return 0;
}
