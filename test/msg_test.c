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

	msg_create_snapshot_req(&buf, 1, 2, 3, 4, true, "test", 5);
	msg_parse(&buf, &msg);

	rs_assert(msg.snapshot_req.term == 1);
	rs_assert(msg.snapshot_req.ss_term == 2);
	rs_assert(msg.snapshot_req.ss_index == 3);
	rs_assert(msg.snapshot_req.offset == 4);
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

	msg_create_snapshot_resp(&buf, 1, true, false);
	msg_parse(&buf, &msg);

	rs_assert(msg.snapshot_resp.term == 1);
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
