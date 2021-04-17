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

#include "metric.h"

#include "rs.h"

#include "sc/sc.h"
#include "sc/sc_buf.h"
#include "sc/sc_log.h"
#include "sc/sc_time.h"

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/resource.h>
#include <sys/statvfs.h>
#include <time.h>
#include <unistd.h>

static size_t metric_free_disk(const char *dir)
{
	int rc;
	struct statvfs st;

	rc = statvfs(dir, &st);
	if (rc < 0) {
		sc_log_error("dir : %s, statvfs : %s \n", dir, strerror(errno));
		return 0;
	}

	return ((size_t) (st.f_bavail) * st.f_bsize);
}

/* Returns the size of physical memory (RAM) in bytes.
 * It looks ugly, but this is the cleanest way to achieve cross platform
 * results. Cleaned up from:
 *
 * http://nadeausoftware.com/articles/2012/09/c_c_tip_how_get_physical_memory_size_system
 *
 * Note that this function:
 * 1) Was released under the following CC attribution license:
 *    http://creativecommons.org/licenses/by/3.0/deed.en_US.
 * 2) Was originally implemented by David Robert Nadeau.
 * 3) Was modified for Redis by Matt Stancliff.
 * 4) This note exists in order to comply with the original license.
 * 5) Resql took it from :
 *    https://github.com/redis/redis/blob/unstable/src/zmalloc.c
 */
static size_t metric_get_physical_memory(void)
{
#if defined(__unix__) || defined(__unix) || defined(unix) ||                   \
	(defined(__APPLE__) && defined(__MACH__))
#if defined(CTL_HW) && (defined(HW_MEMSIZE) || defined(HW_PHYSMEM64))
	int mib[2];
	mib[0] = CTL_HW;
#if defined(HW_MEMSIZE)
	mib[1] = HW_MEMSIZE; /* OSX. --------------------- */
#elif defined(HW_PHYSMEM64)
	mib[1] = HW_PHYSMEM64; /* NetBSD, OpenBSD. --------- */
#endif
	int64_t size = 0; /* 64-bit */
	size_t len = sizeof(size);
	if (sysctl(mib, 2, &size, &len, NULL, 0) == 0)
		return (size_t) size;
	return 0L; /* Failed? */

#elif defined(_SC_PHYS_PAGES) && defined(_SC_PAGESIZE)
	/* FreeBSD, Linux, OpenBSD, and Solaris. -------------------- */
	return (size_t) sysconf(_SC_PHYS_PAGES) *
	       (size_t) sysconf(_SC_PAGESIZE);

#elif defined(CTL_HW) && (defined(HW_PHYSMEM) || defined(HW_REALMEM))
	/* DragonFly BSD, FreeBSD, NetBSD, OpenBSD, and OSX. -------- */
	int mib[2];
	mib[0] = CTL_HW;
#if defined(HW_REALMEM)
	mib[1] = HW_REALMEM;   /* FreeBSD. ----------------- */
#elif defined(HW_PHYSMEM)
	mib[1] = HW_PHYSMEM; /* Others. ------------------ */
#endif
	unsigned int size = 0; /* 32-bit */
	size_t len = sizeof(size);
	if (sysctl(mib, 2, &size, &len, NULL, 0) == 0)
		return (size_t) size;
	return 0L; /* Failed? */
#else
	return 0L; /* Unknown method to get the data. */
#endif
#else
	return 0L; /* Unknown OS. */
#endif
}

#ifdef HAVE_LINUX

size_t metric_get_rss(struct metric *m)
{
	(void) m;

	size_t rss;
	char buf[4096];
	char filename[256];
	int fd, count;
	char *p, *x;

	snprintf(filename, 256, "/proc/%d/stat", getpid());

	fd = open(filename, O_RDONLY);
	if (fd == -1) {
		return 0;
	}

	if (read(fd, buf, 4096) <= 0) {
		close(fd);
		return 0;
	}
	close(fd);

	p = buf;
	count = 23; /* RSS is the 24th field in /proc/<pid>/stat */
	while (p && count--) {
		p = strchr(p, ' ');
		if (p) {
			p++;
		}
	}

	if (!p) {
		return 0;
	}

	x = strchr(p, ' ');
	if (!x) {
		return 0;
	}
	*x = '\0';

	rss = (size_t) strtoll(p, NULL, 10);
	rss *= sysconf(_SC_PAGESIZE);

	return rss;
}
#elif defined(HAVE_OSX)
#include <mach/mach_init.h>
#include <mach/task.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <unistd.h>

size_t metric_get_rss(struct metric *m)
{
	(void) m;

	task_t task = MACH_PORT_NULL;
	struct task_basic_info t_info;
	mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;

	if (task_for_pid(current_task(), getpid(), &task) != KERN_SUCCESS) {
		return 0;
	}

	task_info(task, TASK_BASIC_INFO, (task_info_t) &t_info, &t_info_count);

	return t_info.resident_size;
}
#elif defined(HAVE_FREEBSD)
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/user.h>
#include <unistd.h>

size_t metric_get_rss(struct metric *metric)
{
	(void) metric;

	struct kinfo_proc info;
	size_t infolen = sizeof(info);
	int mib[4];
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_PID;
	mib[3] = getpid();

	if (sysctl(mib, 4, &info, &infolen, NULL, 0) == 0) {
		return (size_t) info.ki_rssize;
	}

	return 0L;
}
#else
size_t metric_get_rss(struct metric *m)
{
	(void) m;
	return 0;
}
#endif

thread_local struct metric *tl_metric;

int metric_init(struct metric *m, const char *dir)
{
	int rc;
	size_t n;
	time_t t;
	struct tm *tm, result;

	tl_metric = m;

	*m = (struct metric){0};

	t = time(NULL);
	tm = localtime_r(&t, &result);
	if (!tm) {
		sc_log_error("localtime : %s \n", strerror(errno));
		return RS_ERROR;
	}

	n = strftime(m->start, sizeof(m->start) - 1, "%d-%m-%Y %H:%M", tm);
	if (n == 0) {
		sc_log_error("localtime : %s \n", strerror(errno));
		return RS_ERROR;
	}

	rc = uname(&m->utsname);
	if (rc != 0) {
		sc_log_error("uname : %s \n", strerror(errno));
		return RS_ERROR;
	}

	m->pid = getpid();
	m->total_memory = metric_get_physical_memory();
	m->start_time = sc_time_ms();
	m->bytes_recv = 0;
	m->bytes_sent = 0;
	m->ss_success = true;

	rs_strncpy(m->dir, dir, sizeof(m->dir) - 1);

	return RS_OK;
}

void metric_term(struct metric *metric)
{
	(void) metric;
}

void metric_recv(int64_t val)
{
	tl_metric->bytes_recv += val;
}

void metric_send(int64_t val)
{
	tl_metric->bytes_sent += val;
}

void metric_fsync(uint64_t val)
{
	struct metric *m = tl_metric;

	if (!m) {
		return;
	}

	m->fsync_max = m->fsync_max > val ? m->fsync_max : val;
	m->fsync_total += val;
	m->fsync_count++;
}

void metric_snapshot(bool success, uint64_t time, size_t size)
{
	struct metric *m = tl_metric;

	m->ss_success = success;
	if (!m->ss_success) {
		return;
	}

	m->ss_size = size;
	m->ss_max = m->ss_max > time ? m->ss_max : time;
	m->ss_total += time;
	m->ss_count++;
}

void metric_encode(struct metric *m, struct sc_buf *buf)
{
	char b[128] = "";
	time_t t;
	size_t sz, n;
	uint64_t ts, val, div;
	struct rusage u;
	struct tm result, *tm;

	memset(&u, 0, sizeof(u));

	sc_buf_put_str(buf, RS_VERSION);
	sc_buf_put_str(buf, RS_GIT_BRANCH);
	sc_buf_put_str(buf, RS_GIT_COMMIT);
	sc_buf_put_fmt(buf, "%s %s %s", m->utsname.sysname, m->utsname.release,
		       m->utsname.machine);

	sc_buf_put_str(buf, DEF_ARCH);
	sc_buf_put_fmt(buf, "%ld", m->pid);

	t = time(NULL);
	tm = localtime_r(&t, &result);
	if (tm != NULL) {
		n = strftime(b, sizeof(b) - 1, "%d-%m-%Y %H:%M", tm);
		if (n == 0) {
			b[0] = '\0'; // print empty string on error.
		}
	}

	sc_buf_put_str(buf, b);
	sc_buf_put_str(buf, m->start);
	sc_buf_put_fmt(buf, "%" PRIu64, m->start_time);

	ts = (sc_time_ms() - m->start_time) / 1000;
	sc_buf_put_fmt(buf, "%" PRIu64, ts);
	sc_buf_put_fmt(buf, "%" PRIu64, ts / (60 * 60 * 24));

	// Prints zero on getrusage failure.
	getrusage(RUSAGE_SELF, &u);
	sc_buf_put_fmt(buf, "%ld.%06ld", u.ru_stime.tv_sec, u.ru_stime.tv_usec);
	sc_buf_put_fmt(buf, "%ld.%06ld", u.ru_utime.tv_sec, u.ru_utime.tv_usec);

	sc_buf_put_fmt(buf, "%" PRIu64, m->bytes_recv);
	sc_buf_put_fmt(buf, "%" PRIu64, m->bytes_sent);
	sc_buf_put_fmt(buf, "%s",
		       sc_bytes_to_size(b, sizeof(b), m->bytes_recv));
	sc_buf_put_fmt(buf, "%s",
		       sc_bytes_to_size(b, sizeof(b), m->bytes_sent));

	sc_buf_put_fmt(buf, "%" PRIu64, m->total_memory);
	sc_buf_put_fmt(buf, "%s",
		       sc_bytes_to_size(b, sizeof(b), m->total_memory));

	sz = metric_get_rss(m);
	sc_buf_put_fmt(buf, "%zu", sz);
	sc_buf_put_fmt(buf, "%s",
		       sc_bytes_to_size(b, sizeof(b), (uint64_t) sz));

	sc_buf_put_fmt(buf, "%f", ((double) m->fsync_max) / 1000000);

	div = (m->fsync_count ? m->fsync_count : 1);
	sc_buf_put_fmt(buf, "%f", ((double) m->fsync_total / div) / 1000000);

	sc_buf_put_fmt(buf, "%s", m->ss_success ? "true" : "false");
	sc_buf_put_fmt(buf, "%zu", m->ss_size);
	sc_buf_put_fmt(buf, "%s", sc_bytes_to_size(b, sizeof(b), m->ss_size));
	sc_buf_put_fmt(buf, "%f", ((double) m->ss_max) / 1000000);

	val = (m->ss_count == 0 ? 1 : m->ss_count);
	sc_buf_put_fmt(buf, "%f", ((double) m->ss_total / val) / 1000000);
	sc_buf_put_str(buf, m->dir);

	sz = rs_dir_size(m->dir);
	sc_buf_put_fmt(buf, "%zu", sz);
	sc_buf_put_str(buf, sc_bytes_to_size(b, sizeof(b), (uint64_t) sz));

	sz = metric_free_disk(m->dir);
	sc_buf_put_fmt(buf, "%zu", sz);
	sc_buf_put_fmt(buf, "%s",
		       sc_bytes_to_size(b, sizeof(b), (uint64_t) sz));
}
