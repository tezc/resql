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

#ifndef RESQL_META_H
#define RESQL_META_H

#include "sc/sc_array.h"
#include "sc/sc_buf.h"

#include <stdint.h>

struct sc_uri;

sc_array_def(struct meta_node, node);

enum meta_role
{
	META_LEADER,
	META_FOLLOWER,
};

extern const char *meta_role_str[];

struct meta_node {
	char *name;
	enum meta_role role;
	struct sc_array_ptr uris;
};

struct meta {
	char *name;
	char *uris;
	uint64_t term;
	uint64_t index;
	uint32_t voter;
	struct sc_array_node nodes;
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

void meta_set_leader(struct meta *m, const char *name);
bool meta_parse_uris(struct meta *m, const char *addrs);

void meta_print(struct meta *m, struct sc_buf *buf);

#endif
