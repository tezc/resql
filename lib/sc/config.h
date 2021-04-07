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

#ifndef RESQL_CONFIG_H
#define RESQL_CONFIG_H

#include "rs.h"

#define sc_array_realloc rs_realloc
#define sc_array_free    rs_free

#define sc_buf_calloc  rs_calloc
#define sc_buf_realloc rs_realloc
#define sc_buf_free    rs_free

#define sc_map_calloc rs_calloc
#define sc_map_free rs_free

#define sc_queue_realloc rs_realloc
#define sc_queue_free    rs_free

#define sc_str_malloc  rs_malloc
#define sc_str_realloc rs_realloc
#define sc_str_free    rs_free

#define sc_sock_malloc  malloc
#define sc_sock_realloc  realloc
#define sc_sock_free  free

#define sc_timer_malloc rs_malloc
#define sc_timer_free   rs_free

#define sc_uri_malloc rs_malloc
#define sc_uri_free   rs_free

#endif
