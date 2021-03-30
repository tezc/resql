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

#include "server.h"
#include "rs.h"
#include "test_util.h"

#include "resql.h"

#include <unistd.h>


static void multi()
{
    test_server_create(0, 3);
    test_server_create(1, 3);
    //test_server_create(2, 3);

    sleep(3);

    test_server_create(2, 3);

    pause();
}



int main(void)
{
    test_execute(multi);

    return 0;
}
