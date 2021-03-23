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

package resql;

abstract class Msg {
    public static final int MSG_LEN_SIZE = 4;

    public static final byte MSG_OK = 0;
    public static final byte MSG_CLUSTER_NAME_MISMATCH = 2;
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
    public static final byte PARAM_NAME = 5;
    public static final byte PARAM_INDEX = 6;

    public static final byte FLAG_OK = 0;
    public static final byte FLAG_ERROR = 1;
    public static final byte FLAG_DONE = 2;
    public static final byte FLAG_STMT = 3;
    public static final byte FLAG_STMT_ID = 4;
    public static final byte FLAG_STMT_PREPARE = 5;
    public static final byte FLAG_STMT_DEL_PREPARED = 6;
    public static final byte FLAG_ROW = 7;
    public static final byte FLAG_END = 8;

    public static final int CLIENT_REQ_HEADER = 14;

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

        final int length = RawBuffer.intLen(MSG_LEN_SIZE) +
                RawBuffer.byteLen(CONNECT_REQ) +
                RawBuffer.intLen(REMOTE_TYPE_CLIENT) +
                RawBuffer.stringLen("resql") +
                RawBuffer.stringLen(clusterName) + RawBuffer.stringLen(name);

        buf.putInt(length);
        buf.put(CONNECT_REQ);
        buf.putInt(REMOTE_TYPE_CLIENT);
        buf.putString("resql");
        buf.putString(clusterName);
        buf.putString(name);
        buf.flip();
    }

    public static void encodeDisconnectReq(RawBuffer buf, byte rc, int flags) {
        final int len = RawBuffer.intLen(MSG_LEN_SIZE) +
                RawBuffer.byteLen(DISCONNECT_REQ) + RawBuffer.byteLen(rc) +
                RawBuffer.intLen(flags);

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