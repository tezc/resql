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

#include "file.h"
#include "resql.h"
#include "rs.h"
#include "server.h"
#include "test_util.h"

#include "sc/sc_log.h"

#include <unistd.h>

static void snapshot_simple()
{
	int rc, x;
	char tmp[32];
	resql *c;
	struct resql_column *row;
	struct resql_result *rs = NULL;

	test_server_create(0, 3);
	test_server_create(1, 3);
	c = test_client_create();

	resql_put_sql(c, "CREATE TABLE snapshot (key TEXT, value TEXT);");

	rc = resql_exec(c, false, &rs);
	client_assert(c, rc == RESQL_OK);

	for (int i = 0; i < 1000; i++) {
		for (int j = 0; j < 1000; j++) {
			snprintf(tmp, sizeof(tmp), "%d", (i * 1000) + j);

			resql_put_sql(
				c,
				"INSERT INTO snapshot VALUES(:key, 'value')");
			resql_bind_param_text(c, ":key", tmp);
		}

		rc = resql_exec(c, false, &rs);
		client_assert(c, rc == RESQL_OK);
	}

	test_server_destroy_all();
	test_server_start(0, 3);
	test_server_start(1, 3);
	test_server_start(2, 3);

	resql_put_sql(c, "Select count(*) from snapshot;");
	rc = resql_exec(c, true, &rs);
	client_assert(c, rc == RESQL_OK);

	rs_assert(resql_row_count(rs) == 1);
	rs_assert(resql_row(rs)[0].intval == 1000000);

	resql_put_sql(c, "Select * from snapshot LIMIT 100;");
	rc = resql_exec(c, true, &rs);
	client_assert(c, rc == RESQL_OK);

	x = 0;
	rs_assert(resql_row_count(rs) == 100);

	while ((row = resql_row(rs)) != NULL) {
		snprintf(tmp, sizeof(tmp), "%d", x++);

		rs_assert(row[0].type == RESQL_TEXT);
		rs_assert(strcmp(tmp, row[0].text) == 0);
	}

	rs_assert(resql_next(rs) == false);
}

int main(void)
{
	test_execute(snapshot_simple);

	return 0;
}
