/*
 *  Resql
 *
 *  Copyright (C) 2021 Ozan Tezcan
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "conf.h"

#include "test_util.h"

#define call_param_test(...)                                                   \
    (param_test(((char *[]){"", __VA_ARGS__}),                                 \
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
    call_param_test("-e");
}

static void param_test2(void)
{
    call_param_test("-e",
                    "--node-name=node2",
                    "--node-bind-url=tcp://node2@127.0.0.1:7600",
                    "--node-advertise-url=tcp://node2@127.0.0.1:7600",
                    "--node-source-addr=127.0.0.1",
                    "--node-source-port=9000",
                    "--node-log-level=debug",
                    "--node-log-destination=stdout",
                    "--node-directory=.",
                    "--node-in-memory=true",
                    "--cluster-name=cluster",
                    "--cluster-nodes=tcp://node2@127.0.0.1:7600",
                    "--advanced-fsync=true",
                    "--advanced-heartbeat=1000"
                    );
}

int main(void)
{
    test_execute(param_test1);
    test_execute(param_test2);

    return 0;
}
