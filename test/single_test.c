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

void test_one()
{
	test_server_create(true, 0, 1);
	test_client_create();
}

void test_sizes()
{
	for (int i = 1; i <= 9; i++) {
		for (int j = 0; j < i; j++) {
			test_server_create(true, j, i);
		}

		test_client_create();
		test_client_destroy_all();
		test_server_destroy_all();
	}
}

void test_client()
{
	int rc;
	resql *c;
	resql_result *rs;

	test_server_create(true, 0, 1);
	c = test_client_create();

	resql_put_sql(c, "SELECT * FROM resql_clients;");
	rc = resql_exec(c, true, &rs);
	client_assert(c, rc == RESQL_OK);
}

void test_one_disk()
{
	test_server_create(false, 0, 1);
	test_client_create();
}

void test_sizes_disk()
{
	for (int i = 1; i <= 9; i++) {
		for (int j = 0; j < i; j++) {
			test_server_create(false, j, i);
		}

		test_client_create();
		test_client_destroy_all();
		test_server_destroy_all();
	}
}

void test_client_disk()
{
	int rc;
	resql *c;
	resql_result *rs;

	test_server_create(false, 0, 1);
	c = test_client_create();

	resql_put_sql(c, "SELECT * FROM resql_clients;");
	rc = resql_exec(c, true, &rs);
	client_assert(c, rc == RESQL_OK);
}

int main()
{
	test_execute(test_one);
	test_execute(test_client);
	test_execute(test_sizes);

	test_execute(test_one_disk);
	test_execute(test_client_disk);
	test_execute(test_sizes_disk);
	return 0;
}
