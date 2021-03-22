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
#include "rs.h"
#include "server.h"
#include "test_util.h"

#include "sc/sc_log.h"

#include <unistd.h>

static void snapshot_simple()
{
    file_clear_dir("/tmp/node0", ".resql");

    int rc, x;
    char tmp[32];
    resql *c;
    struct resql_column *row;
    struct resql_result *rs = NULL;

    test_server_create(0, 1);
    c = test_client_create();

    resql_put_sql(c, "CREATE TABLE snapshot (key TEXT, value TEXT);");

    rc = resql_exec(c, false, &rs);
    client_assert(c, rc == RESQL_OK);

    for (int i = 0; i < 1000; i++) {
        for (int j = 0; j < 1000; j++) {
            snprintf(tmp, sizeof(tmp), "%d", (i * 1000) + j);

            resql_put_sql(c, "INSERT INTO snapshot VALUES(:key, 'value')");
            resql_bind_param_text(c, ":key", tmp);
        }

        rc = resql_exec(c, false, &rs);
        client_assert(c, rc == RESQL_OK);
    }

    test_server_destroy_all();
    test_server_start(0, 1);

    resql_put_sql(c, "Select count(*) from snapshot;");
    rc = resql_exec(c, false, &rs);
    client_assert(c, rc == RESQL_OK);

    assert(resql_row_count(rs) == 1);
    assert(resql_row(rs)[0].intval == 1000000);

    resql_put_sql(c, "Select * from snapshot;");
    rc = resql_exec(c, false, &rs);
    client_assert(c, rc == RESQL_OK);

    x = 0;
    assert(resql_row_count(rs) == 1000000);

    while ((row = resql_row(rs)) != NULL) {
        snprintf(tmp, sizeof(tmp), "%d", x++);

        assert(row[0].type == RESQL_TEXT);
        assert(strcmp(tmp, row[0].text) == 0);
    }

    assert(resql_next(rs) == false);
}

int main(void)
{
    test_execute(snapshot_simple);

    return 0;
}
