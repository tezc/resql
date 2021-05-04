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
#include "test_util.h"

#include "sc/sc_array.h"
#include "sc/sc_uri.h"

#include <assert.h>
#include <string.h>

void init_test()
{
	struct meta m;

	meta_init(&m, "cluster");
	rs_assert(strcmp(m.name, "cluster") == 0);
	meta_term(&m);
}

void copy_test()
{
	bool b;
	struct meta m, m2;

	meta_init(&m, "cluster");
	b = meta_parse_uris(&m, "tcp://node0@127.0.0.1:7600"
				" tcp://node1@127.0.0.1:7601"
				" tcp://node2@127.0.0.1:7602");
	rs_assert(b);

	meta_init(&m2, "cluster");
	meta_copy(&m2, &m);

	rs_assert(meta_exists(&m, "node0"));
	rs_assert(meta_exists(&m, "node1"));
	rs_assert(meta_exists(&m, "node2"));

	rs_assert(meta_exists(&m2, "node0"));
	rs_assert(meta_exists(&m2, "node1"));
	rs_assert(meta_exists(&m2, "node2"));

	meta_term(&m);
	meta_term(&m2);
}

void encode_test()
{
	bool b;
	struct meta m, m2;
	struct sc_buf buf;
	struct sc_uri *uri;

	meta_init(&m, "cluster");
	b = meta_parse_uris(&m, "tcp://node0@127.0.0.1:7600"
				" tcp://node1@127.0.0.1:7601"
				" tcp://node2@127.0.0.1:7602");
	rs_assert(b);

	sc_buf_init(&buf, 1024);
	uri = sc_uri_create("tcp://node3@127.0.0.1:7603");
	meta_add(&m, uri);
	sc_uri_destroy(&uri);

	meta_encode(&m, &buf);

	meta_init(&m2, "");
	meta_decode(&m2, &buf);

	rs_assert(m.prev);
	rs_assert(meta_exists(&m, "node0"));
	rs_assert(meta_exists(&m, "node1"));
	rs_assert(meta_exists(&m, "node2"));
	rs_assert(meta_exists(&m, "node3"));

	rs_assert(m2.prev);
	rs_assert(meta_exists(&m2, "node0"));
	rs_assert(meta_exists(&m2, "node1"));
	rs_assert(meta_exists(&m2, "node2"));
	rs_assert(meta_exists(&m2, "node3"));

	meta_print(&m, &buf);

	meta_term(&m);
	meta_term(&m2);
	sc_buf_term(&buf);
}

void add_test()
{
	bool b;
	struct meta m;
	struct sc_uri *uri;

	meta_init(&m, "cluster");
	b = meta_parse_uris(&m, "tcp://node0@127.0.0.1:7600"
				" tcp://node1@127.0.0.1:7601"
				" tcp://node2@127.0.0.1:7602");
	rs_assert(b);

	uri = sc_uri_create("tcp://node0@127.0.0.1:7600");
	b = meta_add(&m, uri);
	rs_assert(!b);
	sc_uri_destroy(&uri);

	uri = sc_uri_create("tcp://node3@127.0.0.1:7603");
	b = meta_add(&m, uri);
	rs_assert(b);
	sc_uri_destroy(&uri);

	rs_assert(meta_exists(&m, "node0"));
	rs_assert(meta_exists(&m, "node1"));
	rs_assert(meta_exists(&m, "node2"));
	rs_assert(meta_exists(&m, "node3"));

	meta_term(&m);
}

void remove_test()
{
	bool b;
	struct meta m;

	meta_init(&m, "cluster");
	b = meta_parse_uris(&m, "tcp://node0@127.0.0.1:7600"
				" tcp://node1@127.0.0.1:7601"
				" tcp://node2@127.0.0.1:7602");
	rs_assert(b);

	b = meta_remove(&m, "node3");
	rs_assert(!b);

	b = meta_remove(&m, "node2");
	rs_assert(b);

	rs_assert(meta_exists(&m, "node0"));
	rs_assert(meta_exists(&m, "node1"));
	rs_assert(!meta_exists(&m, "node2"));
	rs_assert(!meta_exists(&m, "node3"));

	meta_term(&m);
}

void leader_test()
{
	bool b;
	struct meta m;

	meta_init(&m, "cluster");
	b = meta_parse_uris(&m, "tcp://node0@127.0.0.1:7600"
				" tcp://node1@127.0.0.1:7601"
				" tcp://node2@127.0.0.1:7602");
	rs_assert(b);

	meta_set_leader(&m, "node0");

	for (size_t i = 0; i < sc_array_size(&m.nodes); i++) {
		if (strcmp(m.nodes.elems[i].name, "node0") == 0) {
			rs_assert(m.nodes.elems[i].role == META_LEADER);
		} else {
			rs_assert(m.nodes.elems[i].role == META_FOLLOWER);
		}
	}

	meta_set_leader(&m, "node1");

	for (size_t i = 0; i < sc_array_size(&m.nodes); i++) {
		if (strcmp(m.nodes.elems[i].name, "node1") == 0) {
			rs_assert(m.nodes.elems[i].role == META_LEADER);
		} else {
			rs_assert(m.nodes.elems[i].role == META_FOLLOWER);
		}
	}

	meta_term(&m);
}

void dup_test()
{
	bool b;
	struct meta m;

	meta_init(&m, "cluster");
	b = meta_parse_uris(&m, "tcp://node0@127.0.0.1:7600"
				" tcp://node1@127.0.0.1:7601"
				" tcp://node2@127.0.0.1:7602"
				" tcp://node2@127.0.0.1:7603");
	rs_assert(b);
	rs_assert(m.voter == 3);

	meta_term(&m);
}

void change_test()
{
	bool b;
	struct meta m, m2;
	struct sc_uri *uri;

	meta_init(&m, "cluster");
	b = meta_parse_uris(&m, "tcp://node0@127.0.0.1:7600"
				" tcp://node1@127.0.0.1:7601"
				" tcp://node2@127.0.0.1:7602");
	rs_assert(b);
	rs_assert(m.voter == 3);

	uri = sc_uri_create("tcp://node3@127.0.0.1:7603");
	b = meta_add(&m, uri);
	rs_assert(b);
	sc_uri_destroy(&uri);

	rs_assert(m.prev);
	rs_assert(m.prev->voter == 3);
	rs_assert(m.voter == 4);
	rs_assert(meta_exists(&m, "node3"));
	rs_assert(!meta_exists(m.prev, "node3"));

	meta_init(&m2, "cluster");
	meta_copy(&m2, &m);

	rs_assert(m2.prev);
	rs_assert(m2.prev->voter == 3);
	rs_assert(m2.voter == 4);
	rs_assert(meta_exists(&m2, "node3"));
	rs_assert(!meta_exists(m2.prev, "node3"));

	meta_term(&m);
	meta_term(&m2);
}

void change_success_test()
{
	bool b;
	struct meta m;
	struct sc_uri *uri;

	meta_init(&m, "cluster");
	m.index = 100;
	b = meta_parse_uris(&m, "tcp://node0@127.0.0.1:7600"
				" tcp://node1@127.0.0.1:7601"
				" tcp://node2@127.0.0.1:7602");
	rs_assert(b);
	rs_assert(m.voter == 3);

	uri = sc_uri_create("tcp://node3@127.0.0.1:7603");
	b = meta_add(&m, uri);
	rs_assert(b);
	sc_uri_destroy(&uri);

	meta_remove_prev(&m);
	rs_assert(m.prev == NULL);

	rs_assert(meta_exists(&m, "node0"));
	rs_assert(meta_exists(&m, "node1"));
	rs_assert(meta_exists(&m, "node2"));
	rs_assert(meta_exists(&m, "node3"));

	meta_term(&m);
}

void change_fail_test()
{
	bool b;
	struct meta m;
	struct sc_uri *uri;

	meta_init(&m, "cluster");
	m.index = 100;
	b = meta_parse_uris(&m, "tcp://node0@127.0.0.1:7600"
				" tcp://node1@127.0.0.1:7601"
				" tcp://node2@127.0.0.1:7602");
	rs_assert(b);
	rs_assert(m.voter == 3);

	uri = sc_uri_create("tcp://node3@127.0.0.1:7603");
	b = meta_add(&m, uri);
	rs_assert(b);
	sc_uri_destroy(&uri);

	meta_rollback(&m, 100);

	rs_assert(m.prev != NULL);
	rs_assert(meta_exists(&m, "node0"));
	rs_assert(meta_exists(&m, "node1"));
	rs_assert(meta_exists(&m, "node2"));
	rs_assert(meta_exists(&m, "node3"));

	meta_remove_prev(&m);

	uri = sc_uri_create("tcp://node4@127.0.0.1:7604");
	b = meta_add(&m, uri);
	rs_assert(b);
	sc_uri_destroy(&uri);
	meta_rollback(&m, 99);

	rs_assert(m.prev == NULL);
	rs_assert(meta_exists(&m, "node0"));
	rs_assert(meta_exists(&m, "node1"));
	rs_assert(meta_exists(&m, "node2"));
	rs_assert(meta_exists(&m, "node3"));
	rs_assert(!meta_exists(&m, "node4"));

	meta_term(&m);
}

void replace_test()
{
	bool b;
	struct meta m, m2;
	struct sc_buf buf;

	meta_init(&m, "cluster");
	b = meta_parse_uris(&m, "tcp://node0@127.0.0.1:7600"
				" tcp://node1@127.0.0.1:7601"
				" tcp://node2@127.0.0.1:7602");
	rs_assert(b);

	meta_init(&m2, "cluster");
	b = meta_parse_uris(&m2, "tcp://nodea@127.0.0.1:7600"
				 " tcp://nodeb@127.0.0.1:7601"
				 " tcp://nodec@127.0.0.1:7602");
	rs_assert(b);

	sc_buf_init(&buf, 1024);
	meta_encode(&m2, &buf);
	meta_replace(&m, sc_buf_rbuf(&buf), sc_buf_size(&buf));

	rs_assert(meta_exists(&m, "nodea"));
	rs_assert(meta_exists(&m, "nodeb"));
	rs_assert(meta_exists(&m, "nodec"));

	meta_term(&m);
	meta_term(&m2);
	sc_buf_term(&buf);
}

int main(void)
{
	test_execute(init_test);
	test_execute(copy_test);
	test_execute(encode_test);
	test_execute(add_test);
	test_execute(remove_test);
	test_execute(leader_test);
	test_execute(dup_test);
	test_execute(change_test);
	test_execute(change_success_test);
	test_execute(change_fail_test);
	test_execute(replace_test);

	return 0;
}
