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

#include "entry.h"

#include "rs.h"

#include "sc/sc_buf.h"
#include "sc/sc_crc32.h"

#define ENTRY_CRC_OFFSET   0
#define ENTRY_LEN_OFFSET   4
#define ENTRY_TERM_OFFSET  8
#define ENTRY_SEQ_OFFSET   16
#define ENTRY_CID_OFFSET   24
#define ENTRY_FLAGS_OFFSET 32
#define ENTRY_DATA_OFFSET  36

static uint64_t entry_get_64(const unsigned char *p)
{
	uint64_t val;

	val = (uint64_t) p[0] << 0 | (uint64_t) p[1] << 8 |
	      (uint64_t) p[2] << 16 | (uint64_t) p[3] << 24 |
	      (uint64_t) p[4] << 32 | (uint64_t) p[5] << 40 |
	      (uint64_t) p[6] << 48 | (uint64_t) p[7] << 56;

	return val;
}

static uint32_t entry_get_32(const unsigned char *p)
{
	uint32_t val;

	val = (uint32_t) p[0] << 0 | (uint32_t) p[1] << 8 |
	      (uint32_t) p[2] << 16 | (uint32_t) p[3] << 24;

	return val;
}

void entry_encode(struct sc_buf *b, uint64_t term, uint64_t seq, uint64_t cid,
		  uint32_t flags, void *data, uint32_t len)
{
	uint32_t crc;
	uint32_t enc = entry_encoded_len(len);
	uint32_t head = sc_buf_wpos(b);

	sc_buf_reserve(b, enc);
	sc_buf_set_wpos(b, head + ENTRY_LEN_OFFSET);

	sc_buf_put_32(b, enc);
	sc_buf_put_64(b, term);
	sc_buf_put_64(b, seq);
	sc_buf_put_64(b, cid);
	sc_buf_put_32(b, flags);
	sc_buf_put_raw(b, data, len);

	crc = sc_crc32(0, b->mem + head + ENTRY_CRC_LEN, enc - ENTRY_CRC_LEN);
	sc_buf_set_32_at(b, head, crc);
}

int entry_decode(struct sc_buf *b)
{
	uint32_t head = sc_buf_rpos(b);
	uint32_t crc_val = sc_buf_get_32(b);
	uint32_t len = sc_buf_peek_32(b);
	uint32_t read_pos = sc_buf_rpos(b);
	uint32_t crc_calc;

	if (!sc_buf_valid(b) || len - ENTRY_CRC_LEN > sc_buf_size(b)) {
		goto error;
	}

	crc_calc = sc_crc32(0, b->mem + read_pos, len - ENTRY_CRC_LEN);
	if (crc_val != crc_calc) {
		goto error;
	}

	sc_buf_set_rpos(b, head + len);

	return RS_OK;

error:
	sc_buf_set_rpos(b, head);
	return RS_ERROR;
}

uint32_t entry_encoded_len(uint32_t len)
{
	return ENTRY_HEADER_SIZE + len;
}

uint32_t entry_crc(unsigned char *e)
{
	return entry_get_32(e + ENTRY_CRC_OFFSET);
}

uint32_t entry_len(unsigned char *e)
{
	return entry_get_32(e + ENTRY_LEN_OFFSET);
}

uint64_t entry_term(unsigned char *entry)
{
	return entry_get_64(entry + ENTRY_TERM_OFFSET);
}

uint64_t entry_seq(unsigned char *e)
{
	return entry_get_64(e + ENTRY_SEQ_OFFSET);
}

uint64_t entry_cid(unsigned char *e)
{
	return entry_get_64(e + ENTRY_CID_OFFSET);
}

uint32_t entry_flags(unsigned char *e)
{
	return entry_get_32(e + ENTRY_FLAGS_OFFSET);
}

void *entry_data(unsigned char *e)
{
	return e + ENTRY_DATA_OFFSET;
}

uint32_t entry_data_len(unsigned char *e)
{
	uint32_t total = entry_len(e);
	return total - ENTRY_HEADER_SIZE;
}
