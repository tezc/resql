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

#include "resql.h"
#include "rs.h"
#include "server.h"
#include "test_util.h"

#include "sc/sc_log.h"

#include <unistd.h>

static void client_simple()
{
	int rc;
	struct resql *c;
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

			while ((row = resql_row(rs)) != NULL) {
				rs_assert(row[0].type == RESQL_TEXT);
				rs_assert(strcmp("resql", row[0].text) == 0);
			}

		} while (resql_next(rs));
	}

	for (int i = 0; i < 100; i++) {
		resql_put_sql(c, "Select 'resql'");

		rc = resql_exec(c, false, &rs);
		client_assert(c, rc == RESQL_OK);

		do {
			rs_assert(resql_column_count(rs) == 1);

			while ((row = resql_row(rs)) != NULL) {
				rs_assert(row[0].type == RESQL_TEXT);
				rs_assert(strcmp("resql", row[0].text) == 0);
			}
		} while (resql_next(rs));
	}
}

static void client_prepared()
{
	int rc;
	resql *c;
	resql_result *rs = NULL;
	resql_stmt stmt = 0;
	struct resql_column *row;

	test_server_create(0, 1);
	c = test_client_create();

	rc = resql_del_prepared(c, &stmt);
	client_assert(c, rc == RESQL_SQL_ERROR);

	resql_put_sql(
		c,
		"CREATE TABLE test (a INTEGER PRIMARY KEY, b TEXT, c FLOAT, d BLOB);");
	rc = resql_exec(c, false, &rs);
	client_assert(c, rc == RESQL_OK);

	rc = resql_prepare(
		c, "INSERT INTO test VALUES (:key1, :key2, :key3, :key4);",
		&stmt);
	client_assert(c, rc == RESQL_OK);

	for (int i = 0; i < 100; i++) {
		resql_put_prepared(c, &stmt);
		resql_bind_param_int(c, ":key1", i);
		resql_bind_param_text(c, ":key2", "test");
		resql_bind_param_float(c, ":key3", (double) 3.11);
		resql_bind_param_blob(c, ":key4", 5, "blob");

		rc = resql_exec(c, false, &rs);
		client_assert(c, rc == RESQL_OK);

		do {
			while (resql_row(rs)) {
				rs_assert(true);
			}
		} while (resql_next(rs));
	}

	rc = resql_del_prepared(c, &stmt);
	client_assert(c, rc == RESQL_OK);

	resql_put_sql(c, "SELECT * FROM test");
	rc = resql_exec(c, false, &rs);
	client_assert(c, rc == RESQL_OK);

	int x = 0;
	do {
		rs_assert(resql_column_count(rs) == 4);

		while ((row = resql_row(rs)) != NULL) {
			rs_assert(row[0].type == RESQL_INTEGER);
			rs_assert(row[1].type == RESQL_TEXT);
			rs_assert(row[2].type == RESQL_FLOAT);
			rs_assert(row[3].type == RESQL_BLOB);
			rs_assert(strcmp("a", row[0].name) == 0);
			rs_assert(strcmp("b", row[1].name) == 0);
			rs_assert(strcmp("c", row[2].name) == 0);
			rs_assert(strcmp("d", row[3].name) == 0);

			rs_assert(row[0].intval == x++);
			rs_assert(strcmp(row[1].text, "test") == 0);
			rs_assert(row[2].floatval == (double) 3.11);
			rs_assert(row[3].len == 5);
			rs_assert(strcmp(row[3].blob, "blob") == 0);
		}
	} while (resql_next(rs));
}

static void client_error()
{
	int rc;
	struct resql_result *rs = NULL;
	struct resql_column *row;
	resql *c;
	resql_stmt stmt = 0;

	test_server_create(0, 1);
	c = test_client_create();

	rc = resql_del_prepared(c, &stmt);
	rs_assert(rc == RESQL_SQL_ERROR);

	resql_put_sql(
		c,
		"CREATE TABLE test (a INTEGER PRIMARY KEY, b TEXT, c FLOAT, d BLOB);");
	rc = resql_exec(c, false, &rs);
	client_assert(c, rc == RESQL_OK);

	resql_put_sql(c,
		      "INSERT INTO test VALUES (:key1, :key2, :key3, :key4);");
	resql_bind_param_int(c, ":key1", 1);
	resql_bind_param_text(c, ":key2", "test");
	resql_bind_param_float(c, ":key3", 3.11);
	resql_bind_param_blob(c, ":key4", 5, "blob");

	resql_put_sql(c,
		      "INSERT INTO test VALUES (:key1, :key2, :key3, :key4);");
	resql_bind_param_int(c, ":key1", 1);
	resql_bind_param_text(c, ":key2", "test");
	resql_bind_param_float(c, ":key3", 3.11);
	resql_bind_param_blob(c, ":key4", 5, "blob");

	rc = resql_exec(c, false, &rs);
	client_assert(c, rc == RESQL_SQL_ERROR);

	resql_put_sql(c, "SELECT * FROM test");
	rc = resql_exec(c, false, &rs);
	client_assert(c, rc == RESQL_OK);

	while (resql_next(rs)) {
		rs_assert(true);
	}

	resql_put_sql(c,
		      "INSERT INTO test VALUES (:key1, :key2, :key3, :key4);");
	resql_bind_param_int(c, ":key1", 0);
	resql_bind_param_text(c, ":key2", "test");
	resql_bind_param_float(c, ":key3", 3.11);
	resql_bind_param_blob(c, ":key4", 5, "blob");

	resql_put_sql(c,
		      "INSERT INTO test VALUES (:key1, :key2, :key3, :key4);");
	resql_bind_param_int(c, ":key1", 1);
	resql_bind_param_text(c, ":key2", "test");
	resql_bind_param_float(c, ":key3", 3.11);
	resql_bind_param_blob(c, ":key4", 5, "blob");

	rc = resql_exec(c, false, &rs);
	client_assert(c, rc == RESQL_OK);

	int k = 0;
	do {
		k++;
	} while (resql_next(rs));

	rs_assert(k == 2);

	resql_put_sql(c, "SELECT * FROM test");
	rc = resql_exec(c, false, &rs);
	client_assert(c, rc == RESQL_OK);

	int x = 0;
	do {
		rs_assert(resql_column_count(rs) == 4);

		while ((row = resql_row(rs)) != NULL) {

			rs_assert(row[0].type == RESQL_INTEGER);
			rs_assert(row[1].type == RESQL_TEXT);
			rs_assert(row[2].type == RESQL_FLOAT);
			rs_assert(row[3].type == RESQL_BLOB);
			rs_assert(strcmp("a", row[0].name) == 0);
			rs_assert(strcmp("b", row[1].name) == 0);
			rs_assert(strcmp("c", row[2].name) == 0);
			rs_assert(strcmp("d", row[3].name) == 0);

			rs_assert(row[0].intval == x++);
			rs_assert(strcmp(row[1].text, "test") == 0);
			rs_assert(row[2].floatval == (double) 3.11);
			rs_assert(row[3].len == 5);
			rs_assert(strcmp(row[3].blob, "blob") == 0);
		}
	} while (resql_next(rs));

	rc = resql_prepare(
		c, "INSERT INTO test VALUES (:key1, :key2, :key3, :key4);",
		&stmt);
	client_assert(c, rc == RESQL_OK);
}

static void client_many()
{
	int rc;
	resql *c;
	resql_stmt stmt = 0;
	struct resql_result *rs = NULL;

	test_server_create(0, 1);
	c = test_client_create();

	rc = resql_del_prepared(c, &stmt);
	client_assert(c, rc == RESQL_SQL_ERROR);

	resql_put_sql(
		c,
		"CREATE TABLE test (a INTEGER PRIMARY KEY, b TEXT, c FLOAT, d BLOB);");
	rc = resql_exec(c, false, &rs);
	client_assert(c, rc == RESQL_OK);

	for (int i = 0; i < 1000; i++) {
		resql_put_sql(
			c,
			"INSERT INTO test VALUES (:key1, :key2, :key3, :key4);");
		resql_bind_param_int(c, ":key1", i);
		resql_bind_param_text(c, ":key2", "test");
		resql_bind_param_float(c, ":key3", 3.11);
		resql_bind_param_blob(c, ":key4", 5, "blob");

		rc = resql_exec(c, false, &rs);
		client_assert(c, rc == RESQL_OK);

		do {
			while (resql_row(rs) != NULL) {
				rs_assert(true);
			}
		} while (resql_next(rs));
	}
}

static void client_big()
{
	int rc;
	resql *c;
	resql_stmt stmt = 0;
	struct resql_result *rs = NULL;

	test_server_create(0, 1);
	c = test_client_create();

	rc = resql_del_prepared(c, &stmt);
	client_assert(c, rc == RESQL_SQL_ERROR);

	resql_put_sql(
		c,
		"CREATE TABLE test (a INTEGER PRIMARY KEY, b TEXT, c FLOAT, d BLOB);");
	rc = resql_exec(c, false, &rs);
	client_assert(c, rc == RESQL_OK);

	char *p = calloc(1, 1024 * 1024);
	memset(p, 0, 1024 * 1024);

	for (int i = 0; i < 1000; i++) {
		resql_put_sql(
			c,
			"INSERT INTO test VALUES (:key1, :key2, :key3, :key4);");
		resql_bind_param_int(c, ":key1", i);
		resql_bind_param_text(c, ":key2", "test");
		resql_bind_param_float(c, ":key3", 3.11);
		resql_bind_param_blob(c, ":key4", 1 * 1024, p);

		rc = resql_exec(c, false, &rs);
		client_assert(c, rc == RESQL_OK);

		do {
			while (resql_row(rs)) {
				rs_assert(true);
			}
		} while (resql_next(rs));
	}

	free(p);
}

int main(void)
{
	test_execute(client_error);
	test_execute(client_prepared);
	test_execute(client_big);
	test_execute(client_many);
	test_execute(client_simple);

	return 0;
}
