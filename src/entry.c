/*
 *  Resql
 *
 *  Copyright (C) 2021 Ozan Tezcan
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

#include "entry.h"
#include "rs.h"

#include "sc/sc_buf.h"
#include "sc/sc_crc32.h"

uint64_t entry_get_64(const unsigned char* p)
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

uint32_t entry_get_32(const unsigned char* p)
{
    uint32_t val;

    val = (uint32_t) p[0] << 0 |
          (uint32_t) p[1] << 8 |
          (uint32_t) p[2] << 16 |
          (uint32_t) p[3] << 24;

    return val;
}

void entry_encode(struct sc_buf *buf, uint64_t term, uint64_t seq, uint64_t cid,
                  uint32_t flags, void *data, uint32_t len)
{
    uint32_t crc;
    uint32_t enclen = entry_encoded_len(len);
    uint32_t head = sc_buf_wpos(buf);

    sc_buf_set_wpos(buf, head + ENTRY_LEN_OFFSET);

    sc_buf_put_32(buf, enclen);
    sc_buf_put_64(buf, term);
    sc_buf_put_64(buf, seq);
    sc_buf_put_64(buf, cid);
    sc_buf_put_32(buf, flags);
    sc_buf_put_raw(buf, data, len);

    crc = sc_crc32(0, buf->mem + head + ENTRY_CRC_LEN, enclen - ENTRY_CRC_LEN);
    sc_buf_set_32_at(buf, head, crc);
}

int entry_decode(struct sc_buf *buf)
{
    uint32_t head = sc_buf_rpos(buf);
    uint32_t crc_val = sc_buf_get_32(buf);
    uint32_t len = sc_buf_peek_32(buf);
    uint32_t read_pos = sc_buf_rpos(buf);
    uint32_t crc_calc = sc_crc32(0, buf->mem + read_pos, len - ENTRY_CRC_LEN);

    if (crc_val != crc_calc) {
        sc_buf_set_rpos(buf, head);
        return RS_ERROR;
    }

    sc_buf_set_rpos(buf, head + len);

    return RS_OK;
}

uint32_t entry_encoded_len(uint32_t len)
{
    return ENTRY_HEADER_SIZE + len;
}

uint32_t entry_crc(char *entry)
{
    return entry_get_32((unsigned char*) entry + ENTRY_CRC_OFFSET);
}

uint32_t entry_len(char *entry)
{
    return entry_get_32((unsigned char*)entry + ENTRY_LEN_OFFSET);
}

uint64_t entry_term(char *entry)
{
    return entry_get_64((unsigned char*)entry + ENTRY_TERM_OFFSET);
}

uint64_t entry_seq(char *entry)
{
    return entry_get_64((unsigned char*)entry + ENTRY_SEQ_OFFSET);
}

uint64_t entry_cid(char *entry)
{
    return entry_get_64((unsigned char*)entry + ENTRY_CID_OFFSET);
}

uint32_t entry_flags(char *entry)
{
    return entry_get_32((unsigned char*)entry + ENTRY_FLAGS_OFFSET);
}

void *entry_data(char *entry)
{
    return entry + ENTRY_DATA_OFFSET;
}

uint32_t entry_data_len(char *entry)
{
    uint32_t total = entry_len(entry);
    return total - ENTRY_HEADER_SIZE;
}


