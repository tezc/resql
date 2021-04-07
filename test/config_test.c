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
#include "test_util.h"

#define call_param_test(...)                                                   \
	(param_test(((char *[]){"", __VA_ARGS__}),                             \
		    sizeof(((char *[]){"", __VA_ARGS__})) / sizeof(char *)))

static void param_test(char *opt[], size_t len)
{
	struct conf conf;

	conf_init(&conf);
	conf_read_config(&conf, false, (int) len, opt);
	test_server_create_conf(&conf, 0);
}

static void param_test1(void)
{
	call_param_test("--node-name=node2",
			"--node-bind-url=tcp://node2@127.0.0.1:7600",
			"--node-advertise-url=tcp://node2@127.0.0.1:7600",
			"--node-source-addr=127.0.0.1",
			"--node-source-port=9000", "--node-log-level=debug",
			"--node-log-destination=stdout", "--node-directory=.",
			"--node-in-memory=true", "--cluster-name=cluster",
			"--cluster-nodes=tcp://node2@127.0.0.1:7600",
			"--advanced-fsync=true", "--advanced-heartbeat=1000");
}

int main(void)
{
	test_execute(param_test1);

	return 0;
}
