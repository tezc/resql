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


#include "server.h"
#include "settings.h"

int main(int argc, char *argv[])
{
    int rc;
    struct settings settings;
    struct server *server;

    server_global_init();

    settings_init(&settings);
    settings_read_cmdline(&settings, argc, argv);
    settings_read_file(&settings, settings.cmdline.settings_file);

    server = server_create(&settings);
    rc = server_start(server, false);

    server_global_shutdown();

    return rc;
}
