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

#include <stdio.h>
#include <stdlib.h>

void train_single()
{
	int rc;
	int ops = 0;
	resql *c;
	resql_result *rs;
	resql_stmt stmt;

	struct resql_config config = {
		.urls = "tcp://127.0.0.1:9717",
		.timeout_millis = 10000,
		.cluster_name = "cluster",
	};

	rc = resql_create(&c, &config);
	if (rc != RESQL_OK) {
		printf("Failed to connect to server. \n");
		exit(EXIT_FAILURE);
	}

	resql_put_sql(c, "DROP TABLE IF EXISTS test;");
	resql_put_sql(c, "CREATE TABLE test (key INTEGER, value TEXT);");
	rc = resql_exec(c, false, &rs);
	if (rc != RESQL_OK) {
		printf("Failed : %s \n", resql_errstr(c));
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < 100; i++) {
		ops++;
		resql_put_sql(c, "INSERT INTO test VALUES (:key,:value);");
		resql_bind_param_int(c, ":key", i);
		resql_bind_param_text(c, ":value", "data");

		rc = resql_exec(c, false, &rs);
		if (rc != RESQL_OK) {
			printf("Failed : %s \n", resql_errstr(c));
			exit(EXIT_FAILURE);
		}
	}

	for (int i = 0; i < 100; i++) {
		ops++;
		resql_put_sql(c, "SELECT * FROM test WHERE key = ?;");
		resql_bind_index_int(c, 0, i % 95);
		rc = resql_exec(c, true, &rs);
		if (rc != RESQL_OK) {
			printf("Failed : %s \n", resql_errstr(c));
			exit(EXIT_FAILURE);
		}
	}

	rc = resql_prepare(c, "SELECT * FROM test WHERE key = ?;", &stmt);
	if (rc != RESQL_OK) {
		printf("Failed : %s \n", resql_errstr(c));
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < 100; i++) {
		ops++;
		resql_put_prepared(c, &stmt);
		resql_bind_index_int(c, 0, i % 95);
		rc = resql_exec(c, true, &rs);
		if (rc != RESQL_OK) {
			printf("Failed : %s \n", resql_errstr(c));
			exit(EXIT_FAILURE);
		}
	}

	resql_shutdown(c);

	printf("train_single done (%d).\n", ops);
}

void train_multi()
{
	int rc;
	int ops = 0;
	resql *c;
	resql_result *rs;
	resql_stmt stmt;

	struct resql_config config = {
		.urls = "tcp://127.0.0.1:9717",
		.timeout_millis = 10000,
		.cluster_name = "cluster",
	};

	rc = resql_create(&c, &config);
	if (rc != RESQL_OK) {
		printf("Failed to connect to server. \n");
		exit(EXIT_FAILURE);
	}

	resql_put_sql(c, "DROP TABLE IF EXISTS multi;");
	resql_put_sql(
		c,
		"CREATE TABLE multi (id INTEGER PRIMARY KEY, name TEXT, points FLOAT, data BLOB, date TEXT);");
	rc = resql_exec(c, false, &rs);
	if (rc != RESQL_OK) {
		printf("Failed : %s \n", resql_errstr(c));
		exit(EXIT_FAILURE);
	}

	rc = resql_prepare(c, "INSERT INTO multi VALUES(?, ?, ?, ?, ?);",
			   &stmt);
	if (rc != RESQL_OK) {
		printf("Failed : %s \n", resql_errstr(c));
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < 100; i++) {
		ops++;
		resql_put_prepared(c, &stmt);
		resql_bind_index_int(c, 0, i);
		resql_bind_index_text(c, 1, "jane");
		resql_bind_index_float(c, 2, 3.22);
		resql_bind_index_blob(c, 3, 5, "blob");
		resql_bind_index_null(c, 4);

		rc = resql_exec(c, false, &rs);
		if (rc != RESQL_OK) {
			printf("Failed : %s \n", resql_errstr(c));
			exit(EXIT_FAILURE);
		}
	}

	rc = resql_prepare(c, "SELECT * FROM multi WHERE id = ?;", &stmt);
	if (rc != RESQL_OK) {
		printf("Failed : %s \n", resql_errstr(c));
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < 100; i++) {
		ops++;
		resql_put_prepared(c, &stmt);
		resql_bind_index_int(c, 0, i + 5);
		rc = resql_exec(c, true, &rs);
		if (rc != RESQL_OK) {
			printf("Failed : %s \n", resql_errstr(c));
			exit(EXIT_FAILURE);
		}
	}

	resql_shutdown(c);

	printf("train multi done (%d).\n", ops);
}

int main(int argc, char **argv)
{
	(void) argc;
	(void) argv;

	int rc;
	int ops = 0;
	resql *c;
	resql_result *rs;

	struct resql_config config = {
		.urls = "tcp://127.0.0.1:9717",
		.timeout_millis = 10000,
		.cluster_name = "cluster",
	};

	rc = resql_create(&c, &config);
	if (rc != RESQL_OK) {
		printf("Failed to connect to server. \n");
		exit(EXIT_FAILURE);
	}

	train_single();
	train_multi();

	resql_put_sql(c, "SELECT resql('shutdown', '*');");
	rc = resql_exec(c, false, &rs);
	if (rc != RESQL_OK) {
		printf("Failed : %s \n", resql_errstr(c));
		exit(EXIT_FAILURE);
	}

	printf("trainer done(%d).\n", ops);

	return 0;
}
