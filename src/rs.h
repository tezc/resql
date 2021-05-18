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

#ifndef RESQL_RS_H
#define RESQL_RS_H

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

// clang-format off

#define RS_VERSION "0.1.3"

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
	#elif defined _WIN32 && (defined _MSC_VER || defined __ICL || \
				defined __DMC__ || defined __BORLANDC__)
		#define thread_local __declspec(thread)
	#elif defined __GNUC__ || defined __SUNPRO_C || defined __xlC__
		#define thread_local __thread
	#else
		#error "Cannot define thread_local"
	#endif
#endif

#ifdef RS_ENABLE_ASSERT
#include <stdio.h>
#include <time.h>

#define rs_assert(cond)                                                        \
	if (!(cond)) {                                                         \
		time_t _time;                                                  \
		char _buf[32] = "localtime failed";                            \
		struct tm *_tm, _tmp;                                          \
                                                                               \
		_time = time(NULL);                                            \
		_tm = localtime_r(&_time, &_tmp);                              \
		if (_tm) {                                                     \
			strftime(_buf, sizeof(_buf), "%Y-%m-%d %H:%M:%S", _tm);\
		}                                                              \
                                                                               \
		fprintf(stderr, "[%s][%s:%d] Assert : %s\n", _buf, __FILE__,   \
			__LINE__, #cond);                                      \
		fflush(stderr);                                                \
		abort();                                                       \
	}
#else
	#define rs_assert(cond) (void) (cond)
#endif

enum rs_rc
{
	RS_OK 		= 0,
	RS_ERROR 	= -1,
	RS_DONE 	= -2,
	RS_NOOP 	= -3,
	RS_RETRY 	= -4,
	RS_NONBLOCK 	= -5,
	RS_ENOENT 	= -6,
	RS_EMPTY 	= -7,
	RS_INVALID 	= -8,
	RS_PARTIAL 	= -9,
	RS_EXISTS 	= -10,
	RS_TIMEOUT 	= -11,
	RS_INPROGRESS 	= -12,
	RS_NOTFOUND 	= -13,
	RS_FULL 	= -14,
	RS_FATAL 	= -15,
	RS_FAIL 	= -16,
	RS_SQL_ERROR    = -17,
	RS_SNAPSHOT     = -18,
	RS_REJECT       = -19
};

#define rs_exp(fmt, ...) fmt, __VA_ARGS__

#define rs_abort(...)                                                          \
	(rs_on_abort(RESQL_FILE_NAME, __func__, __LINE__,                      \
		     rs_exp(__VA_ARGS__, "")))

// clang-format on

void rs_global_init();
void rs_global_shutdown();

_Noreturn void rs_on_abort(const char *file, const char *func, int line,
			   const char *fmt, ...);

_Noreturn void rs_exit(const char *fmt, ...);

int rs_snprintf(char *buf, size_t max_len, const char *fmt, ...);
int rs_vsnprintf(char *buf, size_t max_len, const char *fmt, va_list list);
char *rs_strncpy(char *dest, const char *src, size_t max);

size_t rs_dir_size(const char *path);
size_t rs_dir_free(const char *path);

int rs_write_pid_file(char *path);
int rs_delete_pid_file(char *path);

void *rs_malloc(size_t size);
void *rs_calloc(size_t n, size_t size);
void *rs_realloc(void *p, size_t size);
void rs_free(void *p);

int rs_urandom(void *buf, size_t size);

#endif
