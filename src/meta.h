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

#ifndef RESQL_META_H
#define RESQL_META_H

#include "sc/sc_buf.h"

#include <stdint.h>

enum meta_role
{
	META_LEADER,
	META_FOLLOWER,
};

extern const char *meta_role_str[];

struct meta_node {
	char *name;
	bool connected;
	enum meta_role role;
	struct sc_uri **uris;
};

struct meta {
	char *name;
	char *uris;
	uint64_t term;
	uint64_t index;
	uint32_t voter;
	struct meta_node *nodes;

	struct meta *prev;
};

void meta_init(struct meta *m, const char *cluster_name);
void meta_term(struct meta *m);

void meta_copy(struct meta *m, struct meta *src);
void meta_encode(struct meta *m, struct sc_buf *buf);
void meta_decode(struct meta *m, struct sc_buf *buf);
bool meta_add(struct meta *meta, struct sc_uri *uri);
bool meta_remove(struct meta *meta, const char *name);
bool meta_exists(struct meta *m, const char *name);
void meta_remove_prev(struct meta *m);
void meta_rollback(struct meta *m, uint64_t index);
void meta_replace(struct meta *m, void *data, uint32_t len);

void meta_set_connected(struct meta *m, const char *name);
void meta_set_disconnected(struct meta *m, const char *name);
void meta_clear_connection(struct meta *m);
void meta_set_leader(struct meta *m, const char *name);
bool meta_parse_uris(struct meta *m, const char *addrs);

void meta_print(struct meta *m, struct sc_buf *buf);

#endif
