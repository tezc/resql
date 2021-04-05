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

#include "entry.h"

#include "test_util.h"

#include "sc/sc_buf.h"

#include <stdio.h>

void encode_test()
{
    struct sc_buf buf;

    sc_buf_init(&buf, 1024);

    for (uint64_t i = 100000000000; i < 100000000000 + 100000; i++) {
        entry_encode(&buf, 0, 1, 2, 3, "test", 5);
        assert(entry_decode(&buf) == RS_OK);

        if (i % 4096 == 0) {
            sc_buf_compact(&buf);
        }
    }

    sc_buf_term(&buf);
}

int main()
{
    test_execute(encode_test);
}
