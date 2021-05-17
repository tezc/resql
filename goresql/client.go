// BSD-3-Clause
//
// Copyright 2021 Ozan Tezcan
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
// 3. Neither the name of the copyright holder nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
// OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
// OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

package goresql

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

const (
	lenBytes               = 4
	connectReq             = byte(0x00)
	connectResp            = byte(0x01)
	disconnectReq          = byte(0x02)
	clientReq              = byte(0x04)
	clientResp             = byte(0x05)
	connectFlag            = uint32(0)
	clientReqHeader        = 14
	msgOK                  = byte(0)
	msgErr                 = byte(1)
	msgClusterNameMismatch = byte(2)
	msgCorrupt             = byte(3)
	msgUnexpected          = byte(4)
	msgTimeout             = byte(5)
	msgNotLeader           = byte(6)
	msgDiskFull            = byte(7)
	flagOk                 = byte(0)
	flagError              = byte(1)
	flagStmt               = byte(2)
	flagStmtId             = byte(3)
	flagStmtPrepare        = byte(4)
	flagStmtDelPrepared    = byte(5)
	flagOp                 = byte(6)
	flagOpEnd              = byte(7)
	flagRow                = byte(8)
	flagMsgEnd             = byte(9)
	paramInteger           = byte(0)
	paramFloat             = byte(1)
	paramText              = byte(2)
	paramBlob              = byte(3)
	paramNull              = byte(4)
	bindName               = byte(0)
	bindIndex              = byte(1)
	bindEnd                = byte(2)
)

var errConnFail = errors.New("resql: failed to connect")
var errClusterNameMismatch = errors.New("resql: cluster name mismatch")
var errDiskFull = errors.New("resql: disk full")
var errSessionDoesNotExist = errors.New("resql : session does not exist on the server")

// goresql version
const Version = "0.1.3-latest"

type client struct {
	name         string
	clusterName  string
	timeout      int64
	sourceAddr   *net.TCPAddr
	urls         []url.URL
	urlsTerm     uint64
	seq          uint64
	req          buffer
	resp         buffer
	connected    bool
	hasStatement bool
	misuse       bool
	conn         net.Conn
	result       result
	lastRc       uint8
}

type Config struct {
	// client name, if not specified a random value will be assigned
	ClientName string

	// cluster name must match with configured on the server
	ClusterName string

	// timeout for operations, default is infinite
	TimeoutMillis int64

	// outgoing addr and port if you want to specify
	OutgoingAddr string
	OutgoingPort string

	// server urls,  single url is sufficient, default is "tcp://127.0.0.1:7600"
	Urls []string
}

type Resql interface {
	// Prepare prepares statement, this is a remote call to server
	Prepare(sql string) (PreparedStatement, error)

	// Delete deletes prepared statement, this is a remote call to server
	Delete(p PreparedStatement) error

	// PutStatement puts raw sql into current batch
	PutStatement(sql string)

	// PutPrepared puts prepared statement into current batch
	PutPrepared(p PreparedStatement)

	// BindParam binds parameter with value, it applies to the last
	// statement in the batch
	BindParam(param string, val interface{})

	// BindIndex binds parameter index with value, it applies to the last
	// statement in the batch
	BindIndex(index uint32, val interface{})

	// Execute executes current batch, this is a remote call to server
	// readonly should be set if all statements in the batch are readonly.
	Execute(readonly bool) (ResultSet, error)

	// Shutdown terminates client
	Shutdown() error

	// Clear clears current batch
	Clear()
}

type PreparedStatement interface {
	// StatementSql returns prepared statement sql
	StatementSql() string
}

type Row interface {
	// GetIndex returns column value at index
	GetIndex(index int) (interface{}, error)

	// GetColumn returns column value with column name
	GetColumn(columnName string) (interface{}, error)

	// ColumnName returns column name at index. index starts from zero.
	ColumnName(index int) (string, error)

	// ColumnCount returns column count
	ColumnCount() int

	// Read returns row, error will be returned if attempted to read more
	// columns than exists or when there is type mismatch
	Read(columns ...interface{}) error
}

type ResultSet interface {
	// NextResultSet advances to the next result set. false if there is
	// no result set.
	NextResultSet() bool

	// Row returns next row, nil if there is no more rows.
	Row() Row

	// LinesChanged returns lines changed while executing this statement.
	// Returned value is only valid if statement writes to the table.
	// e.g if statement is SELECT, returned value is undefined.
	LinesChanged() int

	// LastRowId returns last row id, only meaningful for INSERT statements.
	LastRowId() int64

	// RowCount returns row count in the current result set
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
	Valid bool // Valid is true if Int64 is not NULL
}

type NullString struct {
	String string
	Valid  bool // Valid is true if String is not NULL
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

// Create create client and connect to the server.
func Create(config *Config) (Resql, error) {
	var name string
	var clusterName string
	var addr net.TCPAddr
	var err error
	var timeout int64
	var urls []string

	urls = config.Urls
	if urls == nil {
		urls = []string{"tcp://127.0.0.1:7600"}
	}

	timeout = config.TimeoutMillis
	if timeout == 0 {
		timeout = math.MaxInt64
	}

	clusterName = config.ClusterName
	if len(clusterName) == 0 {
		clusterName = "cluster"
	}

	name = config.ClientName
	if len(name) == 0 {
		name = randomName()
	}

	if len(config.OutgoingAddr) > 0 {
		addr.IP = net.ParseIP(config.OutgoingAddr)
		if addr.IP == nil {
			return nil, fmt.Errorf("invalid addr %s", config.OutgoingAddr)
		}
	}

	if len(config.OutgoingPort) > 0 {
		addr.Port, err = strconv.Atoi(config.OutgoingPort)
		if err != nil {
			return nil, fmt.Errorf("invalid port %s", config.OutgoingPort)
		}
	}

	s := client{
		name:        name,
		clusterName: clusterName,
		timeout:     timeout,
		sourceAddr:  &addr,
		result: result{
			indexes: map[string]int{},
		},
	}

	for _, urlStr := range urls {
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

		if err == errClusterNameMismatch ||
			err == errSessionDoesNotExist {
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

	c.resp.reset()
	if !c.connected {
		return nil
	}

	encodeDisconnectReq(&c.resp, 0, 0)

	_, err := c.resp.writeTo(c.conn)
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

	c.resp.reset()
	c.encodeConnectReq(&c.resp)

	if c.conn != nil {
		_ = c.conn.Close()
		c.conn = nil
	}

	err = c.connectSock()
	if err != nil {
		return err
	}

	_ = c.conn.SetDeadline(time.Now().Add(5 * time.Second))

	_, err = c.resp.writeTo(c.conn)
	if err != nil {
		return err
	}

	c.resp.reset()

retry:
	_, err = c.resp.readFrom(c.conn)
	if err != nil {
		return err
	}

	if c.resp.len() < 4 {
		goto retry
	}

	if int(c.resp.peekUint32()) > c.resp.len() {
		goto retry
	}

	c.resp.readUint32()

	id := c.resp.readUint8()
	if id != connectResp {
		return errors.New("resql: received invalid message")
	}

	rc := c.resp.readUint8()
	seq := c.resp.readUint64()
	term := c.resp.readUint64()
	nodes := c.resp.readString()

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

	c.lastRc = rc

	switch rc {
	case msgOK:
		break
	case msgClusterNameMismatch:
		return errClusterNameMismatch
	case msgDiskFull:
		return errDiskFull
	default:
		return errConnFail
	}

	if c.req.empty() {
		c.seq = seq
	} else {
		if seq != c.seq && seq != c.seq-1 {
			c.seq = seq
			return errSessionDoesNotExist
		}
	}

	c.connected = true
	c.Clear()

	return nil
}

func (c *client) Clear() {
	c.req.reset()
	c.req.reserve(clientReqHeader)
	c.hasStatement = false
	c.misuse = false
}

func (c *client) connectSock() error {
	u := c.urls[0]
	c.urls = append(c.urls, u)
	c.urls = c.urls[1:]

	if u.Scheme == "tcp" {
		dialer := &net.Dialer{
			Timeout:       2000000000,
			LocalAddr:     c.sourceAddr,
			FallbackDelay: 0,
		}

		conn, err := dialer.Dial("tcp", u.Host)
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
		c.req.writeUint8(bindEnd)
		c.req.writeUint8(flagOpEnd)
	}

	c.hasStatement = true
	c.req.writeUint8(flagOp)
	c.req.writeUint8(flagStmt)
	c.req.writeString(&sql)
}

func (c *client) PutPrepared(p PreparedStatement) {
	if c.hasStatement {
		c.req.writeUint8(bindEnd)
		c.req.writeUint8(flagOpEnd)
	}

	c.hasStatement = true
	c.req.writeUint8(flagOp)
	c.req.writeUint8(flagStmtId)
	c.req.writeUint64(p.(*Prepared).id)
}

func (c *client) bind(val interface{}) {
	switch val.(type) {
	case int:
		c.req.writeUint8(paramInteger)
		c.req.writeUint64(uint64(val.(int)))
	case int64:
		c.req.writeUint8(paramInteger)
		c.req.writeUint64(uint64(val.(int64)))
	case float64:
		c.req.writeUint8(paramFloat)
		c.req.writeUint64(math.Float64bits(val.(float64)))
	case string:
		var s = val.(string)
		c.req.writeUint8(paramText)
		c.req.writeString(&s)
	case []byte:
		c.req.writeUint8(paramBlob)
		c.req.writeBlob(val.([]byte))
	case nil:
		c.req.writeUint8(paramNull)
	default:
		panic("resql: unsupported type for bind")
	}
}

func (c *client) BindParam(param string, val interface{}) {
	if !c.hasStatement {
		c.misuse = true
		return
	}

	c.req.writeUint8(bindName)
	c.req.writeString(&param)
	c.bind(val)
}

func (c *client) BindIndex(index uint32, val interface{}) {
	if !c.hasStatement {
		c.misuse = true
		return
	}

	c.req.writeUint8(bindIndex)
	c.req.writeUint32(index)
	c.bind(val)
}

func (c *client) Prepare(sql string) (PreparedStatement, error) {
	if c.hasStatement {
		c.Clear()
		return nil, errors.New("resql: operation must be a single operation")
	}

	c.req.writeUint8(flagOp)
	c.req.writeUint8(flagStmtPrepare)
	c.req.writeString(&sql)
	c.req.writeUint8(flagOpEnd)
	c.req.writeUint8(flagMsgEnd)

	c.seq++
	encodeClientReq(&c.req, false, c.seq)

	err := c.send()
	if err != nil {
		return nil, err
	}

	if c.resp.readUint8() != flagOp {
		return nil, errors.New("invalid message")
	}

	// skip response size
	c.resp.readUint32()

	return &Prepared{
		id:  c.resp.readUint64(),
		sql: sql,
	}, nil
}

func (c *client) Delete(p PreparedStatement) error {
	if c.hasStatement {
		c.Clear()
		return errors.New("resql: operation must be a single operation")
	}

	c.req.writeUint8(flagOp)
	c.req.writeUint8(flagStmtDelPrepared)
	c.req.writeUint64(p.(*Prepared).id)
	c.req.writeUint8(flagOpEnd)
	c.req.writeUint8(flagMsgEnd)

	c.seq++
	encodeClientReq(&c.req, false, c.seq)

	err := c.send()
	if err != nil {
		return err
	}

	if c.resp.readUint8() != flagOp {
		return errors.New("invalid message")
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

		_ = c.conn.SetDeadline(time.Now().Add(5 * time.Second))

		_, err := c.req.writeTo(c.conn)
		if err != nil {
			c.connected = false
			continue
		}

		c.req.setOffset(0)
		c.resp.reset()

	retry:
		for time.Now().Sub(start).Milliseconds() > c.timeout {
			break
		}

		_, err = c.resp.readFrom(c.conn)
		if err != nil {
			c.connected = false
			continue
		}

		if c.resp.len() < lenBytes {
			goto retry
		}

		if int(c.resp.peekUint32()) > c.resp.len() {
			goto retry
		}

		c.resp.readUint32()
		c.Clear()

		id := c.resp.readUint8()
		if id != clientResp {
			return errors.New("resql: received invalid message")
		}

		rc := c.resp.readUint8()
		if rc != flagOk {
			return fmt.Errorf("error : %s", *c.resp.readString())
		}

		return nil
	}

	return errors.New("resql: operation timeout")
}

func (c *client) Execute(readonly bool) (ResultSet, error) {
	if !c.hasStatement || c.misuse {
		c.Clear()
		return nil, errors.New("resql: missing statement")
	}

	c.req.writeUint8(bindEnd)
	c.req.writeUint8(flagOpEnd)
	c.req.writeUint8(flagMsgEnd)
	c.hasStatement = false

	if !readonly {
		c.seq++
	}

	encodeClientReq(&c.req, readonly, c.seq)

	err := c.send()
	if err != nil {
		return nil, err
	}

	c.result.set(&c.resp)

	return &c.result, nil
}

type result struct {
	buf           *buffer
	lastRowId     int64
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
		return errors.New("resql : index is out of range")
	}

	for i := range columns {
		switch d := columns[i].(type) {
		case *NullInt32:
			if r.values[i] == nil {
				*d = NullInt32{Valid: false, Int32: 0}
				break
			}

			val, ok := r.values[i].(int64)
			if !ok {
				return errors.New("resql: type mismatch")
			}

			*d = NullInt32{Valid: true, Int32: int32(val)}

		case *NullInt64:
			if r.values[i] == nil {
				*d = NullInt64{Valid: false, Int64: 0}
				break
			}

			val, ok := r.values[i].(int64)
			if !ok {
				return errors.New("resql: type mismatch")
			}

			*d = NullInt64{Valid: true, Int64: val}
		case *NullString:
			if r.values[i] == nil {
				*d = NullString{Valid: false, String: ""}
				break
			}

			val, ok := r.values[i].(string)
			if !ok {
				return errors.New("resql: type mismatch")
			}

			*d = NullString{Valid: true, String: val}
		case *NullFloat64:
			if r.values[i] == nil {
				*d = NullFloat64{Valid: false, Float64: 0}
				break
			}

			val, ok := r.values[i].(float64)
			if !ok {
				return errors.New("resql: type mismatch")
			}

			*d = NullFloat64{Valid: true, Float64: val}
		case *[]byte:
			if r.values[i] == nil {
				*d = nil
				break
			}

			val, ok := r.values[i].([]byte)
			if !ok {
				return errors.New("resql: type mismatch")
			}

			*d = val
		default:
			return errors.New("resql: parameter type is unknown")
		}
	}

	return nil
}

func (r *result) set(buf *buffer) {
	for k := range r.indexes {
		delete(r.indexes, k)
	}

	r.names = r.names[:0]
	r.values = r.values[:0]
	r.buf = buf
	r.nextResultSet = r.buf.offset()
	r.NextResultSet()
}

func (r *result) RowCount() int {
	return r.rowCount
}

func (r *result) LinesChanged() int {
	return r.linesChanged
}

func (r *result) LastRowId() int64 {
	return r.lastRowId
}

func (r *result) NextResultSet() bool {
	r.linesChanged = 0
	r.lastRowId = 0
	r.rowCount = -1
	r.columns = -1
	r.remainingRows = -1

	r.buf.setOffset(r.nextResultSet)

	flag := r.buf.readUint8()
	if flag != flagOp {
		return false
	}

	r.nextResultSet = r.buf.offset() + int(r.buf.readUint32())
	r.linesChanged = int(r.buf.readUint32())
	r.lastRowId = int64(r.buf.readUint64())

	if flag = r.buf.readUint8(); flag == flagRow {
		r.columns = int(r.buf.readUint32())

		for i := 0; i < r.columns; i++ {
			s := r.buf.readString()
			r.indexes[*s] = i
			r.names = append(r.names, *s)
		}

		r.rowCount = int(r.buf.readUint32())
		r.remainingRows = r.rowCount
	} else if flag != flagOpEnd {
		panic("Unexpected value")
	}

	return true
}

func (r *result) ColumnCount() int {
	return r.columns
}

func (r *result) GetIndex(index int) (interface{}, error) {
	if index < 0 || index >= len(r.values) {
		return nil, errors.New("resql : index is out of range")
	}

	return r.values[index], nil
}

func (r *result) GetColumn(param string) (interface{}, error) {
	v, found := r.indexes[param]
	if !found {
		return nil, errors.New("resql: column does not exist")
	}

	return r.values[v], nil
}

func (r *result) ColumnName(index int) (string, error) {
	if index < 0 || index >= len(r.names) {
		return "", errors.New("resql : index is out of range")
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
		switch param := r.buf.readUint8(); param {
		case paramInteger:
			r.values = append(r.values, int64(r.buf.readUint64()))
		case paramFloat:
			f := math.Float64frombits(r.buf.readUint64())
			r.values = append(r.values, f)
		case paramText:
			r.values = append(r.values, *r.buf.readString())
		case paramBlob:
			r.values = append(r.values, r.buf.readBlob())
		case paramNull:
			r.values = append(r.values, nil)
		default:
			panic("unknown value : " + strconv.Itoa(int(param)))
		}
	}

	return r
}

func (c *client) encodeConnectReq(buf *buffer) {
	str := "resql"

	total := uint32Len(lenBytes) +
		uint8Len(connectReq) +
		uint32Len(connectFlag) +
		stringLen(&str) +
		stringLen(&c.clusterName) +
		stringLen(&c.name)

	buf.writeUint32(total)
	buf.writeUint8(connectReq)
	buf.writeUint32(connectFlag)
	buf.writeString(&str)
	buf.writeString(&c.clusterName)
	buf.writeString(&c.name)
}

func encodeClientReq(b *buffer, readonly bool, sequence uint64) {
	length := b.len()
	b.position(0)
	b.writeUint32(uint32(length))
	b.writeUint8(clientReq)
	b.writeBool(readonly)
	b.writeUint64(sequence)
	b.setLength(length)
}

func encodeDisconnectReq(b *buffer, rc uint8, flags uint32) {
	total := uint32Len(lenBytes) +
		uint8Len(disconnectReq) +
		uint8Len(rc) +
		uint8Len(rc) +
		uint32Len(flags)

	b.writeUint32(total)
	b.writeUint8(disconnectReq)
	b.writeUint8(rc)
	b.writeUint32(flags)
}
