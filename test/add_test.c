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
#include "test_util.h"

#include <string.h>
#include <unistd.h>

void test_one()
{
	int rc;
	int64_t key, value;
	char blob[64];
	resql *c;
	resql_result *rs;
	struct resql_column *row;

	test_server_create(0, 1);
	test_server_add_auto();
	sleep(3);
	c = test_client_create();

	resql_put_sql(c, "CREATE TABLE test (key INTEGER PRIMARY KEY, val INTEGER, blob BLOB);");
	rc = resql_exec(c, false, &rs);
	client_assert(c, rc == RESQL_OK);

	resql_put_sql(c, "SELECT random(), random(), randomblob(64);");
	rc = resql_exec(c, true, &rs);
	client_assert(c, rc == RESQL_OK);

	resql_put_sql(c, "SELECT random(), random(), randomblob(64);");
	rc = resql_exec(c, false, &rs);
	client_assert(c, rc == RESQL_OK);

	resql_put_sql(c, "SELECT random(), random(), randomblob(64);");
	rc = resql_exec(c, true, &rs);
	client_assert(c, rc == RESQL_OK);

	resql_put_sql(c, "INSERT INTO test VALUES(random(), random(), randomblob(64));");
	rc = resql_exec(c, false, &rs);
	client_assert(c, rc == RESQL_OK);

	resql_put_sql(c, "SELECT * FROM test;");
	rc = resql_exec(c, false, &rs);
	client_assert(c, rc == RESQL_OK);

	row = resql_row(rs);
	key = row[0].intval;
	value = row[1].intval;
	memcpy(blob, row[2].blob, row[2].len);

	test_server_add_auto();
	test_server_add_auto();
	test_server_destroy_leader();

	resql_put_sql(c, "SELECT random(), random(), randomblob(64);");
	rc = resql_exec(c, true, &rs);
	client_assert(c, rc == RESQL_OK);

	resql_put_sql(c, "SELECT * FROM test;");
	rc = resql_exec(c, false, &rs);
	client_assert(c, rc == RESQL_OK);

	rs_assert(key == row[0].intval);
	rs_assert(value == row[1].intval);
	rs_assert(memcmp(blob, row[2].blob, row[2].len) == 0);
}

int main()
{
	test_execute(test_one);

	return 0;
}
