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

#include "msg.h"

#include "rs.h"

#include <assert.h>
#include <inttypes.h>

// clang-format off

static const char *msg_rc_str[] = {
        "MSG_OK",
        "MSG_ERR",
        "MSG_CLUSTER_NAME_MISMATCH",
        "MSG_CORRUPT",
        "MSG_UNEXPECTED",
        "MSG_TIMEOUT",
        "MSG_NOT_LEADER"};

const char *msg_type_str[] = {
        "CONNECT_REQ",
        "CONNECT_RESP",
        "DISCONNECT_REQ",
        "DISCONNECT_RESP",
        "CLIENT_REQ",
        "CLIENT_RESP",
        "APPEND_REQ",
        "APPEND_RESP",
        "PREVOTE_REQ",
        "PREVOTE_RESP",
        "REQVOTE_REQ",
        "REQVOTE_RESP",
        "SNAPSHOT_REQ",
        "SNAPSHOT_RESP",
        "MSG_INFO_REQ",
        "SHUTDOWN_REQ"
};

// clang-format on

bool msg_create_connect_req(struct sc_buf *buf, uint32_t flags,
                            const char *cluster_name, const char *name)
{
    uint32_t head = sc_buf_wpos(buf);
    uint32_t len = MSG_SIZE_LEN + MSG_TYPE_LEN + sc_buf_32_len(flags) +
                   sc_buf_str_len(MSG_RESQL_STR) +
                   sc_buf_str_len(cluster_name) + sc_buf_str_len(name);

    sc_buf_put_32(buf, len);
    sc_buf_put_8(buf, MSG_CONNECT_REQ);
    sc_buf_put_32(buf, flags);
    sc_buf_put_str(buf, MSG_RESQL_STR);
    sc_buf_put_str(buf, cluster_name);
    sc_buf_put_str(buf, name);

    if (!sc_buf_valid(buf)) {
        sc_buf_set_wpos(buf, head);
        return false;
    }

    return true;
}

bool msg_create_connect_resp(struct sc_buf *buf, enum msg_rc rc,
                             uint64_t sequence, uint64_t term,
                             const char *nodes)
{
    uint32_t head = sc_buf_wpos(buf);
    uint32_t len = MSG_SIZE_LEN + MSG_TYPE_LEN + MSG_RC_LEN +
                   sc_buf_64_len(sequence) + +sc_buf_64_len(term) +
                   sc_buf_str_len(nodes);


    sc_buf_put_32(buf, len);
    sc_buf_put_8(buf, MSG_CONNECT_RESP);
    sc_buf_put_8(buf, rc);
    sc_buf_put_64(buf, sequence);
    sc_buf_put_64(buf, term);
    sc_buf_put_str(buf, nodes);

    if (!sc_buf_valid(buf)) {
        sc_buf_set_wpos(buf, head);
        return false;
    }

    return true;
}

bool msg_create_disconnect_req(struct sc_buf *buf, enum msg_rc rc,
                               uint32_t flags)
{
    uint32_t head = sc_buf_wpos(buf);
    uint32_t len = MSG_FIXED_LEN + MSG_RC_LEN + sc_buf_32_len(flags);

    sc_buf_put_32(buf, len);
    sc_buf_put_8(buf, MSG_DISCONNECT_REQ);
    sc_buf_put_8(buf, rc);
    sc_buf_put_32(buf, flags);

    if (!sc_buf_valid(buf)) {
        sc_buf_set_wpos(buf, head);
        return false;
    }

    return true;
}

bool msg_create_disconnect_resp(struct sc_buf *buf, enum msg_rc rc,
                                uint32_t flags)
{
    uint32_t head = sc_buf_wpos(buf);
    uint32_t len = MSG_FIXED_LEN + MSG_RC_LEN + sc_buf_32_len(flags);

    sc_buf_put_32(buf, len);
    sc_buf_put_8(buf, MSG_DISCONNECT_RESP);
    sc_buf_put_8(buf, rc);
    sc_buf_put_32(buf, flags);

    if (!sc_buf_valid(buf)) {
        sc_buf_set_wpos(buf, head);
        return false;
    }

    return true;
}

bool msg_create_client_req(struct sc_buf *buf, bool readonly, uint64_t seq,
                           void *req, uint32_t size)
{
    uint32_t head = sc_buf_wpos(buf);
    uint32_t len = MSG_FIXED_LEN + sc_buf_bool_len(readonly) +
                   sc_buf_64_len(seq) + size;

    sc_buf_put_32(buf, len);
    sc_buf_put_8(buf, MSG_CLIENT_REQ);
    sc_buf_put_bool(buf, readonly);
    sc_buf_put_64(buf, seq);
    sc_buf_put_raw(buf, req, size);

    if (!sc_buf_valid(buf)) {
        sc_buf_set_wpos(buf, head);
        return false;
    }

    return true;
}

bool msg_create_client_resp_header(struct sc_buf *buf)
{
    sc_buf_put_32(buf, 0);
    sc_buf_put_8(buf, MSG_CLIENT_RESP);
    return sc_buf_valid(buf);
}

bool msg_finalize_client_resp(struct sc_buf *buf)
{
    sc_buf_set_32_at(buf, 0, sc_buf_wpos(buf));
    return sc_buf_valid(buf);
}

bool msg_create_prevote_req(struct sc_buf *buf, uint64_t term,
                            uint64_t last_log_index, uint64_t last_log_term)
{
    uint32_t head = sc_buf_wpos(buf);
    uint32_t len = MSG_FIXED_LEN + sc_buf_64_len(term) +
                   sc_buf_64_len(last_log_index) + sc_buf_64_len(last_log_term);

    sc_buf_put_32(buf, len);
    sc_buf_put_8(buf, MSG_PREVOTE_REQ);
    sc_buf_put_64(buf, term);
    sc_buf_put_64(buf, last_log_term);
    sc_buf_put_64(buf, last_log_index);

    if (!sc_buf_valid(buf)) {
        sc_buf_set_wpos(buf, head);
        return false;
    }

    return true;
}

bool msg_create_prevote_resp(struct sc_buf *buf, uint64_t term, uint64_t index,
                             bool granted)
{
    uint32_t head = sc_buf_wpos(buf);
    uint32_t len = MSG_SIZE_LEN + MSG_TYPE_LEN + sc_buf_64_len(term) +
                   sc_buf_64_len(index) + sc_buf_bool_len(granted);

    sc_buf_put_32(buf, len);
    sc_buf_put_8(buf, MSG_PREVOTE_RESP);
    sc_buf_put_64(buf, term);
    sc_buf_put_64(buf, index);
    sc_buf_put_bool(buf, granted);

    if (!sc_buf_valid(buf)) {
        sc_buf_set_wpos(buf, head);
        return false;
    }

    return true;
}

bool msg_create_reqvote_req(struct sc_buf *buf, uint64_t term,
                            uint64_t last_log_index, uint64_t last_log_term)
{
    uint32_t head = sc_buf_wpos(buf);
    uint32_t len = MSG_SIZE_LEN + +MSG_TYPE_LEN + sc_buf_64_len(term) +
                   sc_buf_64_len(last_log_index) + sc_buf_64_len(last_log_term);

    sc_buf_put_32(buf, len);
    sc_buf_put_8(buf, MSG_REQVOTE_REQ);
    sc_buf_put_64(buf, term);
    sc_buf_put_64(buf, last_log_term);
    sc_buf_put_64(buf, last_log_index);

    if (!sc_buf_valid(buf)) {
        sc_buf_set_wpos(buf, head);
        return false;
    }

    return true;
}

bool msg_create_reqvote_resp(struct sc_buf *buf, uint64_t term, uint64_t index,
                             bool granted)
{
    uint32_t head = sc_buf_wpos(buf);
    uint32_t len = MSG_SIZE_LEN + MSG_TYPE_LEN + sc_buf_64_len(term) +
                   sc_buf_64_len(index) + sc_buf_bool_len(granted);

    sc_buf_put_32(buf, len);
    sc_buf_put_8(buf, MSG_REQVOTE_RESP);
    sc_buf_put_64(buf, term);
    sc_buf_put_64(buf, index);
    sc_buf_put_bool(buf, granted);

    if (!sc_buf_valid(buf)) {
        sc_buf_set_wpos(buf, head);
        return false;
    }

    return true;
}

bool msg_create_append_req(struct sc_buf *buf, uint64_t term,
                           uint64_t prev_log_index, uint64_t prev_log_term,
                           uint64_t leader_commit, uint64_t query_sequence,
                           const void *entries, uint32_t size)
{
    uint32_t head = sc_buf_wpos(buf);
    uint32_t len = MSG_FIXED_LEN + sc_buf_64_len(term) +
                   sc_buf_64_len(prev_log_index) +
                   sc_buf_64_len(prev_log_term) + sc_buf_64_len(prev_log_term) +
                   sc_buf_64_len(query_sequence) + size;

    sc_buf_put_32(buf, len);
    sc_buf_put_8(buf, MSG_APPEND_REQ);
    sc_buf_put_64(buf, term);
    sc_buf_put_64(buf, prev_log_index);
    sc_buf_put_64(buf, prev_log_term);
    sc_buf_put_64(buf, leader_commit);
    sc_buf_put_64(buf, query_sequence);
    sc_buf_put_raw(buf, entries, size);

    if (!sc_buf_valid(buf)) {
        sc_buf_set_wpos(buf, head);
        return false;
    }

    return true;
}

bool msg_create_append_resp(struct sc_buf *buf, uint64_t term, uint64_t index,
                            uint64_t query_sequence, bool success)
{
    uint32_t head = sc_buf_wpos(buf);
    uint32_t len = MSG_FIXED_LEN + sc_buf_64_len(term) + sc_buf_64_len(index) +
                   sc_buf_64_len(query_sequence) + sc_buf_bool_len(success);

    sc_buf_put_32(buf, len);
    sc_buf_put_8(buf, MSG_APPEND_RESP);
    sc_buf_put_64(buf, term);
    sc_buf_put_64(buf, index);
    sc_buf_put_64(buf, query_sequence);
    sc_buf_put_bool(buf, success);

    if (!sc_buf_valid(buf)) {
        sc_buf_set_wpos(buf, head);
        return false;
    }

    return true;
}

bool msg_create_snapshot_req(struct sc_buf *buf, uint64_t term,
                             uint64_t ss_term, uint64_t ss_index,
                             uint64_t offset, bool done, const void *data,
                             uint32_t size)
{
    uint32_t head = sc_buf_wpos(buf);
    uint32_t len = MSG_FIXED_LEN + sc_buf_64_len(term) +
                   sc_buf_64_len(ss_term) + sc_buf_64_len(ss_index) +
                   sc_buf_64_len(offset) + sc_buf_bool_len(done) + size;

    sc_buf_put_32(buf, len);
    sc_buf_put_8(buf, MSG_SNAPSHOT_REQ);
    sc_buf_put_64(buf, term);
    sc_buf_put_64(buf, ss_term);
    sc_buf_put_64(buf, ss_index);
    sc_buf_put_64(buf, offset);
    sc_buf_put_bool(buf, done);
    sc_buf_put_raw(buf, data, size);

    if (!sc_buf_valid(buf)) {
        sc_buf_set_wpos(buf, head);
        return false;
    }

    return true;
}

bool msg_create_snapshot_resp(struct sc_buf *buf, uint64_t term, bool success,
                              bool done)
{
    uint32_t head = sc_buf_wpos(buf);
    uint32_t len = MSG_FIXED_LEN + sc_buf_64_len(term) +
                   sc_buf_bool_len(success) + sc_buf_bool_len(done);

    sc_buf_put_32(buf, len);
    sc_buf_put_8(buf, MSG_SNAPSHOT_RESP);
    sc_buf_put_64(buf, term);
    sc_buf_put_bool(buf, success);
    sc_buf_put_bool(buf, done);

    if (!sc_buf_valid(buf)) {
        sc_buf_set_wpos(buf, head);
        return false;
    }

    return true;
}

bool msg_create_info_req(struct sc_buf *buf, void *data, uint32_t size)
{
    uint32_t head = sc_buf_wpos(buf);
    uint32_t len = MSG_FIXED_LEN + size;

    sc_buf_put_32(buf, len);
    sc_buf_put_8(buf, MSG_INFO_REQ);
    sc_buf_put_raw(buf, data, size);

    if (!sc_buf_valid(buf)) {
        sc_buf_set_wpos(buf, head);
        return false;
    }

    return true;
}

bool msg_create_shutdown_req(struct sc_buf *buf, bool now)
{
    uint32_t head = sc_buf_wpos(buf);
    uint32_t len = MSG_FIXED_LEN + sc_buf_bool_len(now);

    sc_buf_put_32(buf, len);
    sc_buf_put_8(buf, MSG_SHUTDOWN_REQ);
    sc_buf_put_bool(buf, now);

    if (!sc_buf_valid(buf)) {
        sc_buf_set_wpos(buf, head);
        return false;
    }

    return true;
}

int msg_len(struct sc_buf *buf)
{
    if (sc_buf_size(buf) < MSG_SIZE_LEN) {
        return -1;
    }

    return sc_buf_peek_32(buf);
}

int msg_parse(struct sc_buf *buf, struct msg *msg)
{
    uint32_t len;
    void *p;
    struct sc_buf tmp;

    if (sc_buf_size(buf) < MSG_SIZE_LEN) {
        return RS_PARTIAL;
    }

    len = sc_buf_peek_32(buf);
    if (len > MSG_MAX_SIZE) {
        return RS_INVALID;
    }

    if (len == 0) {
        return RS_PARTIAL;
    }

    if (len > sc_buf_size(buf)) {
        return RS_PARTIAL;
    }

    p = sc_buf_rbuf(buf);
    sc_buf_mark_read(buf, len);

    tmp = sc_buf_wrap(p, len, SC_BUF_READ);

    msg->len = sc_buf_get_32(&tmp);
    msg->type = (enum msg_type) sc_buf_get_8(&tmp);

    switch (msg->type) {
    case MSG_CONNECT_REQ:
        msg->connect_req.flags = sc_buf_get_32(&tmp);
        msg->connect_req.protocol = sc_buf_get_str(&tmp);
        msg->connect_req.cluster_name = sc_buf_get_str(&tmp);
        msg->connect_req.name = sc_buf_get_str(&tmp);
        break;

    case MSG_CONNECT_RESP:
        msg->connect_resp.rc = (enum msg_rc) sc_buf_get_8(&tmp);
        msg->connect_resp.sequence = sc_buf_get_64(&tmp);
        msg->connect_resp.term = sc_buf_get_64(&tmp);
        msg->connect_resp.nodes = sc_buf_get_str(&tmp);
        break;

    case MSG_DISCONNECT_REQ:
        msg->disconnect_req.rc = (enum msg_rc) sc_buf_get_8(&tmp);
        msg->disconnect_req.flags = sc_buf_get_32(&tmp);
        break;

    case MSG_DISCONNECT_RESP:
        msg->disconnect_resp.rc = (enum msg_rc) sc_buf_get_8(&tmp);
        msg->disconnect_resp.flags = sc_buf_get_32(&tmp);
        break;

    case MSG_CLIENT_REQ:
        msg->client_req.readonly = sc_buf_get_8(&tmp);
        msg->client_req.seq = sc_buf_get_64(&tmp);
        msg->client_req.buf = sc_buf_rbuf(&tmp);
        msg->client_req.len = sc_buf_size(&tmp);
        sc_buf_mark_read(&tmp, msg->client_req.len);
        break;

    case MSG_CLIENT_RESP:
        msg->client_resp.buf = sc_buf_rbuf(&tmp);
        msg->client_resp.len = sc_buf_size(&tmp);
        break;

    case MSG_APPEND_REQ:
        msg->append_req.term = sc_buf_get_64(&tmp);
        msg->append_req.prev_log_index = sc_buf_get_64(&tmp);
        msg->append_req.prev_log_term = sc_buf_get_64(&tmp);
        msg->append_req.leader_commit = sc_buf_get_64(&tmp);
        msg->append_req.round = sc_buf_get_64(&tmp);
        msg->append_req.buf = sc_buf_rbuf(&tmp);
        msg->append_req.len = sc_buf_size(&tmp);
        sc_buf_mark_read(&tmp, msg->append_req.len);
        break;

    case MSG_APPEND_RESP:
        msg->append_resp.term = sc_buf_get_64(&tmp);
        msg->append_resp.index = sc_buf_get_64(&tmp);
        msg->append_resp.round = sc_buf_get_64(&tmp);
        msg->append_resp.success = sc_buf_get_bool(&tmp);
        break;

    case MSG_PREVOTE_REQ:
        msg->prevote_req.term = sc_buf_get_64(&tmp);
        msg->prevote_req.last_log_term = sc_buf_get_64(&tmp);
        msg->prevote_req.last_log_index = sc_buf_get_64(&tmp);
        break;

    case MSG_PREVOTE_RESP:
        msg->prevote_resp.term = sc_buf_get_64(&tmp);
        msg->prevote_resp.index = sc_buf_get_64(&tmp);
        msg->prevote_resp.granted = sc_buf_get_bool(&tmp);
        break;

    case MSG_REQVOTE_REQ:
        msg->reqvote_req.term = sc_buf_get_64(&tmp);
        msg->reqvote_req.last_log_term = sc_buf_get_64(&tmp);
        msg->reqvote_req.last_log_index = sc_buf_get_64(&tmp);
        break;

    case MSG_REQVOTE_RESP:
        msg->reqvote_resp.term = sc_buf_get_64(&tmp);
        msg->reqvote_resp.index = sc_buf_get_64(&tmp);
        msg->reqvote_resp.granted = sc_buf_get_bool(&tmp);
        break;

    case MSG_SNAPSHOT_REQ:
        msg->snapshot_req.term = sc_buf_get_64(&tmp);
        msg->snapshot_req.ss_term = sc_buf_get_64(&tmp);
        msg->snapshot_req.ss_index = sc_buf_get_64(&tmp);
        msg->snapshot_req.offset = sc_buf_get_64(&tmp);
        msg->snapshot_req.done = sc_buf_get_bool(&tmp);
        msg->snapshot_req.buf = sc_buf_rbuf(&tmp);
        msg->snapshot_req.len = sc_buf_size(&tmp);
        sc_buf_mark_read(&tmp, msg->snapshot_req.len);
        break;

    case MSG_SNAPSHOT_RESP:
        msg->snapshot_resp.term = sc_buf_get_64(&tmp);
        msg->snapshot_resp.success = sc_buf_get_bool(&tmp);
        msg->snapshot_resp.done = sc_buf_get_bool(&tmp);
        break;

    case MSG_INFO_REQ:
        msg->info_req.buf = sc_buf_rbuf(&tmp);
        msg->info_req.len = sc_buf_size(&tmp);
        sc_buf_mark_read(&tmp, msg->info_req.len);
        break;

    case MSG_SHUTDOWN_REQ:
        msg->shutdown_req.now = sc_buf_get_bool(&tmp);
        break;

    default:
        break;
    }

    return sc_buf_valid(&tmp) ? RS_OK : RS_INVALID;
}

static void msg_print_connect_req(struct msg *msg, struct sc_buf *buf)
{
    struct msg_connect_req *m = &msg->connect_req;

    sc_buf_put_text(buf, "| %-15s | %s \n", "Protocol", m->protocol);
    sc_buf_put_text(buf, "| %-15s | %"PRIu32" \n", "Flags", m->flags);
    sc_buf_put_text(buf, "| %-15s | %s \n", "Cluster name", m->cluster_name);
    sc_buf_put_text(buf, "| %-15s | %s \n", "Name", m->name);
}

static void msg_print_connect_resp(struct msg *msg, struct sc_buf *buf)
{
    struct msg_connect_resp *m = &msg->connect_resp;

    sc_buf_put_text(buf, "| %-15s | %s   \n", "Rc", msg_rc_str[m->rc]);
    sc_buf_put_text(buf, "| %-15s | %"PRIu64" \n", "Sequence", m->sequence);
    sc_buf_put_text(buf, "| %-15s | %"PRIu64" \n", "Term", m->term);
    sc_buf_put_text(buf, "| %-15s | %s \n", "Nodes", m->nodes);
}

static void msg_print_disconnect_req(struct msg *msg, struct sc_buf *buf)
{
    struct msg_disconnect_req *m = &msg->disconnect_req;

    sc_buf_put_text(buf, "| %-15s | %s   \n", "Rc ", msg_rc_str[m->rc]);
    sc_buf_put_text(buf, "| %-15s | %"PRIu32" \n", "Flags ", m->flags);
}

static void msg_print_disconnect_resp(struct msg *msg, struct sc_buf *buf)
{
    struct msg_disconnect_resp *m = &msg->disconnect_resp;

    sc_buf_put_text(buf, "| %-15s | %s  \n", "Rc", msg_rc_str[m->rc]);
    sc_buf_put_text(buf, "| %-15s | %"PRIu32" \n", "Flags", m->flags);
}

static void msg_print_client_req(struct msg *msg, struct sc_buf *buf)
{
    struct msg_client_req *m = &msg->client_req;

    sc_buf_put_text(buf, "| %-15s | %s \n", "Readonly",
                    m->readonly ? "true" : "false");
    sc_buf_put_text(buf, "| %-15s | %"PRIu64" \n", "Sequence", m->seq);
    sc_buf_put_text(buf, "| %-15s | %"PRIu32" \n", "Length", m->len);
}

static void msg_print_client_resp(struct msg *msg, struct sc_buf *buf)
{
    struct msg_client_resp *m = &msg->client_resp;

    sc_buf_put_text(buf, "| %-15s | %"PRIu32" \n", "Length", m->len);
}

static void msg_print_append_req(struct msg *msg, struct sc_buf *buf)
{
    struct msg_append_req *m = &msg->append_req;

    sc_buf_put_text(buf, "| %-15s | %"PRIu64" \n", "Term", m->term);
    sc_buf_put_text(buf, "| %-15s | %"PRIu64" \n", "Prev Log Index",
                    m->prev_log_index);
    sc_buf_put_text(buf, "| %-15s | %"PRIu64" \n", "Prev Log Term",
                    m->prev_log_term);
    sc_buf_put_text(buf, "| %-15s | %"PRIu64" \n", "Leader Commit",
                    m->leader_commit);
    sc_buf_put_text(buf, "| %-15s | %"PRIu64" \n", "Round", m->round);
    sc_buf_put_text(buf, "| %-15s | %"PRIu32" \n", "Data len", m->len);
}

static void msg_print_append_resp(struct msg *msg, struct sc_buf *buf)
{
    struct msg_append_resp *m = &msg->append_resp;

    sc_buf_put_text(buf, "| %-15s | %"PRIu64" \n", "Term", m->term);
    sc_buf_put_text(buf, "| %-15s | %"PRIu64" \n", "Index", m->index);
    sc_buf_put_text(buf, "| %-15s | %"PRIu64" \n", "Round", m->round);
    sc_buf_put_text(buf, "| %-15s | %s \n", "Success",
                    (m->success) ? "true" : "false");
}

static void msg_print_prevote_req(struct msg *msg, struct sc_buf *buf)
{
    struct msg_prevote_req *m = &msg->prevote_req;

    sc_buf_put_text(buf, "| %-15s | %"PRIu64" \n", "Term", m->term);
    sc_buf_put_text(buf, "| %-15s | %"PRIu64" \n", "Last Log Index",
                    m->last_log_index);
    sc_buf_put_text(buf, "| %-15s | %"PRIu64" \n", "Last Log Term",
                    m->last_log_term);
}

static void msg_print_prevote_resp(struct msg *msg, struct sc_buf *buf)
{
    struct msg_prevote_resp *m = &msg->prevote_resp;

    sc_buf_put_text(buf, "| %-15s | %"PRIu64" \n", "Term", m->term);
    sc_buf_put_text(buf, "| %-15s | %"PRIu64" \n", "Index", m->index);
    sc_buf_put_text(buf, "| %-15s | %s \n", "Granted",
                    m->granted ? "true" : "false");
}

static void msg_print_reqvote_req(struct msg *msg, struct sc_buf *buf)
{
    struct msg_reqvote_req *m = &msg->reqvote_req;

    sc_buf_put_text(buf, "| %-15s | %"PRIu64" \n", "Term", m->term);
    sc_buf_put_text(buf, "| %-15s | %"PRIu64" \n", "Last Log Index",
                    m->last_log_index);
    sc_buf_put_text(buf, "| %-15s | %"PRIu64" \n", "Last Log Term",
                    m->last_log_term);
}

static void msg_print_reqvote_resp(struct msg *msg, struct sc_buf *buf)
{
    struct msg_reqvote_resp *m = &msg->reqvote_resp;

    sc_buf_put_text(buf, "| %-15s | %"PRIu64" \n", "Term", m->term);
    sc_buf_put_text(buf, "| %-15s | %"PRIu64" \n", "Index", m->index);
    sc_buf_put_text(buf, "| %-15s | %s \n", "Granted",
                    m->granted ? "true" : "false");
}

static void msg_print_snapshot_req(struct msg *msg, struct sc_buf *buf)
{
    struct msg_snapshot_req *m = &msg->snapshot_req;

    sc_buf_put_text(buf, "| %-15s | %"PRIu64" \n", "Term", m->term);
    sc_buf_put_text(buf, "| %-15s | %"PRIu64" \n", "SS term", m->ss_term);
    sc_buf_put_text(buf, "| %-15s | %"PRIu64" \n", "SS index", m->ss_index);
    sc_buf_put_text(buf, "| %-15s | %"PRIu64" \n", "Offset", m->offset);
    sc_buf_put_text(buf, "| %-15s | %"PRIu32" \n", "Data len", m->len);
}

static void msg_print_snapshot_resp(struct msg *msg, struct sc_buf *buf)
{
    struct msg_snapshot_resp *m = &msg->snapshot_resp;

    sc_buf_put_text(buf, "| %-15s | %"PRIu64" \n", "Term", m->term);
    sc_buf_put_text(buf, "| %-15s | %s \n", "Success",
                    m->success ? "true" : "false");
    sc_buf_put_text(buf, "| %-15s | %s \n", "Done", m->done ? "true" : "false");
}

static void msg_print_info_req(struct msg *msg, struct sc_buf *buf)
{
    struct msg_info_req *m = &msg->info_req;
    sc_buf_put_text(buf, "| %-15s | %"PRIu32" \n", "Data len", m->len);
}

static void msg_print_shutdown_req(struct msg *msg, struct sc_buf *buf)
{
    struct msg_shutdown_req *m = &msg->shutdown_req;
    sc_buf_put_text(buf, "| %-15s | %s \n", "Now", m->now ? "true" : "false");
}

void msg_print(struct msg *msg, struct sc_buf *buf)
{
    const char *msg_name = msg_type_str[msg->type];

    sc_buf_put_text(buf, "\n");
    sc_buf_put_text(buf, "%s \n", "------------------------------------");
    sc_buf_put_text(buf, "| %s (%"PRIu32" bytes) \n", msg_name, msg->len);
    sc_buf_put_text(buf, "%s \n", "------------------------------------");

    switch (msg->type) {
    case MSG_CONNECT_REQ:
        msg_print_connect_req(msg, buf);
        break;
    case MSG_CONNECT_RESP:
        msg_print_connect_resp(msg, buf);
        break;
    case MSG_DISCONNECT_REQ:
        msg_print_disconnect_req(msg, buf);
        break;
    case MSG_DISCONNECT_RESP:
        msg_print_disconnect_resp(msg, buf);
        break;
    case MSG_CLIENT_REQ:
        msg_print_client_req(msg, buf);
        break;
    case MSG_CLIENT_RESP:
        msg_print_client_resp(msg, buf);
        break;
    case MSG_APPEND_REQ:
        msg_print_append_req(msg, buf);
        break;
    case MSG_APPEND_RESP:
        msg_print_append_resp(msg, buf);
        break;
    case MSG_PREVOTE_REQ:
        msg_print_prevote_req(msg, buf);
        break;
    case MSG_PREVOTE_RESP:
        msg_print_prevote_resp(msg, buf);
        break;
    case MSG_REQVOTE_REQ:
        msg_print_reqvote_req(msg, buf);
        break;
    case MSG_REQVOTE_RESP:
        msg_print_reqvote_resp(msg, buf);
        break;
    case MSG_SNAPSHOT_REQ:
        msg_print_snapshot_req(msg, buf);
        break;
    case MSG_SNAPSHOT_RESP:
        msg_print_snapshot_resp(msg, buf);
        break;
    case MSG_INFO_REQ:
        msg_print_info_req(msg, buf);
        break;
    case MSG_SHUTDOWN_REQ:
        msg_print_shutdown_req(msg, buf);
        break;

    default:
        assert(0);
    }

    sc_buf_put_text(buf, "%s \n", "------------------------------------ \n");
}
