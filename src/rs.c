/*
 *  Resql
 *
 *  Copyright (C) 2021 Resql Authors
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
#include "rs.h"

#include "sc/sc.h"
#include "sc/sc_log.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

thread_local char rs_err_str[128];
thread_local int rs_err;

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

int rs_set_daemon()
{
    pid_t pid;

    if ((pid = fork()) < 0) {
        sc_log_error("Daemon : Fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    umask(0);
    setsid();

    if (chdir("/") < 0) {
        sc_log_error("Daemon : chdir failed");
        exit(EXIT_FAILURE);
    }

    fclose(stderr);
    fclose(stdout);

    return RS_OK;
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

#include <netinet/ip.h>
#include <sys/un.h>

union sockaddr_union
{
    struct sockaddr sa;
    struct sockaddr_in in4;
    struct sockaddr_in6 in6;
    struct sockaddr_un un;
    struct sockaddr_storage storage;
};

int rs_systemd_notify(const char *msg)
{
    int fd, rc = 0;
    const char *s;
    union sockaddr_union addr = {.sa.sa_family = AF_UNIX};
    struct iovec iovec = {.iov_base = (char *) msg, .iov_len = strlen(msg)};
    struct msghdr msghdr = {.msg_name = &addr,
                            .msg_iov = &iovec,
                            .msg_iovlen = 1};
    if (!msg) {
        errno = EINVAL;
        return -1;
    }

    s = getenv("NOTIFY_SOCKET");
    if (!s) {
        errno = EINVAL;
        sc_log_error("systemd : NOTIFY_SOCKET variable is missing. \n");
        return -1;
    }

    if ((s[0] != '@' && s[0] != '/') || s[1] == '\0') {
        errno = ENOENT;
        sc_log_error("systemd : NOTIFY_SOCKET path is invalid %s \n", s);
        return -1;
    }

    fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (fd < 0) {
        sc_log_error("systemd : Failed to create socket. \n");
        return -1;
    }

    strncpy(addr.un.sun_path, s, sizeof(addr.un.sun_path) - 1);
    if (addr.un.sun_path[0] == '@') {
        addr.un.sun_path[0] = '\0';
    }

    msghdr.msg_namelen = offsetof(struct sockaddr_un, sun_path) + strlen(s);
    if (msghdr.msg_namelen > sizeof(struct sockaddr_un)) {
        msghdr.msg_namelen = sizeof(struct sockaddr_un);
    }

    rc = sendmsg(fd, &msghdr, MSG_NOSIGNAL);
    if (rc < 0) {
        sc_log_error("systemd : Failed to send msg : %s. \n", strerror(errno));
        rc = -1;
    }

    close(fd);

    return rc > 0 ? 0 : -1;
}
