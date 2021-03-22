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
    sc_log_error("vsnprintf failed writing to : %p, max_len : %zu, fmt : %s",
                 (void *) buf, max_len, fmt);
    buf[0] = '\0';

    return 0;
}

void rs_vasprintf(char **buf, const char *fmt, va_list args)
{
    int rc;

    rc = vasprintf(buf, fmt, args);
    if (rc == -1) {
        rs_abort("vasprintf : %s \n", strerror(errno));
    }
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
    ssize_t write_len, written;
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
            sc_log_error("Failed to remove old PID file : '%s'", dir);
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

    write_len = strlen(pidstr);
    written = write(fd, pidstr, write_len);
    if (written != write_len) {
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
        rs_abort("Out of memory for size %lu \n", size);
    }

    return mem;
}

void *rs_calloc(size_t n, size_t size)
{
    void *mem;

    mem = calloc(n, size);
    if (mem == NULL && size != 0) {
        rs_abort("Out of memory for size %lu \n", size);
    }

    return mem;
}

void *rs_realloc(void *p, size_t size)
{
    void *mem;

    mem = realloc(p, size);
    if (mem == NULL && size != 0) {
        rs_abort("Out of memory for size %lu \n", size);
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

    sc_log_error("%s:%s:%d, msg : %s, errno : %d, errnostr : %s \n", file, func,
                 line, buf, errno, strerror(errno));
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

thread_local struct sc_rand rc4;

void rs_rand_init()
{
    unsigned char buf[256];

    file_random(buf, sizeof(buf));
    sc_rand_init(&rc4, buf);
}

unsigned int rs_rand()
{
    unsigned int val;

    sc_rand_read(&rc4, &val, sizeof(val));

    return val;
}
