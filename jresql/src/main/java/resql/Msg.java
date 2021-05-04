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

package resql;

abstract class Msg {
    public static final int MSG_LEN_SIZE = 4;

    public static final byte MSG_OK = 0;
    public static final byte MSG_ERR = 1;
    public static final byte MSG_CLUSTER_NAME_MISMATCH = 2;
    public static final byte MSG_CORRUPT = 3;
    public static final byte MSG_UNEXPECTED = 4;
    public static final byte MSG_TIMEOUT = 5;
    public static final byte MSG_NOT_LEADER = 6;
    public static final byte MSG_DISK_FULL = 7;
    public static final byte CONNECT_REQ = 0;
    public static final byte CONNECT_RESP = 1;
    public static final byte DISCONNECT_REQ = 2;
    public static final byte DISCONNECT_RESP = 3;
    public static final byte CLIENT_REQ = 4;
    public static final byte CLIENT_RESP = 5;
    private static final byte REMOTE_TYPE_CLIENT = 0x00;

    public static final byte PARAM_INTEGER = 0;
    public static final byte PARAM_FLOAT = 1;
    public static final byte PARAM_TEXT = 2;
    public static final byte PARAM_BLOB = 3;
    public static final byte PARAM_NULL = 4;

    public static final byte BIND_NAME = 0;
    public static final byte BIND_INDEX = 1;
    public static final byte BIND_END = 2;

    public static final byte FLAG_OK = 0;
    public static final byte FLAG_ERROR = 1;
    public static final byte FLAG_STMT = 2;
    public static final byte FLAG_STMT_ID = 3;
    public static final byte FLAG_STMT_PREPARE = 4;
    public static final byte FLAG_STMT_DEL_PREPARED = 5;
    public static final byte FLAG_OP = 6;
    public static final byte FLAG_OP_END = 7;
    public static final byte FLAG_ROW = 8;
    public static final byte FLAG_MSG_END = 9;

    public static final int CLIENT_REQ_HEADER = 14;

    public static String rcToText(int rc) {

        switch (rc) {
            case MSG_OK:
                return "ok";
            case MSG_ERR:
                return "err";
            case MSG_CLUSTER_NAME_MISMATCH:
                return "cluster name mismatch";
            case MSG_CORRUPT:
                return "corrupt";
            case MSG_TIMEOUT:
                return "timeout";
            case MSG_NOT_LEADER:
                return "not leader";
            case MSG_DISK_FULL:
                return "disk is full";
            default:
                return "unknown rc";

        }
    }

    public static int remaining(RawBuffer buf) {
        int len = buf.getInt();
        if (len > buf.remaining() + buf.position()) {
            buf.position(0);
            return len;
        }

        return 0;
    }

    public static void encodeConnectReq(RawBuffer buf, String clusterName,
            String name) {

        final int length = RawBuffer.intLen(MSG_LEN_SIZE) + RawBuffer.byteLen(
                CONNECT_REQ) + RawBuffer.intLen(REMOTE_TYPE_CLIENT) +
                RawBuffer.stringLen("resql") + RawBuffer.stringLen(
                clusterName) + RawBuffer.stringLen(name);

        buf.putInt(length);
        buf.put(CONNECT_REQ);
        buf.putInt(REMOTE_TYPE_CLIENT);
        buf.putString("resql");
        buf.putString(clusterName);
        buf.putString(name);
        buf.flip();
    }

    public static void encodeDisconnectReq(RawBuffer buf, byte rc, int flags) {
        final int len = RawBuffer.intLen(MSG_LEN_SIZE) + RawBuffer.byteLen(
                DISCONNECT_REQ) + RawBuffer.byteLen(rc) + RawBuffer.intLen(
                flags);

        buf.putInt(len);
        buf.put(DISCONNECT_REQ);
        buf.put(rc);
        buf.putInt(flags);
        buf.flip();
    }

    public static void reserveClientReqHeader(RawBuffer buf) {
        buf.position(CLIENT_REQ_HEADER);
    }

    public static void encodeClientReq(RawBuffer buf, boolean readonly,
            long seq) {
        int pos = buf.position();

        buf.position(0);
        buf.putInt(pos);
        buf.put(Msg.CLIENT_REQ);
        buf.putBoolean(readonly);
        buf.putLong(seq);
        buf.position(pos);
    }
}