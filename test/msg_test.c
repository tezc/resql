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

#include "msg.h"
#include "test_util.h"

static void connectreq_test()
{
	struct msg msg;
	struct sc_buf buf;
	struct sc_buf buf2;

	sc_buf_init(&buf, 1024);
	sc_buf_init(&buf2, 1024);

	msg_create_connect_req(&buf, 0, "test", "node");
	msg_parse(&buf, &msg);

	rs_assert(strcmp(msg.connect_req.cluster_name, "test") == 0);
	rs_assert(strcmp(msg.connect_req.name, "node") == 0);
	rs_assert(strcmp(msg.connect_req.protocol, "resql") == 0);
	rs_assert(msg.connect_req.flags == 0);

	msg_print(&msg, &buf2);

	sc_buf_term(&buf);
	sc_buf_term(&buf2);
}

static void connectresp_test()
{
	struct msg msg;
	struct sc_buf buf;
	struct sc_buf buf2;

	sc_buf_init(&buf, 1024);
	sc_buf_init(&buf2, 1024);

	msg_create_connect_resp(&buf, MSG_CLUSTER_NAME_MISMATCH, 100, 100,
				"node");
	msg_parse(&buf, &msg);

	rs_assert(msg.connect_resp.rc == MSG_CLUSTER_NAME_MISMATCH);
	rs_assert(msg.connect_resp.sequence == 100);
	rs_assert(msg.connect_resp.term == 100);
	rs_assert(strcmp(msg.connect_resp.nodes, "node") == 0);

	msg_print(&msg, &buf2);

	sc_buf_term(&buf);
	sc_buf_term(&buf2);
}

static void disconnectreq_test()
{
	struct msg msg;
	struct sc_buf buf;
	struct sc_buf buf2;

	sc_buf_init(&buf, 1024);
	sc_buf_init(&buf2, 1024);

	msg_create_disconnect_req(&buf, MSG_CLUSTER_NAME_MISMATCH, 12);
	msg_parse(&buf, &msg);

	rs_assert(msg.disconnect_req.rc == MSG_CLUSTER_NAME_MISMATCH);
	rs_assert(msg.disconnect_req.flags == 12);

	msg_print(&msg, &buf2);

	sc_buf_term(&buf);
	sc_buf_term(&buf2);
}

static void disconnectresp_test()
{
	struct msg msg;
	struct sc_buf buf;
	struct sc_buf buf2;

	sc_buf_init(&buf, 1024);
	sc_buf_init(&buf2, 1024);

	msg_create_disconnect_resp(&buf, MSG_CLUSTER_NAME_MISMATCH, 12);
	msg_parse(&buf, &msg);

	rs_assert(msg.disconnect_resp.rc == MSG_CLUSTER_NAME_MISMATCH);
	rs_assert(msg.disconnect_resp.flags == 12);

	msg_print(&msg, &buf2);

	sc_buf_term(&buf);
	sc_buf_term(&buf2);
}

static void clientreq_test()
{
	struct msg msg;
	struct sc_buf buf;
	struct sc_buf buf2;

	sc_buf_init(&buf, 1024);
	sc_buf_init(&buf2, 1024);

	msg_create_client_req(&buf, false, 13, "test", 5);
	msg_parse(&buf, &msg);

	rs_assert(msg.client_req.readonly == false);
	rs_assert(msg.client_req.seq == 13);
	rs_assert(strcmp((char *) msg.client_req.buf, "test") == 0);

	msg_print(&msg, &buf2);

	sc_buf_term(&buf);
	sc_buf_term(&buf2);
}

static void clientresp_test()
{
	struct msg msg;
	struct sc_buf buf;
	struct sc_buf buf2;
	struct sc_buf tmp;

	sc_buf_init(&buf, 1024);
	sc_buf_init(&buf2, 1024);

	msg_create_client_resp_header(&buf);
	sc_buf_put_str(&buf, "test");
	msg_finalize_client_resp(&buf);
	msg_parse(&buf, &msg);

	rs_assert(msg.client_resp.len == 9);
	tmp = sc_buf_wrap(msg.client_resp.buf, msg.client_resp.len,
			  SC_BUF_READ);
	rs_assert(strcmp(sc_buf_get_str(&tmp), "test") == 0);

	msg_print(&msg, &buf2);

	sc_buf_term(&buf);
	sc_buf_term(&buf2);
}

static void appendreq_test()
{
	struct msg msg;
	struct sc_buf buf;
	struct sc_buf buf2;

	sc_buf_init(&buf, 1024);
	sc_buf_init(&buf2, 1024);

	msg_create_append_req(&buf, 1, 2, 3, 4, 5, "test", 5);
	msg_parse(&buf, &msg);

	rs_assert(msg.append_req.term == 1);
	rs_assert(msg.append_req.prev_log_index == 2);
	rs_assert(msg.append_req.prev_log_term == 3);
	rs_assert(msg.append_req.leader_commit == 4);
	rs_assert(msg.append_req.round == 5);
	rs_assert(strcmp((char *) msg.append_req.buf, "test") == 0);

	msg_print(&msg, &buf2);

	sc_buf_term(&buf);
	sc_buf_term(&buf2);
}

static void appendresp_test()
{
	struct msg msg;
	struct sc_buf buf;
	struct sc_buf buf2;

	sc_buf_init(&buf, 1024);
	sc_buf_init(&buf2, 1024);

	msg_create_append_resp(&buf, 1, 2, 3, true);
	msg_parse(&buf, &msg);

	rs_assert(msg.append_resp.term == 1);
	rs_assert(msg.append_resp.index == 2);
	rs_assert(msg.append_resp.round == 3);
	rs_assert(msg.append_resp.success == true);

	msg_print(&msg, &buf2);

	sc_buf_term(&buf);
	sc_buf_term(&buf2);
}

static void prevotereq_test()
{
	struct msg msg;
	struct sc_buf buf;
	struct sc_buf buf2;

	sc_buf_init(&buf, 1024);
	sc_buf_init(&buf2, 1024);

	msg_create_prevote_req(&buf, 1, 2, 3);
	msg_parse(&buf, &msg);

	rs_assert(msg.prevote_req.term == 1);
	rs_assert(msg.prevote_req.last_log_index == 2);
	rs_assert(msg.prevote_req.last_log_term == 3);

	msg_print(&msg, &buf2);

	sc_buf_term(&buf);
	sc_buf_term(&buf2);
}

static void prevoteresp_test()
{
	struct msg msg;
	struct sc_buf buf;
	struct sc_buf buf2;

	sc_buf_init(&buf, 1024);
	sc_buf_init(&buf2, 1024);

	msg_create_prevote_resp(&buf, 1, 2, true);
	msg_parse(&buf, &msg);

	rs_assert(msg.prevote_resp.term == 1);
	rs_assert(msg.prevote_resp.index == 2);
	rs_assert(msg.prevote_resp.granted == true);

	msg_print(&msg, &buf2);

	sc_buf_term(&buf);
	sc_buf_term(&buf2);
}

static void reqvotereq_test()
{
	struct msg msg;
	struct sc_buf buf;
	struct sc_buf buf2;

	sc_buf_init(&buf, 1024);
	sc_buf_init(&buf2, 1024);

	msg_create_reqvote_req(&buf, 1, 2, 3);
	msg_parse(&buf, &msg);

	rs_assert(msg.reqvote_req.term == 1);
	rs_assert(msg.reqvote_req.last_log_index == 2);
	rs_assert(msg.reqvote_req.last_log_term == 3);

	msg_print(&msg, &buf2);

	sc_buf_term(&buf);
	sc_buf_term(&buf2);
}

static void reqvoteresp_test()
{
	struct msg msg;
	struct sc_buf buf;
	struct sc_buf buf2;

	sc_buf_init(&buf, 1024);
	sc_buf_init(&buf2, 1024);

	msg_create_reqvote_resp(&buf, 1, 2, true);
	msg_parse(&buf, &msg);

	rs_assert(msg.reqvote_resp.term == 1);
	rs_assert(msg.reqvote_resp.index == 2);
	rs_assert(msg.reqvote_resp.granted == true);

	msg_print(&msg, &buf2);

	sc_buf_term(&buf);
	sc_buf_term(&buf2);
}

static void snapshotreq_test()
{
	struct msg msg;
	struct sc_buf buf;
	struct sc_buf buf2;

	sc_buf_init(&buf, 1024);
	sc_buf_init(&buf2, 1024);

	msg_create_snapshot_req(&buf, 1, 2, 3, 4, 5, true, "test", 5);
	msg_parse(&buf, &msg);

	rs_assert(msg.snapshot_req.term == 1);
	rs_assert(msg.snapshot_req.round == 2);
	rs_assert(msg.snapshot_req.ss_term == 3);
	rs_assert(msg.snapshot_req.ss_index == 4);
	rs_assert(msg.snapshot_req.offset == 5);
	rs_assert(msg.snapshot_req.done == true);
	rs_assert(strcmp((char *) msg.snapshot_req.buf, "test") == 0);

	msg_print(&msg, &buf2);

	sc_buf_term(&buf);
	sc_buf_term(&buf2);
}

static void snapshotresp_test()
{
	struct msg msg;
	struct sc_buf buf;
	struct sc_buf buf2;

	sc_buf_init(&buf, 1024);
	sc_buf_init(&buf2, 1024);

	msg_create_snapshot_resp(&buf, 1, 3, true, false);
	msg_parse(&buf, &msg);

	rs_assert(msg.snapshot_resp.term == 1);
	rs_assert(msg.snapshot_resp.round == 3);
	rs_assert(msg.snapshot_resp.success == true);
	rs_assert(msg.snapshot_resp.done == false);

	msg_print(&msg, &buf2);

	sc_buf_term(&buf);
	sc_buf_term(&buf2);
}

static void inforeq_test()
{
	struct msg msg;
	struct sc_buf buf;
	struct sc_buf buf2;

	sc_buf_init(&buf, 1024);
	sc_buf_init(&buf2, 1024);

	msg_create_info_req(&buf, "test", 5);
	msg_parse(&buf, &msg);

	rs_assert(strcmp((char *) msg.info_req.buf, "test") == 0);

	msg_print(&msg, &buf2);

	sc_buf_term(&buf);
	sc_buf_term(&buf2);
}

static void shutdownreq_test()
{
	struct msg msg;
	struct sc_buf buf;
	struct sc_buf buf2;

	sc_buf_init(&buf, 1024);
	sc_buf_init(&buf2, 1024);

	msg_create_shutdown_req(&buf, false);
	msg_parse(&buf, &msg);

	rs_assert(msg.shutdown_req.now == false);

	msg_print(&msg, &buf2);

	sc_buf_term(&buf);
	sc_buf_term(&buf2);
}

int main(void)
{
	test_execute(connectreq_test);
	test_execute(connectresp_test);
	test_execute(disconnectreq_test);
	test_execute(disconnectresp_test);
	test_execute(clientreq_test);
	test_execute(clientresp_test);
	test_execute(appendreq_test);
	test_execute(appendresp_test);
	test_execute(prevotereq_test);
	test_execute(prevoteresp_test);
	test_execute(reqvotereq_test);
	test_execute(reqvoteresp_test);
	test_execute(snapshotreq_test);
	test_execute(snapshotresp_test);
	test_execute(inforeq_test);
	test_execute(shutdownreq_test);

	return 0;
}
