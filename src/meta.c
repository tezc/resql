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

#include "meta.h"

#include "rs.h"

#include "sc/sc_array.h"
#include "sc/sc_str.h"
#include "sc/sc_uri.h"

#include <inttypes.h>

const char *meta_role_str[] = {
	"leader",
	"follower",
};

void meta_node_init(struct meta_node *n, struct sc_uri *uri)
{
	n->name = sc_str_create(uri->userinfo);

	sc_array_init(&n->uris);
	sc_array_add(&n->uris, uri);

	n->role = META_FOLLOWER;
}

void meta_node_term(struct meta_node *n)
{
	struct sc_uri *uri;

	sc_str_destroy(&n->name);

	sc_array_foreach (&n->uris, uri) {
		sc_uri_destroy(&uri);
	}
	sc_array_term(&n->uris);
}

void meta_node_encode(struct meta_node *n, struct sc_buf *buf)
{
	struct sc_uri *uri;

	sc_buf_put_str(buf, n->name);
	sc_buf_put_8(buf, n->role);
	sc_buf_put_32(buf, (uint32_t) sc_array_size(&n->uris));

	sc_array_foreach (&n->uris, uri) {
		sc_buf_put_str(buf, uri->str);
	}
}

void meta_node_decode(struct meta_node *n, struct sc_buf *buf)
{
	uint32_t count;
	struct sc_uri *uri;

	n->name = sc_str_create(sc_buf_get_str(buf));
	n->role = (enum meta_role) sc_buf_get_8(buf);

	sc_array_init(&n->uris);

	count = sc_buf_get_32(buf);

	for (size_t i = 0; i < count; i++) {
		uri = sc_uri_create(sc_buf_get_str(buf));
		assert(uri);

		sc_array_add(&n->uris, uri);
	}
}

void meta_init(struct meta *m, const char *cluster_name)
{
	*m = (struct meta){0};

	m->name = sc_str_create(cluster_name);
	m->uris = sc_str_create("");

	sc_array_init(&m->nodes);
}

void meta_term(struct meta *m)
{
	struct meta_node node;

	sc_str_destroy(&m->name);
	sc_str_destroy(&m->uris);

	sc_array_foreach (&m->nodes, node) {
		meta_node_term(&node);
	}
	sc_array_term(&m->nodes);

	if (m->prev) {
		meta_term(m->prev);
		rs_free(m->prev);
		m->prev = NULL;
	}
}

void meta_node_copy(struct meta_node *n, struct meta_node *src)
{
	struct sc_uri *uri;

	n->name = sc_str_create(src->name);
	n->role = src->role;

	sc_array_init(&n->uris);

	sc_array_foreach (&src->uris, uri) {
		sc_array_add(&n->uris, sc_uri_create(uri->str));
	}
}

void meta_copy(struct meta *m, struct meta *src)
{
	struct meta_node n, other;

	meta_term(m);
	meta_init(m, src->name);

	sc_str_set(&m->uris, src->uris);
	m->term = src->term;
	m->index = src->index;
	m->voter = src->voter;

	sc_array_foreach (&src->nodes, other) {
		meta_node_copy(&n, &other);
		sc_array_add(&m->nodes, n);
	}

	if (src->prev != NULL) {
		m->prev = rs_malloc(sizeof(*m->prev));
		meta_init(m->prev, src->prev->name);
		meta_copy(m->prev, src->prev);
	}
}

void meta_encode(struct meta *m, struct sc_buf *buf)
{
	struct meta_node n;

	sc_buf_put_str(buf, m->name);
	sc_buf_put_str(buf, m->uris);
	sc_buf_put_64(buf, m->term);
	sc_buf_put_64(buf, m->index);
	sc_buf_put_32(buf, m->voter);
	sc_buf_put_32(buf, (uint32_t) sc_array_size(&m->nodes));

	sc_array_foreach (&m->nodes, n) {
		meta_node_encode(&n, buf);
	}

	if (!m->prev) {
		sc_buf_put_bool(buf, false);
		return;
	}

	sc_buf_put_bool(buf, true);
	meta_encode(m->prev, buf);
}

void meta_decode(struct meta *m, struct sc_buf *buf)
{
	bool prev;
	uint32_t count;
	struct meta_node n;

	sc_str_set(&m->name, sc_buf_get_str(buf));
	sc_str_set(&m->uris, sc_buf_get_str(buf));
	m->term = sc_buf_get_64(buf);
	m->index = sc_buf_get_64(buf);
	m->voter = sc_buf_get_32(buf);

	count = sc_buf_get_32(buf);
	for (size_t i = 0; i < count; i++) {
		meta_node_decode(&n, buf);
		sc_array_add(&m->nodes, n);
	}

	prev = sc_buf_get_bool(buf);
	if (prev) {
		m->prev = rs_malloc(sizeof(*m->prev));
		meta_init(m->prev, m->name);
		meta_decode(m->prev, buf);
	}
}

static void meta_update(struct meta *m)
{
	size_t sz;
	struct sc_buf tmp;
	struct meta_node n;
	struct sc_uri *uri;

	m->voter = (uint32_t) sc_array_size(&m->nodes);

	sc_buf_init(&tmp, 1024);

	for (size_t i = 0; i < sc_array_size(&m->nodes); i++) {
		n = sc_array_at(&m->nodes, i);
		if (n.role == META_LEADER) {
			sz = sc_array_size(&n.uris);
			for (size_t j = 0; j < sz; j++) {
				uri = sc_array_at(&n.uris, j);
				sc_buf_put_text(&tmp, "%s ", uri->str);
			}
			break;
		}
	}

	for (size_t i = 0; i < sc_array_size(&m->nodes); i++) {
		n = sc_array_at(&m->nodes, i);
		if (n.role != META_LEADER) {
			sz = sc_array_size(&n.uris);
			for (size_t j = 0; j < sz; j++) {
				uri = sc_array_at(&n.uris, j);
				sc_buf_put_text(&tmp, "%s ", uri->str);
			}
		}
	}

	sc_str_set(&m->uris, sc_buf_rbuf(&tmp));
	sc_buf_term(&tmp);
}

static bool meta_validate(struct meta *m, struct sc_uri *uri)
{
	size_t n, u;
	struct sc_uri *r;
	struct meta_node node;

	if (*uri->userinfo == '\0' || *uri->scheme == '\0' ||
	    *uri->port == '\0') {
		return false;
	}

	n = sc_array_size(&m->nodes);
	for (size_t i = 0; i < n; i++) {
		node = sc_array_at(&m->nodes, i);
		if (strcmp(node.name, uri->userinfo) == 0) {
			return false;
		}

		u = sc_array_size(&node.uris);
		for (size_t j = 0; j < u; j++) {
			r = sc_array_at(&node.uris, j);

			if (strcmp(r->host, uri->host) == 0 &&
			    strcmp(r->port, uri->port) == 0) {
				return false;
			}
		}
	}

	return true;
}

bool meta_add(struct meta *m, struct sc_uri *uri)
{
	struct meta_node node;
	struct meta *tmp;

	assert(m->prev == NULL);

	if (!meta_validate(m, uri)) {
		return false;
	}

	tmp = rs_malloc(sizeof(*tmp));
	meta_init(tmp, "");
	meta_copy(tmp, m);
	m->prev = tmp;

	meta_node_init(&node, sc_uri_create(uri->str));
	node.role = META_FOLLOWER;
	sc_array_add(&m->nodes, node);

	meta_update(m);

	return true;
}

bool meta_remove(struct meta *m, const char *name)
{
	struct meta *tmp;
	struct meta_node n;

	assert(m->prev == NULL);

	if (!meta_exists(m, name)) {
		return false;
	}

	tmp = rs_malloc(sizeof(*tmp));
	meta_init(tmp, "");
	meta_copy(tmp, m);
	m->prev = tmp;

	for (size_t i = 0; i < sc_array_size(&m->nodes); i++) {
		n = sc_array_at(&m->nodes, i);
		if (strcmp(n.name, name) == 0) {
			meta_node_term(&n);
			sc_array_del(&m->nodes, i);
			break;
		}
	}

	meta_update(m);

	return true;
}

bool meta_exists(struct meta *m, const char *name)
{
	struct meta_node n;

	for (size_t i = 0; i < sc_array_size(&m->nodes); i++) {
		n = sc_array_at(&m->nodes, i);
		if (strcmp(n.name, name) == 0) {
			return true;
		}
	}

	return false;
}

void meta_remove_prev(struct meta *m)
{
	assert(m->prev != NULL);

	meta_term(m->prev);
	rs_free(m->prev);
	m->prev = NULL;
}

void meta_rollback(struct meta *m, uint64_t index)
{
	struct meta *tmp = m->prev;

	if (!m->prev || m->prev->index <= index) {
		return;
	}

	assert(m->prev != NULL);

	m->prev = NULL;
	meta_copy(m, tmp);
	meta_term(tmp);
	rs_free(tmp);

	assert(m->prev == NULL);
}

void meta_replace(struct meta *m, void *data, uint32_t len)
{
	struct sc_buf tmp = sc_buf_wrap(data, len, SC_BUF_READ);

	meta_term(m);
	meta_init(m, "");
	meta_decode(m, &tmp);
}

void meta_set_leader(struct meta *m, const char *name)
{
	for (size_t i = 0; i < sc_array_size(&m->nodes); i++) {
		if (strcmp(m->nodes.elems[i].name, name) == 0) {
			m->nodes.elems[i].role = META_LEADER;
			continue;
		}

		if (m->nodes.elems[i].role == META_LEADER) {
			m->nodes.elems[i].role = META_FOLLOWER;
		}
	}

	meta_update(m);
}

bool meta_parse_uris(struct meta *m, const char *addrs)
{
	bool found;
	bool rc = false;
	char *dup, *save = NULL;
	const char *tok;
	struct meta_node n;
	struct sc_uri *uri;

	dup = sc_str_create(addrs);
	if (dup == NULL) {
		return false;
	}

	while ((tok = sc_str_token_begin(dup, &save, " ")) != NULL) {
		if (*tok == '\0') {
			break;
		}

		uri = sc_uri_create(tok);
		if (uri == NULL || *uri->userinfo == '\0' ||
		    *uri->port == '\0' || *uri->host == '\0') {
			goto clean_uri;
		}

		found = false;

		for (size_t i = 0; i < sc_array_size(&m->nodes); i++) {
			if (strcmp(uri->userinfo, m->nodes.elems[i].name) ==
			    0) {
				sc_array_add(&m->nodes.elems[i].uris, uri);
				found = true;
				break;
			}
		}

		if (!found) {
			meta_node_init(&n, uri);
			sc_array_add(&m->nodes, n);
		}
	}

	meta_update(m);
	rc = true;

clean_uri:
	sc_str_destroy(&dup);

	if (!rc) {
		sc_str_destroy(&m->name);
		sc_str_destroy(&m->uris);
	}

	return rc;
}

void meta_print(struct meta *m, struct sc_buf *buf)
{
	sc_buf_put_text(buf, "\n| -------------------------------\n");
	sc_buf_put_text(buf, "| Cluster : %s \n", m->name);
	sc_buf_put_text(buf, "| Term    : %" PRIu64 " \n", m->term);

	for (size_t i = 0; i < sc_array_size(&m->nodes); i++) {
		sc_buf_put_text(buf, "| Node    : %s, Role : %s \n",
				m->nodes.elems[i].name,
				meta_role_str[m->nodes.elems[i].role]);
	}
	sc_buf_put_text(buf, "| -------------------------------\n");
}
