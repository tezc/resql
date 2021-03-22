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

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import static java.nio.charset.StandardCharsets.UTF_8;

class RawBuffer {
    private ByteBuffer buf;

    public RawBuffer(int size) {
        buf = ByteBuffer.allocateDirect(size);
        buf.order(ByteOrder.LITTLE_ENDIAN);
    }

    public RawBuffer(ByteBuffer buf) {
        this.buf = buf;
        buf.order(ByteOrder.LITTLE_ENDIAN);
    }

    public void reserve(int len) {
        if (buf.remaining() < len) {
            int count = buf.capacity() - buf.remaining();
            int size = ((count + len + 1024) / 1024) * 1024;
            ByteBuffer b = ByteBuffer.allocateDirect(size);
            b.order(ByteOrder.LITTLE_ENDIAN);
            buf.flip();
            b.put(buf);
            buf = b;
        }
    }

    public int cap() {
        return buf.capacity();
    }

    public ByteBuffer backend() {
        return buf;
    }

    public void put(byte value) {
        reserve(1);
        buf.put(value);
    }

    public void put(int b) {
        put((byte) b);
    }

    public void put(byte[] b, int offset, int len) {
        reserve(len);
        buf.put(b, offset, len);
    }

    public void put(RawBuffer src) {
        reserve(src.remaining());
        buf.put(src.backend());
    }

    public void put(int i, byte value) {
        buf.put(i, value);
    }


    public void putBoolean(boolean b) {
        reserve(1);
        buf.put((byte) (b ? 1 : 0));
    }


    public void putInt(int value) {
        reserve(4);
        buf.putInt(value);
    }

    public void writeInt(int index, int value) {
        buf.putInt(index, value);
    }

    public void putLong(long value) {
        reserve(8);
        buf.putLong(value);
    }

    public void writeLong(int index, long value) {
        buf.putLong(index, value);
    }

    public void putDouble(double value) {
        reserve(8);
        buf.putDouble(value);
    }

    public double getDouble() {
        return buf.getDouble();
    }


    public void putString(String value) {
        if (value == null) {
            putInt(-1);
            return;
        }

        byte[] str = value.getBytes(UTF_8);

        putInt(str.length);

        reserve(str.length + 1);
        buf.put(str);
        buf.put((byte) '\0');
    }

    public void putBlob(ByteBuffer blob) {
        putInt(blob.remaining());
        put(blob);
    }

    public void putBlob(byte[] blob) {
        putInt(blob.length);
        put(blob);
    }

    public void put(byte[] value) {
        reserve(value.length);
        buf.put(value);
    }


    public int get() {
        return buf.get();
    }


    public int get(byte[] b, int offset, int len) {
        int min = Math.min(len, buf.remaining());
        buf.get(b, offset, min);

        return min;
    }

    public void get(RawBuffer buf) {
        buf.put(this.buf);
    }

    public boolean getBoolean() {
        return buf.get() == 1;
    }

    public short readShort() {
        return buf.getShort();
    }


    public int getInt() {
        return buf.getInt();
    }

    public int readInt(int index) {
        return buf.getInt(index);
    }


    public long getLong() {
        return buf.getLong();
    }

    public long readLong(int index) {
        return buf.getLong(index);
    }


    public String getString() {
        int len = getInt();
        if (len == -1) {
            return null;
        }

        byte[] strBuf = new byte[len];

        buf.get(strBuf);
        byte b = buf.get(); // Skip '\0'
        assert (b == 0);

        return new String(strBuf, UTF_8);
    }

    public RawBuffer getBuffer(int len) {
        ByteBuffer dup = buf.duplicate();

        dup.limit(dup.position() + len);
        buf.position(buf.position() + len);

        return new RawBuffer(dup.slice());
    }

    RawBuffer getRawBuffer() {
        RawBuffer buf = new RawBuffer(getInt());
        get(buf);

        return buf.flip();
    }

    byte[] getBlob() {
        byte[] b = new byte[getInt()];
        get(b, 0, b.length);

        return b;
    }

    public boolean hasRemaining() {
        return buf.hasRemaining();
    }

    public int remaining() {
        return buf.remaining();
    }

    public void rewind() {
        buf.rewind();
    }

    public int position() {
        return buf.position();
    }

    public RawBuffer position(int pos) {
        buf.position(pos);
        return this;
    }

    public byte read(int index) {
        return buf.get(index);
    }

    public RawBuffer flip() {
        buf.flip();
        return this;
    }

    public void compact() {
        buf.compact();
    }

    public RawBuffer clear() {
        buf.clear();
        return this;
    }

    public int limit() {
        return buf.limit();
    }

    public void limit(int limit) {
        buf.limit(limit);
    }

    public void put(ByteBuffer src) {
        reserve(src.remaining());
        buf.put(src);

    }

    public static int intLen(int i) {
        return Integer.BYTES;
    }

    public static int longLen(long l) {
        return Long.BYTES;
    }

    public static int byteLen(int b) {
        return Byte.BYTES;
    }

    public static int stringLen(String s) {
        if (s == null) {
            return intLen(-1);
        }

        byte[] str = s.getBytes(UTF_8);

        return intLen(str.length) + str.length + byteLen('\0');
    }

    public static int booleanLen(boolean b) {
        return 1;
    }
}
