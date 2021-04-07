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

// clang-format off
static uint64_t entry_get_64(const unsigned char *p)
{
    uint64_t val;

    val = (uint64_t) p[0] << 0 |
          (uint64_t) p[1] << 8 |
          (uint64_t) p[2] << 16 |
          (uint64_t) p[3] << 24 |
          (uint64_t) p[4] << 32 |
          (uint64_t) p[5] << 40 |
          (uint64_t) p[6] << 48 |
          (uint64_t) p[7] << 56;

    return val;
}

static uint32_t entry_get_32(const unsigned char *p)
{
    uint32_t val;

    val = (uint32_t) p[0] << 0 |
          (uint32_t) p[1] << 8 |
          (uint32_t) p[2] << 16 |
          (uint32_t) p[3] << 24;

    return val;
}
// clang-format on

void entry_encode(struct sc_buf *buf, uint64_t term, uint64_t seq, uint64_t cid,
                  uint32_t flags, void *data, uint32_t len)
{
    uint32_t crc;
    uint32_t enc = entry_encoded_len(len);
    uint32_t head = sc_buf_wpos(buf);

    sc_buf_reserve(buf, enc);
    sc_buf_set_wpos(buf, head + ENTRY_LEN_OFFSET);

    sc_buf_put_32(buf, enc);
    sc_buf_put_64(buf, term);
    sc_buf_put_64(buf, seq);
    sc_buf_put_64(buf, cid);
    sc_buf_put_32(buf, flags);
    sc_buf_put_raw(buf, data, len);

    crc = sc_crc32(0, buf->mem + head + ENTRY_CRC_LEN, enc - ENTRY_CRC_LEN);
    sc_buf_set_32_at(buf, head, crc);
}

int entry_decode(struct sc_buf *buf)
{
    uint32_t head = sc_buf_rpos(buf);
    uint32_t crc_val = sc_buf_get_32(buf);
    uint32_t len = sc_buf_peek_32(buf);
    uint32_t read_pos = sc_buf_rpos(buf);
    uint32_t crc_calc;

    if (!sc_buf_valid(buf) || len - ENTRY_CRC_LEN > sc_buf_size(buf)) {
        goto error;
    }

    crc_calc = sc_crc32(0, buf->mem + read_pos, len - ENTRY_CRC_LEN);
    if (crc_val != crc_calc) {
        goto error;
    }

    sc_buf_set_rpos(buf, head + len);

    return RS_OK;

error:
    sc_buf_set_rpos(buf, head);
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
