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

#ifndef SC_MUTEX_H
#define SC_MUTEX_H

#define SC_MUTEX_VERSION "2.0.0"

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <pthread.h>
#endif

struct sc_mutex {
#if defined(_WIN32) || defined(_WIN64)
	CRITICAL_SECTION mtx;
#else
	pthread_mutex_t mtx;
#endif
};

/**
 * Create mutex.
 *
 * Be warned on Windows, mutexes are recursive, on Posix default
 * mutex type is not recursive. Edit code if that bothers you. Pass
 * PTHREAD_MUTEX_RECURSIVE instead of PTHREAD_MUTEX_NORMAL.
 *
 * @param mtx mtx
 * @return    '0' on success, '-1' on error.
 */
int sc_mutex_init(struct sc_mutex *mtx);

/**
 * Destroy mutex
 *
 * @param mtx mtx
 * @return    '0' on success, '-1' on error.
 */
int sc_mutex_term(struct sc_mutex *mtx);

/**
 * @param mtx mtx
 */
void sc_mutex_lock(struct sc_mutex *mtx);

/**
 * @param mtx mtx
 */
void sc_mutex_unlock(struct sc_mutex *mtx);

#endif
