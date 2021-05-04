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

#include <stdio.h>

void write_test()
{
	int rc;
	int count;
	char tmp[32];
	resql *c;
	resql_result *rs;
	resql_stmt stmt1, stmt2;
	struct resql_column *row;

	test_server_create(0, 3);
	test_server_create(1, 3);
	test_server_create(2, 3);

	c = test_client_create();

	rc = resql_prepare(c, "SELECT 1", &stmt1);
	client_assert(c, rc == RESQL_OK);

	rc = resql_prepare(c, "SELECT 1", &stmt2);
	client_assert(c, rc == RESQL_OK);

	test_server_destroy_leader();
	test_server_destroy_leader();
	test_server_start_auto(3);

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

	resql_put_sql(c, "SELECT * FROM snapshot;");
	rc = resql_exec(c, true, &rs);
	client_assert(c, rc == RESQL_OK);

	count = resql_row_count(rs);
	rs_assert(count == 2000);

	for (int i = 0; i < count; i++) {
		snprintf(tmp, sizeof(tmp), "%d", i);
		row = resql_row(rs);
		rs_assert(strcmp(row[0].text, tmp) == 0);
		rs_assert(strcmp(row[1].text, "value") == 0);
	}
}

void restart_test()
{
	int rc;
	int count;
	char tmp[32];
	resql *c;
	resql_result *rs;
	resql_stmt stmt1, stmt2;
	struct resql_column *row;

	test_server_create(0, 3);
	test_server_create(1, 3);
	test_server_create(2, 3);

	c = test_client_create();

	rc = resql_prepare(c, "SELECT 1", &stmt1);
	client_assert(c, rc == RESQL_OK);

	rc = resql_prepare(c, "SELECT 1", &stmt2);
	client_assert(c, rc == RESQL_OK);

	test_server_destroy(0);
	test_server_destroy(1);
	test_server_start(0, 3);
	test_server_start(1, 3);

	resql_put_sql(c, "DROP TABLE IF EXISTS snapshot;");
	resql_put_sql(c, "CREATE TABLE snapshot (key TEXT, value TEXT);");

	rc = resql_exec(c, false, &rs);
	client_assert(c, rc == RESQL_OK);

	printf("Client created tables after restart \n");

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

	resql_put_sql(c, "SELECT * FROM snapshot;");
	rc = resql_exec(c, true, &rs);
	client_assert(c, rc == RESQL_OK);

	count = resql_row_count(rs);
	rs_assert(count == 2000);

	for (int i = 0; i < count; i++) {
		snprintf(tmp, sizeof(tmp), "%d", i);
		row = resql_row(rs);
		rs_assert(strcmp(row[0].text, tmp) == 0);
		rs_assert(strcmp(row[1].text, "value") == 0);
	}
}

int main()
{
	test_execute(write_test);
	test_execute(restart_test);

	return 0;
}
