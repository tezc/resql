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

#ifndef __FILE_OFFSET_BITS
#define __FILE_OFFSET_BITS 64
#endif

#include "file.h"

#include "rs.h"

#include "sc/sc_log.h"
#include "sc/sc_str.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <ftw.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

struct file *file_create()
{
	struct file *f;

	f = rs_malloc(sizeof(*f));
	file_init(f);

	return f;
}

void file_destroy(struct file *f)
{
	file_term(f);
	rs_free(f);
}

void file_init(struct file *f)
{
	f->path = NULL;
	f->fp = NULL;
}

int file_term(struct file *f)
{
	int rc;

	rc = file_close(f);
	sc_str_destroy(&f->path);

	return rc;
}

int file_open(struct file *f, const char *path, const char *mode)
{
	FILE *fp;

	fp = fopen(path, mode);
	if (fp == NULL) {
		goto err;
	}

	f->fp = fp;
	sc_str_set(&f->path, path);

	return RS_OK;

err:
	sc_log_error("file : %s, fopen : %s \n", path, strerror(errno));
	return errno == ENOSPC ? RS_FULL : RS_ERROR;
}

int file_close(struct file *f)
{
	int rc;
	const char *err;
	FILE *fp;

	if (f->fp == NULL) {
		return RS_OK;
	}

	fp = f->fp;
	f->fp = NULL;

	rc = fclose(fp);
	if (rc != 0) {
		err = strerror(errno);
		sc_log_error("file : %s, fclose : %s \n", f->path, err);

		return RS_ERROR;
	}

	return RS_OK;
}

ssize_t file_size(struct file *f)
{
	return file_size_at(f->path);
}

ssize_t file_size_at(const char *path)
{
	int rc;
	struct stat st;

	rc = stat(path, &st);
	if (rc != 0) {
		sc_log_warn("file : %s, stat : %s \n", path, strerror(errno));
		return -1;
	}

	return st.st_size;
}

int file_remove(struct file *f)
{
	return file_remove_path(f->path);
}

int file_flush(struct file *f)
{
	int rc;
	const char *err;

	rc = fflush(f->fp);
	if (rc != 0) {
		goto err;
	}

	rc = fsync(fileno(f->fp));
	if (rc != 0) {
		goto err;
	}

	return RS_OK;

err:
	err = strerror(errno);
	sc_log_error("file : %s, flush : %s \n", f->path, err);

	return errno == ENOSPC ? RS_FULL : RS_ERROR;
}

int file_write(struct file *f, const void *ptr, size_t size)
{
	size_t wr;
	const char *err;

	wr = fwrite(ptr, 1, size, f->fp);
	if (wr != size) {
		err = strerror(errno);
		sc_log_error("file : %s, write : %s  \n", f->path, err);

		return errno == ENOSPC ? RS_FULL : RS_ERROR;
	}

	return RS_OK;
}

int file_write_at(struct file *f, size_t off, const void *ptr, size_t size)
{
	int rc;

	rc = file_lseek(f, off);
	if (rc != 0) {
		sc_log_error("file : %zu, file : %s, err : %s \n", off, f->path,
			     strerror(errno));
		return RS_ERROR;
	}

	return file_write(f, ptr, size);
}

int file_read(struct file *f, void *ptr, size_t size)
{
	size_t read;
	const char *err;

	read = fread(ptr, 1, size, f->fp);
	if (read != size) {
		err = strerror(errno);
		sc_log_error("file : %s, read : %s \n", f->path, err);

		return RS_ERROR;
	}

	return RS_OK;
}

int file_lseek(struct file *f, size_t offset)
{
	int rc;
	const char *err;

	rc = fseek(f->fp, offset, SEEK_SET);
	if (rc != 0) {
		err = strerror(errno);
		sc_log_error("file : %s, fseek : %s \n", f->path, err);

		return RS_ERROR;
	}

	return RS_OK;
}

const char *file_path(struct file *f)
{
	return f->path;
}

int file_mkdir(const char *path)
{
	int rc;
	char buf[PATH_MAX];

	strcpy(buf, path);

	for (char *p = buf + 1; *p; p++) {
		if (*p == '/') {
			*p = '\0';

			rc = mkdir(buf, S_IRWXU);
			if (rc != 0 && errno != EEXIST) {
				goto err;
			}

			*p = '/';
		}
	}

	rc = mkdir(buf, S_IRWXU);
	if (rc != 0 && errno != EEXIST) {
		goto err;
	}

	return RS_OK;

err:
	sc_log_error("file : %s, mkdir : %s \n", buf, strerror(errno));
	return RS_ERROR;
}

static int file_rm(const char *path, const struct stat *s, int t, struct FTW *b)
{
	(void) s;
	(void) t;
	(void) b;

	return remove(path);
}

int file_rmdir(const char *path)
{
	return nftw(path, file_rm, 64, FTW_DEPTH | FTW_PHYS);
}

int file_clear_dir(const char *path, const char *pattern)
{
	int rc;
	int ret = RS_OK;
	char buf[PATH_MAX];
	const char *err;
	DIR *dir;
	struct dirent *next;

	dir = opendir(path);
	if (dir == NULL) {
		err = strerror(errno);
		sc_log_error("file : %s, opendir : (%s) \n", path, err);
		return RS_ERROR;
	}

	while ((next = readdir(dir)) != NULL) {
		if (strstr(next->d_name, pattern)) {
			rs_snprintf(buf, PATH_MAX, "%s/%s", path, next->d_name);

			rc = remove(buf);
			if (rc != 0) {
				sc_log_error("file : %s, remove :%s \n", buf,
					     strerror(errno));
				ret = RS_ERROR;
				goto clean;
			}
		}
	}

clean:
	rc = closedir(dir);
	if (rc != 0) {
		err = strerror(errno);
		sc_log_error("file : %s, closedir : %s \n", path, err);

		return RS_ERROR;
	}

	return ret;
}

int file_remove_path(const char *path)
{
	int rc;
	const char *err;

	rc = remove(path);
	if (rc != 0 && errno != ENOENT) {
		err = strerror(errno);
		sc_log_error("file : %s, remove : %s \n", path, err);

		return RS_ERROR;
	}

	return RS_OK;
}

int file_unlink(const char *path)
{
	int rc;
	const char *err;

	rc = unlink(path);
	if (rc != 0) {
		err = strerror(errno);
		sc_log_error("file : %s, unlink : %s \n", path, err);
		return RS_ERROR;
	}

	return RS_OK;
}

bool file_exists_at(const char *path)
{
	return access(path, F_OK) != -1;
}

int file_copy(const char *dst, const char *src)
{
	int rc = RS_OK, fd_src, fd_dest;
	ssize_t n_read, n_written;
	char *out;
	char buf[4096 * 8];

	fd_src = open(src, O_RDONLY);
	if (fd_src < 0) {
		rc = RS_ERROR;
		goto cleanup_src;
	}

	fd_dest = open(dst, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (fd_dest < 0) {
		rc = RS_ERROR;
		goto cleanup_dest;
	}

	while (true) {
		n_read = read(fd_src, buf, sizeof(buf));
		if (n_read == 0) {
			break;
		}
		if (n_read < 0) {
			rc = RS_ERROR;
			goto cleanup_dest;
		}

		out = buf;

		do {
			n_written = write(fd_dest, out, (size_t) n_read);
			if (n_written < 0) {
				if (errno == EINTR) {
					continue;
				}

				rc = errno == ENOSPC ? RS_FULL : RS_ERROR;
				goto cleanup_dest;
			}

			n_read -= n_written;
			out += n_written;

		} while (n_read > 0);
	}

cleanup_dest:
	close(fd_dest);
cleanup_src:
	close(fd_src);

	if (rc != RS_OK) {
		remove(dst);
	}

	return rc;
}

int file_rename(const char *dst, const char *src)
{
	int rc;

	rc = rename(src, dst);
	if (rc != 0) {
		sc_log_error("rename : %s to %s : %s \n", src, dst,
			     strerror(errno));
		return errno == ENOSPC ? RS_FULL : RS_ERROR;
	}

	return RS_OK;
}

int file_fsync(const char *path)
{
	int fd, rc, ret = RS_OK;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		sc_log_error("open : %s, err : %s \n", path, strerror(errno));
		return RS_ERROR;
	}

	rc = fsync(fd);
	if (rc != 0) {
		sc_log_error("fsync : %s, err : %s \n", path, strerror(errno));
		ret = RS_ERROR;
	}

	close(fd);

	return ret;
}
