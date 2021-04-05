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

#include "test_util.h"

#include "file.h"
#include "resql.h"
#include "server.h"

#include "sc/sc_log.h"
#include "sc/sc_str.h"

#include <errno.h>

int init;
static int count;
static struct server *cluster[9];

// clang-format off

#define node0 "tcp://node0@127.0.0.1:7600"
#define node1 node0 " tcp://node1@127.0.0.1:7601"
#define node2 node1 " tcp://node2@127.0.0.1:7602"
#define node3 node2 " tcp://node3@127.0.0.1:7603"
#define node4 node3 " tcp://node4@127.0.0.1:7604"
#define node5 node4 " tcp://node5@127.0.0.1:7605"
#define node6 node5 " tcp://node6@127.0.0.1:7606"
#define node7 node6 " tcp://node7@127.0.0.1:7607"
#define node8 node7 " tcp://node8@127.0.0.1:7608"

static const char *nodes[] = {
        node0,
        node1,
        node2,
        node3,
        node4,
        node5,
        node6,
        node7,
        node8,
};

static const char *names[] = {
        "node0",
        "node1",
        "node2",
        "node3",
        "node4",
        "node5",
        "node6",
        "node7",
        "node8",
};

static const char *urls[] = {
        "tcp://node0@127.0.0.1:7600",
        "tcp://node1@127.0.0.1:7601",
        "tcp://node2@127.0.0.1:7602",
        "tcp://node3@127.0.0.1:7603",
        "tcp://node4@127.0.0.1:7604",
        "tcp://node5@127.0.0.1:7605",
        "tcp://node6@127.0.0.1:7606",
        "tcp://node7@127.0.0.1:7607",
        "tcp://node8@127.0.0.1:7608",
};

static const char *dirs[] = {
        "/tmp/node0",
        "/tmp/node1",
        "/tmp/node2",
        "/tmp/node3",
        "/tmp/node4",
        "/tmp/node5",
        "/tmp/node6",
        "/tmp/node7",
        "/tmp/node8",
};

// clang-format on

void init_all()
{
    if (!init) {
        setvbuf(stdout, NULL, _IONBF, 0);
        setvbuf(stderr, NULL, _IONBF, 0);

        sc_log_set_thread_name("test");
        server_global_init();
        init = 1;
    }
}

void cleanup()
{
    int rc;

    for (int i = 0; i < 9; i++) {
        rc = file_rmdir(dirs[i]);
        rs_assert(!(rc != 0 && errno != ENOENT));

        rc = file_mkdir(dirs[i]);
        rs_assert(rc == 0);
    }

    rc = file_rmdir("/tmp/resql_test");
    rs_assert(!(rc != 0 && errno != ENOENT));

    rc = file_mkdir("/tmp/resql_test");
    rs_assert(rc == 0);
}

void test_util_run(void (*test_fn)(void), const char *fn_name)
{
    init_all();
    cleanup();

    sc_log_info("[ Running ] %s \n", fn_name);
    test_fn();
    test_client_destroy_all();
    test_server_destroy_all();
    sc_log_info("[ Passed  ] %s  \n", fn_name);
}

struct server *test_server_start(int id, int cluster_size)
{
    char *opt[] = {""};

    int rc;
    struct conf conf;
    struct server *s;

    assert(id >= 0 && id < 9);
    assert(cluster[id] == NULL);
    assert(cluster_size > 0 && cluster_size <= 9);

    conf_init(&conf);

    sc_str_set(&conf.node.log_level, "DEBUG");
    sc_str_set(&conf.node.name, names[id]);
    sc_str_set(&conf.node.bind_url, urls[id]);
    sc_str_set(&conf.node.ad_url, urls[id]);
    sc_str_set(&conf.cluster.nodes, nodes[cluster_size - 1]);
    sc_str_set(&conf.node.dir, dirs[id]);

    conf_read_config(&conf, false, sizeof(opt) / sizeof(char *), opt);

    s = server_create(&conf);

    rc = server_start(s, true);
    if (rc != RS_OK) {
        abort();
    }

    cluster[id] = s;
    count++;

    return s;
}

struct server *test_server_create_conf(struct conf *conf, int id)
{
    int rc;
    struct server *s;

    s = server_create(conf);

    rc = server_start(s, true);
    if (rc != RS_OK) {
        abort();
    }

    cluster[id] = s;
    count++;

    return s;
}

struct server *test_server_create(int id, int cluster_size)
{
    char *opt[] = {"", "-e"};

    int rc;
    struct conf conf;
    struct server *s;

    assert(id >= 0 && id < 9);
    assert(cluster[id] == NULL);
    assert(cluster_size > 0 && cluster_size <= 9);

    conf_init(&conf);

    sc_str_set(&conf.node.log_level, "DEBUG");
    sc_str_set(&conf.node.name, names[id]);
    sc_str_set(&conf.node.bind_url, urls[id]);
    sc_str_set(&conf.node.ad_url, urls[id]);
    sc_str_set(&conf.cluster.nodes, nodes[cluster_size - 1]);
    sc_str_set(&conf.node.dir, dirs[id]);

    conf_read_config(&conf, false, sizeof(opt) / sizeof(char *), opt);

    s = server_create(&conf);

    rc = server_start(s, true);
    if (rc != RS_OK) {
        abort();
    }

    cluster[id] = s;
    count++;

    return s;
}

void test_server_destroy(int id)
{
    int rc;

    rs_assert(id >= 0 && id < 9);
    rs_assert(cluster[id] != NULL);

    rc = server_stop(cluster[id]);
    rs_assert(rc == RS_OK);

    cluster[id] = NULL;
    count--;
}

void test_server_destroy_all()
{
    int rc;

    for (int i = 0; i < 9; i++) {
        if (cluster[i] != NULL) {
            rc = server_stop(cluster[i]);
            rs_assert(rc == RS_OK);
            cluster[i] = NULL;
        }
    }

    count = 0;
}

void test_destroy_leader()
{
    int rc, n;
    char *endp;
    const char *head;
    resql *c;
    resql_result *rs;

    c = test_client_create();
    resql_put_sql(c, "SELECT name FROM resql_info WHERE role = 'leader';");
    rc = resql_exec(c, true, &rs);
    client_assert(c, rc == RESQL_OK);

    head = resql_row(rs)[0].text + strlen("node");
    endp = (char *) head + 1;
    n = (int) strtoul(head, &endp, 10);

    test_server_destroy(n);
}

resql *clients[256];
int client_count;

resql *test_client_create()
{
    rs_assert(client_count < 256);

    int rc;
    bool found;
    const char *url = urls[0];
    resql *c;

    for (int i = 0; i < 9; i++) {
        if (cluster[i] != NULL) {
            url = urls[i];
            break;
        }
    }

    struct resql_config conf = {.urls = url, .timeout_millis = 600000};

    rc = resql_create(&c, &conf);
    if (rc != RESQL_OK) {
        printf("Failed rs : %d \n", rc);
        abort();
    }

    found = false;
    for (int i = 0; i < 256; i++) {
        if (clients[i] == NULL) {
            clients[i] = c;
            found = true;
            break;
        }
    }

    rs_assert(found);
    client_count++;

    return c;
}

void test_client_destroy(resql *c)
{
    bool found;
    int rc;

    rc = resql_shutdown(c);
    if (rc != RESQL_OK) {
        printf("Failed rs : %d \n", rc);
        abort();
    }

    found = false;
    for (int i = 0; i < 256; i++) {
        if (clients[i] == c) {
            clients[i] = NULL;
            found = true;
            break;
        }
    }

    rs_assert(found);
    client_count--;
}

void test_client_destroy_all()
{
    int rc;

    for (int i = 0; i < 256; i++) {
        if (clients[i] != NULL) {
            rc = resql_shutdown(clients[i]);
            if (rc != RESQL_OK) {
                printf("Failed rs : %d \n", rc);
                abort();
            }
            clients[i] = NULL;
        }
    }

    client_count = 0;
}
