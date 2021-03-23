//
// MIT License
//
// Copyright (c) 2021 Ozan Tezcan
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

package goresql

import (
	"encoding/binary"
	"errors"
	"io"
)

const (
	maxUint32       = ^uint32(0)
	maxInt32        = int32(maxUint32 >> 1)
	maxStrLen       = int(maxInt32) >> 1
	nilStrLen       = maxUint32
	maxInt          = int(^uint(0) >> 1)
	smallBufferSize = 1024
	minRead         = 512
)

var errTooLarge = errors.New("goresql.buffer: too large")
var errEmpty = errors.New("goresql.buffer: buffer empty")
var errNegativeRead = errors.New("goresql.buffer: reader returned negative count from read")

type buffer struct {
	buf []byte // contents are the bytes buf[off : len(buf)]
	off int    // read at &buf[off], write at &buf[len(buf)]
}

func uint8Len(b byte) uint32 {
	return 1
}

func uint32Len(b uint32) uint32 {
	return 4
}

func stringLen(s *string) uint32 {
	if s == nil {
		return uint32Len(nilStrLen)
	}

	sz := len(*s)
	if sz > maxStrLen {
		panic("too big")
	}

	return uint32Len(uint32(sz)) + uint32(sz) + uint8Len('\000')
}

func (b *buffer) writeBool(val bool) {
	t := byte(0)

	if val {
		t = 1
	}

	b.writeUint8(t)
}

func (b *buffer) writeUint8(val uint8) {
	m, ok := b.tryGrowByReslice(1)
	if !ok {
		m = b.grow(1)
	}
	b.buf[m] = val
}

func (b *buffer) readUint8() uint8 {
	if b.empty() {
		panic(errEmpty)
	}
	c := b.buf[b.off]
	b.off++
	return c
}

func (b *buffer) writeUint32(val uint32) {
	m, ok := b.tryGrowByReslice(4)
	if !ok {
		m = b.grow(4)
	}

	binary.LittleEndian.PutUint32(b.buf[m:], val)
}

func (b *buffer) writeUint64(val uint64) {
	m, ok := b.tryGrowByReslice(8)
	if !ok {
		m = b.grow(8)
	}

	binary.LittleEndian.PutUint64(b.buf[m:], val)
}

func (b *buffer) readUint32() uint32 {
	n := b.peekUint32()
	b.off += 4

	return n
}

func (b *buffer) readUint64() uint64 {
	if b.len() < 8 {
		panic(errEmpty)
	}

	val := binary.LittleEndian.Uint64(b.buf[b.off:])
	b.off += 8

	return val
}

func (b *buffer) peekUint32() uint32 {
	if b.len() < 4 {
		panic(errEmpty)
	}

	return binary.LittleEndian.Uint32(b.buf[b.off:])
}

func (b *buffer) position(n int) {
	if n > b.cap() {
		panic(errTooLarge)
	}

	b.buf = b.buf[n:n]
}

func (b *buffer) setLength(n int) {
	if n > b.cap() {
		panic(errTooLarge)
	}

	b.buf = b.buf[0:n]
}

func (b *buffer) setOffset(n int) {
	if n > b.cap() {
		panic(errTooLarge)
	}

	b.off = n
}

func (b *buffer) reserve(n int) {
	_, ok := b.tryGrowByReslice(n)
	if !ok {
		_ = b.grow(n)
	}
}

// read reads the next len(p) bytes from the buffer or until the buffer
// is drained. The return value n is the number of bytes read. If the
// buffer has no data to return, err is io.EOF (unless len(p) is zero);
// otherwise it is nil.
func (b *buffer) read(p []byte) (n int, err error) {
	if b.empty() {
		// buffer is empty, reset to recover space.
		b.reset()
		if len(p) == 0 {
			return 0, nil
		}
		return 0, io.EOF
	}
	n = copy(p, b.buf[b.off:])
	b.off += n
	return n, nil
}

// write appends the contents of p to the buffer, growing the buffer as
// needed. The return value n is the length of p; err is always nil. If the
// buffer becomes too large, write will panic with ErrTooLarge.
func (b *buffer) write(p []byte) (n int, err error) {
	m, ok := b.tryGrowByReslice(len(p))
	if !ok {
		m = b.grow(len(p))
	}
	return copy(b.buf[m:], p), nil
}

func (b *buffer) readString() *string {
	length := b.readUint32()

	if int(length) == maxStrLen {
		return nil
	}

	if b.len() < int(length+1) {
		panic(errEmpty)
	}

	tmp := make([]byte, length)
	n := copy(tmp, b.buf[b.off:])
	b.off += n + 1
	s := (string)(tmp)

	return &s
}

func (b *buffer) writeString(s *string) {
	if s == nil {
		b.writeUint32(nilStrLen)
		return
	}

	b.writeUint32(uint32(len(*s)))

	m, ok := b.tryGrowByReslice(len(*s))
	if !ok {
		m = b.grow(len(*s))
	}
	copy(b.buf[m:], *s)

	b.writeUint8('\000')
}

func (b *buffer) readBlob() []byte {
	length := b.readUint32()

	s := b.buf[b.off : b.off+int(length)]
	b.off += int(length)

	return s
}

func (b *buffer) writeBlob(blob []byte) {
	b.writeUint32(uint32(len(blob)))

	m, ok := b.tryGrowByReslice(len(blob))
	if !ok {
		m = b.grow(len(blob))
	}
	copy(b.buf[m:], blob)
}

// writeTo writes data to w until the buffer is drained or an error occurs.
// The return value n is the number of bytes written; it always fits into an
// int, but it is int64 to match the io.WriterTo interface. Any error
// encountered during the write is also returned.
func (b *buffer) writeTo(w io.Writer) (n int64, err error) {
	if nBytes := b.len(); nBytes > 0 {
		m, e := w.Write(b.buf[b.off:])
		if m > nBytes {
			panic("resql.buffer.writeTo: invalid write count")
		}
		b.off += m
		n = int64(m)
		if e != nil {
			return n, e
		}
		// all bytes should have been written, by definition of
		// write method in io.Writer
		if m != nBytes {
			return n, io.ErrShortWrite
		}
	}
	return n, nil
}

// readFrom reads data from r until EOF and appends it to the buffer, growing
// the buffer as needed. The return value n is the number of bytes read. Any
// error except io.EOF encountered during the read is also returned. If the
// buffer becomes too large, readFrom will panic with ErrTooLarge.
func (b *buffer) readFrom(r io.Reader) (n int64, err error) {
	for {
		i := b.grow(minRead)
		b.buf = b.buf[:i]
		m, e := r.Read(b.buf[i:cap(b.buf)])
		if m < 0 {
			panic(errNegativeRead)
		}

		b.buf = b.buf[:i+m]
		n += int64(m)

		if e != nil {
			return n, e
		}

		return n, nil
	}
}

// grow grows the buffer to guarantee space for n more bytes.
// It returns the index where bytes should be written.
// If the buffer can't grow it will panic with ErrTooLarge.
func (b *buffer) grow(n int) int {
	m := b.len()
	// If buffer is empty, reset to recover space.
	if m == 0 && b.off != 0 {
		b.reset()
	}
	// Try to grow by means of a reslice.
	if i, ok := b.tryGrowByReslice(n); ok {
		return i
	}
	if b.buf == nil && n <= smallBufferSize {
		b.buf = make([]byte, n, smallBufferSize)
		return 0
	}
	c := cap(b.buf)
	if n <= c/2-m {
		// We can slide things down instead of allocating a new
		// slice. We only need m+n <= c to slide, but
		// we instead let capacity get twice as large so we
		// don't spend all our time copying.
		copy(b.buf, b.buf[b.off:])
	} else if c > maxInt-c-n {
		panic(errTooLarge)
	} else {
		// Not enough space anywhere, we need to allocate.
		buf := makeSlice(2*c + n)
		copy(buf, b.buf[b.off:])
		b.buf = buf
	}
	// Restore b.off and len(b.buf).
	b.off = 0
	b.buf = b.buf[:m+n]
	return m
}

// tryGrowByReslice is a inlineable version of grow for the fast-case where the
// internal buffer only needs to be resliced.
// It returns the index where bytes should be written and whether it succeeded.
func (b *buffer) tryGrowByReslice(n int) (int, bool) {
	if l := len(b.buf); n <= cap(b.buf)-l {
		b.buf = b.buf[:l+n]
		return l, true
	}
	return 0, false
}

func (b *buffer) offset() int {
	return b.off
}

// reset resets the buffer to be empty,
// but it retains the underlying storage for use by future writes.
// reset is the same as Truncate(0).
func (b *buffer) reset() {
	b.buf = b.buf[:0]
	b.off = 0
}

// empty reports whether the unread portion of the buffer is empty.
func (b *buffer) empty() bool { return len(b.buf) <= b.off }

// len returns the number of bytes of the unread portion of the buffer;
// b.len() == len(b.Bytes()).
func (b *buffer) len() int { return len(b.buf) - b.off }

// cap returns the capacity of the buffer's underlying byte slice, that is, the
// total space allocated for the buffer's data.
func (b *buffer) cap() int { return cap(b.buf) }

// makeSlice allocates a slice of size n. If the allocation fails, it panics
// with ErrTooLarge.
func makeSlice(n int) []byte {
	// If the make fails, give a known error.
	defer func() {
		if recover() != nil {
			panic(errTooLarge)
		}
	}()
	return make([]byte, n)
}
