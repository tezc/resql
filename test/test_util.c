/*
 *  Resql
 *
 *  Copyright (C) 2021 Resql Authors
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

#include "sc/sc_log.h"
#include "server.h"

int init;

static void init_all()
{
    if (!init) {
        setvbuf(stdout, NULL, _IONBF, 0);
        setvbuf(stderr, NULL, _IONBF, 0);

        sc_log_set_thread_name("test");
        server_global_init();
        init = 1;
    }
}

void test_util_run(void (*test_fn)(void), const char *fn_name)
{
    init_all();

    sc_log_info("[ Running ] %s \n", fn_name);
    test_fn();
    sc_log_info("[ Passed  ] %s  \n", fn_name);
}
