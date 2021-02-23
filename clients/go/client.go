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
	"errors"
	"fmt"
	"math"
	"math/rand"
	"net"
	"net/url"
	"strconv"
	"strings"
	"time"
)

var ErrMissingStatement = errors.New("resql: missing statement")
var ErrInvalidMessage = errors.New("resql: received invalid message")
var ErrConnFail = errors.New("resql: failed to connect")
var ErrClusterNameMismatch = errors.New("resql: cluster name mismatch")
var ErrParamTypeMismatch = errors.New("resql: parameter type mismatch")

var ErrTimeout = errors.New("resql: operation timeout")
var ErrDisconnected = errors.New("resql: disconnected")
var ErrNotSingle = errors.New("resql: operation must be a single operation")
var ErrParamUnknown = errors.New("resql: parameter type is unknown")
var ErrColumnMissing = errors.New("resql: column does not exist")
var ErrIndexOutOfRange = errors.New("resql : index is out of range")
var ErrSessionDoesNotExist = errors.New("resql : session does not exist on the server")

type client struct {
	name         string
	clusterName  string
	timeout      int64
	sourceAddr   *net.TCPAddr
	urls         []url.URL
	urlsTerm     uint64
	seq          uint64
	req          Buffer
	resp         Buffer
	connected    bool
	hasStatement bool
	misuse       bool
	conn         net.Conn
	result       result
}

type Config struct {
	Name        string
	ClusterName string
	Timeout     int64
	SourceAddr  string
	SourcePort  string
	Urls        []string
}

type Resql interface {
	Prepare(sql string) (PreparedStatement, error)
	Delete(p PreparedStatement) error
	PutStatement(sql string)
	PutPrepared(p PreparedStatement)
	BindParam(param string, val interface{})
	BindIndex(index uint32, val interface{})
	Execute(readonly bool) (ResultSet, error)
	Shutdown() error
	Clear()
}

type PreparedStatement interface {
	StatementSql() string
}

type Row interface {
	ValueByIndex(index int) (interface{}, error)
	ValueByColumn(columnName string) (interface{}, error)
	ColumnName(index int) (string, error)
	ColumnCount() int
	Read(columns ...interface{}) error
}

type ResultSet interface {
	NextResultSet() bool
	Row() Row
	LinesChanged() int
	RowCount() int
}

type Prepared struct {
	id  uint64
	sql string
}

func (p *Prepared) StatementSql() string {
	return p.sql
}

type NullInt32 struct {
	Int32 int32
	Valid bool // Valid is true if Int32 is not NULL
}

type NullInt64 struct {
	Int64 int64
	Valid bool // Valid is true if Int32 is not NULL
}

type NullString struct {
	String string
	Valid  bool // Valid is true if Int32 is not NULL
}

type NullFloat64 struct {
	Float64 float64
	Valid   bool // Valid is true if Int32 is not NULL
}

var seededRand = rand.New(rand.NewSource(time.Now().UnixNano()))
var letters = []rune("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")

func randomName() string {
	b := make([]rune, 16)
	for i := range b {
		b[i] = letters[seededRand.Intn(len(letters))]
	}
	return string(b)
}

func Create(config *Config) (Resql, error) {
	var name string
	var addr net.TCPAddr
	var err error

	if config.Urls == nil {
		return nil, errors.New("Url cannot be nil")
	}

	name = config.Name
	if len(name) == 0 {
		name = randomName()
	}

	if len(config.SourceAddr) > 0 {
		addr.IP = net.ParseIP(config.SourceAddr)
		if addr.IP == nil {
			return nil, fmt.Errorf("invalid source addr %s", config.SourceAddr)
		}
	}

	if len(config.SourcePort) > 0 {
		addr.Port, err = strconv.Atoi(config.SourcePort)
		if err != nil {
			return nil, fmt.Errorf("invalid source addr %s", config.SourcePort)
		}
	}

	s := client{
		name:        name,
		clusterName: config.ClusterName,
		timeout:     config.Timeout,
		sourceAddr:  &addr,
		result: result{
			indexes: map[string]int{},
		},
	}

	for _, urlStr := range config.Urls {
		u, err := url.Parse(urlStr)
		if err != nil {
			return nil, err
		}

		if u.Scheme != "tcp" && u.Scheme != "unix" {
			return nil, fmt.Errorf("invalid protocol : %s", u.Scheme)
		}

		s.urls = append(s.urls, *u)
	}

	start := time.Now()

	for time.Now().Sub(start).Milliseconds() < s.timeout {
		err = s.connect()

		if err == ErrClusterNameMismatch ||
			err == ErrSessionDoesNotExist {
			return nil, err
		}

		if err == nil {
			return &s, nil
		}
	}

	if err != nil {
		return nil, err
	}

	return &s, nil
}

func (c *client) Shutdown() error {
	var ret error = nil

	c.resp.Reset()
	if !c.connected {
		return nil
	}

	encodeDisconnectReq(&c.resp, 0, 0)

	_, err := c.resp.WriteTo(c.conn)
	if err != nil {
		ret = err
	}

	err = c.conn.Close()
	if err != nil && ret != nil {
		ret = err
	}

	c.connected = false
	return ret
}

func (c *client) connect() error {
	var err error

	c.resp.Reset()
	c.encodeConnectReq(&c.resp)

	if c.conn != nil {
		_ = c.conn.Close()
		c.conn = nil
	}

	err = c.connectSock()
	if err != nil {
		return err
	}

	_, err = c.resp.WriteTo(c.conn)
	if err != nil {
		return err
	}

	c.resp.Reset()

retry:
	_, err = c.resp.ReadFrom(c.conn)
	if err != nil {
		return err
	}

	if c.resp.Len() < 4 {
		goto retry
	}

	if int(c.resp.PeekUint32()) > c.resp.Len() {
		goto retry
	}

	c.resp.ReadUint32()

	id := c.resp.ReadUint8()
	if id != connectResp {
		return ErrInvalidMessage
	}

	rc := c.resp.ReadUint8()
	seq := c.resp.ReadUint64()
	term := c.resp.ReadUint64()
	nodes := c.resp.ReadString()

	if term > c.urlsTerm {
		c.urls = c.urls[:0]
		n := strings.Split(*nodes, " ")
		for _, u := range n {
			u, err := url.Parse(u)
			if err != nil {
				continue
			}
			c.urls = append(c.urls, *u)
		}
		c.urlsTerm = term
	}

	if rc == msgClusterNameMismatch {
		return ErrClusterNameMismatch
	}

	if rc != msgOK {
		return ErrConnFail
	}

	if c.req.empty() {
		c.seq = seq
	} else {
		if seq != c.seq && seq != c.seq-1 {
			c.seq = seq
			return ErrSessionDoesNotExist
		}
	}

	c.connected = true
	c.Clear()
	return nil
}

func (c *client) Clear() {
	c.req.Reset()
	c.req.Reserve(clientReqHeader)
}

func (c *client) connectSock() error {
	u := c.urls[0]
	c.urls = append(c.urls, u)
	c.urls = c.urls[1:]

	if u.Scheme == "tcp" {
		dialer := &net.Dialer{
			Timeout:       200000000,
			LocalAddr:     c.sourceAddr,
			FallbackDelay: 0,
		}

		conn, err := dialer.Dial("tcp", u.Host)
		if err != nil {
			return err
		}

		tcp := conn.(*net.TCPConn)

		err = tcp.SetReadBuffer(32 * 1024)
		if err != nil {
			return err
		}

		err = tcp.SetWriteBuffer(32 * 1024)
		if err != nil {
			return err
		}

		c.conn = conn

		return nil
	}

	return errors.New("fail")
}

func (c *client) PutStatement(sql string) {
	if c.hasStatement {
		c.req.WriteUint8(flagEnd)
	}

	c.hasStatement = true
	c.req.WriteUint8(flagStmt)
	c.req.WriteString(&sql)
}

func (c *client) PutPrepared(p PreparedStatement) {
	if c.hasStatement {
		c.req.WriteUint8(flagEnd)
	}

	c.hasStatement = true
	c.req.WriteUint8(flagStmtId)
	c.req.WriteUint64(p.(*Prepared).id)
}

func (c *client) Bind(val interface{}) {
	switch val.(type) {
	case int:
		c.req.WriteUint8(paramInteger)
		c.req.WriteUint64(uint64(val.(int)))
	case int64:
		c.req.WriteUint8(paramInteger)
		c.req.WriteUint64(uint64(val.(int64)))
	case float64:
		c.req.WriteUint8(paramFloat)
		c.req.WriteUint64(math.Float64bits(val.(float64)))
	case string:
		var s = val.(string)
		c.req.WriteUint8(paramText)
		c.req.WriteString(&s)
	case []byte:
		c.req.WriteUint8(paramBlob)
		c.req.WriteBlob(val.([]byte))
	case nil:
		c.req.WriteUint8(paramNull)
	default:
		panic("resql: unsupported type for bind")
	}
}

func (c *client) BindParam(param string, val interface{}) {
	if !c.hasStatement {
		c.misuse = true
		return
	}

	c.req.WriteUint8(paramName)
	c.req.WriteString(&param)
	c.Bind(val)
}

func (c *client) BindIndex(index uint32, val interface{}) {
	if !c.hasStatement {
		c.misuse = true
		return
	}

	c.req.WriteUint8(paramIndex)
	c.req.WriteUint32(index)
	c.Bind(val)
}

func (c *client) Prepare(sql string) (PreparedStatement, error) {
	if c.hasStatement {
		return nil, ErrNotSingle
	}

	c.req.WriteUint8(flagStmtPrepare)
	c.req.WriteString(&sql)
	c.req.WriteUint8(flagEnd)

	c.seq++
	encodeClientReq(&c.req, false, c.seq)

	err := c.send()
	c.Clear()

	if err != nil {
		return nil, err
	}

	id := c.resp.ReadUint8()
	if id != clientResp {
		return nil, ErrDisconnected
	}

	rc := c.resp.ReadUint8()
	if rc == flagError {
		return nil, fmt.Errorf("sql error : %s", *c.resp.ReadString())
	}

	return &Prepared{
		id:  c.resp.ReadUint64(),
		sql: sql,
	}, nil
}

func (c *client) Delete(p PreparedStatement) error {
	if c.hasStatement {
		return ErrNotSingle
	}

	c.req.WriteUint8(flagStmtDelPrepared)
	c.req.WriteUint64(p.(*Prepared).id)
	c.req.WriteUint8(flagEnd)

	c.seq++
	encodeClientReq(&c.req, false, c.seq)
	err := c.send()
	c.Clear()

	if err != nil {
		return ErrDisconnected
	}

	id := c.resp.ReadUint8()
	if id != clientResp {
		return ErrInvalidMessage
	}

	rc := c.resp.ReadUint8()
	if rc == flagError {
		return fmt.Errorf("sql error : %s", *c.resp.ReadString())
	}

	return nil
}

func (c *client) send() error {
	start := time.Now()

	for time.Now().Sub(start).Milliseconds() < c.timeout {
		if !c.connected {
			err := c.connect()
			if err != nil {
				continue
			}
		}

		_, err := c.req.WriteTo(c.conn)
		if err != nil {
			c.connected = false
			continue
		}

		c.req.SetOffset(0)
		c.resp.Reset()

	retry:
		for time.Now().Sub(start).Milliseconds() > c.timeout {
			break
		}

		_, err = c.resp.ReadFrom(c.conn)
		if err != nil {
			c.connected = false
			continue
		}

		if c.resp.Len() < 4 {
			goto retry
		}

		if int(c.resp.PeekUint32()) > c.resp.Len() {
			goto retry
		}

		c.resp.ReadUint32()
		return nil
	}

	return ErrTimeout
}

func (c *client) Execute(readonly bool) (ResultSet, error) {
	if !c.hasStatement || c.misuse {
		c.misuse = false
		return nil, ErrMissingStatement
	}

	c.req.WriteUint8(flagEnd)
	c.hasStatement = false

	if !readonly {
		c.seq++
	}

	encodeClientReq(&c.req, readonly, c.seq)

	err := c.send()
	c.Clear()
	if err != nil {
		return nil, err
	}

	id := c.resp.ReadUint8()
	if id != clientResp {
		return nil, ErrDisconnected
	}

	rc := c.resp.ReadUint8()
	if rc == flagError {
		return nil, fmt.Errorf("sql error : %s", *c.resp.ReadString())
	}

	c.result.set(&c.resp)

	return &c.result, nil
}

type result struct {
	buf           *Buffer
	linesChanged  int
	nextResultSet int
	rowCount      int
	remainingRows int
	columns       int

	indexes map[string]int
	names   []string
	values  []interface{}
}

func (r *result) Read(columns ...interface{}) error {

	if len(columns) > len(r.values) {
		return ErrIndexOutOfRange
	}

	for i, _ := range columns {
		switch d := columns[i].(type) {
		case *NullInt32:
			if r.values[i] == nil {
				*d = NullInt32{Valid: false, Int32: 0}
				break
			}

			val, ok := r.values[i].(int64)
			if !ok {
				return ErrParamTypeMismatch
			}

			*d = NullInt32{Valid: true, Int32: int32(val)}

		case *NullInt64:
			if r.values[i] == nil {
				*d = NullInt64{Valid: false, Int64: 0}
				break
			}

			val, ok := r.values[i].(int64)
			if !ok {
				return ErrParamTypeMismatch
			}

			*d = NullInt64{Valid: true, Int64: val}
		case *NullString:
			if r.values[i] == nil {
				*d = NullString{Valid: false, String: ""}
				break
			}

			val, ok := r.values[i].(string)
			if !ok {
				return ErrParamTypeMismatch
			}

			*d = NullString{Valid: true, String: val}
		case *NullFloat64:
			if r.values[i] == nil {
				*d = NullFloat64{Valid: false, Float64: 0}
				break
			}

			val, ok := r.values[i].(float64)
			if !ok {
				return ErrParamTypeMismatch
			}

			*d = NullFloat64{Valid: true, Float64: val}
		case *[]byte:
			if r.values[i] == nil {
				*d = nil
				break
			}

			val, ok := r.values[i].([]byte)
			if !ok {
				return ErrParamTypeMismatch
			}

			*d = val
		default:
			return ErrParamUnknown
		}
	}

	return nil
}

func (r *result) set(buf *Buffer) {
	for k := range r.indexes {
		delete(r.indexes, k)
	}

	r.names = r.names[:0]
	r.values = r.values[:0]
	r.buf = buf
	r.nextResultSet = r.buf.Offset()
	r.NextResultSet()
}

func (r *result) RowCount() int {
	return r.rowCount
}

func (r *result) LinesChanged() int {
	return r.linesChanged
}

func (r *result) NextResultSet() bool {
	r.linesChanged = -1
	r.rowCount = -1
	r.columns = -1
	r.remainingRows = -1

	r.buf.SetOffset(r.nextResultSet)

	flag := r.buf.ReadUint8()
	if flag != flagStmt {
		return false
	}

	r.nextResultSet = r.buf.Offset() + int(r.buf.ReadUint32())

	switch flag = r.buf.ReadUint8(); flag {
	case flagRow:
		r.columns = int(r.buf.ReadUint32())

		for i := 0; i < r.columns; i++ {
			s := r.buf.ReadString()
			r.indexes[*s] = i
			r.names = append(r.names, *s)
		}

		r.rowCount = int(r.buf.ReadUint32())
		r.remainingRows = r.rowCount
	case flagDone:
		r.linesChanged = int(r.buf.ReadUint32())
	default:
		panic("Unexpected value")
	}

	return true
}

func (r *result) ColumnCount() int {
	return r.columns
}

func (r *result) ValueByIndex(index int) (interface{}, error) {
	if index < 0 || index >= len(r.values) {
		return nil, ErrIndexOutOfRange
	}

	return r.values[index], nil
}

func (r *result) ValueByColumn(param string) (interface{}, error) {
	v, found := r.indexes[param]
	if !found {
		return nil, ErrColumnMissing
	}

	return r.values[v], nil
}

func (r *result) ColumnName(index int) (string, error) {
	if index < 0 || index >= len(r.names) {
		return "", ErrIndexOutOfRange
	}

	return r.names[index], nil
}

func (r *result) Row() Row {
	if r.remainingRows <= 0 {
		return nil
	}
	r.remainingRows--
	r.values = r.values[:0]

	for i := 0; i < r.columns; i++ {
		switch param := r.buf.ReadUint8(); param {
		case paramInteger:
			r.values = append(r.values, int64(r.buf.ReadUint64()))
		case paramFloat:
			r.values = append(r.values, math.Float64frombits(r.buf.ReadUint64()))
		case paramText:
			r.values = append(r.values, *r.buf.ReadString())
		case paramBlob:
			r.values = append(r.values, r.buf.ReadBlob())
		case paramNull:
			r.values = append(r.values, nil)
		default:
			panic("unknown value : " + strconv.Itoa(int(param)))
		}
	}

	return r
}

const (
	lenBytes               = 4
	connectReq             = byte(0x00)
	connectResp            = byte(0x01)
	disconnectReq          = byte(0x02)
	clientReq              = byte(0x04)
	clientResp             = byte(0x05)
	remoteTypeClient       = byte(0x00)
	rcOk                   = byte(0x00)
	clientReqHeader        = 14
	flagOK                 = byte(0)
	flagError              = byte(1)
	flagDone               = byte(2)
	flagStmt               = byte(3)
	flagStmtId             = byte(4)
	flagStmtPrepare        = byte(5)
	flagStmtDelPrepared    = byte(6)
	flagRow                = byte(7)
	flagEnd                = byte(8)
	msgOK                  = byte(0)
	msgClusterNameMismatch = byte(2)
	paramInteger           = byte(0)
	paramFloat             = byte(1)
	paramText              = byte(2)
	paramBlob              = byte(3)
	paramNull              = byte(4)
	paramName              = byte(5)
	paramIndex             = byte(6)
)

func (c *client) encodeConnectReq(buf *Buffer) {
	x := "resql"

	total := Uint32Len(lenBytes) +
		Uint8Len(connectReq) +
		StringLen(&x) +
		Uint8Len(remoteTypeClient) +
		StringLen(&c.clusterName) +
		StringLen(&c.name)

	buf.WriteUint32(total)
	buf.WriteUint8(connectReq)
	buf.WriteString(&x)
	buf.WriteUint8(remoteTypeClient)
	buf.WriteString(&c.clusterName)
	buf.WriteString(&c.name)
}

func encodeClientReq(b *Buffer, readonly bool, sequence uint64) {
	length := b.Len()
	b.Position(0)
	b.WriteUint32(uint32(length))
	b.WriteUint8(clientReq)
	b.WriteBool(readonly)
	b.WriteUint64(sequence)
	b.SetLength(length)
}

func encodeDisconnectReq(b *Buffer, rc uint8, flags uint32) {
	total := Uint32Len(lenBytes) +
		Uint8Len(disconnectReq) +
		Uint8Len(rc) +
		Uint32Len(flags)

	b.WriteUint32(total)
	b.WriteUint8(disconnectReq)
	b.WriteUint8(rc)
	b.WriteUint32(flags)
}
