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

#include "page.h"

#include "entry.h"
#include "file.h"
#include "metric.h"
#include "rs.h"

#include "sc/sc.h"
#include "sc/sc_array.h"
#include "sc/sc_crc32.h"
#include "sc/sc_log.h"
#include "sc/sc_str.h"
#include "sc/sc_time.h"

#include <errno.h>
#include <inttypes.h>

#define PAGE_ENTRY_MAX_SIZE    (2u * 1024 * 1024 * 1024)
#define PAGE_END_MARK	       0
#define PAGE_END_MARK_LEN      4
#define PAGE_VERSION_OFFSET    0
#define PAGE_HEADER_RESERVED   4
#define PAGE_HEADER_LEN	       32
#define PAGE_VERSION	       1
#define PAGE_CRC_OFFSET	       28
#define PAGE_PREV_INDEX_OFFSET 8
#define PAGE_MAX_SIZE	       (1u * 1024 * 1024 * 1024)

#define PAGE_INITIAL_SIZE (32 * 1024 * 1024)

static void page_read(struct page *p)
{
	int rc;
	uint32_t read_pos, remaining;
	unsigned char *entry;

	sc_array_clear(p->entries);

	while (true) {
		remaining = sc_buf_size(&p->buf);
		if (remaining == 0 ||
		    (remaining >= PAGE_END_MARK_LEN &&
		     sc_buf_peek_32(&p->buf) == PAGE_END_MARK)) {
			goto out;
		}

		if (remaining < ENTRY_HEADER_SIZE) {
			sc_log_warn("Partial entry on page %s \n", p->path);
			goto out;
		}

		entry = sc_buf_rbuf(&p->buf);
		rc = entry_decode(&p->buf);
		if (rc != RS_OK) {
			sc_log_warn("Partial entry on page : %s\n", p->path);
			goto out;
		}

		sc_array_add(p->entries, entry);
	}

out:
	read_pos = sc_buf_rpos(&p->buf);
	sc_buf_set_wpos(&p->buf, read_pos);
}

int page_init(struct page *p, const char *path, int64_t len,
	      uint64_t prev_index)
{
	int rc;
	int64_t file_len;
	uint32_t crc_val, crc_calc;

	p->init = false;

	if (len > PAGE_MAX_SIZE) {
		sc_log_error("Requested too big page : %" PRIi64 " \n",
			     p->map.err);
		return RS_ERROR;
	}

	file_len = file_size_at(path);

	len = sc_max(len, file_len);
	len = sc_max(len, PAGE_INITIAL_SIZE);

	rc = sc_mmap_init(&p->map, path, O_CREAT | O_RDWR,
			  PROT_READ | PROT_WRITE, MAP_SHARED, 0, (size_t) len);
	if (rc != 0) {
		sc_log_error("Mmap : %s \n", p->map.err);
		return errno == ENOSPC ? RS_FULL : RS_ERROR;
	}

	p->init = true;
	p->path = sc_str_create(path);
	p->buf = sc_buf_wrap(p->map.ptr, (uint32_t) p->map.len, SC_BUF_REF);

	sc_array_create(p->entries, 1024);
	sc_buf_set_wpos(&p->buf, (uint32_t) p->map.len);

	crc_val = sc_buf_peek_32_at(&p->buf, PAGE_CRC_OFFSET);
	crc_calc = sc_crc32(0, p->buf.mem, PAGE_HEADER_LEN - 4);

	if (crc_calc != crc_val) {
		if (file_len != -1) {
			sc_log_error("Corrupt page : %s \n", path);
		}

		page_clear(p, prev_index);
		return RS_OK;
	}

	p->flush_pos = 0;
	p->flush_index = 0;

	p->prev_index = sc_buf_peek_64_at(&p->buf, PAGE_PREV_INDEX_OFFSET);
	sc_buf_set_wpos(&p->buf, (uint32_t) p->map.len);
	sc_buf_set_rpos(&p->buf, PAGE_HEADER_LEN);

	page_read(p);

	return RS_OK;
}

void page_term(struct page *p)
{
	int rc;

	if (!p || !p->init) {
		return;
	}

	page_fsync(p, page_last_index(p));

	sc_array_destroy(p->entries);
	sc_str_destroy(&p->path);

	rc = sc_mmap_term(&p->map);
	if (rc != 0) {
		sc_log_error("sc_mmap_term : %s\n", p->map.err);
	}
}

int page_reserve(struct page *p, uint32_t size)
{
	int rc, ret = RS_OK;
	uint64_t count = p->map.len - page_quota(p);
	uint64_t prev_index = p->prev_index;
	uint64_t last_index = page_last_index(p);
	uint64_t entry_count = page_entry_count(p);
	uint64_t cap = p->map.len;
	char *path;

	if (page_quota(p) >= size) {
		return RS_OK;
	}

	while (cap <= PAGE_MAX_SIZE && cap < count + size) {
		cap = sc_to_pow2(cap + 1);
	}

	if (cap > PAGE_MAX_SIZE) {
		return RS_FAIL;
	}

	path = sc_str_create(p->path);

	page_term(p);

	rc = page_init(p, path, cap, p->prev_index);
	if (rc != RS_OK) {
		ret = RS_FAIL;
		// Revert back to old size
		rc = page_init(p, path, -1, p->prev_index);
		if (rc != RS_OK) {
			sc_log_error("Rollback failed : %s \n", path);
			ret = rc;
			goto out;
		}
	}

	if (prev_index != p->prev_index || last_index != page_last_index(p) ||
	    entry_count != page_entry_count(p)) {
		rs_abort("Log page is corrupt : %s \n", path);
	}

out:
	sc_str_destroy(&path);
	return ret;
}

bool page_isempty(struct page *p)
{
	return sc_array_size(p->entries) == 0;
}

void page_clear(struct page *p, uint64_t prev_index)
{
	uint32_t crc;

	p->prev_index = prev_index;
	p->flush_index = 0;
	p->flush_pos = 0;

	sc_buf_clear(&p->buf);
	sc_array_clear(p->entries);

	memset(p->map.ptr, 0, PAGE_HEADER_LEN);

	p->buf = sc_buf_wrap(p->map.ptr, (uint32_t) p->map.len, SC_BUF_REF);
	sc_buf_set_32_at(&p->buf, PAGE_VERSION_OFFSET, PAGE_VERSION);
	sc_buf_set_64_at(&p->buf, PAGE_PREV_INDEX_OFFSET, prev_index);

	crc = sc_crc32(0, p->buf.mem, PAGE_HEADER_LEN - 4);
	sc_buf_set_32_at(&p->buf, PAGE_CRC_OFFSET, crc);

	sc_buf_set_wpos(&p->buf, PAGE_HEADER_LEN);
	sc_buf_set_32_at(&p->buf, PAGE_HEADER_LEN, PAGE_END_MARK);

	page_fsync(p, 0);
}

uint32_t page_entry_count(struct page *p)
{
	return (uint32_t) sc_array_size(p->entries);
}

uint64_t page_last_index(struct page *p)
{
	return p->prev_index + sc_array_size(p->entries);
}

uint64_t page_last_term(struct page *p)
{
	assert(sc_array_size(p->entries) != 0);

	return entry_term(sc_array_last(p->entries));
}

unsigned char *page_last_entry(struct page *p)
{
	if (sc_array_size(p->entries) == 0) {
		return NULL;
	}

	return sc_array_last(p->entries);
}

uint32_t page_quota(struct page *p)
{
	uint32_t size;

	size = sc_buf_quota(&p->buf);

	return size - ENTRY_HEADER_SIZE - PAGE_END_MARK_LEN;
}

uint32_t page_cap(struct page *p)
{
	return sc_buf_cap(&p->buf) - PAGE_HEADER_LEN - ENTRY_HEADER_SIZE -
	       PAGE_END_MARK_LEN;
}

void page_fsync(struct page *p, uint64_t index)
{
	int rc;
	uint64_t ts;
	uint32_t pos, last;

	if (index <= p->prev_index || index > page_last_index(p) ||
	    p->flush_index >= index) {
		return;
	}

	pos = sc_buf_wpos(&p->buf);
	if (p->flush_pos >= pos) {
		return;
	}

	last = p->flush_pos;

	ts = sc_time_mono_ns();
	rc = sc_mmap_msync(&p->map, (last & ~(4095)), (pos - last));
	if (rc != 0) {
		// This should never fail
		rs_abort("msync : %s \n", strerror(errno));
	}
	metric_fsync(sc_time_mono_ns() - ts);

	p->flush_pos = pos;
	p->flush_index = page_last_index(p);
}

void page_create_entry(struct page *p, uint64_t term, uint64_t seq,
		       uint64_t cid, uint32_t flags, void *data, uint32_t len)
{
	unsigned char *entry;

	entry = sc_buf_wbuf(&p->buf);
	sc_array_add(p->entries, entry);

	assert(entry_encoded_len(len) <= page_quota(p));

	entry_encode(&p->buf, term, seq, cid, flags, data, len);
	sc_buf_set_32(&p->buf, PAGE_END_MARK);
}

void page_put_entry(struct page *p, unsigned char *e)
{
	rs_assert(entry_len(e) <= page_quota(p));

	uint32_t len = entry_len(e) - ENTRY_CRC_LEN;
	uint32_t crc0 = entry_crc(e);
	unsigned char *d = e + ENTRY_CRC_LEN;

	rs_assert(crc0 == sc_crc32(0, d, len));

	sc_array_add(p->entries, sc_buf_wbuf(&p->buf));
	sc_buf_put_raw(&p->buf, e, entry_len(e));
	sc_buf_set_32(&p->buf, PAGE_END_MARK);
}

unsigned char *page_entry_at(struct page *p, uint64_t index)
{
	if (index <= p->prev_index ||
	    index > p->prev_index + sc_array_size(p->entries)) {
		return NULL;
	}

	return p->entries[index - p->prev_index - 1];
}

void page_get_entries(struct page *p, uint64_t index, uint32_t limit,
		      unsigned char **entries, uint32_t *size, uint32_t *count)
{
	uint32_t total = 0, n = 0;
	size_t sz;
	uint64_t pos;
	unsigned char *head;

	if (index <= p->prev_index ||
	    index > p->prev_index + sc_array_size(p->entries)) {
		*entries = NULL;
		*size = 0;
		*count = 0;
		return;
	}

	pos = index - p->prev_index - 1;
	head = p->entries[pos];

	sz = sc_array_size(p->entries);
	for (; pos < sz; pos++) {
		n++;
		total += entry_len(p->entries[pos]);
		if (total >= limit) {
			break;
		}
	}

	*entries = head;
	*size = total;
	*count = n;
}

void page_remove_after(struct page *p, uint64_t index)
{
	uint32_t pos;
	unsigned char *entry;

	if (index <= p->prev_index) {
		page_clear(p, p->prev_index);
		return;
	}

	entry = page_entry_at(p, index + 1);
	if (entry == NULL) {
		return;
	}

	pos = (uint32_t) (entry - p->map.ptr);
	sc_buf_set_wpos(&p->buf, pos);
	sc_buf_set_32(&p->buf, PAGE_END_MARK);

	p->flush_pos = sc_min(pos - 4, p->flush_pos);
	page_fsync(p, index);

	while (p->prev_index + sc_array_size(p->entries) > index) {
		sc_array_del_last(p->entries);
	}
}
