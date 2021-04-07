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

#include "conf.h"
#include "resql.h"
#include "server.h"
#include "test_util.h"

#include "sc/sc_uri.h"

#include <stdio.h>
#include <unistd.h>

void write_test()
{
	int rc;
	char tmp[32];
	resql *c;
	resql_result *rs;

	test_server_create(0, 2);
	test_server_create(1, 2);

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
}

void kill_test()
{
	int rc;
	char *l1, *l2, *l3;
	const char *n1, *n2, *n3;
	resql *c;
	resql_result *rs;

	test_server_create(0, 3);
	test_server_create(1, 3);
	test_server_create(2, 3);

	c = test_client_create();

	resql_put_sql(c, "DROP TABLE IF EXISTS test;");
	resql_put_sql(c, "CREATE TABLE test (x TEXT);");
	resql_put_sql(c, "INSERT INTO test VALUES(strftime('%s','now'));");
	resql_put_sql(c, "INSERT INTO test VALUES(datetime());");
	resql_put_sql(c, "INSERT INTO test VALUES(random());");
	rc = resql_exec(c, false, &rs);
	client_assert(c, rc == RESQL_OK);

	resql_put_sql(c, "SELECT * FROM test");
	rc = resql_exec(c, true, &rs);
	client_assert(c, rc == RESQL_OK);

	rs_assert(resql_row_count(rs) == 3);

	l1 = strdup(resql_row(rs)[0].text);
	l2 = strdup(resql_row(rs)[0].text);
	l3 = strdup(resql_row(rs)[0].text);

	test_destroy_leader();

	resql_put_sql(c, "SELECT * FROM test");
	rc = resql_exec(c, true, &rs);
	client_assert(c, rc == RESQL_OK);

	rs_assert(resql_row_count(rs) == 3);

	n1 = resql_row(rs)[0].text;
	n2 = resql_row(rs)[0].text;
	n3 = resql_row(rs)[0].text;

	rs_assert(strcmp(l1, n1) == 0);
	rs_assert(strcmp(l2, n2) == 0);
	rs_assert(strcmp(l3, n3) == 0);

	free(l1);
	free(l2);
	free(l3);
}

void kill2_test()
{
	int rc;
	char *l1, *l2, *l3;
	const char *n1, *n2, *n3;
	resql *c;
	resql_result *rs;

	test_server_create(0, 7);
	test_server_create(1, 7);
	test_server_create(2, 7);
	test_server_create(3, 7);
	test_server_create(4, 7);
	test_server_create(5, 7);
	test_server_create(6, 7);

	c = test_client_create();

	resql_put_sql(c, "DROP TABLE IF EXISTS test;");
	resql_put_sql(c, "CREATE TABLE test (x TEXT);");
	resql_put_sql(c, "INSERT INTO test VALUES(strftime('%s','now'));");
	resql_put_sql(c, "INSERT INTO test VALUES(datetime());");
	resql_put_sql(c, "INSERT INTO test VALUES(random());");
	rc = resql_exec(c, false, &rs);
	client_assert(c, rc == RESQL_OK);

	resql_put_sql(c, "SELECT * FROM test");
	rc = resql_exec(c, true, &rs);
	client_assert(c, rc == RESQL_OK);

	rs_assert(resql_row_count(rs) == 3);

	l1 = strdup(resql_row(rs)[0].text);
	l2 = strdup(resql_row(rs)[0].text);
	l3 = strdup(resql_row(rs)[0].text);

	for (int i = 0; i < 3; i++) {
		test_destroy_leader();

		resql_put_sql(c, "SELECT * FROM test");
		rc = resql_exec(c, true, &rs);
		client_assert(c, rc == RESQL_OK);

		rs_assert(resql_row_count(rs) == 3);

		n1 = resql_row(rs)[0].text;
		n2 = resql_row(rs)[0].text;
		n3 = resql_row(rs)[0].text;

		rs_assert(strcmp(l1, n1) == 0);
		rs_assert(strcmp(l2, n2) == 0);
		rs_assert(strcmp(l3, n3) == 0);
	}

	free(l1);
	free(l2);
	free(l3);
}

void pause_test()
{
	test_server_create(0, 3);
	test_server_create(1, 3);
	test_server_create(2, 3);

	sleep(10);

	test_client_create();
}

int main()
{
	test_execute(kill2_test);
	test_execute(pause_test);
	test_execute(kill_test);
	test_execute(write_test);
}
