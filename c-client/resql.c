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

#ifndef _XOPEN_SOURCE
    #define _XOPEN_SOURCE 700
#endif

#if defined(_WIN32) || defined(_WIN64)
    #include <Ws2tcpip.h>
    #include <windows.h>
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
    #define rs_poll WSAPoll

typedef SOCKET sc_sock_int;

#else
    #include <sys/socket.h>
typedef int sc_sock_int;
    #define rs_poll poll
#endif

#include "resql.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
    #pragma warning(disable : 4996)
#else
    #include <poll.h>
#endif


#if defined(_WIN32) || defined(_WIN64)
    #include <assert.h>
    #include <windows.h>

#else
    #include <assert.h>
    #include <stdlib.h>
    #include <sys/time.h>
    #include <time.h>
#endif

static uint64_t sc_time_mono_ms()
{
#if defined(_WIN32) || defined(_WIN64)
    //  System frequency does not change at run-time, cache it
    static int64_t frequency = 0;
    if (frequency == 0) {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        assert(freq.QuadPart != 0);
        frequency = freq.QuadPart;
    }
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return (int64_t)(count.QuadPart * 1000) / frequency;
#else
    int rc;
    struct timespec ts;

    rc = clock_gettime(CLOCK_MONOTONIC, &ts);
    assert(rc == 0);
    (void) rc;

    return (uint64_t)((uint64_t) ts.tv_sec * 1000 +
                      (uint64_t) ts.tv_nsec / 1000000);
#endif
}

static int sc_time_sleep(uint64_t millis)
{
#if defined(_WIN32) || defined(_WIN64)
    Sleep((DWORD) millis);
    return 0;
#else
    int rc;
    struct timespec t, rem;

    rem.tv_sec = millis / 1000;
    rem.tv_nsec = (millis % 1000) * 1000000;

    do {
        t = rem;
        rc = nanosleep(&t, &rem);
    } while (rc != 0 && errno == EINTR);

    return rc;
#endif
}

struct sc_uri
{
    const char *str;
    const char *scheme;
    const char *host;
    const char *userinfo;
    const char *port;
    const char *path;
    const char *query;
    const char *fragment;

    char buf[];
};

static struct sc_uri *sc_uri_create(const char *str)
{
    const char *s1 = "%.*s%.*s%.*s%.*s%.*s%.*s%.*s%.*s";
    const char *s2 = "%.*s%c%.*s%c%.*s%c%.*s%c%.*s%c%.*s%c%.*s%c";
    const char *authority = "//";

    int diff, ret;
    unsigned long val;
    size_t full_len, parts_len;
    size_t scheme_len, authority_len = 0, userinfo_len = 0;
    size_t host_len = 0, port_len = 0, path_len;
    size_t query_len = 0, fragment_len = 0;

    char *scheme, *userinfo = "", *host = "", *port = "";
    char *path, *query = "", *fragment = "";
    char *ptr, *dest, *parse_end;
    char *pos = (char *) str;
    struct sc_uri *uri;

    if (str == NULL || (ptr = strstr(pos, ":")) == NULL) {
        return NULL;
    }

    scheme = pos;
    scheme_len = ptr - str + 1;
    pos = ptr + 1;

    if (*pos == '/' && *(pos + 1) == '/') {
        authority_len = 2;
        pos += authority_len;

        ptr = strchr(pos, '@');
        if (ptr != NULL) {
            userinfo = pos;
            userinfo_len = ptr - pos + strlen("@");
            pos = ptr + 1;
        }

        ptr = pos + strcspn(pos, *pos == '[' ? "]" : ":/?#");
        host = pos;
        host_len = ptr - pos + (*host == '[');
        pos = host + host_len;

        if (*host == '[' && *(host + host_len - 1) != ']') {
            return NULL;
        }

        ptr = strchr(pos, ':');
        if (ptr != NULL) {
            if (*(ptr + 1) == '\0') {
                return NULL;
            }

            errno = 0;
            val = strtoul(ptr + 1, &parse_end, 10);
            if (errno != 0 || val > 65536) {
                return NULL;
            }

            port = ptr;
            port_len = parse_end - ptr;
            pos = port + port_len;
        }
    }

    path = pos;
    path_len = strcspn(path, "?#");
    pos = path + path_len;

    ptr = strchr(pos, '?');
    if (ptr != NULL) {
        query = ptr;
        query_len = strcspn(query, "#");
        pos = query + query_len;
    }

    if (*pos == '#') {
        fragment = pos;
        fragment_len = strlen(pos);
    }

    full_len = scheme_len + authority_len + userinfo_len + host_len + port_len +
               path_len + query_len + fragment_len + 1;

    parts_len = full_len - authority_len;
    parts_len += 7; // NULL characters for each part.
    parts_len -= (scheme_len != 0);
    parts_len -= (userinfo_len != 0);
    parts_len -= (port_len != 0);
    parts_len -= (query_len != 0);
    parts_len -= (fragment_len != 0);

    uri = resql_malloc(sizeof(*uri) + parts_len + full_len);
    if (uri == NULL) {
        return NULL;
    }

    ret = snprintf(uri->buf, full_len, s1, scheme_len, scheme, authority_len,
                   authority, userinfo_len, userinfo, host_len, host, port_len,
                   port, path_len, path, query_len, query, fragment_len,
                   fragment);
    if (ret < 0 || (size_t) ret != full_len - 1) {
        goto error;
    }

    dest = uri->buf + strlen(uri->buf) + 1;

    scheme_len -= (scheme_len != 0);     // Skip ":"
    userinfo_len -= (userinfo_len != 0); // Skip "@"
    diff = port_len != 0;
    port_len -= diff; // Skip ":"
    port += diff;     // Skip ":"
    diff = (query_len != 0);
    query_len -= diff; // Skip "?"
    query += diff;     // Skip "?"
    diff = (fragment_len != 0);
    fragment_len -= diff; // Skip "#"
    fragment += diff;     // Skip "#"

    ret = sprintf(dest, s2, scheme_len, scheme, 0, userinfo_len, userinfo, 0,
                  host_len, host, 0, port_len, port, 0, path_len, path, 0,
                  query_len, query, 0, fragment_len, fragment, 0);
    if (ret < 0 || (size_t) ret != parts_len - 1) {
        goto error;
    }

    uri->str = uri->buf;
    uri->scheme = dest;
    uri->userinfo = dest + scheme_len + 1;
    uri->host = uri->userinfo + userinfo_len + 1;
    uri->port = uri->host + host_len + 1;
    uri->path = uri->port + port_len + 1;
    uri->query = uri->path + path_len + 1;
    uri->fragment = uri->query + query_len + 1;

    return uri;

error:
    resql_free(uri);
    return NULL;
}

static void sc_uri_destroy(struct sc_uri *uri)
{
    resql_free(uri);
}

struct sc_str
{
    uint32_t len;
    char buf[];
};

#define sc_str_meta(str)                                                       \
    ((struct sc_str *) ((char *) (str) -offsetof(struct sc_str, buf)))

#define sc_str_bytes(n) ((n) + sizeof(struct sc_str) + 1)
#ifndef SC_SIZE_MAX
    #define SC_SIZE_MAX (UINT32_MAX - sizeof(struct sc_str) - 1)
#endif

static char *sc_str_create_len(const char *str, uint32_t len)
{
    struct sc_str *copy;

    if (str == NULL) {
        return NULL;
    }

    copy = resql_malloc(sc_str_bytes(len));
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy->buf, str, len);
    copy->buf[len] = '\0';
    copy->len = len;

    return copy->buf;
}

static char *sc_str_create(const char *str)
{
    size_t size;

    if (str == NULL || (size = strlen(str)) > SC_SIZE_MAX) {
        return NULL;
    }

    return sc_str_create_len(str, (uint32_t) size);
}

static void sc_str_destroy(char *str)
{
    if (str == NULL) {
        return;
    }

    resql_free(sc_str_meta(str));
}

static void swap(char *str, char *d)
{
    char tmp;
    char *c = str + sc_str_meta(str)->len;

    tmp = *c;
    *c = *d;
    *d = tmp;
}

static const char *sc_str_token_begin(char *str, char **save, const char *delim)
{
    char *it = str;

    if (str == NULL) {
        return NULL;
    }

    if (*save != NULL) {
        it = *save;
        swap(str, it);
        if (*it == '\0') {
            return NULL;
        }
        it++;
    }

    *save = it + strcspn(it, delim);
    swap(str, *save);

    return it;
}

static void sc_str_token_end(char *str, char **save)
{
    char *end;

    if (str == NULL) {
        return;
    }

    end = str + sc_str_meta(str)->len;
    if (*end == '\0') {
        return;
    }

    swap(str, *save);
}

#define SC_BUF_CORRUPT 1u
#define SC_BUF_OOM     3u

#define SC_BUF_REF  8
#define SC_BUF_DATA 16
#define SC_BUF_READ (SC_BUF_REF | SC_BUF_DATA)

struct sc_buf
{
    unsigned char *mem;
    uint32_t cap;
    uint32_t limit;
    uint32_t rpos;
    uint32_t wpos;

    unsigned int error;
    bool ref;
};

// clang-format off
static inline uint32_t sc_buf_8_len(uint8_t val) { (void)val; return 1; }
static inline uint32_t sc_buf_32_len(uint32_t val) { (void)val; return 4; }
// clang-format on

static inline uint32_t sc_buf_str_len(const char *str)
{
    const uint32_t bytes = sc_buf_32_len(UINT32_MAX) + sc_buf_8_len('\0');

    if (str == NULL) {
        return sc_buf_32_len(UINT32_MAX);
    }

    assert(strlen(str) <= INT32_MAX - 5);

    return bytes + (uint32_t) strlen(str);
}

static struct sc_buf sc_buf_wrap(void *data, uint32_t len, int flags)
{
    struct sc_buf buf = {.mem = data,
                         .cap = len,
                         .limit = flags & SC_BUF_REF ? len : UINT32_MAX,
                         .wpos = flags & SC_BUF_DATA ? len : 0,
                         .rpos = 0,
                         .ref = (bool) (flags & SC_BUF_REF),
                         .error = 0};

    return buf;
}

static bool sc_buf_init(struct sc_buf *buf, uint32_t cap)
{
    void *mem = NULL;

    *buf = (struct sc_buf){0};

    if (cap > 0) {
        mem = resql_malloc(cap);
        if (mem == NULL) {
            return false;
        }
    }

    *buf = sc_buf_wrap(mem, cap, 0);

    return true;
}

static void sc_buf_term(struct sc_buf *buf)
{
    if (!buf->ref) {
        resql_free(buf->mem);
    }
}

static void sc_buf_compact(struct sc_buf *buf)
{
    uint32_t copy;

    if (buf->rpos == buf->wpos) {
        buf->rpos = 0;
        buf->wpos = 0;
    }

    if (buf->rpos != 0) {
        copy = buf->wpos - buf->rpos;
        memmove(buf->mem, buf->mem + buf->rpos, copy);
        buf->rpos = 0;
        buf->wpos = copy;
    }
}

static bool sc_buf_reserve(struct sc_buf *buf, uint32_t len)
{
    uint32_t size;
    void *tmp;

    if (buf->wpos + len > buf->cap) {
        if (buf->ref) {
            return false;
        }

        sc_buf_compact(buf);

        if (buf->wpos + len > buf->cap) {
            size = ((buf->cap + len + 4095) / 4096) * 4096;
            if (size > buf->limit || buf->cap >= UINT32_MAX - 4096) {
                buf->error |= SC_BUF_OOM;
                return false;
            }

            tmp = resql_realloc(buf->mem, size);
            if (tmp == NULL) {
                buf->error |= SC_BUF_OOM;
                return false;
            }

            buf->cap = size;
            buf->mem = tmp;
        }
    }

    return true;
}

static bool sc_buf_valid(struct sc_buf *buf)
{
    return buf->error == 0;
}

static uint32_t sc_buf_quota(struct sc_buf *buf)
{
    return buf->cap - buf->wpos;
}

static uint32_t sc_buf_size(struct sc_buf *buf)
{
    return buf->wpos - buf->rpos;
}

static void sc_buf_clear(struct sc_buf *buf)
{
    buf->rpos = 0;
    buf->wpos = 0;
    buf->error = 0;
}

static void sc_buf_mark_read(struct sc_buf *buf, uint32_t len)
{
    buf->rpos += len;
}

static void sc_buf_mark_write(struct sc_buf *buf, uint32_t len)
{
    buf->wpos += len;
}

static uint32_t sc_buf_rpos(struct sc_buf *buf)
{
    return buf->rpos;
}

static void sc_buf_set_rpos(struct sc_buf *buf, uint32_t pos)
{
    assert(buf->wpos >= pos);
    buf->rpos = pos;
}

static void sc_buf_set_wpos(struct sc_buf *buf, uint32_t pos)
{
    assert(pos <= buf->cap);
    buf->wpos = pos;
}

static uint32_t sc_buf_wpos(struct sc_buf *buf)
{
    return buf->wpos;
}

static void *sc_buf_rbuf(struct sc_buf *buf)
{
    return buf->mem + buf->rpos;
}

static void *sc_buf_wbuf(struct sc_buf *buf)
{
    return buf->mem + buf->wpos;
}

static uint32_t sc_buf_set_data(struct sc_buf *buf, uint32_t offset,
                                const void *src, uint32_t len)
{
    if (buf->error != 0 || (offset + len > buf->cap)) {
        buf->error |= SC_BUF_CORRUPT;
        return 0;
    }

    memcpy(&buf->mem[offset], src, len);
    return len;
}

static void sc_buf_put_raw(struct sc_buf *buf, const void *ptr, uint32_t len)
{
    if (!sc_buf_reserve(buf, len)) {
        return;
    }

    buf->wpos += sc_buf_set_data(buf, buf->wpos, ptr, len);
}

static uint16_t sc_buf_peek_8_pos(struct sc_buf *buf, uint32_t pos,
                                  uint8_t *val)
{
    if (pos + sizeof(*val) > buf->wpos) {
        buf->error |= SC_BUF_CORRUPT;
        *val = 0;
        return 0;
    }

    *val = buf->mem[pos];

    return sizeof(*val);
}

static uint32_t sc_buf_peek_32_pos(struct sc_buf *buf, uint32_t pos,
                                   uint32_t *val)
{
    unsigned char *p;

    if (pos + sizeof(*val) > buf->wpos) {
        buf->error |= SC_BUF_CORRUPT;
        *val = 0;
        return 0;
    }

    p = &buf->mem[pos];

    *val = (uint32_t) p[0] << 0 | (uint32_t) p[1] << 8 | (uint32_t) p[2] << 16 |
           (uint32_t) p[3] << 24;

    return sizeof(*val);
}

static uint32_t sc_buf_peek_64_pos(struct sc_buf *buf, uint32_t pos,
                                   uint64_t *val)
{
    unsigned char *p;

    if (pos + sizeof(*val) > buf->wpos) {
        buf->error |= SC_BUF_CORRUPT;
        *val = 0;
        return 0;
    }

    p = &buf->mem[pos];

    *val = (uint64_t) p[0] << 0 | (uint64_t) p[1] << 8 | (uint64_t) p[2] << 16 |
           (uint64_t) p[3] << 24 | (uint64_t) p[4] << 32 |
           (uint64_t) p[5] << 40 | (uint64_t) p[6] << 48 |
           (uint64_t) p[7] << 56;

    return sizeof(*val);
}

static uint32_t sc_buf_set_8_pos(struct sc_buf *buf, uint32_t pos,
                                 const uint8_t *val)
{
    if (buf->error != 0 || (pos + sizeof(*val) > buf->cap)) {
        buf->error |= SC_BUF_CORRUPT;
        return 0;
    }

    buf->mem[pos] = *val;

    return sizeof(*val);
}

static uint32_t sc_buf_set_32_pos(struct sc_buf *buf, uint32_t pos,
                                  const uint32_t *val)
{
    unsigned char *p;

    if (buf->error != 0 || (pos + sizeof(*val) > buf->cap)) {
        buf->error |= SC_BUF_CORRUPT;
        return 0;
    }

    p = &buf->mem[pos];

    p[0] = (unsigned char) (*val >> 0);
    p[1] = (unsigned char) (*val >> 8);
    p[2] = (unsigned char) (*val >> 16);
    p[3] = (unsigned char) (*val >> 24);

    return sizeof(*val);
}

static uint32_t sc_buf_set_64_pos(struct sc_buf *buf, uint32_t pos,
                                  const uint64_t *val)
{
    unsigned char *p;

    if (buf->error != 0 || (pos + sizeof(*val) > buf->cap)) {
        buf->error |= SC_BUF_CORRUPT;
        return 0;
    }

    p = &buf->mem[pos];

    p[0] = (unsigned char) (*val >> 0);
    p[1] = (unsigned char) (*val >> 8);
    p[2] = (unsigned char) (*val >> 16);
    p[3] = (unsigned char) (*val >> 24);
    p[4] = (unsigned char) (*val >> 32);
    p[5] = (unsigned char) (*val >> 40);
    p[6] = (unsigned char) (*val >> 48);
    p[7] = (unsigned char) (*val >> 56);

    return sizeof(*val);
}

static uint32_t sc_buf_peek_32_at(struct sc_buf *buf, uint32_t pos)
{
    uint32_t val;

    sc_buf_peek_32_pos(buf, pos, &val);
    return val;
}


static uint32_t sc_buf_peek_32(struct sc_buf *buf)
{
    return sc_buf_peek_32_at(buf, buf->rpos);
}

static void sc_buf_set_8_at(struct sc_buf *buf, uint32_t pos, uint8_t val)
{
    sc_buf_set_8_pos(buf, pos, &val);
}

static void sc_buf_set_32_at(struct sc_buf *buf, uint32_t pos, uint32_t val)
{
    sc_buf_set_32_pos(buf, pos, &val);
}

static void sc_buf_set_64_at(struct sc_buf *buf, uint32_t pos, uint64_t val)
{
    sc_buf_set_64_pos(buf, pos, &val);
}

static uint8_t sc_buf_get_8(struct sc_buf *buf)
{
    uint8_t val;

    buf->rpos += sc_buf_peek_8_pos(buf, buf->rpos, &val);

    return val;
}

static uint32_t sc_buf_get_32(struct sc_buf *buf)
{
    uint32_t val;

    buf->rpos += sc_buf_peek_32_pos(buf, buf->rpos, &val);

    return val;
}

static uint64_t sc_buf_get_64(struct sc_buf *buf)
{
    uint64_t val;

    buf->rpos += sc_buf_peek_64_pos(buf, buf->rpos, &val);

    return val;
}

static double sc_buf_get_double(struct sc_buf *buf)
{
    double d;
    uint64_t val;

    val = sc_buf_get_64(buf);
    memcpy(&d, &val, 8);

    return d;
}

static const char *sc_buf_get_str(struct sc_buf *buf)
{
    int len;
    const char *str;

    len = sc_buf_get_32(buf);
    if (len == -1) {
        return NULL;
    }

    str = (char *) buf->mem + buf->rpos;
    buf->rpos += len + 1;

    return str;
}

static void *sc_buf_get_blob(struct sc_buf *buf, uint32_t len)
{
    void *blob;

    if (len == 0) {
        return NULL;
    }

    if (buf->rpos + len > buf->wpos) {
        buf->error |= SC_BUF_CORRUPT;
        return NULL;
    }

    blob = buf->mem + buf->rpos;
    buf->rpos += len;

    return blob;
}

static void sc_buf_put_8(struct sc_buf *buf, uint8_t val)
{
    if (!sc_buf_reserve(buf, sizeof(val))) {
        return;
    }

    buf->wpos += sc_buf_set_8_pos(buf, buf->wpos, &val);
}

static void sc_buf_put_32(struct sc_buf *buf, uint32_t val)
{
    if (!sc_buf_reserve(buf, sizeof(val))) {
        return;
    }

    buf->wpos += sc_buf_set_32_pos(buf, buf->wpos, &val);
}

static void sc_buf_put_64(struct sc_buf *buf, uint64_t val)
{
    if (!sc_buf_reserve(buf, sizeof(val))) {
        return;
    }

    buf->wpos += sc_buf_set_64_pos(buf, buf->wpos, &val);
}

static void sc_buf_put_double(struct sc_buf *buf, double val)
{
    uint64_t sw;

    memcpy(&sw, &val, 8);
    sc_buf_put_64(buf, sw);
}

static void sc_buf_put_str(struct sc_buf *buf, const char *str)
{
    size_t size;

    if (str == NULL) {
        sc_buf_put_32(buf, UINT32_MAX);
        return;
    }

    size = strlen(str);
    if (size >= UINT32_MAX) {
        buf->error |= SC_BUF_CORRUPT;
        return;
    }

    sc_buf_put_32(buf, (uint32_t) size);
    sc_buf_put_raw(buf, str, (uint32_t) size + sc_buf_8_len('\0'));
}

static void sc_buf_put_blob(struct sc_buf *buf, const void *ptr, uint32_t len)
{
    sc_buf_put_32(buf, len);
    sc_buf_put_raw(buf, ptr, len);
}

#if defined(_WIN32) || defined(_WIN64)
    #include <Ws2tcpip.h>
    #include <windows.h>
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")

typedef SOCKET sc_sock_int;

#else
    #include <sys/socket.h>
typedef int sc_sock_int;
#endif

#define SC_SOCK_BUF_SIZE 32768

enum sc_sock_rc
{
    SC_SOCK_WANT_READ = -4,
    SC_SOCK_WANT_WRITE = -2,
    SC_SOCK_ERROR = -1,
    SC_SOCK_OK = 0
};

enum sc_sock_ev
{
    SC_SOCK_NONE = 0u,
    SC_SOCK_READ = 1u,
    SC_SOCK_WRITE = 2u,
};

enum sc_sock_family
{
    SC_SOCK_INET = AF_INET,
    SC_SOCK_INET6 = AF_INET6,
    SC_SOCK_UNIX = AF_UNIX
};

struct sc_sock_fd
{
    sc_sock_int fd;
    enum sc_sock_ev op;
    int type; // user data
    int index;
};

struct sc_sock
{
    struct sc_sock_fd fdt;
    bool blocking;
    int family;
    char err[128];
};

#if defined(_WIN32) || defined(_WIN64)
    #include <Ws2tcpip.h>
    #include <afunix.h>
    #include <assert.h>

    #pragma warning(disable : 4996)
    #define sc_close(n)    closesocket(n)
    #define sc_unlink(n)   DeleteFileA(n)
    #define SC_ERR         SOCKET_ERROR
    #define SC_INVALID     INVALID_SOCKET
    #define SC_EAGAIN      WSAEWOULDBLOCK
    #define SC_EINPROGRESS WSAEINPROGRESS
    #define SC_EINTR       WSAEINTR

typedef int socklen_t;

static int sc_sock_err()
{
    return WSAGetLastError();
}

static void sc_sock_errstr(struct sc_sock *sock, int gai_err)
{
    int rc;
    DWORD err = WSAGetLastError();
    LPSTR errstr = 0;

    rc = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL, err, 0, (LPSTR) &errstr, 0, NULL);
    if (rc != 0) {
        strncpy(sock->err, errstr, sizeof(sock->err) - 1);
        LocalFree(errstr);
    }
}

static int sc_sock_set_blocking(struct sc_sock *sock, bool blocking)
{
    int mode = blocking ? 0 : 1;
    int rc = ioctlsocket(sock->fdt.fd, FIONBIO, &mode);

    return rc == 0 ? 0 : -1;
}

static int sc_sock_startup()
{
    int rc;
    WSADATA data;

    rc = WSAStartup(MAKEWORD(2, 2), &data);
    if (rc != 0 || (LOBYTE(data.wVersion) != 2 || HIBYTE(data.wVersion) != 2)) {
        return -1;
    }

    return 0;
}

static int sc_sock_cleanup()
{
    int rc;

    rc = WSACleanup();
    return rc != 0 ? -1 : 0;
}


#else

    #include <netdb.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <sys/un.h>
    #include <unistd.h>

    #define sc_close(n)    close(n)
    #define sc_unlink(n)   unlink(n)
    #define SC_ERR         (-1)
    #define SC_INVALID     (-1)
    #define SC_EAGAIN      EAGAIN
    #define SC_EINPROGRESS EINPROGRESS
    #define SC_EINTR       EINTR

static int sc_sock_startup()
{
    return 0;
}

static int sc_sock_cleanup()
{
    return 0;
}

static int sc_sock_err()
{
    return errno;
}

static void sc_sock_errstr(struct sc_sock *sock, int gai_err)
{
    const char *str = gai_err ? gai_strerror(gai_err) : strerror(errno);

    strncpy(sock->err, str, sizeof(sock->err) - 1);
}

static int sc_sock_set_blocking(struct sc_sock *sock, bool blocking)
{
    int flags = fcntl(sock->fdt.fd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }

    flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
    return (fcntl(sock->fdt.fd, F_SETFL, flags) == 0) ? 0 : -1;
}

#endif


static void sc_sock_init(struct sc_sock *sock, int type, bool blocking,
                         int family)
{
    sock->fdt.fd = -1;
    sock->fdt.type = type;
    sock->fdt.op = SC_SOCK_NONE;
    sock->fdt.index = -1;
    sock->blocking = blocking;
    sock->family = family;

    memset(sock->err, 0, sizeof(sock->err));
}

static int sc_sock_close(struct sc_sock *sock)
{
    int rc = 0;

    if (sock->fdt.fd != -1) {
        rc = sc_close(sock->fdt.fd);
        sock->fdt.fd = -1;
    }

    return (rc == 0) ? 0 : -1;
}

static int sc_sock_term(struct sc_sock *sock)
{
    int rc;

    rc = sc_sock_close(sock);
    if (rc != 0) {
        sc_sock_errstr(sock, 0);
    }

    return rc;
}

static int sc_sock_set_rcvtimeo(struct sc_sock *sock, int ms)
{
    int rc;
    void *p;

    struct timeval tv = {
            .tv_usec = ms % 1000,
            .tv_sec = ms / 1000,
    };

    p = (void *) &tv;

    rc = setsockopt(sock->fdt.fd, SOL_SOCKET, SO_RCVTIMEO, p, sizeof(tv));
    if (rc != 0) {
        sc_sock_errstr(sock, 0);
    }

    return rc == 0 ? SC_SOCK_OK : SC_SOCK_ERROR;
}

static int sc_sock_set_sndtimeo(struct sc_sock *sock, int ms)
{
    int rc;
    void *p;

    struct timeval tv = {
            .tv_usec = ms % 1000,
            .tv_sec = ms / 1000,
    };

    p = (void *) &tv;

    rc = setsockopt(sock->fdt.fd, SOL_SOCKET, SO_SNDTIMEO, p, sizeof(tv));
    if (rc != 0) {
        sc_sock_errstr(sock, 0);
    }

    return rc == 0 ? SC_SOCK_OK : SC_SOCK_ERROR;
}

static int sc_sock_finish_connect(struct sc_sock *sock)
{
    int result, rc;
    socklen_t len = sizeof(result);

    rc = getsockopt(sock->fdt.fd, SOL_SOCKET, SO_ERROR, (void *) &result, &len);
    if (rc != 0 || result != 0) {
        sc_sock_errstr(sock, 0);
        return SC_SOCK_ERROR;
    }

    return SC_SOCK_OK;
}

static int sc_sock_connect_unix(struct sc_sock *sock, const char *addr)
{
    int rc;
    const size_t len = strlen(addr);
    struct sockaddr_un addr_un = {.sun_family = AF_UNIX};

    if (len >= sizeof(addr_un.sun_path)) {
        return -1;
    }

    strcpy(addr_un.sun_path, addr);

    rc = connect(sock->fdt.fd, (struct sockaddr *) &addr_un, sizeof(addr_un));
    if (rc != 0) {
        sc_sock_errstr(sock, 0);
    }

    return rc;
}

static int sc_sock_connect(struct sc_sock *sock, const char *dest_addr,
                           const char *dest_port, const char *source_addr,
                           const char *source_port)
{
    const int bf = SC_SOCK_BUF_SIZE;
    const socklen_t sz = sizeof(bf);

    int rc, rv = SC_SOCK_OK;
    struct addrinfo inf = {.ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM};
    struct addrinfo *servinfo = NULL, *bindinfo = NULL, *p, *s;

    if (sock->family == AF_UNIX) {
        sc_sock_int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd == SC_INVALID) {
            goto error_unix;
        }

        sock->fdt.fd = fd;

        rc = setsockopt(sock->fdt.fd, SOL_SOCKET, SO_RCVBUF, (void *) &bf, sz);
        if (rc != 0) {
            goto error_unix;
        }

        rc = setsockopt(sock->fdt.fd, SOL_SOCKET, SO_SNDBUF, (void *) &bf, sz);
        if (rc != 0) {
            goto error_unix;
        }

        rc = sc_sock_connect_unix(sock, dest_addr);
        if (rc != 0) {
            goto error_unix;
        }

        return 0;

error_unix:
        sc_sock_errstr(sock, 0);
        sc_sock_close(sock);

        return -1;
    }

    rc = getaddrinfo(dest_addr, dest_port, &inf, &servinfo);
    if (rc != 0) {
        sc_sock_errstr(sock, rc);
        return -1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        void *tmp;
        sc_sock_int fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd == SC_INVALID) {
            continue;
        }

        sock->family = p->ai_family;
        sock->fdt.fd = fd;

        rc = sc_sock_set_blocking(sock, sock->blocking);
        if (rc != 0) {
            goto error;
        }

        tmp = (void *) &(int){1};
        rc = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, tmp, sizeof(int));
        if (rc != 0) {
            goto error;
        }

        rc = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, tmp, sizeof(int));
        if (rc != 0) {
            goto error;
        }

        rc = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (void *) &bf, sizeof(int));
        if (rc != 0) {
            goto error;
        }

        rc = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (void *) &bf, sizeof(int));
        if (rc != 0) {
            goto error;
        }

        if (source_addr || source_port) {
            rc = getaddrinfo(source_addr, source_port, &inf, &bindinfo);
            if (rc != 0) {
                sc_sock_errstr(sock, rc);
                goto error_gai;
            }

            for (s = bindinfo; s != NULL; s = s->ai_next) {
                rc = bind(sock->fdt.fd, s->ai_addr, (socklen_t) s->ai_addrlen);
                if (rc != -1) {
                    break;
                }
            }

            freeaddrinfo(bindinfo);

            if (rc == -1) {
                goto error;
            }
        }

        rc = connect(sock->fdt.fd, p->ai_addr, (socklen_t) p->ai_addrlen);
        if (rc != 0) {
            if (!sock->blocking && (sc_sock_err() == SC_EINPROGRESS ||
                                    sc_sock_err() == SC_EAGAIN)) {
                rv = SC_SOCK_WANT_WRITE;
                goto end;
            }

            sc_sock_close(sock);
            continue;
        }

        goto end;
    }

error:
    sc_sock_errstr(sock, 0);
error_gai:
    sc_sock_close(sock);
    rv = -1;
end:
    freeaddrinfo(servinfo);

    return rv;
}


static int sc_sock_send(struct sc_sock *sock, char *buf, int len, int flags)
{
    int n;

    if (len <= 0) {
        return len;
    }

retry:
    n = (int) send(sock->fdt.fd, buf, (size_t) len, flags);
    if (n == SC_ERR) {
        int err = sc_sock_err();
        if (err == SC_EINTR) {
            goto retry;
        }

        if (err == SC_EAGAIN) {
            return SC_SOCK_WANT_WRITE;
        }

        sc_sock_errstr(sock, 0);
        n = SC_SOCK_ERROR;
    }

    return n;
}

static int sc_sock_recv(struct sc_sock *sock, char *buf, int len, int flags)
{
    int n;

    if (len <= 0) {
        return len;
    }

retry:
    n = (int) recv(sock->fdt.fd, buf, (size_t) len, flags);
    if (n == 0) {
        return SC_SOCK_ERROR;
    } else if (n == SC_ERR) {
        int err = sc_sock_err();
        if (err == SC_EINTR) {
            goto retry;
        }

        if (err == SC_EAGAIN) {
            return SC_SOCK_WANT_READ;
        }

        sc_sock_errstr(sock, 0);
        n = SC_SOCK_ERROR;
    }

    return n;
}


static const char *sc_sock_error(struct sc_sock *sock)
{
    sock->err[sizeof(sock->err) - 1] = '\0';
    return sock->err;
}

enum task_flag
{
    TASK_FLAG_OK = 0,
    TASK_FLAG_ERROR,
    TASK_FLAG_DONE,
    TASK_FLAG_STMT,
    TASK_FLAG_STMT_ID,
    TASK_FLAG_STMT_PREPARE,
    TASK_FLAG_STMT_DEL_PREPARED,
    TASK_FLAG_ROW,
    TASK_FLAG_END
};

enum task_param
{
    TASK_PARAM_INTEGER = 0,
    TASK_PARAM_FLOAT,
    TASK_PARAM_TEXT,
    TASK_PARAM_BLOB,
    TASK_PARAM_NULL
};

enum task_param_type
{
    TASK_PARAM_NAME = 5,
    TASK_PARAM_INDEX = 6
};

enum msg_rc
{
    MSG_OK,
    MSG_ERR,
    MSG_CLUSTER_NAME_MISMATCH,
    MSG_CORRUPT,
    MSG_UNEXPECTED,
    MSG_TIMEOUT,
    MSG_NOT_LEADER,
};


struct resql_result
{
    uint32_t next_result;
    int changes;
    int64_t last_row_id;
    int row_count;
    int remaining_rows;
    uint32_t row_pos;
    int column_count;
    struct sc_buf buf;
    struct resql_column *row;
    int column_cap;
};

void resql_reset_rows(struct resql_result *rs)
{
    sc_buf_set_rpos(&rs->buf, rs->row_pos);
    rs->remaining_rows = rs->row_count;
}

bool resql_next(struct resql_result *rs)
{
    enum task_flag flag;
    size_t size;

    sc_buf_set_rpos(&rs->buf, rs->next_result);

    if (sc_buf_get_8(&rs->buf) != TASK_FLAG_STMT) {
        return false;
    }

    rs->next_result = sc_buf_rpos(&rs->buf) + sc_buf_get_32(&rs->buf);
    rs->row_count = -1;
    rs->remaining_rows = -1;
    rs->changes = sc_buf_get_32(&rs->buf);
    rs->last_row_id = sc_buf_get_64(&rs->buf);

    flag = (enum task_flag) sc_buf_get_8(&rs->buf);
    if (flag == TASK_FLAG_ROW) {
        rs->column_count = sc_buf_get_32(&rs->buf);
        if (rs->column_count > rs->column_cap) {
            size = sizeof(*rs->row) * rs->column_count;
            rs->row = resql_realloc(rs->row, size);
            rs->column_cap = rs->column_count;
        }

        for (int i = 0; i < rs->column_count; i++) {
            rs->row[i].name = sc_buf_get_str(&rs->buf);
        }

        rs->row_count = sc_buf_get_32(&rs->buf);
        rs->remaining_rows = rs->row_count;
        rs->row_pos = sc_buf_rpos(&rs->buf);

        return true;
    }

    return flag == TASK_FLAG_DONE;
}

struct resql_column *resql_row(struct resql_result *rs)
{
    enum resql_type param;

    if (rs->remaining_rows <= 0) {
        return NULL;
    }

    rs->remaining_rows--;

    for (int i = 0; i < rs->column_count; i++) {
        param = (enum resql_type) sc_buf_get_8(&rs->buf);

        switch (param) {
        case RESQL_INTEGER:
            rs->row[i].type = RESQL_INTEGER;
            rs->row[i].len = -1;
            rs->row[i].intval = sc_buf_get_64(&rs->buf);
            break;
        case RESQL_FLOAT:
            rs->row[i].type = RESQL_FLOAT;
            rs->row[i].len = -1;
            rs->row[i].floatval = sc_buf_get_double(&rs->buf);
            break;
        case RESQL_TEXT:
            rs->row[i].type = RESQL_TEXT;
            rs->row[i].len = sc_buf_peek_32(&rs->buf);
            rs->row[i].text = sc_buf_get_str(&rs->buf);
            break;
        case RESQL_BLOB:
            rs->row[i].type = RESQL_BLOB;
            rs->row[i].len = sc_buf_get_32(&rs->buf);
            rs->row[i].blob =
                    sc_buf_get_blob(&rs->buf, (uint32_t) rs->row[i].len);
            break;
        case RESQL_NULL:
            rs->row[i].type = RESQL_NULL;
            rs->row[i].blob = NULL;
            rs->row[i].len = -1;
            break;
        default:
            return false;
            break;
        }
    }

    return rs->row;
}

int resql_row_count(struct resql_result *rs)
{
    return rs->row_count;
}

int resql_changes(struct resql_result *rs)
{
    return rs->changes;
}

int64_t resql_last_row_id(struct resql_result *rs)
{
    return rs->last_row_id;
}

int resql_column_count(struct resql_result *rs)
{
    return rs->column_count;
}

#define MSG_REMOTE_LEN 1u
#define MSG_RC_LEN     1u
#define MSG_MAX_SIZE   (2 * 1000 * 1000 * 1000)

#define MSG_SIZE_LEN          4
#define MSG_TYPE_LEN          1
#define MSG_FIXED_LEN         (MSG_SIZE_LEN + MSG_TYPE_LEN)
#define MSG_SEQ_LEN           8
#define MSG_READONLY_LEN      1
#define MSG_CLIENT_REQ_HEADER (MSG_FIXED_LEN + MSG_SEQ_LEN + MSG_READONLY_LEN)
#define MSG_RESQL_STR         "resql"
#define MSG_REMOTE_CLIENT     0

// clang-format off

enum msg_type
{
    MSG_CONNECT_REQ     = 0x00,
    MSG_CONNECT_RESP    = 0x01,
    MSG_DISCONNECT_REQ  = 0x02,
    MSG_DISCONNECT_RESP = 0x03,
    MSG_CLIENT_REQ      = 0x04,
    MSG_CLIENT_RESP     = 0x05,
};

// clang-format on

struct msg_connect_resp
{
    enum msg_rc rc;
    uint64_t sequence;
    uint64_t term;
    const char *nodes;
};

struct msg_disconnect_req
{
    enum msg_rc rc;
    uint32_t flags;
};

struct msg_disconnect_resp
{
    enum msg_rc rc;
    uint32_t flags;
};

struct msg_client_resp
{
    uint32_t len;
    unsigned char *buf;
};

struct msg
{
    union
    {
        struct msg_connect_resp connect_resp;
        struct msg_disconnect_req disconnect_req;
        struct msg_disconnect_resp disconnect_resp;
        struct msg_client_resp client_resp;
    };

    enum msg_type type;
    uint32_t len;
};

static int msg_len(struct sc_buf *buf)
{
    if (sc_buf_size(buf) < MSG_SIZE_LEN) {
        return -1;
    }

    return sc_buf_peek_32(buf);
}

static int msg_parse(struct sc_buf *buf, struct msg *msg)
{
    uint32_t len;
    void *p;
    struct sc_buf tmp;

    if (sc_buf_size(buf) < MSG_SIZE_LEN) {
        return RESQL_PARTIAL;
    }

    len = sc_buf_peek_32(buf);
    if (len > MSG_MAX_SIZE) {
        return RESQL_INVALID;
    }

    if (len == 0) {
        return RESQL_PARTIAL;
    }

    if (len > sc_buf_size(buf)) {
        return RESQL_PARTIAL;
    }

    p = sc_buf_rbuf(buf);
    sc_buf_mark_read(buf, len);

    tmp = sc_buf_wrap(p, len, SC_BUF_READ);

    msg->len = sc_buf_get_32(&tmp);
    msg->type = (enum msg_type) sc_buf_get_8(&tmp);

    switch (msg->type) {
    case MSG_CONNECT_RESP:
        msg->connect_resp.rc = (enum msg_rc) sc_buf_get_8(&tmp);
        msg->connect_resp.sequence = sc_buf_get_64(&tmp);
        msg->connect_resp.term = sc_buf_get_64(&tmp);
        msg->connect_resp.nodes = sc_buf_get_str(&tmp);
        break;

    case MSG_DISCONNECT_REQ:
        msg->disconnect_req.rc = (enum msg_rc) sc_buf_get_8(&tmp);
        msg->disconnect_req.flags = sc_buf_get_32(&tmp);
        break;

    case MSG_DISCONNECT_RESP:
        msg->disconnect_resp.rc = (enum msg_rc) sc_buf_get_8(&tmp);
        msg->disconnect_resp.flags = sc_buf_get_32(&tmp);
        break;

    case MSG_CLIENT_RESP:
        msg->client_resp.buf = sc_buf_rbuf(&tmp);
        msg->client_resp.len = sc_buf_size(&tmp);
        break;

    default:
        return RESQL_ERROR;
        break;
    }

    return sc_buf_valid(&tmp) ? RESQL_OK : RESQL_ERROR;
}

static bool msg_create_connect_req(struct sc_buf *buf, const char *cluster_name,
                                   const char *name)
{
    uint32_t head = sc_buf_wpos(buf);
    uint32_t len = MSG_SIZE_LEN + MSG_TYPE_LEN +
                   sc_buf_32_len(MSG_REMOTE_CLIENT) +
                   sc_buf_str_len(MSG_RESQL_STR) +
                   sc_buf_str_len(cluster_name) + sc_buf_str_len(name);

    sc_buf_put_32(buf, len);
    sc_buf_put_8(buf, MSG_CONNECT_REQ);
    sc_buf_put_32(buf, MSG_REMOTE_CLIENT);
    sc_buf_put_str(buf, MSG_RESQL_STR);
    sc_buf_put_str(buf, cluster_name);
    sc_buf_put_str(buf, name);

    if (!sc_buf_valid(buf)) {
        sc_buf_set_wpos(buf, head);
        return false;
    }

    return true;
}

static bool msg_create_disconnect_req(struct sc_buf *buf, enum msg_rc rc,
                                      uint32_t flags)
{
    uint32_t head = sc_buf_wpos(buf);
    uint32_t len = MSG_FIXED_LEN + MSG_RC_LEN + sc_buf_32_len(flags);

    sc_buf_put_32(buf, len);
    sc_buf_put_8(buf, MSG_DISCONNECT_REQ);
    sc_buf_put_8(buf, rc);
    sc_buf_put_32(buf, flags);

    if (!sc_buf_valid(buf)) {
        sc_buf_set_wpos(buf, head);
        return false;
    }

    return true;
}

static bool msg_create_client_req_header(struct sc_buf *buf)
{
    sc_buf_set_wpos(buf, MSG_CLIENT_REQ_HEADER);
    return sc_buf_valid(buf);
}

static bool msg_finalize_client_req(struct sc_buf *buf, bool readonly,
                                    uint64_t seq)
{
    sc_buf_set_32_at(buf, 0, sc_buf_wpos(buf));
    sc_buf_set_8_at(buf, 4, MSG_CLIENT_REQ);
    sc_buf_set_8_at(buf, 5, (uint8_t) readonly);
    sc_buf_set_64_at(buf, 6, seq);

    return sc_buf_valid(buf);
}

struct resql
{
    char *name;
    char *cluster_name;
    char *source_addr;
    char *source_port;

    uint32_t timeout;

    bool connected;
    bool statement;
    bool error;

    uint64_t seq;

    struct sc_sock sock;
    struct sc_uri *uris[16];
    int uri_count;
    int uri_trial;
    uint64_t uri_term;
    struct resql_result rs;

    struct sc_buf resp;
    struct sc_buf req;
    struct msg msg;
    char err[256];
};

static void resql_disconnect(struct resql *c)
{
    if (c->connected) {
        c->connected = false;
        sc_sock_term(&c->sock);
    }
}

static void resql_reset(struct resql *c)
{
    c->uri_term = 0;
    c->uri_trial = 0;
    c->error = false;
    c->statement = false;

    sc_buf_clear(&c->resp);
    resql_disconnect(c);
    resql_clear(c);
}

static void resql_err(struct resql *c, const char *fmt, ...)
{
    va_list va;

    va_start(va, fmt);
    vsnprintf(c->err, sizeof(c->err), fmt, va);
    va_end(va);
}

static void resql_fatal(struct resql *c, const char *fmt, ...)
{
    va_list va;

    va_start(va, fmt);
    vsnprintf(c->err, sizeof(c->err), fmt, va);
    va_end(va);

    resql_reset(c);
}

int resql_copy_uris(struct resql *c, struct msg *msg)
{
#define CAP 16

    int count = 0;
    const char *token;
    char *save = NULL, *s;
    struct sc_uri *uri;
    struct sc_uri *uris[CAP];

    if (c->uri_term < msg->connect_resp.term) {
        s = sc_str_create(msg->connect_resp.nodes);
        if (!s) {
            return RESQL_OOM;
        }

        while ((token = sc_str_token_begin(s, &save, " ")) != NULL) {
            uri = sc_uri_create(token);
            if (!uri) {
                continue;
            }

            uris[count++] = uri;
            if (count == CAP) {
                break;
            }
        }

        sc_str_destroy(s);

        if (count == 0) {
            return RESQL_OK;
        }

        for (int i = 0; i < c->uri_count; i++) {
            sc_uri_destroy(c->uris[i]);
        }

        memcpy(c->uris, uris, sizeof(uris));
        c->uri_count = count;
        c->uri_trial = 0;
        c->uri_term = msg->connect_resp.term;
    }

    return RESQL_OK;
}

int resql_recv_connect_resp(struct resql *c)
{
    bool b;
    int rc, ret;
    uint64_t seq;
    struct msg msg;
    struct sc_buf *resp = &c->resp;

    sc_buf_clear(resp);

    b = msg_create_connect_req(resp, c->cluster_name, c->name);
    if (!b) {
        resql_err(c, "out of memory");
        return RESQL_FATAL;
    }

    rc = sc_sock_send(&c->sock, sc_buf_rbuf(resp), sc_buf_size(resp), 0);
    if (rc < 0) {
        resql_err(c, "sock send failure.");
        return RESQL_ERROR;
    }

    sc_buf_clear(resp);

retry:
    rc = sc_sock_recv(&c->sock, sc_buf_wbuf(resp), sc_buf_quota(resp), 0);
    if (rc < 0) {
        resql_err(c, "sock recv failure.");
        return RESQL_ERROR;
    }

    sc_buf_mark_write(resp, (uint32_t) rc);

    ret = msg_parse(resp, &msg);
    if (ret == RESQL_PARTIAL) {
        goto retry;
    }

    if (ret == RESQL_INVALID || msg.type != MSG_CONNECT_RESP) {
        resql_err(c, "received malformed message.");
        return RESQL_FATAL;
    }

    rc = resql_copy_uris(c, &msg);
    if (rc != RESQL_OK) {
        resql_err(c, "out of memory");
        return RESQL_FATAL;
    }

    rc = msg.connect_resp.rc;
    if (rc != MSG_OK) {
        resql_err(c, "connect request has been rejected by the server.");
        return RESQL_ERROR;
    }

    if (sc_buf_size(&c->req) == 0) {
        c->seq = msg.connect_resp.sequence;
    } else {
        seq = msg.connect_resp.sequence;
        if (seq != c->seq && seq != c->seq - 1) {
            resql_err(c, "Client does not exist on the server anymore.(%d, %d)",
                      seq, c->seq);
            c->seq = seq;
            return RESQL_FATAL;
        }
    }

    c->connected = true;

    return RESQL_OK;
}

int resql_connect(struct resql *c)
{
    int rc, family;
    const char *host;
    struct sc_uri *uri;

    if (c->connected) {
        c->connected = false;
        sc_sock_term(&c->sock);
    }

    uri = c->uris[c->uri_trial % c->uri_count];
    c->uri_trial++;

    family = strcmp(uri->scheme, "unix") == 0 ? SC_SOCK_UNIX :
             *uri->host == '['                ? SC_SOCK_INET6 :
                                                SC_SOCK_INET;
    host = family == SC_SOCK_UNIX ? uri->path : uri->host;

    sc_sock_init(&c->sock, 0, false, family);

    rc = sc_sock_connect(&c->sock, host, uri->port, c->source_addr,
                         c->source_port);
    if (rc == SC_SOCK_WANT_WRITE) {
        struct pollfd fds = {.fd = c->sock.fdt.fd, .events = POLLOUT};

retry:
        rc = rs_poll(&fds, 1, c->timeout);
        if (rc < 0 && errno == EINTR) {
            goto retry;
        }

        rc = (rc == 0) ? -1 : sc_sock_finish_connect(&c->sock);
    }

    if (rc != 0) {
        rc = RESQL_ERROR;
        goto sock_error;
    }

    rc = sc_sock_set_blocking(&c->sock, true);
    if (rc != SC_SOCK_OK) {
        goto sock_fatal;
    }

    rc = sc_sock_set_sndtimeo(&c->sock, 2000);
    if (rc != SC_SOCK_OK) {
        goto sock_fatal;
    }

    rc = sc_sock_set_rcvtimeo(&c->sock, 2000);
    if (rc != SC_SOCK_OK) {
        goto sock_fatal;
    }

    rc = resql_recv_connect_resp(c);
    if (rc != RESQL_OK) {
        goto cleanup;
    }

    return RESQL_OK;

sock_fatal:
    rc = RESQL_FATAL;
sock_error:
    resql_err(c, sc_sock_error(&c->sock));
cleanup:
    sc_sock_term(&c->sock);
    return rc;
}

int resql_init()
{
    return sc_sock_startup();
}

int resql_term()
{
    return sc_sock_cleanup();
}

static void resql_generate_name(struct resql *c, char *dest)
{
    static char charset[] = "0123456789"
                            "abcdefghijklmnopqrstuvwxyz"
                            "ABCDEFGHIJKLMNOPQRSTUVWXYZ-_";
    unsigned int num, len;
    uint64_t ts;
    uint32_t generated[40] = {0};
    char *end, *p = (char *) generated;

    ts = sc_time_mono_ms();

    num = (unsigned int) ((rand() % 16) + 8 + (ts % 4));
    len = (num % 16) + 8;
    end = p + (sizeof(generated[0]) * len);

    memcpy(p, &c, sizeof(c));
    p += sizeof(c);
    memcpy(p, &ts, sizeof(ts));
    p += sizeof(ts);

    while (p + sizeof(num) < end) {
        num = rand();
        memcpy(p, &num, sizeof(num));
        p += sizeof(num);
    }

    for (unsigned int i = 0; i < len; i++) {
        dest[i] = charset[generated[i] % (sizeof(charset) - 1)];
    }

    dest[len] = '\0';
}

int resql_create(struct resql **client, struct resql_config *conf)
{
    bool b;
    int rc;
    uint64_t start;
    char *uris, *save = NULL;
    const char *token;
    char name[32];
    const char *user = conf->client_name;
    struct resql *c;
    struct sc_uri *u;

    c = resql_calloc(1, sizeof(*c));
    if (!c) {
        return RESQL_OOM;
    }

    if (!conf->client_name) {
        resql_generate_name(c, name);
        user = name;
    }

    *client = c;

    b = sc_buf_init(&c->resp, 1024);
    b |= sc_buf_init(&c->req, 1024);
    if (!b) {
        goto oom;
    }

    c->rs = (struct resql_result){0};

    c->name = sc_str_create(user);
    c->cluster_name =
            sc_str_create(conf->cluster_name ? conf->cluster_name : "cluster");

    c->source_addr = sc_str_create(conf->outgoing_addr);
    c->source_port = sc_str_create(conf->outgoing_port);

    if (!c->name || !c->cluster_name ||
        (!c->source_addr && conf->outgoing_addr) ||
        (!c->source_port && conf->outgoing_port)) {
        goto oom;
    }

    c->timeout = conf->timeout_millis != 0 ? conf->timeout_millis : UINT32_MAX;
    c->uri_count = 0;
    c->uri_trial = 0;

    uris = sc_str_create(conf->urls ? conf->urls : "tcp://127.0.0.1:7600");
    if (!uris) {
        goto oom;
    }

    while ((token = sc_str_token_begin(uris, &save, " ")) != NULL) {
        u = sc_uri_create(token);
        if (u == NULL) {
            rc = RESQL_CONFIG_ERROR;
            resql_err(c, "Cannot parse : %s", token);
            sc_str_destroy(uris);
            goto error;
        }

        c->uris[c->uri_count++] = u;
        if (c->uri_count + 1 == sizeof(c->uris) / sizeof(c->uris[0])) {
            break;
        }
    }

    sc_str_token_end(uris, &save);
    sc_str_destroy(uris);

    *client = c;

    start = sc_time_mono_ms();
    while (true) {
        if (sc_time_mono_ms() - start > c->timeout) {
            return RESQL_ERROR;
        }

        rc = resql_connect(c);
        if (rc == RESQL_OK) {
            break;
        }

        if (rc == RESQL_FATAL) {
            resql_reset(c);
            return RESQL_ERROR;
        }

        sc_time_sleep(500);
    }

    resql_clear(c);

    return RESQL_OK;
oom:
    rc = RESQL_OOM;
    resql_err(c, "Out of memory.");
error:
    return rc;
}

int resql_shutdown(struct resql *c)
{
    if (!c) {
        return RESQL_OK;
    }

    if (c->connected) {
        sc_buf_clear(&c->req);
        msg_create_disconnect_req(&c->req, MSG_OK, 0);
        sc_sock_send(&c->sock, sc_buf_rbuf(&c->req), sc_buf_size(&c->req), 0);
        sc_sock_term(&c->sock);
    }

    sc_str_destroy(c->name);
    sc_str_destroy(c->cluster_name);
    sc_str_destroy(c->source_addr);
    sc_str_destroy(c->source_port);

    for (int i = 0; i < c->uri_count; i++) {
        sc_uri_destroy(c->uris[i]);
    }
    c->uri_count = 0;

    resql_free(c->rs.row);
    sc_buf_term(&c->req);
    sc_buf_term(&c->resp);
    resql_free(c);

    return RESQL_OK;
}

const char *resql_errstr(struct resql *c)
{
    c->err[sizeof(c->err) - 1] = '\0';
    return c->err;
}

int resql_send_req(struct resql *c, struct sc_buf *resp)
{
    bool b;
    int rc, n;
    uint8_t flag;

    uint64_t start = sc_time_mono_ms();

retry_op:
    if (sc_time_mono_ms() - start > c->timeout) {
        resql_err(c, "Operation timeout.");
        return RESQL_ERROR;
    }

    if (!c->connected) {
        rc = resql_connect(c);
        if (rc == RESQL_FATAL) {
            resql_reset(c);
            return RESQL_ERROR;
        }

        if (rc != RESQL_OK) {
            sc_time_sleep(500);
            goto retry_op;
        }
    }

    n = sc_sock_send(&c->sock, sc_buf_rbuf(&c->req), sc_buf_size(&c->req), 0);
    if (n < 0) {
        c->connected = false;
        sc_sock_term(&c->sock);
        goto retry_op;
    }

    sc_buf_clear(&c->resp);

retry_recv:
    n = sc_sock_recv(&c->sock, sc_buf_wbuf(&c->resp), sc_buf_quota(&c->resp),
                     0);
    if (n <= 0) {
        c->connected = false;
        sc_sock_term(&c->sock);
        goto retry_op;
    }

    sc_buf_mark_write(&c->resp, (uint32_t) n);

    rc = msg_parse(&c->resp, &c->msg);
    if (rc == RESQL_PARTIAL) {
        rc = msg_len(&c->resp);
        if (rc != -1) {
            b = sc_buf_reserve(&c->resp, (uint32_t) rc);
            if (!b) {
                resql_fatal(c, "Out of memory.");
                return RESQL_ERROR;
            }
        }

        goto retry_recv;
    }

    resql_clear(c);

    if (rc == RESQL_ERROR || rc == RESQL_INVALID) {
        resql_fatal(c, "Out of memory.");
        return RESQL_ERROR;
    }

    *resp = sc_buf_wrap(c->msg.client_resp.buf, c->msg.client_resp.len,
                        SC_BUF_READ);

    flag = sc_buf_get_8(resp);
    if (flag == TASK_FLAG_ERROR) {
        resql_err(c, sc_buf_get_str(resp));
        return RESQL_SQL_ERROR;
    }

    return RESQL_OK;
}

int resql_prepare(struct resql *c, const char *sql, resql_stmt *stmt)
{
    int rc;
    uint64_t id;
    struct sc_buf tmp;

    if (c->statement) {
        resql_err(c, "'Prepare' must be a single operation.");
        resql_clear(c);
        return RESQL_SQL_ERROR;
    }

    sc_buf_put_8(&c->req, TASK_FLAG_STMT_PREPARE);
    sc_buf_put_str(&c->req, sql);
    sc_buf_put_8(&c->req, TASK_FLAG_END);

    c->seq++;
    msg_finalize_client_req(&c->req, false, c->seq);
    sc_buf_set_rpos(&c->req, 0);

    rc = resql_send_req(c, &tmp);
    if (rc != RESQL_OK) {
        return rc;
    }

    id = sc_buf_get_64(&tmp);
    if (!sc_buf_valid(&tmp)) {
        resql_fatal(c, "Received malformed response");
        return RESQL_ERROR;
    }

    *stmt = id;
    return RESQL_OK;
}

int resql_del_prepared(struct resql *c, resql_stmt *stmt)
{
    int rc;
    struct sc_buf tmp;

    if (c->statement) {
        resql_err(c, "'Delete prepared' must be a single operation.");
        resql_clear(c);
        return RESQL_SQL_ERROR;
    }

    sc_buf_put_8(&c->req, TASK_FLAG_STMT_DEL_PREPARED);
    sc_buf_put_64(&c->req, *stmt);
    sc_buf_put_8(&c->req, TASK_FLAG_END);

    c->seq++;
    msg_finalize_client_req(&c->req, false, c->seq);
    sc_buf_set_rpos(&c->req, 0);
    *stmt = 0;

    rc = resql_send_req(c, &tmp);
    if (rc != RESQL_OK) {
        return rc;
    }

    return RESQL_OK;
}

void resql_bind_param_int(resql *c, const char *param, int64_t val)
{
    if (!c->statement) {
        c->error = true;
        resql_err(c, "missing statement before binding");
        return;
    }

    sc_buf_put_8(&c->req, TASK_PARAM_NAME);
    sc_buf_put_str(&c->req, param);
    sc_buf_put_8(&c->req, TASK_PARAM_INTEGER);
    sc_buf_put_64(&c->req, (uint64_t) val);
}

void resql_bind_param_float(resql *c, const char *param, double val)
{
    if (!c->statement) {
        c->error = true;
        resql_err(c, "missing statement before binding");
        return;
    }

    sc_buf_put_8(&c->req, TASK_PARAM_NAME);
    sc_buf_put_str(&c->req, param);
    sc_buf_put_8(&c->req, TASK_PARAM_FLOAT);
    sc_buf_put_double(&c->req, val);
}

void resql_bind_param_text(resql *c, const char *param, const char *val)
{
    if (!c->statement) {
        c->error = true;
        resql_err(c, "missing statement before binding");
        return;
    }

    sc_buf_put_8(&c->req, TASK_PARAM_NAME);
    sc_buf_put_str(&c->req, param);
    sc_buf_put_8(&c->req, TASK_PARAM_TEXT);
    sc_buf_put_str(&c->req, val);
}

void resql_bind_param_blob(resql *c, const char *param, int len, void *data)
{
    if (!c->statement) {
        c->error = true;
        resql_err(c, "missing statement before binding");
        return;
    }

    sc_buf_put_8(&c->req, TASK_PARAM_NAME);
    sc_buf_put_str(&c->req, param);
    sc_buf_put_8(&c->req, TASK_PARAM_BLOB);
    sc_buf_put_blob(&c->req, data, (uint32_t) len);
}

void resql_bind_param_null(resql *c, const char *param)
{
    if (!c->statement) {
        c->error = true;
        resql_err(c, "missing statement before binding");
        return;
    }

    sc_buf_put_8(&c->req, TASK_PARAM_NAME);
    sc_buf_put_str(&c->req, param);
    sc_buf_put_8(&c->req, TASK_PARAM_NULL);
}

void resql_bind_index_int(resql *c, int index, int64_t val)
{
    if (!c->statement) {
        c->error = true;
        resql_err(c, "missing statement before binding");
        return;
    }

    sc_buf_put_8(&c->req, TASK_PARAM_INDEX);
    sc_buf_put_32(&c->req, (uint32_t) index);
    sc_buf_put_8(&c->req, TASK_PARAM_INTEGER);
    sc_buf_put_64(&c->req, (uint64_t) val);
}

void resql_bind_index_float(resql *c, int index, double val)
{
    if (!c->statement) {
        c->error = true;
        resql_err(c, "missing statement before binding");
        return;
    }

    sc_buf_put_8(&c->req, TASK_PARAM_INDEX);
    sc_buf_put_32(&c->req, (uint32_t) index);
    sc_buf_put_8(&c->req, TASK_PARAM_FLOAT);
    sc_buf_put_double(&c->req, val);
}
void resql_bind_index_text(resql *c, int index, const char *val)
{
    if (!c->statement) {
        c->error = true;
        resql_err(c, "missing statement before binding");
        return;
    }

    sc_buf_put_8(&c->req, TASK_PARAM_INDEX);
    sc_buf_put_32(&c->req, (uint32_t) index);
    sc_buf_put_8(&c->req, TASK_PARAM_TEXT);
    sc_buf_put_str(&c->req, val);
}

void resql_bind_index_blob(resql *c, int index, int len, void *data)
{
    if (!c->statement) {
        c->error = true;
        resql_err(c, "missing statement before binding");
        return;
    }

    sc_buf_put_8(&c->req, TASK_PARAM_INDEX);
    sc_buf_put_32(&c->req, (uint32_t) index);
    sc_buf_put_8(&c->req, TASK_PARAM_BLOB);
    sc_buf_put_blob(&c->req, data, (uint32_t) len);
}

void resql_bind_index_null(resql *c, int index)
{
    if (!c->statement) {
        c->error = true;
        resql_err(c, "missing statement before binding");
        return;
    }

    sc_buf_put_8(&c->req, TASK_PARAM_INDEX);
    sc_buf_put_32(&c->req, (uint32_t) index);
    sc_buf_put_8(&c->req, TASK_PARAM_NULL);
}

void resql_put_prepared(struct resql *c, const resql_stmt *stmt)
{
    if (c->statement) {
        sc_buf_put_8(&c->req, TASK_FLAG_END);
    }

    sc_buf_put_8(&c->req, TASK_FLAG_STMT_ID);
    sc_buf_put_64(&c->req, *stmt);

    c->statement = true;
}

void resql_put_sql(struct resql *c, const char *sql)
{
    if (c->statement) {
        sc_buf_put_8(&c->req, TASK_FLAG_END);
    }

    sc_buf_put_8(&c->req, TASK_FLAG_STMT);
    sc_buf_put_str(&c->req, sql);

    c->statement = true;
}

int resql_exec(struct resql *c, bool readonly, struct resql_result **rs)
{
    int rc;
    struct sc_buf tmp;

    *rs = NULL;

    if (!c->statement) {
        resql_err(c, "Missing statement.");
        goto error;
    }

    if (c->error) {
        goto error;
    }

    if (!sc_buf_valid(&c->req)) {
        resql_err(c, "Out of memory!.");
        goto error;
    }

    sc_buf_put_8(&c->req, TASK_FLAG_END);
    c->statement = false;

    if (!readonly) {
        c->seq++;
    }

    msg_finalize_client_req(&c->req, readonly, c->seq);
    sc_buf_set_rpos(&c->req, 0);

    rc = resql_send_req(c, &tmp);
    if (rc != RESQL_OK) {
        goto error;
    }

    c->rs.buf = tmp;
    c->rs.next_result = sc_buf_rpos(&c->rs.buf);
    resql_next(&c->rs);

    *rs = &c->rs;
    return RESQL_OK;

error:
    resql_clear(c);
    return RESQL_SQL_ERROR;
}

void resql_clear(struct resql *c)
{
    c->error = false;
    c->statement = false;
    sc_buf_clear(&c->req);
    msg_create_client_req_header(&c->req);
}
