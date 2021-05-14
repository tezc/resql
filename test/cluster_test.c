/*
 * BSD-3-Clause
 *
 * Copyright 2021 Ozan Tezcan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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

	test_server_create(true, 0, 2);
	test_server_create(false, 1, 2);

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

	test_server_create(true, 0, 3);
	test_server_create(false, 1, 3);
	test_server_create(true, 2, 3);

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

	test_server_destroy_leader();

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

	test_server_create(false, 0, 7);
	test_server_create(true, 1, 7);
	test_server_create(true, 2, 7);
	test_server_create(false, 3, 7);
	test_server_create(true, 4, 7);
	test_server_create(false, 5, 7);
	test_server_create(true, 6, 7);

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
		test_server_destroy_leader();

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
	test_server_create(true, 0, 3);
	test_server_create(false, 1, 3);
	test_server_create(true, 2, 3);

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
