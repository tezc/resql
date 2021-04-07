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

#include "rs.h"

#include "file.h"

#include "sc/sc.h"
#include "sc/sc_log.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

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
	DIR *d;
	struct dirent *de;
	struct stat buf;
	int exists;
	size_t total_size = 0;
	char file_path[PATH_MAX];

	d = opendir(path);
	if (d == NULL) {
		return 0;
	}

	for (de = readdir(d); de != NULL; de = readdir(d)) {
		snprintf(file_path, PATH_MAX, "%s/%s", path, de->d_name);
		exists = stat(file_path, &buf);
		if (exists == 0) {
			total_size += buf.st_size;
		} else {
			printf("%s \n", strerror(errno));
		}
	}

	closedir(d);

	return total_size;
}

int rs_write_pid_file(char *path)
{
	int fd, rc;
	size_t len;
	ssize_t wr;
	char dir[PATH_MAX];
	char pidstr[32];

	struct stat sb;
	struct flock lock = {.l_type = F_WRLCK,
			     .l_start = 0,
			     .l_whence = SEEK_SET,
			     .l_len = 0};

	snprintf(dir, PATH_MAX, "%s/%s", path, ".pid");

	rc = stat(dir, &sb);
	if (rc == 0) {
		rc = unlink(dir);
		if (rc == -1) {
			sc_log_error("Failed to remove old PID file : '%s'",
				     dir);
			exit(EXIT_FAILURE);
		}
	}

	fd = open(dir, O_WRONLY | O_CREAT | O_CLOEXEC, 0444);
	if (fd < 0) {
		sc_log_error("Failed to create PID file '%s'", dir);
		exit(EXIT_FAILURE);
	}

	rc = fcntl(fd, F_SETLK, &lock);
	if (rc < 0) {
		close(fd);
		sc_log_error("Failed to set lock for PID file : '%s'", dir);
		exit(EXIT_FAILURE);
	}

	sprintf(pidstr, "%ld", (long) getpid());

	len = strlen(pidstr);
	wr = write(fd, pidstr, len);
	if (wr < 0 || (size_t) wr != len) {
		close(fd);
		sc_log_error("Failed to write PID number at :'%s'", dir);
		exit(EXIT_FAILURE);
	}

	close(fd);

	return RS_OK;
}

int rs_delete_pid_file(char *path)
{
	char dir[PATH_MAX];

	snprintf(dir, PATH_MAX, "%s/%s", path, ".pid");

	if (unlink(dir)) {
		sc_log_warn("Failed to delete PID file at '%s' \n", dir);
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

	sc_log_error("%s:%s:%d, msg : %s, errno : %d, error : %s \n", file,
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

	sc_log_error("%s \n", buf);
	exit(EXIT_FAILURE);
}

thread_local struct sc_rand tl_rc4;

void rs_rand_init()
{
	unsigned char buf[256];

	file_random(buf, sizeof(buf));
	sc_rand_init(&tl_rc4, buf);
}

unsigned int rs_rand()
{
	unsigned int val;

	sc_rand_read(&tl_rc4, &val, sizeof(val));

	return val;
}
