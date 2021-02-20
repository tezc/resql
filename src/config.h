/*
 *  reSQL
 *
 *  Copyright (C) 2021 reSQL Authors
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

#ifndef RESQL_CONFIG_H
#define RESQL_CONFIG_H

#include "rs.h"

#define sc_array_realloc rs_realloc
#define sc_array_free    rs_free

#define sc_buf_malloc  rs_malloc
#define sc_buf_realloc rs_realloc
#define sc_buf_free    rs_free

#define sc_map_calloc rs_calloc
#define sc_map_free rs_free

#define sc_queue_realloc rs_realloc
#define sc_queue_free    rs_free

#define sc_str_malloc  rs_malloc
#define sc_str_realloc rs_realloc
#define sc_str_free    rs_free

#define sc_timer_malloc rs_malloc
#define sc_timer_free   rs_free

#define sc_uri_malloc rs_malloc
#define sc_uri_free   rs_free

#endif
