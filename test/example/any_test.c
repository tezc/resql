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
#include "sc/sc_uri.h"

#include <stdio.h>

int main()
{
    resql *c;
    resql_result *rs;
    const char *urls =
            "tcp://127.0.0.1:8080";

    struct resql_config config = {.cluster_name = "cluster",
            .client_name = "any",
            .timeout = 10000,
            .urls = urls};

    resql_create(&c, &config);

    resql_put_sql(c, "SELECT count(*) FROM multi;");
    resql_exec(c, true, &rs);

    while (resql_result_next(rs)) {
        while (resql_result_row(rs)) {
            printf("%s \n", resql_result_column_name(rs, 0));
            printf("%ld \n", resql_result_int(rs, 0));
        }
    }

    return 0;
}

