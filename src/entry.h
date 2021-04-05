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


#ifndef RESQL_ENTRY_H
#define RESQL_ENTRY_H

#include <stddef.h>
#include <stdint.h>

#define ENTRY_CRC_LEN     4
#define ENTRY_HEADER_SIZE 36

struct sc_buf;

void entry_encode(struct sc_buf *buf, uint64_t term, uint64_t seq, uint64_t cid,
                  uint32_t flags, void *data, uint32_t len);

int entry_decode(struct sc_buf *buf);

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
    for ((entry) = ((unsigned char *) (buf));                                  \
         (entry) < ((unsigned char *) (buf)) + (len);                          \
         (entry) += entry_len(entry))

#endif
