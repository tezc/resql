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


#include "file.h"

#include "rs.h"

#include "sc/sc_log.h"
#include "sc/sc_str.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ftw.h>

struct file *file_create()
{
    struct file *file;

    file = rs_malloc(sizeof(*file));
    file_init(file);

    return file;
}

void file_destroy(struct file *file)
{
    file_term(file);
    rs_free(file);
}

void file_init(struct file *file)
{
    file->path = NULL;
    file->fp = NULL;
}

void file_term(struct file *file)
{
    file_close(file);
    sc_str_destroy(file->path);
}

int file_open(struct file *file, const char *path, const char *mode)
{
    FILE *fp;

    fp = fopen(path, mode);
    if (fp == NULL) {
        return RS_ERROR;
    }

    file->fp = fp;
    sc_str_set(&file->path, path);

    return RS_OK;
}

int file_close(struct file *file)
{
    int rc;
    FILE *fp;

    fp = file->fp;
    if (fp != NULL) {
        file->fp = NULL;

        rc = fclose(fp);
        if (rc != 0) {
            return RS_ERROR;
        }
    }

    return RS_OK;
}

ssize_t file_size(struct file *file)
{
    return file_size_at(file->path);
}

int64_t file_size_at(const char *path)
{
    int rc;
    struct stat st;

    rc = stat(path, &st);
    if (rc != 0) {
        return rc;
    }

    return st.st_size;
}

int file_remove(struct file *file)
{
    return file_remove_path(file->path);
}

int file_flush(struct file *file)
{
    int rc;

    rc = fflush(file->fp);
    if (rc != 0) {
        sc_log_error("Failed to flush file at %s \n", file_path(file));
    }

    return rc;
}

int file_write(struct file *file, const void *ptr, size_t size)
{
    size_t wr;

    wr = fwrite(ptr, 1, size, file->fp);
    if (wr != size) {
        sc_log_error("Failed to write %zu bytes, written : %zu  \n", size, wr);
        return RS_ERROR;
    }

    return RS_OK;
}

int file_write_at(struct file *file, size_t off, const void *ptr, size_t size)
{
    int rc;

    rc = file_lseek(file, off);
    if (rc != 0) {
        return RS_ERROR;
    }

    return file_write(file, ptr, size);
}

int file_read(struct file *file, void *ptr, size_t size)
{
    size_t read;

    read = fread(ptr, 1, size, file->fp);
    if (read != size) {
        sc_log_error("Failed to read %zu bytes, written : % lu  \n", size,
                     read);
        return RS_ERROR;
    }

    return RS_OK;
}

int file_lseek(struct file *file, size_t offset)
{
    return fseek(file->fp, offset, SEEK_SET);
}

const char *file_path(struct file *file)
{
    return file->path;
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
                return RS_ERROR;
            }

            *p = '/';
        }
    }

    rc = mkdir(buf, S_IRWXU);
    if (rc != 0 && errno != EEXIST) {
        return RS_ERROR;
    }

    return RS_OK;
}

static int file_rm(const char *path, const struct stat *s, int t, struct FTW *b)
{
    (void) s;
    (void) t;
    (void) b;

    return remove(path);
}

int file_rmdir(const char* path)
{
    return nftw(path, file_rm, 64, FTW_DEPTH | FTW_PHYS);
}

int file_clear_dir(const char *path, const char *pattern)
{
    int rc;
    int ret = RS_OK;
    char buf[PATH_MAX];
    DIR *dir;
    struct dirent *next_file;

    dir = opendir(path);
    if (dir == NULL) {
        sc_log_error("Open directory at : %s (%s) \n", path, strerror(errno));
        return RS_ERROR;
    }

    while ((next_file = readdir(dir)) != NULL) {
        if (strstr(next_file->d_name, pattern)) {
            rs_snprintf(buf, PATH_MAX, "%s/%s", path, next_file->d_name);

            rc = remove(buf);
            if (rc != 0) {
                sc_log_error("Remove file at : %s, (%s) \n", buf,
                             strerror(errno));
                ret = RS_ERROR;
                goto clean;
            }
        }
    }

clean:
    rc = closedir(dir);
    if (rc != 0) {
        sc_log_error("Close dir at : %s, err : %s \n", path, strerror(errno));
        return RS_ERROR;
    }

    return ret;
}

int file_remove_path(const char *path)
{
    int rc;

    rc = remove(path);
    if (rc != 0) {
        return RS_ERROR;
    }

    return RS_OK;
}

bool file_exists_at(const char *path)
{
    return access(path, F_OK) != -1;
}

int file_remove_if_exists(const char *path)
{
    bool exists;

    exists = file_exists_at(path);
    if (exists) {
        return file_remove_path(path);
    }

    return RS_OK;
}

char *file_full_path(const char *path, char *resolved)
{
    return realpath(path, resolved);
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
                } else {
                    rc = RS_ERROR;
                    goto cleanup_dest;
                }
            }

            n_read -= n_written;
            out += n_written;

        } while (n_read > 0);
    }

cleanup_dest:
    close(fd_dest);
cleanup_src:
    close(fd_src);

    return rc;
}

void file_random(void *buf, size_t size)
{
    int fd;
    ssize_t sz;

    memset(buf, 0, size);

    fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        sc_log_error("Failed to open /dev/urandom :%s \n", strerror(errno));
        return;
    }

retry:
    sz = read(fd, buf, size);
    if (sz < 0 && errno == EINTR) {
        goto retry;
    }

    close(fd);
}
