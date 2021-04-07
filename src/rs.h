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


#ifndef RESQL_RS_H
#define RESQL_RS_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define RS_VERSION "0.0.17-latest"

#ifndef RESQL_FILE_NAME
    #define RESQL_FILE_NAME "unknown file name"
#endif

#ifdef RS_COMPILER_GIT_BRANCH
    #define RS_GIT_BRANCH RS_COMPILER_GIT_BRANCH
#else
    #define RS_GIT_BRANCH "UNKNOWN"
#endif

#ifdef RS_COMPILER_GIT_COMMIT
    #define RS_GIT_COMMIT RS_COMPILER_GIT_COMMIT
#else
    #define RS_GIT_COMMIT "UNKNOWN"
#endif

#define RS_STORE_EXTENSION ".resql"

#define rs_entry(ptr, type, elem)                                              \
    ((type *) ((char *) (ptr) -offsetof(type, elem)))

#ifndef thread_local
    #if __STDC_VERSION__ >= 201112 && !defined __STDC_NO_THREADS__
        #define thread_local _Thread_local
    #elif defined _WIN32 && (defined _MSC_VER || defined __ICL ||              \
                             defined __DMC__ || defined __BORLANDC__)
        #define thread_local __declspec(thread)
    /* note that ICC (linux) and Clang are covered by __GNUC__ */
    #elif defined __GNUC__ || defined __SUNPRO_C || defined __xlC__
        #define thread_local __thread
    #else
        #error "Cannot define thread_local"
    #endif
#endif

#ifdef RS_ENABLE_ASSERT
    #include <stdio.h>
    #include <time.h>


    #define rs_assert(cond)                                                    \
        if (!(cond)) {                                                         \
            time_t _time;                                                      \
            char _buf[32] = "localtime failed";                                \
            struct tm *_tm, _tmp;                                              \
                                                                               \
            _time = time(NULL);                                                \
            _tm = localtime_r(&_time, &_tmp);                                  \
            if (_tm) {                                                         \
                strftime(_buf, sizeof(_buf), "%Y-%m-%d %H:%M:%S", _tm);        \
            }                                                                  \
                                                                               \
            fprintf(stderr, "[%s][%s:%d] Assert : %s\n", _buf, __FILE__,       \
                    __LINE__, #cond);                                          \
            fflush(stderr);                                                    \
            abort();                                                           \
        }
#else
    #define rs_assert(cond) (void) (cond)
#endif

// clang-format off
enum rs_rc
{
    RS_OK = 0,
    RS_ERROR = -1,
    RS_DONE = -2,
    RS_NOOP = -3,
    RS_RETRY = -4,
    RS_NONBLOCK = -5,
    RS_ENOENT = -6,
    RS_EMPTY = -7,
    RS_INVALID = -8,
    RS_PARTIAL = -9,
    RS_EXISTS = -10,
    RS_TIMEOUT = -11,
    RS_INPROGRESS = -12,
    RS_NOTFOUND = -13,
    RS_FULL = -14
};

// clang-format on

#define rs_exp(fmt, ...) fmt, __VA_ARGS__

#define rs_abort(...)                                                          \
    (rs_on_abort(RESQL_FILE_NAME, __func__, __LINE__, rs_exp(__VA_ARGS__, "")))

_Noreturn void rs_on_abort(const char *file, const char *func, int line,
                           const char *fmt, ...);

_Noreturn void rs_exit(const char *fmt, ...);


int rs_snprintf(char *buf, size_t max_len, const char *fmt, ...);
int rs_vsnprintf(char *buf, size_t max_len, const char *fmt, va_list list);
char *rs_strncpy(char *dest, const char *src, size_t max);

size_t rs_dir_size(const char *path);

int rs_write_pid_file(char *path);
int rs_delete_pid_file(char *path);

void *rs_malloc(size_t size);
void *rs_calloc(size_t n, size_t size);
void *rs_realloc(void *p, size_t size);
void rs_free(void *p);

void rs_rand_init();
unsigned int rs_rand();


#endif
