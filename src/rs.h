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


#ifndef RESQL_RS_H
#define RESQL_RS_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#define RS_VERSION  "0.0.4"

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

#define RS_STORE_EXTENSION  ".resql"

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

enum rs_rc
{
    RS_OK = 0,
    RS_ERROR = -1,
    RS_DONE = -2,
    RS_NOOP = -3,
    RS_NOTOPEN = -4,
    RS_RETRY = -5,
    RS_NONBLOCK = -6,
    RS_ENOENT = -7,
    RS_EMPTY = -8,
    RS_INVALID = -9,
    RS_PARTIAL = -10,
    RS_EXISTS = -11,
    RS_TIMEOUT = -12,
    RS_INPROGRESS = -13,
    RS_NOTFOUND = -14,
    RS_OOM = -15,
    RS_FULL = -16
};

#define rs_exp(fmt, ...) fmt, __VA_ARGS__

#define rs_abort(...)                                                          \
    (rs_on_abort(RESQL_FILE_NAME, __func__, __LINE__,                      \
                 rs_exp(__VA_ARGS__, "")))

_Noreturn void rs_on_abort(const char *file, const char *func, int line,
                           const char *fmt, ...);

_Noreturn void rs_exit(const char *fmt, ...);


int rs_snprintf(char *buf, size_t max_len, const char *fmt, ...);
int rs_vsnprintf(char *buf, size_t max_len, const char *fmt, va_list list);
void rs_vasprintf(char **buf, const char *fmt, va_list args);
char* rs_strncpy(char *dest, const char* src, size_t max);

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
