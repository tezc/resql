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

#ifndef RESQL_MSG_H
#define RESQL_MSG_H

#include "sc/sc_buf.h"
#include "sc/sc_list.h"

#include <stdint.h>

#define MSG_CONNECT_TYPE 0x01
#define MSG_RC_LEN	 1u
#define MSG_MAX_SIZE	 (2 * 1000 * 1000 * 1000)

#define MSG_FIXED_HEADER_SIZE 8

#define MSG_SIZE_POS	      0
#define MSG_SIZE_LEN	      4
#define MSG_TYPE_LEN	      1
#define MSG_FIXED_LEN	      (MSG_SIZE_LEN + MSG_TYPE_LEN)
#define MSG_TYPE_POS	      4
#define MSG_SEQ_LEN	      8
#define MSG_READONLY_LEN      1
#define MSG_CLIENT_REQ_HEADER (MSG_FIXED_LEN + MSG_SEQ_LEN + MSG_READONLY_LEN)

#define MSG_RESQL_STR "resql"

extern const char *msg_rc_str[];

// clang-format off

enum msg_flag {
	MSG_FLAG_OK		   = 0x00,
	MSG_FLAG_ERROR		   = 0x01,
	MSG_FLAG_STMT		   = 0x02,
	MSG_FLAG_STMT_ID	   = 0x03,
	MSG_FLAG_STMT_PREPARE	   = 0x04,
	MSG_FLAG_STMT_DEL_PREPARED = 0x05,
	MSG_FLAG_OP		   = 0x06,
	MSG_FLAG_OP_END		   = 0x07,
	MSG_FLAG_ROW		   = 0x08,
	MSG_FLAG_MSG_END	   = 0x09,

};

enum msg_param {
	MSG_PARAM_INTEGER	   = 0x00,
	MSG_PARAM_FLOAT		   = 0x01,
	MSG_PARAM_TEXT		   = 0x02,
	MSG_PARAM_BLOB		   = 0x03,
	MSG_PARAM_NULL		   = 0x04
};

enum msg_bind {
	MSG_BIND_NAME		   = 0x00,
	MSG_BIND_INDEX		   = 0x01,
	MSG_BIND_END		   = 0x02,
};

enum msg_rc {
	MSG_OK			   = 0x00,
	MSG_ERR			   = 0x01,
	MSG_CLUSTER_NAME_MISMATCH  = 0x02,
	MSG_CORRUPT		   = 0x03,
	MSG_UNEXPECTED		   = 0x04,
	MSG_TIMEOUT		   = 0x05,
	MSG_NOT_LEADER		   = 0x06,
	MSG_DISK_FULL		   = 0x07,
};

enum msg_remote {
	MSG_CLIENT		   = 0x00u,
	MSG_NODE		   = 0x01u
};

enum msg_type {
	MSG_CONNECT_REQ		   = 0x00,
	MSG_CONNECT_RESP	   = 0x01,
	MSG_DISCONNECT_REQ	   = 0x02,
	MSG_DISCONNECT_RESP	   = 0x03,
	MSG_CLIENT_REQ		   = 0x04,
	MSG_CLIENT_RESP		   = 0x05,
	MSG_APPEND_REQ		   = 0x06,
	MSG_APPEND_RESP		   = 0x07,
	MSG_PREVOTE_REQ		   = 0x08,
	MSG_PREVOTE_RESP	   = 0x09,
	MSG_REQVOTE_REQ		   = 0x0A,
	MSG_REQVOTE_RESP	   = 0x0B,
	MSG_SNAPSHOT_REQ	   = 0x0C,
	MSG_SNAPSHOT_RESP	   = 0x0D,
	MSG_INFO_REQ		   = 0x0E,
	MSG_SHUTDOWN_REQ	   = 0x0F
};

// clang-format on

struct msg_connect_req {
	uint32_t flags;
	const char *protocol;
	const char *cluster_name;
	const char *name;
};

struct msg_connect_resp {
	enum msg_rc rc;
	uint64_t sequence;
	uint64_t term;
	const char *nodes;
};

struct msg_disconnect_req {
	enum msg_rc rc;
	uint32_t flags;
};

struct msg_disconnect_resp {
	enum msg_rc rc;
	uint32_t flags;
};

struct msg_client_req {
	uint64_t seq;
	bool readonly;
	unsigned char *buf;
	uint32_t len;
};

struct msg_client_resp {
	uint32_t len;
	unsigned char *buf;
};

struct msg_append_req {
	uint64_t term;
	uint64_t prev_log_index;
	uint64_t prev_log_term;
	uint64_t leader_commit;
	uint64_t round;

	unsigned char *buf;
	uint32_t len;
};

struct msg_append_resp {
	uint64_t index;
	uint64_t term;
	uint64_t round;
	bool success;
};

struct msg_prevote_req {
	uint64_t term;
	uint64_t last_log_term;
	uint64_t last_log_index;
};

struct msg_prevote_resp {
	uint64_t term;
	uint64_t index;
	bool granted;
};

struct msg_reqvote_req {
	uint64_t term;
	uint64_t last_log_term;
	uint64_t last_log_index;
};

struct msg_reqvote_resp {
	uint64_t term;
	uint64_t index;
	bool granted;
};

struct msg_snapshot_req {
	uint64_t term;
	uint64_t ss_term;
	uint64_t ss_index;
	uint64_t offset;
	bool done;

	unsigned char *buf;
	uint32_t len;
};

struct msg_snapshot_resp {
	uint64_t term;
	bool success;
	bool done;
};

struct msg_info_req {
	unsigned char *buf;
	uint32_t len;
};

struct msg_shutdown_req {
	bool now;
};

struct msg {
	struct sc_list list;

	union {
		struct msg_connect_req connect_req;
		struct msg_connect_resp connect_resp;
		struct msg_disconnect_req disconnect_req;
		struct msg_disconnect_resp disconnect_resp;
		struct msg_client_req client_req;
		struct msg_client_resp client_resp;
		struct msg_append_req append_req;
		struct msg_append_resp append_resp;
		struct msg_prevote_req prevote_req;
		struct msg_prevote_resp prevote_resp;
		struct msg_reqvote_req reqvote_req;
		struct msg_reqvote_resp reqvote_resp;
		struct msg_snapshot_req snapshot_req;
		struct msg_snapshot_resp snapshot_resp;
		struct msg_info_req info_req;
		struct msg_shutdown_req shutdown_req;
	};

	enum msg_type type;
	uint32_t len;
};

void msg_print(struct msg *msg, struct sc_buf *buf);

bool msg_create_connect_req(struct sc_buf *buf, uint32_t flags,
			    const char *cluster_name, const char *name);

bool msg_create_connect_resp(struct sc_buf *buf, enum msg_rc rc, uint64_t seq,
			     uint64_t term, const char *nodes);

bool msg_create_disconnect_req(struct sc_buf *buf, enum msg_rc rc,
			       uint32_t flags);
bool msg_create_disconnect_resp(struct sc_buf *buf, enum msg_rc rc,
				uint32_t flags);
bool msg_create_client_req(struct sc_buf *buf, bool readonly, uint64_t seq,
			   void *req, uint32_t size);
bool msg_create_client_resp_header(struct sc_buf *buf);
bool msg_finalize_client_resp(struct sc_buf *buf);

bool msg_create_prevote_req(struct sc_buf *buf, uint64_t term,
			    uint64_t last_log_index, uint64_t last_log_term);

bool msg_create_prevote_resp(struct sc_buf *buf, uint64_t term, uint64_t index,
			     bool granted);

bool msg_create_reqvote_req(struct sc_buf *buf, uint64_t term,
			    uint64_t last_log_index, uint64_t last_log_term);

bool msg_create_reqvote_resp(struct sc_buf *buf, uint64_t term, uint64_t index,
			     bool granted);

bool msg_create_append_req(struct sc_buf *buf, uint64_t term,
			   uint64_t prev_log_index, uint64_t prev_log_term,
			   uint64_t leader_commit, uint64_t round,
			   const void *entries, uint32_t size);

bool msg_create_append_resp(struct sc_buf *buf, uint64_t term, uint64_t index,
			    uint64_t query_sequence, bool success);

bool msg_create_snapshot_req(struct sc_buf *buf, uint64_t term,
			     uint64_t ss_term, uint64_t ss_index,
			     uint64_t offset, bool done, const void *data,
			     uint32_t size);

bool msg_create_snapshot_resp(struct sc_buf *buf, uint64_t term, bool success,
			      bool done);

bool msg_create_info_req(struct sc_buf *buf, void *data, uint32_t size);
bool msg_create_shutdown_req(struct sc_buf *buf, bool now);

int msg_len(struct sc_buf *buf);
int msg_parse(struct sc_buf *buf, struct msg *msg);

#endif
