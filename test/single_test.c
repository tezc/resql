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

#include "resql.h"
#include "test_util.h"

void test_one()
{
    test_server_create(0, 1);
    test_client_create();
}

void test_sizes()
{
    for (int i = 1; i <= 9; i++) {
        for (int j = 0; j < i; j++) {
            test_server_create(j, i);
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

    test_server_create(0, 1);
    c = test_client_create();

    resql_put_sql(c, "SELECT * FROM resql_sessions;");
    rc = resql_exec(c, true, &rs);
    client_assert(c, rc == RESQL_OK);
}

int main()
{
    test_execute(test_one);
    test_execute(test_client);
    test_execute(test_sizes);
    return 0;
}
