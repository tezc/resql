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

#include "rs.h"

#include "file.h"
#include "state.h"

#include "sc/sc.h"
#include "sc/sc_crc32.h"
#include "sc/sc_log.h"
#include "sc/sc_signal.h"
#include "sc/sc_sock.h"
#include "sc/sc_time.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>

void rs_global_init()
{
	int rc;

	srand((int) sc_time_mono_ns());
	sc_crc32_init();

	rc = sc_log_init();
	if (rc != 0) {
		rs_exit("Failed to init log.");
	}

	rc = sc_signal_init();
	if (rc != 0) {
		rs_exit("signal_init : %s ", strerror(errno));
	}

	rc = sc_sock_startup();
	if (rc != 0) {
		rs_exit("sc_sock_startup : %s ", strerror(errno));
	}

	rc = state_global_init();
	if (rc != RS_OK) {
		rs_exit("state_global_init failure");
	}
}
void rs_global_shutdown()
{
	int rc;

	rc = sc_sock_cleanup();
	if (rc != 0) {
		rs_exit("sc_sock_cleanup : %s ", strerror(errno));
	}

	rc = state_global_shutdown();
	if (rc != RS_OK) {
		rs_exit("state_global_shutdown failure");
	}
}

int rs_snprintf(char *buf, size_t max_len, const char *fmt, ...)
{
	int rc;
	va_list list;

	va_start(list, fmt);
	rc = rs_vsnprintf(buf, max_len, fmt, list);
	va_end(list);

	return rc;
}

int rs_vsnprintf(char *buf, size_t max_len, const char *fmt, va_list list)
{
	int rc;

	rc = vsnprintf(buf, max_len, fmt, list);
	if (rc < 0 || (size_t) rc > max_len) {
		goto err;
	}

	return rc;

err:
	sc_log_error(
		"vsnprintf failed writing to : %p, max_len : %zu, fmt : %s",
		(void *) buf, max_len, fmt);
	buf[0] = '\0';

	return 0;
}

char *rs_strncpy(char *dest, const char *src, size_t max)
{
	char *ret;

	ret = strncpy(dest, src, max);
	dest[max - 1] = '\0';

	return ret;
}

size_t rs_dir_size(const char *path)
{
	int rc;
	const char *err;
	DIR *d;
	struct dirent *de;
	struct stat st;
	size_t total_size = 0;
	char file[PATH_MAX];

	d = opendir(path);
	if (d == NULL) {
		sc_log_error("dir : %s, opendir : %s", path, strerror(errno));
		return 0;
	}

	for (de = readdir(d); de != NULL; de = readdir(d)) {
		snprintf(file, PATH_MAX, "%s/%s", path, de->d_name);

		rc = stat(file, &st);
		if (rc != 0) {
			err = strerror(errno);
			sc_log_error("file : %s, stat : %s \n", file, err);
			continue;
		}

		total_size += st.st_size;
	}

	rc = closedir(d);
	if (rc != 0) {
		err = strerror(errno);
		sc_log_error("dir : %s, closedir : %s \n", path, err);
	}

	return total_size;
}

size_t rs_dir_free(const char *dir)
{
	int rc;
	struct statvfs st;

	rc = statvfs(dir, &st);
	if (rc != 0) {
		sc_log_error("dir : %s, statvfs : %s \n", dir, strerror(errno));
		return 0;
	}

	return ((size_t) (st.f_bavail) * st.f_bsize);
}

int rs_write_pid_file(char *path)
{
	int fd, rc, ret = RS_OK;
	size_t len;
	ssize_t wr;
	char file[PATH_MAX];
	char pidstr[32];

	struct stat sb;
	struct flock lock = {
		.l_type = F_WRLCK,
		.l_start = 0,
		.l_whence = SEEK_SET,
		.l_len = 0,
	};

	snprintf(file, PATH_MAX, "%s/%s", path, ".pid");

	rc = stat(file, &sb);
	if (rc == 0) {
		rc = unlink(file);
		if (rc == -1) {
			sc_log_error("unlink : %s, err : %s \n", file,
				     strerror(errno));
			return RS_ERROR;
		}
	}

	fd = open(file, O_WRONLY | O_CREAT | O_CLOEXEC, 0444);
	if (fd < 0) {
		sc_log_error("open : %s, err : %s \n", file, strerror(errno));
		return errno == ENOSPC ? RS_FULL : RS_ERROR;
	}

	rc = fcntl(fd, F_SETLK, &lock);
	if (rc < 0) {
		sc_log_error("fcntl : %s, err : %s \n", file, strerror(errno));
		ret = RS_ERROR;
		goto out;
	}

	sprintf(pidstr, "%ld", (long) getpid());

	len = strlen(pidstr);
	wr = write(fd, pidstr, len);
	if (wr < 0 || (size_t) wr != len) {
		sc_log_error("write : %s, err : %s \n", file, strerror(errno));
		ret = errno == ENOSPC ? RS_FULL : RS_ERROR;
	}
out:
	close(fd);
	return ret;
}

int rs_delete_pid_file(char *path)
{
	int rc;
	char file[PATH_MAX];

	snprintf(file, PATH_MAX, "%s/%s", path, ".pid");

	rc = unlink(file);
	if (rc != 0) {
		sc_log_warn("Failed to delete PID file at '%s' \n", file);
		return RS_ERROR;
	}

	return RS_OK;
}

void *rs_malloc(size_t size)
{
	void *mem;

	mem = malloc(size);
	if (mem == NULL && size != 0) {
		rs_abort("Out of memory for size %zu \n", size);
	}

	return mem;
}

void *rs_calloc(size_t n, size_t size)
{
	void *mem;

	mem = calloc(n, size);
	if (mem == NULL && size != 0) {
		rs_abort("Out of memory for size %zu \n", size);
	}

	return mem;
}

void *rs_realloc(void *p, size_t size)
{
	void *mem;

	mem = realloc(p, size);
	if (mem == NULL && size != 0) {
		rs_abort("Out of memory for size %zu \n", size);
	}

	return mem;
}

void rs_free(void *p)
{
	free(p);
}

_Noreturn void rs_on_abort(const char *file, const char *func, int line,
			   const char *fmt, ...)
{
	char buf[4096] = {'\0'};

	if (fmt != NULL) {
		va_list args;

		va_start(args, fmt);
		rs_vsnprintf(buf, sizeof(buf), fmt, args);
		va_end(args);
	}

	sc_log_error("%s:%s:%d, msg : %s [errno : %d, error : %s] \n", file,
		     func, line, buf, errno, strerror(errno));
	abort();
}

_Noreturn void rs_exit(const char *fmt, ...)
{
	char buf[4096] = {'\0'};

	if (fmt != NULL) {
		va_list args;

		va_start(args, fmt);
		rs_vsnprintf(buf, sizeof(buf), fmt, args);
		va_end(args);
	}

	sc_log_error("%s [errno : %d, error : %s] \n", buf, errno,
		     strerror(errno));
	exit(EXIT_FAILURE);
}

thread_local struct sc_rand tl_rc4;

int rs_rand_init()
{
	int rc;
	unsigned char buf[256];

	rc = rs_urandom(buf, sizeof(buf));
	if (rc != RS_OK) {
		return rc;
	}

	sc_rand_init(&tl_rc4, buf);

	return RS_OK;
}

unsigned int rs_rand()
{
	unsigned int val;

	sc_rand_read(&tl_rc4, &val, sizeof(val));

	return val;
}

int rs_urandom(void *buf, size_t size)
{
	int fd;
	ssize_t sz;

	memset(buf, 0, size);

	fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0) {
		sc_log_error("open /dev/urandom : %s \n", strerror(errno));
		return RS_ERROR;
	}

retry:
	sz = read(fd, buf, size);
	if (sz < 0 && errno == EINTR) {
		goto retry;
	}

	close(fd);

	if (sz < 0) {
		sc_log_error("read /dev/urandom :%s \n", strerror(errno));
		return RS_ERROR;
	}

	return RS_OK;
}
