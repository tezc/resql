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

#ifndef RESQL_ENTRY_H
#define RESQL_ENTRY_H

#include <stddef.h>
#include <stdint.h>

#define ENTRY_CRC_LEN	  4
#define ENTRY_HEADER_SIZE 36

struct sc_buf;

void entry_encode(struct sc_buf *b, uint64_t term, uint64_t seq, uint64_t cid,
		  uint32_t flags, void *data, uint32_t len);

int entry_decode(struct sc_buf *b);

uint32_t entry_encoded_len(uint32_t len);
uint32_t entry_crc(unsigned char *e);
uint32_t entry_len(unsigned char *e);
uint64_t entry_term(unsigned char *e);
uint64_t entry_seq(unsigned char *e);

uint64_t entry_cid(unsigned char *e);
uint32_t entry_flags(unsigned char *e);

void *entry_data(unsigned char *e);
uint32_t entry_data_len(unsigned char *e);

#define entry_foreach(buf, len, entry)                                         \
	for ((entry) = ((unsigned char *) (buf));                              \
	     (entry) < ((unsigned char *) (buf)) + (len);                      \
	     (entry) += entry_len(entry))

#endif
