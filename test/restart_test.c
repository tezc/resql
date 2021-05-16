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

static void restart_simple()
{
	file_clear_dir("/tmp/node0", ".resql");

	int rc;
	resql *c;
	struct resql_column *row;
	struct resql_result *rs = NULL;

	test_server_create(true, 0, 1);
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
		} while (resql_next(rs) == RESQL_OK);
	}

	test_server_destroy_all();
	test_server_start(true, 0, 1);

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
		} while (resql_next(rs) == RESQL_OK);
	}
}

static void restart_simple2()
{
	file_clear_dir("/tmp/node0", ".resql");

	int rc;
	resql *c;
	struct resql_column *row;
	struct resql_result *rs = NULL;

	test_server_create(true, 0, 1);
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
		} while (resql_next(rs) == RESQL_OK);
	}

	test_server_destroy_all();

	test_server_start(true, 0, 1);
	test_server_destroy_all();

	test_client_destroy_all();
	test_server_start(true, 0, 1);
	test_client_create();
}

int main(void)
{
	test_execute(restart_simple);
	test_execute(restart_simple2);

	return 0;
}
