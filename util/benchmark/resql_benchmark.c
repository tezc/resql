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

#include "hdr_histogram.h"
#include "resql.h"
#include "sc_mutex.h"
#include "sc_option.h"
#include "sc_thread.h"
#include "sc_time.h"

#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define RST    "\x1b[0m"
#define COLOR  "\x1b[1;36m"
#define GREEN  "\x1b[1;32m"
#define YELLOW "\x1b[1;33m"
#define RED    "\x1b[1;31m"

#define rs_exit(...)                                                           \
	printf("\n" RED);                                                      \
	printf(__VA_ARGS__);                                                   \
	printf(RST "\n\n");                                                    \
	fflush(stdout);                                                        \
	exit(1);

const char *fsync_stmt = "SELECT name, fsync_average_ms, fsync_max_ms "
			 "FROM resql_nodes;";
const char *clear_stmt = "DROP TABLE IF EXISTS bench_resql;";
const char *create_stmt = "CREATE TABLE bench_resql (id INTEGER PRIMARY KEY, "
			  "col1 TEXT, col2 TEXT, col3 TEXT);";
const char *read_stmt = "SELECT * FROM bench_resql WHERE id = ?;";
const char *write_stmt = "INSERT INTO bench_resql VALUES (?, ?, ?, ?);";

_Atomic uint64_t ops;
_Atomic uint64_t end_ts;
_Atomic uint64_t start_ts;

struct resql_benchmark {
	char *url;
	char *cluster_name;

	unsigned long long table_size;
	unsigned long long ops;
	unsigned long long clients;
	unsigned int batch;
	unsigned int test;
	char *str;

	unsigned int ready;
	struct sc_mutex ready_mtx;

	pthread_cond_t cond;
	pthread_mutex_t mtx;
};

#define RESQL_BENCHMARK_VERSION "0.1.3-latest"

static struct resql_benchmark bench;

static void resql_benchmark_usage()
{
	printf("\n resql-benchmark version : %s \n\n", RESQL_BENCHMARK_VERSION);
	printf(" -u=<uri>      --uri=<uri>         ex: --uri=tcp://127.0.0.1:7600               \n"
	       " -b=<count>    --batch=<count>     default is 1, e.g 5 inserts/selects in one op\n"
	       " -i=<count>    --initial=<count>   initial table size                           \n"
	       " -n=<name>     --name=<name>       cluster name, default is 'cluster'           \n"
	       " -c=<count>    --clients=<count>   default is 10                                \n"
	       " -o=<ops>      --ops=<ops>         default is 10000                             \n"
	       " -t=<test>     --test=<test>       e.g -t=readonly                              \n"
	       "                                      Valid values are                          \n"
	       "                                        - readonly  : %%100                     \n"
	       "                                        - readheavy : %%80 select, %%20 insert  \n"
	       "                                        - mixed     : %%50 select, %%50 insert  \n"
	       "                                        - writeonly : %%100 insert              \n"
	       " -h            --help              Print this help and exit  	                \n"
	       " -v,           --version           Print version and exit                       \n"
	       "\n\n");
}

void resql_benchmark_conf(struct resql_benchmark *b, int argc, char *argv[])
{
	// clang-format off
    static struct sc_option_item options[] = {
            {'b', "batch"},
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
	char *val;
	char *endp;

	for (int i = 1; i < argc; i++) {
		ch = sc_option_at(&opt, i, &val);

		switch (ch) {
		case 'b':
			if (val == NULL) {
				rs_exit("Invalid -b option \n");
			}

			errno = 0;
			bench.batch = strtoull(val, &endp, 10);
			if (endp == val || errno != 0) {
				rs_exit("Invalid -b option : %s \n", val);
			}

			break;
		case 'c':
			if (val == NULL) {
				rs_exit("Invalid -c option \n");
			}

			errno = 0;
			bench.clients = strtoull(val, &endp, 10);
			if (endp == val || errno != 0) {
				rs_exit("Invalid -c option : %s \n", val);
			}

			break;
		case 'i':
			if (val == NULL) {
				rs_exit("Invalid -i option \n");
			}

			errno = 0;
			bench.table_size = strtoull(val, &endp, 10);
			if (endp == val || errno != 0) {
				rs_exit("Invalid -i option : %s \n", val);
			}
			break;
		case 'n':
			if (val == NULL) {
				rs_exit("Invalid -n option \n");
			}

			free(bench.cluster_name);
			bench.cluster_name = strdup(val);
			break;
		case 'o':
			if (val == NULL) {
				rs_exit("Invalid -o option \n");
			}

			errno = 0;
			bench.ops = strtoull(val, &endp, 10);
			if (endp == val || errno != 0) {
				rs_exit("Invalid -c option : %s \n", val);
			}

			if (bench.ops < 3000) {
				rs_exit("-o must be greater than 3000.");
			}

			break;
		case 't':
			if (val == NULL) {
				rs_exit("Invalid -t option \n");
			}

			if (strcmp("readonly", val) == 0) {
				bench.test = 100;
				bench.str = "readonly (%100 SELECT)";
			} else if (strcmp("readheavy", val) == 0) {
				bench.test = 80;
				bench.str = "readheavy (%80 SELECT,%20 INSERT)";
			} else if (strcmp("mixed", val) == 0) {
				bench.test = 50;
				bench.str = "mixed (%50 SELECT, %50 INSERT)";
			} else if (strcmp("writeonly", val) == 0) {
				bench.test = 0;
				bench.str = "writeonly (%100 INSERT)";
			} else {
				printf("Invalid -t option %s \n", val);
				exit(1);
			}
			break;

		case 'u':
			free(b->url);
			b->url = strdup(val);
			break;

		case 'h':
		case 'v':
			resql_benchmark_usage();
			exit(0);
		default:
			printf("resql-benchmark : Unknown option %s \n",
			       argv[argc]);
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

	b->url = strdup("unix:///tmp/resql tcp://127.0.0.1:7600");
	b->clients = 10;
	b->batch = 1;
	b->ops = 10000;
	b->table_size = 10000;
	b->test = 80;
	b->str = "readheavy (%80 SELECT, %20 INSERT)";
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

struct task {
	struct sc_thread thread;
	unsigned int id;
	unsigned int *ready;
	struct sc_mutex *ready_mtx;
	pthread_cond_t *cond;
	pthread_mutex_t *mtx;

	struct hdr_histogram *hdr;
	unsigned long long total_ops;
	unsigned long long ops;
	unsigned int test;
	struct resql_config *config;
};

_Atomic unsigned int start;
unsigned int seed;

unsigned int rand_int()
{
	seed = (214013u * seed + 2531011u);
	return (seed >> 16u) % 100;
}

void *client_fn(void *arg)
{
	int rc;
	uint64_t id, it, n;
	resql *c;
	resql_stmt r, w;
	resql_result *rs;
	struct task *t = arg;
	struct resql_config *config = t->config;

	seed = time(NULL);

	rc = hdr_init(1, INT64_C(3600000000), 3, &t->hdr);
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

	sc_mutex_lock(t->ready_mtx);
	*t->ready += 1;
	sc_mutex_unlock(t->ready_mtx);

	pthread_mutex_lock(t->mtx);
	while (!start) {
		rc = pthread_cond_wait(t->cond, t->mtx);
		if (rc != 0) {
			rs_exit("%s", strerror(rc));
		}
	}
	pthread_mutex_unlock(t->mtx);

	id = t->id * 1000000000ull;
	it = id;

	uint64_t m = 0;
	atomic_compare_exchange_strong(&start_ts, &m, sc_time_mono_ns());

	while (t->ops != 0) {
		uint64_t s, diff;
		unsigned int val, batch;

		val = rand_int();
		batch = bench.batch <= t->ops ? bench.batch : t->ops;

		if (val < t->test) {
			unsigned int read_index = t->test == 100 ? 0 : id;

			for (unsigned int i = 0; i < batch; i++) {
				resql_put_prepared(c, &r);
				resql_bind_index_int(c, 0, read_index++);
			}

			s = sc_time_mono_ns();

			rc = resql_exec(c, true, &rs);
			if (rc != RESQL_OK) {
				rs_exit("Client : %s \n", resql_errstr(c));
			}

			diff = (sc_time_mono_ns() - s) / 1000;
			hdr_record_values(t->hdr, diff, batch);

		} else {
			for (unsigned int i = 0; i < batch; i++) {
				it++;
				resql_put_prepared(c, &w);
				resql_bind_index_int(c, 0, it);
				resql_bind_index_text(c, 1, "dummy");
				resql_bind_index_text(c, 2, "dummy");
				resql_bind_index_text(c, 3, "dummy");
			}

			s = sc_time_mono_ns();
			rc = resql_exec(c, false, &rs);
			if (rc != RESQL_OK) {
				rs_exit("Fail : %s \n", resql_errstr(c));
			}

			diff = (sc_time_mono_ns() - s) / 1000;
			hdr_record_values(t->hdr, diff, batch);
		}

		n = atomic_fetch_add_explicit(&ops, batch,
					      memory_order_release);
		if (n == t->total_ops - batch) {
			end_ts = sc_time_mono_ns();
		}

		t->ops -= batch;
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
	const char *bar = "||||||||||||||||||||||||||||||||||||||||||||||||||";
	const size_t len = strlen(bar);

	double val = (percentage * 100);
	unsigned int left = (unsigned int) (percentage * len);
	unsigned int right = len - left;
	const char *s = (val == 100) ? GREEN : YELLOW;

	printf("\r%s%.2f%%" RST " [%.*s%*s]", s, val, left, bar, right, "");
	fflush(stdout);
}

#define MAX(a, b) ((a) < (b) ? (b) : (a))
#define RST	  "\x1b[0m"
#define COLUMN	  "\x1b[0;35m"

void resql_print_seperator(int total, const int *columns)
{
	int x = -1;
	int pos = 0;

	for (int i = 0; i < total; i++) {
		if (i == pos) {
			x++;
			pos += columns[x] + 3;
			printf("+");
		} else {
			printf("-");
		}
	}
	printf("\n");
}

void resql_print_fsync(struct resql_result *rs)
{
	int total = 1, count;
	uint32_t columns;
	int *p = NULL;
	struct resql_column *row;

	count = resql_column_count(rs);
	p = calloc(sizeof(int), count);

	columns = resql_column_count(rs);
	while ((row = resql_row(rs)) != NULL) {
		for (size_t i = 0; i < columns; i++) {
			p[i] = MAX(p[i], (int) strlen(row[i].name));
		}
	}

	resql_reset_rows(rs);

	while ((row = resql_row(rs)) != NULL) {
		columns = resql_column_count(rs);
		for (uint32_t i = 0; i < columns; i++) {
			switch (row[i].type) {
			case RESQL_TEXT:
				p[i] = MAX(p[i], row[i].len);
				break;
			case RESQL_NULL:
				goto out;
			default:
				printf("Error, result set corrupt! \n");
				goto out;
			}
		}
	}

	resql_reset_rows(rs);

	for (int i = 0; i < count; i++) {
		total += p[i] + 3;
	}

	resql_print_seperator(total, p);
	columns = resql_column_count(rs);
	row = resql_row(rs);
	for (uint32_t i = 0; i < columns; i++) {
		printf("|" COLUMN " %-*s " RST, p[i], row[i].name);
	}
	printf("|\n");

	resql_print_seperator(total, p);

	do {
		columns = resql_column_count(rs);
		for (uint32_t i = 0; i < columns; i++) {
			switch (row[i].type) {
			case RESQL_TEXT:
				printf("| %-*s ", p[i], row[i].text);
				break;
			case RESQL_NULL:
				goto out;
			default:
				printf("Error, result set corrupt! \n");
				goto out;
			}
		}

		printf("|\n");

		resql_print_seperator(total, p);
	} while ((row = resql_row(rs)) != NULL);

out:
	free(p);
}

int main(int argc, char **argv)
{
	int rc;
	size_t total_ops, client_ops, curr;
	struct task *t;
	struct sigaction action;
	struct hdr_histogram *h;
	resql *c;
	// struct resql_column *row;
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
	printf("Batch              : %u \n", bench.batch);
	printf("Test               : %s \n", bench.str);
	printf("CREATE SQL         : %s \n", create_stmt);
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

	rc = resql_prepare(c, "INSERT INTO bench_resql VALUES(?, ?, ?, ?);",
			   &stmt);
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

		if (i > 0 && i % 8192 == 0) {
			rc = resql_exec(c, false, &rs);
			if (rc != RESQL_OK) {
				rs_exit("Failed : %s \n", resql_errstr(c));
			}

			print_progress((double) i / (double) bench.table_size);
		}
	}

	if (bench.table_size % 8192) {
		rc = resql_exec(c, false, &rs);
		if (rc != RESQL_OK) {
			rs_exit("Benchmark setup : %s \n", resql_errstr(c));
		}
	}

	print_progress(1);
	printf("\n\n");

	t = calloc(1, bench.clients * sizeof(*t));

	total_ops = bench.ops;
	client_ops = bench.ops / bench.clients;

	for (unsigned int i = 0; i < bench.clients; i++) {
		curr = total_ops < client_ops * 2 ? total_ops : client_ops;
		total_ops -= curr;

		t[i] = (struct task){
			.id = i + 1,
			.cond = &bench.cond,
			.mtx = &bench.mtx,
			.ready_mtx = &bench.ready_mtx,
			.ready = &bench.ready,
			.total_ops = bench.ops,
			.ops = curr,
			.test = bench.test,
			.config = &config,
		};

		sc_thread_init(&t[i].thread);

		rc = sc_thread_start(&t[i].thread, client_fn, &t[i]);
		if (rc != 0) {
			rs_exit("Thread : %s \n", sc_thread_err(&t[i].thread));
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
		uint64_t val = atomic_load_explicit(&ops, memory_order_acquire);

		print_progress(((double) val / (double) bench.ops));

		if (val == bench.ops) {
			break;
		}

		sleep_micro(400000);
	}

	for (unsigned int i = 0; i < bench.clients; i++) {
		rc = sc_thread_term(&t[i].thread);
		if (rc != 0) {
			rs_exit("Client failed. \n");
		}
	}

	for (unsigned int i = 0; i < bench.clients; i++) {
		hdr_add(h, t[i].hdr);
		hdr_close(t[i].hdr);
	}

	printf("\n\n");

	resql_put_sql(c, fsync_stmt);
	rc = resql_exec(c, false, &rs);
	if (rc != RESQL_OK) {
		rs_exit("Benchmark failed : %s \n", resql_errstr(c));
	}

	resql_print_fsync(rs);

	double total = (double) (end_ts - ts);

	const double th = (double) bench.ops / (total / 1e9);
	const double p0 = ((double) hdr_min(h)) / (double) 1000.0f;
	const double p50 = hdr_value_at_percentile(h, 50.0) / (double) 1000.0f;
	const double p90 = hdr_value_at_percentile(h, 90.0) / (double) 1000.0f;
	const double p95 = hdr_value_at_percentile(h, 95.0) / (double) 1000.0f;
	const double p99 = hdr_value_at_percentile(h, 99.0) / (double) 1000.0f;
	const double p999 = hdr_value_at_percentile(h, 99.9) / (double) 1000.0f;
	const double max = hdr_max(h) / (double) 1000.0f;
	const double avg = hdr_mean(h) / (double) 1000.0f;

	printf("\nlatency (milliseconds): \n\n");
	printf("p0.00  : %.3f ms\n", p0);
	printf("p0.50  : %.3f ms\n", p50);
	printf("p0.90  : %.3f ms\n", p90);
	printf("p0.95  : %.3f ms\n", p95);
	printf("p0.99  : %.3f ms\n", p99);
	printf("p0.999 : %.3f ms\n", p999);
	printf("max    : %.3f ms\n", max);
	printf("\n\n");

	printf("Total time  : " COLOR " %.3f seconds.\n" RST, total / 1e9);
	printf("Avg latency : " COLOR " %.3f milliseconds.\n" RST, avg);
	printf("Throughput  : " COLOR " %.3f operations per second.\n" RST, th);
	printf("\n\n");

	resql_put_sql(c, clear_stmt);
	rc = resql_exec(c, false, &rs);
	if (rc != RESQL_OK) {
		rs_exit("Benchmark clean up failed : %s \n", resql_errstr(c));
	}

	resql_shutdown(c);
	hdr_close(h);
	free(t);
	free(bench.cluster_name);

	return 0;
}

