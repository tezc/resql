/*
 * MIT License
 *
 * Copyright (c) 2021 Resql Authors
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

#include "hdr_histogram.h"
#include "resql.h"
#include "sc_mutex.h"
#include "sc_option.h"
#include "sc_thread.h"
#include "sc_time.h"

#include <errno.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RESET  "\x1b[0m"
#define COLOR  "\x1b[1;36m"
#define GREEN  "\x1b[1;32m"
#define YELLOW "\x1b[1;33m"
#define RED    "\x1b[1;31m"

#define rs_exit(...)                                                           \
    printf("\n");                                                              \
    printf(RED);                                                               \
    printf(__VA_ARGS__);                                                       \
    printf("\n\n");                                                            \
    fflush(stdout);                                                            \
    exit(1);

const char *clear_stmt = "DROP TABLE IF EXISTS bench_resql;";
const char *create_stmt =
        "CREATE TABLE bench_resql (id INTEGER PRIMARY KEY, col1 TEXT, col2 TEXT, col3 TEXT);";
const char *read_stmt = "SELECT * FROM bench_resql WHERE id = ?;";
const char *write_stmt = "INSERT INTO bench_resql VALUES (?, ?, ?, ?);";

_Atomic uint64_t ops;

struct resql_benchmark
{
    char *url;
    char *cluster_name;

    unsigned long long table_size;
    unsigned long long ops;
    unsigned long long clients;
    unsigned int test;
    char *test_str;

    unsigned int ready;
    struct sc_mutex ready_mtx;

    pthread_cond_t cond;
    pthread_mutex_t mtx;
};

#define RESQL_BENCHMARK_VERSION "0.0.16-latest"

static struct resql_benchmark bench;

static void resql_benchmark_usage()
{
    printf("\n resql-benchmark version : %s \n\n", RESQL_BENCHMARK_VERSION);
    printf(" -u=<uri>      --uri=<uri>         ex: --uri=tcp://127.0.0.1:7600             \n"
           " -i=<count>    --initial=<count>   initial table size                         \n"
           " -n=<name>     --name=<name>       cluster name, default is 'cluster'         \n"
           " -c=<count>    --clients=<count>   default is 10                              \n"
           " -o=<ops>      --ops=<ops>         default is 10000                           \n"
           " -t=<test>     --test=<test>       e.g -t=readonly                            \n"
           "                                      Valid values are                        \n"
           "                                        - readonly  : %%100 select            \n"
           "                                        - readheavy : %%80 select, %%20 insert\n"
           "                                        - mixed     : %%50 select, %%50 insert\n"
           "                                        - writeonly : %%100 insert            \n"
           " -h            --help              Print this help and exit                   \n"
           " -v,           --version           Print version and exit                     \n"
           "\n\n");
}

void resql_benchmark_conf(struct resql_benchmark *b, int argc, char *argv[])
{
    // clang-format off
    static struct sc_option_item options[] = {
            {'c', "clients"},
            {'i', "initial"},
            {'n', "name"},
            {'h', "help"},
            {'o', "ops"},
            {'t', "test"},
            {'u', "uri"},
            {'v', "version"},
    };
    // clang-format on

    struct sc_option opt = {.argv = argv,
                            .count = sizeof(options) / sizeof(options[0]),
                            .options = options};
    char ch;
    char *value;
    char *endp;

    for (int i = 1; i < argc; i++) {
        ch = sc_option_at(&opt, i, &value);

        switch (ch) {
        case 'c':
            if (value == NULL) {
                rs_exit("Invalid -c option \n");
            }

            errno = 0;
            bench.clients = strtoull(value, &endp, 10);
            if (endp == value || errno != 0) {
                rs_exit("Invalid -c option : %s \n", value);
            }

            break;
        case 'i':
            if (value == NULL) {
                rs_exit("Invalid -i option \n");
            }

            errno = 0;
            bench.table_size = strtoull(value, &endp, 10);
            if (endp == value || errno != 0) {
                rs_exit("Invalid -i option : %s \n", value);
            }
            break;
        case 'n':
            if (value == NULL) {
                rs_exit("Invalid -n option \n");
            }

            free(bench.cluster_name);
            bench.cluster_name = strdup(value);
            break;
        case 'o':
            if (value == NULL) {
                rs_exit("Invalid -o option \n");
            }

            errno = 0;
            bench.ops = strtoull(value, &endp, 10);
            if (endp == value || errno != 0) {
                rs_exit("Invalid -c option : %s \n", value);
            }

            if (bench.ops < 3000) {
                rs_exit("-o must be greater than 3000.");
            }

            break;
        case 't':
            if (value == NULL) {
                rs_exit("Invalid -t option \n");
            }

            if (strncmp("readonly", value, strlen("readonly")) == 0) {
                bench.test = 100;
                bench.test_str = "readonly";
            } else if (strncmp("readheavy", value, strlen("readheavy")) == 0) {
                bench.test = 80;
                bench.test_str = "readheavy";
            } else if (strncmp("mixed", value, strlen("mixed")) == 0) {
                bench.test = 50;
                bench.test_str = "mixed";
            } else if (strncmp("writeonly", value, strlen("writeonly")) == 0) {
                bench.test = 0;
                bench.test_str = "writeonly";
            } else {
                printf("Invalid -t option %s \n", value);
                exit(1);
            }
            break;

        case 'u':
            free(b->url);
            b->url = strdup(value);
            break;

        case 'h':
        case 'v':
            resql_benchmark_usage();
            exit(0);
        default:
            printf("resql-benchmark : Unknown option %s \n", argv[argc]);
            resql_benchmark_usage();
            exit(1);
        }
    }
}

void resql_benchmark_init(struct resql_benchmark *b)
{
    int rc;
    pthread_mutexattr_t attr;
    pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

    b->url = strdup("tcp://127.0.0.1:7600");
    b->clients = 1;
    b->ops = 10000;
    b->table_size = 10000;
    b->test = 80;
    b->test_str = "readheavy";
    b->cluster_name = strdup("cluster");

    sc_mutex_init(&bench.ready_mtx);

    rc = pthread_mutexattr_init(&attr);
    if (rc != 0) {
        rs_exit("%s", strerror(rc));
    }

    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);

    rc = pthread_mutex_init(&mut, &attr);
    if (rc != 0) {
        rs_exit("%s", strerror(rc));
    }

    bench.mtx = mut;

    rc = pthread_cond_init(&bench.cond, NULL);
    if (rc != 0) {
        rs_exit("%s", strerror(rc));
    }

    pthread_mutexattr_destroy(&attr);
}

static void resql_signal(int sig)
{
    (void) sig;

    printf("Shutting down.. \n");
    exit(0);
}


struct task
{
    struct sc_thread thread;
    unsigned int id;
    unsigned int *ready;
    struct sc_mutex *ready_mtx;
    pthread_cond_t *cond;
    pthread_mutex_t *mtx;

    struct hdr_histogram *hdr;
    unsigned long long ops;
    unsigned int test;
    struct resql_config *config;
};

unsigned int start;
unsigned int seed;

unsigned int fastrand()
{
    seed = (214013u * seed + 2531011u);
    return (seed >> 16u) % 100;
}

void *client_fn(void *arg)
{
    int rc;
    uint64_t id;
    resql *c;
    resql_stmt r, w;
    resql_result *rs;
    struct task *task = arg;
    struct resql_config *config = task->config;

    seed = time(NULL);

    rc = hdr_init(1, INT64_C(3600000000), 3, &task->hdr);
    if (rc != 0) {
        rs_exit("hdr failure : %s \n", strerror(rc));
    }

    rc = resql_create(&c, config);
    if (rc != RESQL_OK) {
        rs_exit("Failed to connect to server at %s \n", bench.url);
    }

    rc = resql_prepare(c, read_stmt, &r);
    if (rc != RESQL_OK) {
        rs_exit("prepare failed : %s \n", resql_errstr(c));
    }

    rc = resql_prepare(c, write_stmt, &w);
    if (rc != RESQL_OK) {
        rs_exit("prepare failed : %s \n", resql_errstr(c));
    }

    sc_mutex_lock(task->ready_mtx);
    *task->ready += 1;
    sc_mutex_unlock(task->ready_mtx);

    pthread_mutex_lock(task->mtx);
    while (!start) {
        rc = pthread_cond_wait(task->cond, task->mtx);
        if (rc != 0) {
            rs_exit("%s", strerror(rc));
        }
    }
    pthread_mutex_unlock(task->mtx);

    id = task->id * 1000000000ull;

    while (task->ops != 0) {
        uint64_t s;
        unsigned int val = fastrand();
        ops++;

        if (val <= task->test) {
            resql_put_prepared(c, &r);
            resql_bind_index_int(c, 0, id);

            s = sc_time_mono_ns();

            rc = resql_exec(c, true, &rs);
            if (rc != RESQL_OK) {
                rs_exit("client failed : %s \n", resql_errstr(c));
            }

            hdr_record_value(task->hdr, (sc_time_mono_ns() - s) / 1000);
        } else {
            id++;
            resql_put_prepared(c, &w);
            resql_bind_index_int(c, 0, id);
            resql_bind_index_text(c, 1, "dummy");
            resql_bind_index_text(c, 2, "dummy");
            resql_bind_index_text(c, 3, "dummy");

            s = sc_time_mono_ns();
            rc = resql_exec(c, false, &rs);
            if (rc != RESQL_OK) {
                rs_exit("client failed : %s \n", resql_errstr(c));
            }
            hdr_record_value(task->hdr, (sc_time_mono_ns() - s) / 1000);
        }

        task->ops--;
    }

    resql_shutdown(c);
    return 0;
}

int sleep_micro(uint64_t us)
{
#if defined(_WIN32) || defined(_WIN64)
    Sleep((DWORD) millis);
    return 0;
#else
    int rc;
    struct timespec t, rem;

    rem.tv_sec = us / 1000 / 1000;
    rem.tv_nsec = (us % 1000) * 1000;

    do {
        t = rem;
        rc = nanosleep(&t, &rem);
    } while (rc != 0 && errno == EINTR);

    return rc;
#endif
}

void print_progress(double percentage)
{
    const char* bar = "||||||||||||||||||||||||||||||||||||||||||||||||||";
    const size_t len = strlen(bar);

    unsigned int val = (unsigned int) (percentage * 100);
    unsigned int left = (unsigned int) (percentage * len);
    unsigned int right = len - left;
    const char *s = (val == 100) ? GREEN : YELLOW;

    printf("\r%s%3d%%" RESET " [%.*s%*s]", s, val, left, bar, right, "");
    fflush(stdout);
}

int main(int argc, char **argv)
{
    int rc;
    struct task *t;
    struct sigaction action;
    struct hdr_histogram *h;
    resql *c;
    resql_stmt stmt;
    resql_result *rs;

    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_handler = resql_signal;

    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGINT, &action, NULL);

    resql_benchmark_init(&bench);
    resql_benchmark_conf(&bench, argc, argv);

    rc = hdr_init(1, INT64_C(3600000000), 3, &h);
    if (rc != 0) {
        rs_exit("hdr : %s \n", strerror(rc));
    }

    printf("\n-----------------------------------------------------\n\n");
    printf("Cluster name       : %s \n", bench.cluster_name);
    printf("Client count       : %llu \n", bench.clients);
    printf("Initial table size : %llu \n", bench.table_size);
    printf("Url                : %s \n", bench.url);
    printf("Operations         : %llu \n", bench.ops);
    printf("Test               : %s \n", bench.test_str);
    printf("Table SQL          : %s \n", create_stmt);
    printf("SELECT SQL         : %s \n", read_stmt);
    printf("INSERT SQL         : %s \n", write_stmt);
    printf("\n\n");

    struct resql_config config = {
            .urls = bench.url,
            .timeout_millis = 8000,
            .cluster_name = "cluster",
    };

    rc = resql_create(&c, &config);
    if (rc != RESQL_OK) {
        rs_exit("Failed to connect to server at %s \n", bench.url);
    }

    resql_put_sql(c, clear_stmt);
    resql_put_sql(c, create_stmt);

    rc = resql_exec(c, false, &rs);
    if (rc != RESQL_OK) {
        rs_exit("Benchmark setup failed : %s \n", resql_errstr(c));
    }

    rc = resql_prepare(c, "INSERT INTO bench_resql VALUES(?, ?, ?, ?);", &stmt);
    if (rc != RESQL_OK) {
        rs_exit("Failed to connect to server at %s \n", bench.url);
    }

    printf("Step-1 : Loading table .... \n");

    for (size_t i = 0; i < bench.table_size; i++) {
        resql_put_prepared(c, &stmt);
        resql_bind_index_int(c, 0, i);
        resql_bind_index_text(c, 1, "dummy");
        resql_bind_index_text(c, 2, "dummy");
        resql_bind_index_text(c, 3, "dummy");

        if (i % 8192 == 0) {
            rc = resql_exec(c, false, &rs);
            if (rc != RESQL_OK) {
                rs_exit("Benchmark setup failed : %s \n", resql_errstr(c));
            }

            print_progress((double) i / (double) bench.table_size);
        }
    }

    if (bench.table_size % 8192) {
        rc = resql_exec(c, false, &rs);
        if (rc != RESQL_OK) {
            rs_exit("Benchmark setup failed : %s \n", resql_errstr(c));
        }
    }

    print_progress(1);
    printf("\n\n");

    t = calloc(1, bench.clients * sizeof(*t));

    for (unsigned int i = 0; i < bench.clients; i++) {
        t[i] = (struct task){.id = i + 1,
                             .cond = &bench.cond,
                             .mtx = &bench.mtx,
                             .ready_mtx = &bench.ready_mtx,
                             .ready = &bench.ready,
                             .ops = bench.ops / bench.clients,
                             .test = bench.test,
                             .config = &config};

        sc_thread_init(&t[i].thread);

        rc = sc_thread_start(&t[i].thread, client_fn, &t[i]);
        if (rc != 0) {
            rs_exit("Thread failure : %s \n", sc_thread_err(&t[i].thread));
        }
    }

    while (true) {
        unsigned long long val;

        sc_mutex_lock(&bench.ready_mtx);
        val = bench.ready;
        sc_mutex_unlock(&bench.ready_mtx);

        if (val == bench.clients) {
            break;
        }

        sc_time_sleep(2);
    }

    printf("Step-2 : Executing statements .... \n");

    uint64_t ts = sc_time_mono_ns();

    start = 1;
    rc = pthread_cond_broadcast(&bench.cond);
    if (rc != 0) {
        rs_exit("%s", strerror(rc));
    }

    while (true) {
        uint64_t val = ops;

        print_progress(((double) val / (double) bench.ops));

        if (val == bench.ops) {
            break;
        }

        sleep_micro(50);
    }

    printf("\n\n");

    for (unsigned int i = 0; i < bench.clients; i++) {
        rc = sc_thread_term(&t[i].thread);
        if (rc != 0) {
            rs_exit("Client failed. \n");
        }
    }

    uint64_t total = sc_time_mono_ns() - ts;

    for (unsigned int i = 0; i < bench.clients; i++) {
        hdr_add(h, t[i].hdr);
        hdr_close(t[i].hdr);
    }

    const double thput = (double) bench.ops / ((double) total / 1e9);
    const double p0 = ((double) hdr_min(h)) / (double) 1000.0f;
    const double p50 = hdr_value_at_percentile(h, 50.0) / (double) 1000.0f;
    const double p90 = hdr_value_at_percentile(h, 90.0) / (double) 1000.0f;
    const double p95 = hdr_value_at_percentile(h, 95.0) / (double) 1000.0f;
    const double p99 = hdr_value_at_percentile(h, 99.0) / (double) 1000.0f;
    const double p100 = ((double) hdr_max(h)) / (double) 1000.0f;
    const double avg = hdr_mean(h) / (double) 1000.0f;

    printf("\nlatency (milliseconds): \n\n");
    printf("p0.00 : %.3f ms\n", p0);
    printf("p0.50 : %.3f ms\n", p50);
    printf("p0.90 : %.3f ms\n", p90);
    printf("p0.95 : %.3f ms\n", p95);
    printf("p0.99 : %.3f ms\n", p99);
    printf("max   : %.3f ms\n", p100);
    printf("\n\n");

    printf("Total time  : " COLOR " %.3f seconds. \n" RESET,
           (double) total / 1e9);
    printf("Avg latency : " COLOR " %.3f milliseconds. \n" RESET, avg);
    printf("Throughput  : " COLOR " %.3f operations per second. \n" RESET,
           thput);
    printf("\n\n");

    resql_put_sql(c, clear_stmt);
    rc = resql_exec(c, false, &rs);
    if (rc != RESQL_OK) {
        rs_exit("Benchmark setup failed : %s \n", resql_errstr(c));
    }

    resql_shutdown(c);
    hdr_close(h);
    free(t);
    free(bench.cluster_name);

    return 0;
}