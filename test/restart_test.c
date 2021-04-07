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

static void restart_simple()
{
	file_clear_dir("/tmp/node0", ".resql");

	int rc;
	resql *c;
	struct resql_column *row;
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
	resql *c;
	struct resql_column *row;
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
