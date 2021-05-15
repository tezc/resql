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

	test_server_create(true, 0, 3);
	test_server_create(true, 1, 3);
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

	test_server_destroy(0);
	test_server_destroy(1);
	test_server_start(true, 0, 3);
	test_server_start(true, 1, 3);
	test_server_start(true, 2, 3);

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

static void snapshot_big()
{
	int rc, x;
	char tmp[4096] = {0};
	resql *c;
	struct resql_column *row;
	struct resql_result *rs = NULL;

	test_server_create(true, 0, 3);
	test_server_create(true, 1, 3);
	c = test_client_create();

	resql_put_sql(c, "CREATE TABLE snapshot (key TEXT, value TEXT);");

	rc = resql_exec(c, false, &rs);
	client_assert(c, rc == RESQL_OK);

	for (int i = 0; i < 100; i++) {
		for (int j = 0; j < 100; j++) {
			snprintf(tmp, sizeof(tmp), "%d", (i * 1000) + j);

			resql_put_sql(
				c,
				"INSERT INTO snapshot VALUES(:key, :value)");
			resql_bind_param_text(c, ":key", tmp);
			resql_bind_param_blob(c, ":value", sizeof(tmp), tmp);
		}

		rc = resql_exec(c, false, &rs);
		client_assert(c, rc == RESQL_OK);
	}

	test_server_destroy(0);
	test_server_destroy(1);
	test_server_start(true, 0, 3);
	test_server_start(true, 1, 3);
	test_server_start(true, 2, 3);

	resql_put_sql(c, "Select count(*) from snapshot;");
	rc = resql_exec(c, true, &rs);
	client_assert(c, rc == RESQL_OK);

	rs_assert(resql_row_count(rs) == 1);
	rs_assert(resql_row(rs)[0].intval == 10000);

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

static void snapshot_two()
{
	int rc, x;
	char tmp[4096] = {0};
	resql *c;
	struct resql_column *row;
	struct resql_result *rs = NULL;

	test_server_create(true, 0, 1);
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

	test_server_add_auto(true);

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

	test_server_add_auto(true);
	test_server_destroy_leader();

	c = test_client_create();
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

static void snapshot_simple_disk()
{
	int rc, x;
	char tmp[32];
	resql *c;
	struct resql_column *row;
	struct resql_result *rs = NULL;

	test_server_create(false, 0, 3);
	test_server_create(false, 1, 3);
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

	test_server_destroy(0);
	test_server_destroy(1);
	test_server_start(false, 0, 3);
	test_server_start(false, 1, 3);
	test_server_start(false, 2, 3);

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

static void snapshot_big_disk()
{
	int rc, x;
	char tmp[4096] = {0};
	resql *c;
	struct resql_column *row;
	struct resql_result *rs = NULL;

	test_server_create(false, 0, 3);
	test_server_create(false, 1, 3);
	c = test_client_create();

	resql_put_sql(c, "CREATE TABLE snapshot (key TEXT, value TEXT);");

	rc = resql_exec(c, false, &rs);
	client_assert(c, rc == RESQL_OK);

	for (int i = 0; i < 100; i++) {
		for (int j = 0; j < 100; j++) {
			snprintf(tmp, sizeof(tmp), "%d", (i * 1000) + j);

			resql_put_sql(
				c,
				"INSERT INTO snapshot VALUES(:key, :value)");
			resql_bind_param_text(c, ":key", tmp);
			resql_bind_param_blob(c, ":value", sizeof(tmp), tmp);
		}

		rc = resql_exec(c, false, &rs);
		client_assert(c, rc == RESQL_OK);
	}

	test_server_destroy(0);
	test_server_destroy(1);
	test_server_start(false, 0, 3);
	test_server_start(false, 1, 3);
	test_server_start(false, 2, 3);

	resql_put_sql(c, "Select count(*) from snapshot;");
	rc = resql_exec(c, true, &rs);
	client_assert(c, rc == RESQL_OK);

	rs_assert(resql_row_count(rs) == 1);
	rs_assert(resql_row(rs)[0].intval == 10000);

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

static void snapshot_two_disk()
{
	int rc, x;
	char tmp[4096] = {0};
	resql *c;
	struct resql_column *row;
	struct resql_result *rs = NULL;

	test_server_create(false, 0, 1);
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

	test_server_add_auto(false);

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

	test_server_add_auto(false);
	test_server_destroy_leader();

	c = test_client_create();
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
	test_execute(snapshot_two);
	test_execute(snapshot_simple);
	test_execute(snapshot_big);

	test_execute(snapshot_two_disk);
	test_execute(snapshot_simple_disk);
	test_execute(snapshot_big_disk);

	return 0;
}
