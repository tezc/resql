//
// MIT License
//
// Copyright (c) 2021 Resql Authors
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

package resql

import (
	"encoding/binary"
	"errors"
	"io"
)

const (
	MaxUint32 = ^uint32(0)
	MinUint32 = 0
	MaxInt32  = int32(MaxUint32 >> 1)
	MinInt32  = -MaxInt32 - 1
	MaxStrLen = int(MaxInt32) >> 1
	NilStrLen = MaxUint32
	maxInt = int(^uint(0) >> 1)
	smallBufferSize = 1024
	MinRead = 512
)

var ErrTooLarge = errors.New("resql.Buffer: too large")
var errEmpty = errors.New("resql.Buffer: buffer empty")
var errNegativeRead = errors.New("bytes.Buffer: reader returned negative count from Read")

type Buffer struct {
	buf []byte // contents are the bytes buf[off : len(buf)]
	off int    // read at &buf[off], write at &buf[len(buf)]
}

func Uint8Len(b byte) uint32 {
	return 1
}

func Uint32Len(b uint32) uint32 {
	return 4
}

func Uint64Len(b uint64) uint32 {
	return 8
}

func BoolLen(b bool) uint32 {
	return 1
}

func StringLen(s *string) uint32 {
	if s == nil {
		return Uint32Len(NilStrLen)
	}

	sz := len(*s)
	if sz > MaxStrLen {
		panic("too big")
	}

	return Uint32Len(uint32(sz)) + uint32(sz) + Uint8Len('\000')
}

func (b *Buffer) WriteBool(val bool) {
	t := byte(0)

	if val {
		t = 1
	}

	b.WriteUint8(t)
}

func (b *Buffer) WriteUint8(val uint8) {
	m, ok := b.tryGrowByReslice(1)
	if !ok {
		m = b.grow(1)
	}
	b.buf[m] = val
}

func (b *Buffer) ReadUint8() uint8 {
	if b.empty() {
		panic(errEmpty)
	}
	c := b.buf[b.off]
	b.off++
	return c
}

func (b *Buffer) WriteUint32(val uint32) {
	m, ok := b.tryGrowByReslice(4)
	if !ok {
		m = b.grow(4)
	}

	binary.LittleEndian.PutUint32(b.buf[m:], val)
}

func (b *Buffer) WriteUint64(val uint64) {
	m, ok := b.tryGrowByReslice(8)
	if !ok {
		m = b.grow(8)
	}

	binary.LittleEndian.PutUint64(b.buf[m:], val)
}

func (b *Buffer) ReadUint32() uint32 {
	n := b.PeekUint32()
	b.off += 4

	return n
}

func (b *Buffer) ReadUint64() uint64 {
	if b.Len() < 4 {
		panic(errEmpty)
	}

	val := binary.LittleEndian.Uint64(b.buf[b.off:])
	b.off += 8

	return val
}

func (b *Buffer) PeekUint32() uint32 {
	if b.Len() < 4 {
		panic(errEmpty)
	}

	return binary.LittleEndian.Uint32(b.buf[b.off:])
}

func (b *Buffer) Position (n int) {
	if n > b.Cap() {
		panic(ErrTooLarge)
	}

	b.buf = b.buf[n:n]
}

func (b *Buffer) SetLength (n int) {
	if n > b.Cap() {
		panic(ErrTooLarge)
	}

	b.buf = b.buf[0:n]
}

func (b *Buffer) SetOffset (n int) {
	if n > b.Cap() {
		panic(ErrTooLarge)
	}

	b.off = n
}

func (b *Buffer) Reserve(n int) {
	_, ok := b.tryGrowByReslice(n)
	if !ok {
		_ = b.grow(n)
	}
}

// Read reads the next len(p) bytes from the buffer or until the buffer
// is drained. The return value n is the number of bytes read. If the
// buffer has no data to return, err is io.EOF (unless len(p) is zero);
// otherwise it is nil.
func (b *Buffer) Read(p []byte) (n int, err error) {
	if b.empty() {
		// Buffer is empty, reset to recover space.
		b.Reset()
		if len(p) == 0 {
			return 0, nil
		}
		return 0, io.EOF
	}
	n = copy(p, b.buf[b.off:])
	b.off += n
	return n, nil
}

// Write appends the contents of p to the buffer, growing the buffer as
// needed. The return value n is the length of p; err is always nil. If the
// buffer becomes too large, Write will panic with ErrTooLarge.
func (b *Buffer) Write(p []byte) (n int, err error) {
	m, ok := b.tryGrowByReslice(len(p))
	if !ok {
		m = b.grow(len(p))
	}
	return copy(b.buf[m:], p), nil
}

func (b *Buffer) ReadString() *string {
	length := b.ReadUint32()

	if int(length) == MaxStrLen {
		return nil
	}

	if b.Len() < int(length + 1) {
		panic(errEmpty)
	}

	tmp := make([]byte, length)
	n := copy(tmp, b.buf[b.off:])
	b.off += n + 1
	s := (string)(tmp)

	return &s
}

func (b *Buffer) WriteString(s *string) {
	if s == nil {
		b.WriteUint32(NilStrLen)
		return
	}

	b.WriteUint32(uint32(len(*s)))

	m, ok := b.tryGrowByReslice(len(*s))
	if !ok {
		m = b.grow(len(*s))
	}
	copy(b.buf[m:], *s)

	b.WriteUint8('\000')
}

func (b *Buffer) ReadBlob() []byte {
	length := b.ReadUint32()

	s := b.buf[b.off:b.off + int(length)]
	b.off += int(length)

	return s
}

func (b *Buffer) WriteBlob(blob []byte) {
	b.WriteUint32(uint32(len(blob)))

	m, ok := b.tryGrowByReslice(len(blob))
	if !ok {
		m = b.grow(len(blob))
	}
	copy(b.buf[m:], blob)
}



// WriteTo writes data to w until the buffer is drained or an error occurs.
// The return value n is the number of bytes written; it always fits into an
// int, but it is int64 to match the io.WriterTo interface. Any error
// encountered during the write is also returned.
func (b *Buffer) WriteTo(w io.Writer) (n int64, err error) {
	if nBytes := b.Len(); nBytes > 0 {
		m, e := w.Write(b.buf[b.off:])
		if m > nBytes {
			panic("resql.Buffer.WriteTo: invalid Write count")
		}
		b.off += m
		n = int64(m)
		if e != nil {
			return n, e
		}
		// all bytes should have been written, by definition of
		// Write method in io.Writer
		if m != nBytes {
			return n, io.ErrShortWrite
		}
	}
	return n, nil
}

// ReadFrom reads data from r until EOF and appends it to the buffer, growing
// the buffer as needed. The return value n is the number of bytes read. Any
// error except io.EOF encountered during the read is also returned. If the
// buffer becomes too large, ReadFrom will panic with ErrTooLarge.
func (b *Buffer) ReadFrom(r io.Reader) (n int64, err error) {
	for {
		i := b.grow(MinRead)
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
func (b *Buffer) grow(n int) int {
	m := b.Len()
	// If buffer is empty, reset to recover space.
	if m == 0 && b.off != 0 {
		b.Reset()
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
		panic(ErrTooLarge)
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
func (b *Buffer) tryGrowByReslice(n int) (int, bool) {
	if l := len(b.buf); n <= cap(b.buf)-l {
		b.buf = b.buf[:l+n]
		return l, true
	}
	return 0, false
}

func (b *Buffer) Offset() int {
	return b.off
}

// Reset resets the buffer to be empty,
// but it retains the underlying storage for use by future writes.
// Reset is the same as Truncate(0).
func (b *Buffer) Reset() {
	b.buf = b.buf[:0]
	b.off = 0
}

// empty reports whether the unread portion of the buffer is empty.
func (b *Buffer) empty() bool { return len(b.buf) <= b.off }

// Len returns the number of bytes of the unread portion of the buffer;
// b.Len() == len(b.Bytes()).
func (b *Buffer) Len() int { return len(b.buf) - b.off }

// Cap returns the capacity of the buffer's underlying byte slice, that is, the
// total space allocated for the buffer's data.
func (b *Buffer) Cap() int { return cap(b.buf) }

// makeSlice allocates a slice of size n. If the allocation fails, it panics
// with ErrTooLarge.
func makeSlice(n int) []byte {
	// If the make fails, give a known error.
	defer func() {
		if recover() != nil {
			panic(ErrTooLarge)
		}
	}()
	return make([]byte, n)
}
