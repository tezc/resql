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
