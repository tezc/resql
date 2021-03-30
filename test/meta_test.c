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
#include "meta.h"
#include "test_util.h"

#include "sc/sc_uri.h"
#include "sc/sc_array.h"

#include <assert.h>
#include <string.h>

void init_test()
{
    struct meta m;

    meta_init(&m, "cluster");
    assert(strcmp(m.name, "cluster") == 0);
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
    assert(b);

    meta_init(&m2, "cluster");
    meta_copy(&m2, &m);

    assert(meta_exists(&m, "node0"));
    assert(meta_exists(&m, "node1"));
    assert(meta_exists(&m, "node2"));

    assert(meta_exists(&m2, "node0"));
    assert(meta_exists(&m2, "node1"));
    assert(meta_exists(&m2, "node2"));


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
    assert(b);

    sc_buf_init(&buf, 1024);
    uri = sc_uri_create("tcp://node3@127.0.0.1:7603");
    meta_add(&m, uri);
    sc_uri_destroy(uri);

    meta_encode(&m, &buf);

    meta_init(&m2, "");
    meta_decode(&m2, &buf);

    assert(m.prev);
    assert(meta_exists(&m, "node0"));
    assert(meta_exists(&m, "node1"));
    assert(meta_exists(&m, "node2"));
    assert(meta_exists(&m, "node3"));

    assert(m2.prev);
    assert(meta_exists(&m2, "node0"));
    assert(meta_exists(&m2, "node1"));
    assert(meta_exists(&m2, "node2"));
    assert(meta_exists(&m2, "node3"));

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
    assert(b);

    uri = sc_uri_create("tcp://node0@127.0.0.1:7600");
    b = meta_add(&m, uri);
    assert(!b);
    sc_uri_destroy(uri);

    uri = sc_uri_create("tcp://node3@127.0.0.1:7603");
    b = meta_add(&m, uri);
    assert(b);
    sc_uri_destroy(uri);


    assert(meta_exists(&m, "node0"));
    assert(meta_exists(&m, "node1"));
    assert(meta_exists(&m, "node2"));
    assert(meta_exists(&m, "node3"));

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
    assert(b);

    b = meta_remove(&m, "node3");
    assert(!b);

    b = meta_remove(&m, "node2");
    assert(b);

    assert(meta_exists(&m, "node0"));
    assert(meta_exists(&m, "node1"));
    assert(!meta_exists(&m, "node2"));
    assert(!meta_exists(&m, "node3"));

    meta_term(&m);
}

void connection_test()
{
    bool b;
    struct meta m;

    meta_init(&m, "cluster");
    b = meta_parse_uris(&m, "tcp://node0@127.0.0.1:7600"
                            " tcp://node1@127.0.0.1:7601"
                            " tcp://node2@127.0.0.1:7602");
    assert(b);

    meta_set_connected(&m, "node0");

    for (size_t i = 0; i < sc_array_size(m.nodes); i++) {
        if (strcmp(m.nodes[i].name, "node0") == 0) {
            assert(m.nodes[i].connected == true);
            break;
        }
    }

    meta_clear_connection(&m);

    for (size_t i = 0; i < sc_array_size(m.nodes); i++) {
        if (strcmp(m.nodes[i].name, "node0") == 0) {
            assert(m.nodes[i].connected == false);
            break;
        }
    }

    meta_set_connected(&m, "node0");
    meta_set_disconnected(&m, "node0");

    for (size_t i = 0; i < sc_array_size(m.nodes); i++) {
        if (strcmp(m.nodes[i].name, "node0") == 0) {
            assert(m.nodes[i].connected == false);
            break;
        }
    }

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
    assert(b);

    meta_set_leader(&m, "node0");

    for (size_t i = 0; i < sc_array_size(m.nodes); i++) {
        if (strcmp(m.nodes[i].name, "node0") == 0) {
            assert(m.nodes[i].role == META_LEADER);
        } else {
            assert(m.nodes[i].role == META_FOLLOWER);
        }
    }

    meta_set_leader(&m, "node1");

    for (size_t i = 0; i < sc_array_size(m.nodes); i++) {
        if (strcmp(m.nodes[i].name, "node1") == 0) {
            assert(m.nodes[i].role == META_LEADER);
        } else {
            assert(m.nodes[i].role == META_FOLLOWER);
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
    assert(b);
    assert(m.voter == 3);

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
    assert(b);
    assert(m.voter == 3);

    uri = sc_uri_create("tcp://node3@127.0.0.1:7603");
    b = meta_add(&m, uri);
    assert(b);
    sc_uri_destroy(uri);

    assert(m.prev);
    assert(m.prev->voter == 3);
    assert(m.voter == 4);
    assert(meta_exists(&m, "node3"));
    assert(!meta_exists(m.prev, "node3"));

    meta_init(&m2, "cluster");
    meta_copy(&m2, &m);

    assert(m2.prev);
    assert(m2.prev->voter == 3);
    assert(m2.voter == 4);
    assert(meta_exists(&m2, "node3"));
    assert(!meta_exists(m2.prev, "node3"));

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
    assert(b);
    assert(m.voter == 3);

    uri = sc_uri_create("tcp://node3@127.0.0.1:7603");
    b = meta_add(&m, uri);
    assert(b);
    sc_uri_destroy(uri);

    meta_remove_prev(&m);
    assert(m.prev == NULL);

    assert(meta_exists(&m, "node0"));
    assert(meta_exists(&m, "node1"));
    assert(meta_exists(&m, "node2"));
    assert(meta_exists(&m, "node3"));

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
    assert(b);
    assert(m.voter == 3);

    uri = sc_uri_create("tcp://node3@127.0.0.1:7603");
    b = meta_add(&m, uri);
    assert(b);
    sc_uri_destroy(uri);

    meta_rollback(&m, 100);

    assert(m.prev != NULL);
    assert(meta_exists(&m, "node0"));
    assert(meta_exists(&m, "node1"));
    assert(meta_exists(&m, "node2"));
    assert(meta_exists(&m, "node3"));

    meta_remove_prev(&m);

    uri = sc_uri_create("tcp://node4@127.0.0.1:7604");
    b = meta_add(&m, uri);
    assert(b);
    sc_uri_destroy(uri);
    meta_rollback(&m, 99);

    assert(m.prev == NULL);
    assert(meta_exists(&m, "node0"));
    assert(meta_exists(&m, "node1"));
    assert(meta_exists(&m, "node2"));
    assert(meta_exists(&m, "node3"));
    assert(!meta_exists(&m, "node4"));

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
    assert(b);

    meta_init(&m2, "cluster");
    b = meta_parse_uris(&m2, "tcp://nodea@127.0.0.1:7600"
                            " tcp://nodeb@127.0.0.1:7601"
                            " tcp://nodec@127.0.0.1:7602");
    assert(b);

    sc_buf_init(&buf, 1024);
    meta_encode(&m2, &buf);
    meta_replace(&m, sc_buf_rbuf(&buf), sc_buf_size(&buf));

    assert(meta_exists(&m, "nodea"));
    assert(meta_exists(&m, "nodeb"));
    assert(meta_exists(&m, "nodec"));

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
    test_execute(connection_test);
    test_execute(leader_test);
    test_execute(dup_test);
    test_execute(change_test);
    test_execute(change_success_test);
    test_execute(change_fail_test);
    test_execute(replace_test);

    return 0;
}
