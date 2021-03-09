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


#include "conf.h"
#include "server.h"

int main(int argc, char *argv[])
{
    int rc;
    struct conf config;
    struct server *server;

    server_global_init();

    conf_init(&config);
    conf_read_config(&config, true, argc, argv);

    server = server_create(&config);
    rc = server_start(server, false);
    server_destroy(server);

    server_global_shutdown();

    return rc;
}
