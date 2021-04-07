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
