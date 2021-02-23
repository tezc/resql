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
	"bytes"
	"fmt"
	"os"
	"testing"
)

var c Resql

func TestMain(m *testing.M) {
	s, err := Create(&Config{
		Name:        "",
		ClusterName: "cluster",
		Timeout:     5000,
		SourceAddr:  "",
		SourcePort:  "",
		Urls:        []string{"tcp://127.0.0.1:7600"},
	})

	if err != nil {
		fmt.Println(err)
		os.Exit(-1)
	}

	c = s
	ret := m.Run()

	err = s.Shutdown()
	if err != nil {
		fmt.Println(err)
		os.Exit(-1)
	}

	os.Exit(ret)
}

func equal(t *testing.T, a interface{}, b interface{}) {
	t.Helper()

	tx, err := a.([]byte)
	if err == true {
		bx, _ := b.([]byte)

		if !bytes.Equal(tx, bx) {
			t.Fatalf("%s != %s", a, b)
		}

		return
	}

	if a != b {
		t.Fatalf("%s != %s", a, b)
	}
}

func notEqual(t *testing.T, a interface{}, b interface{}) {
	t.Helper()

	if a == b {
		t.Fatalf("%s != %s", a, b)
	}
}

func TestNull(t *testing.T) {

	c.PutStatement("DROP TABLE IF EXISTS gotest;")
	c.PutStatement("CREATE TABLE gotest (id INTEGER, num INTEGER, name TEXT, " +
		"points FLOAT, data BLOB, date TEXT);")

	_, err := c.Execute(false)
	if err != nil {
		t.Fatal(err)
	}

	c.PutStatement("INSERT INTO gotest VALUES(?, ?, ?, ?, ?, ?);")
	c.BindIndex(0, nil)
	c.BindIndex(1, nil)
	c.BindIndex(2, nil)
	c.BindIndex(3, nil)
	c.BindIndex(4, nil)
	c.BindIndex(5, nil)

	_, err = c.Execute(false)
	if err != nil {
		t.Fatal(err)
	}

	c.PutStatement("SELECT * FROM gotest;")
	rs, err := c.Execute(true)
	if err != nil {
		t.Fatal(err)
	}

	var id NullInt32
	var num NullInt64
	var name NullString
	var points NullFloat64
	var data []byte
	var date NullString

	r := rs.Row()
	err = r.Read(&id, &num, &name, &points, &data, &date)
	if err != nil {
		t.Fatal(err)
	}

	equal(t, id.Valid, false)
	equal(t, name.Valid, false)
	equal(t, num.Valid, false)
	equal(t, points.Valid, false)
	equal(t, data, nil)
	equal(t, date.Valid, false)
}

func TestWrongTypeRead(t *testing.T) {
	c.PutStatement("DROP TABLE IF EXISTS gotest;")
	c.PutStatement("CREATE TABLE gotest (id INTEGER PRIMARY KEY, name TEXT, " +
		"points FLOAT, data BLOB);")

	_, err := c.Execute(false)
	if err != nil {
		t.Fatal(err)
	}

	c.PutStatement("INSERT INTO gotest VALUES(?, ?, ?, ?);")
	c.BindIndex(0, 0)
	c.BindIndex(1, "jane")
	c.BindIndex(2, 3.22)
	c.BindIndex(3, []byte("test"))

	_, err = c.Execute(false)
	if err != nil {
		t.Fatal(err)
	}

	c.PutStatement("SELECT * FROM gotest;")
	rs, err := c.Execute(true)
	if err != nil {
		t.Fatal(err)
	}

	var wrongInt32 NullInt32
	var wrongInt64 NullInt64
	var wrongFloat NullFloat64
	var wrongString NullString
	var id NullInt32
	var name NullString
	var points NullFloat64
	var data []byte

	r := rs.Row()
	err = r.Read(&wrongFloat, &name, &points, &data)
	notEqual(t, err, nil)

	err = r.Read(&id, &wrongFloat, &points, &data)
	notEqual(t, err, nil)

	err = r.Read(&id, &name, &wrongString, &data)
	notEqual(t, err, nil)

	err = r.Read(&id, &name, &points, &wrongFloat)
	notEqual(t, err, nil)

	err = r.Read(&data, &name, &points, &data)
	notEqual(t, err, nil)

	err = r.Read(&id, &wrongInt32, &points, &data)
	notEqual(t, err, nil)

	err = r.Read(&id, &wrongInt64, &points, &data)
	notEqual(t, err, nil)
}

func TestSingle(t *testing.T) {

	c.PutStatement("DROP TABLE IF EXISTS gotest;")
	c.PutStatement("CREATE TABLE gotest (id INTEGER PRIMARY KEY, name TEXT, " +
		"points FLOAT, data BLOB, date TEXT);")

	_, err := c.Execute(false)
	if err != nil {
		t.Fatal(err)
	}

	c.PutStatement("INSERT INTO gotest VALUES(?, ?, ?, ?, ?);")
	c.BindIndex(0, 0)
	c.BindIndex(1, "jane")
	c.BindIndex(2, 3.22)
	c.BindIndex(3, []byte("test"))
	c.BindIndex(4, nil)

	_, err = c.Execute(false)
	if err != nil {
		t.Fatal(err)
	}

	c.PutStatement("SELECT * FROM gotest;")
	rs, err := c.Execute(true)
	if err != nil {
		t.Fatal(err)
	}

	var id NullInt32
	var name NullString
	var points NullFloat64
	var data []byte
	var date NullString

	err = rs.Row().Read(&id, &name, &points, &data, &date)
	if err != nil {
		t.Fatal(err)
	}

	equal(t, id.Int32, int32(0))
	equal(t, name.String, "jane")
	equal(t, points.Float64, 3.22)
	equal(t, data, []byte("test"))
	equal(t, date.Valid, false)
}

func TestMany(t *testing.T) {

	c.PutStatement("DROP TABLE IF EXISTS gotest;")
	c.PutStatement("CREATE TABLE gotest (id INTEGER PRIMARY KEY, name TEXT, " +
		"points FLOAT, data BLOB, date TEXT);")

	_, err := c.Execute(false)
	if err != nil {
		t.Fatal(err)
	}

	for i := 0; i < 100000; i++ {
		c.PutStatement("INSERT INTO gotest VALUES(?, ?, ?, ?, ?);")
		c.BindIndex(0, i)
		c.BindIndex(1, "jane")
		c.BindIndex(2, 3.22)
		c.BindIndex(3, []byte("test"))
		c.BindIndex(4, nil)
	}

	_, err = c.Execute(false)
	if err != nil {
		t.Fatal(err)
	}

	c.PutStatement("SELECT * FROM gotest;")
	rs, err := c.Execute(true)
	if err != nil {
		t.Fatal(err)
	}

	var id NullInt32
	var name NullString
	var points NullFloat64
	var data []byte
	var date NullString

	for i, r := 0, rs.Row(); r != nil; i, r = i + 1, rs.Row()  {
		err := r.Read(&id, &name, &points, &data, &date)
		if err != nil {
			t.Fatal(err)
		}

		equal(t, id.Int32, int32(i))
		equal(t, name.String, "jane")
		equal(t, points.Float64, 3.22)
		equal(t, data, []byte("test"))
		equal(t, date.Valid, false)
	}
}

func TestPreparedIndex(t *testing.T) {
	c.PutStatement("DROP TABLE IF EXISTS gotest;")
	c.PutStatement("CREATE TABLE gotest (id INTEGER PRIMARY KEY, name TEXT, " +
		"points FLOAT, data BLOB, date TEXT);")

	_, err := c.Execute(false)
	if err != nil {
		t.Fatal(err)
	}

	stmt, err := c.Prepare("INSERT INTO gotest VALUES(?, ?, ?, ?, ?);")
	if err != nil {
		t.Fatal(err)
	}

	c.PutPrepared(stmt)
	c.BindIndex(0, 0)
	c.BindIndex(1, "jane")
	c.BindIndex(2, 3.22)
	c.BindIndex(3, []byte("test"))
	c.BindIndex(4, nil)

	_, err = c.Execute(false)
	if err != nil {
		t.Fatal(err)
	}

	c.PutStatement("SELECT * FROM gotest;")
	rs, err := c.Execute(true)
	if err != nil {
		t.Fatal(err)
	}

	var id NullInt32
	var name NullString
	var points NullFloat64
	var data []byte
	var date NullString

	err = rs.Row().Read(&id, &name, &points, &data, &date)
	if err != nil {
		t.Fatal(err)
	}

	equal(t, id.Int32, int32(0))
	equal(t, name.String, "jane")
	equal(t, points.Float64, 3.22)
	equal(t, data, []byte("test"))
	equal(t, date.Valid, false)

	err = c.Delete(stmt)
	if err != nil {
		t.Fatal(err)
	}
}

func TestPreparedIndexMulti(t *testing.T) {
	c.PutStatement("DROP TABLE IF EXISTS gotest;")
	c.PutStatement("CREATE TABLE gotest (id INTEGER PRIMARY KEY, name TEXT, " +
		"points FLOAT, data BLOB, date TEXT);")

	_, err := c.Execute(false)
	if err != nil {
		t.Fatal(err)
	}

	stmt, err := c.Prepare("INSERT INTO gotest VALUES(?, ?, ?, ?, ?);")
	if err != nil {
		t.Fatal(err)
	}

	for i := 0; i < 1000; i++ {
		c.PutPrepared(stmt)
		c.BindIndex(0, i)
		c.BindIndex(1, "jane")
		c.BindIndex(2, 3.22)
		c.BindIndex(3, []byte("test"))
		c.BindIndex(4, nil)
	}

	_, err = c.Execute(false)
	if err != nil {
		t.Fatal(err)
	}

	c.PutStatement("SELECT * FROM gotest;")
	rs, err := c.Execute(true)
	if err != nil {
		t.Fatal(err)
	}

	var id NullInt32
	var name NullString
	var points NullFloat64
	var data []byte
	var date NullString

	for i, r := 0, rs.Row(); r != nil; i, r = i + 1, rs.Row()  {
		err := r.Read(&id, &name, &points, &data, &date)
		if err != nil {
			t.Fatal(err)
		}

		equal(t, id.Int32, int32(i))
		equal(t, name.String, "jane")
		equal(t, points.Float64, 3.22)
		equal(t, data, []byte("test"))
		equal(t, date.Valid, false)
	}

	err = c.Delete(stmt)
	if err != nil {
		t.Fatal(err)
	}
}

func TestPreparedParam(t *testing.T) {
	c.PutStatement("DROP TABLE IF EXISTS gotest;")
	c.PutStatement("CREATE TABLE gotest (id INTEGER PRIMARY KEY, name TEXT, " +
		"points FLOAT, data BLOB, date TEXT);")

	_, err := c.Execute(false)
	if err != nil {
		t.Fatal(err)
	}

	stmt, err := c.Prepare("INSERT INTO gotest " +
		"VALUES(:id, :name, :points, :data, :date);")
	if err != nil {
		t.Fatal(err)
	}

	c.PutPrepared(stmt)
	c.BindParam(":id", 0)
	c.BindParam(":name", "jane")
	c.BindParam(":points", 3.22)
	c.BindParam(":data", []byte("test"))
	c.BindParam(":date", nil)

	_, err = c.Execute(false)
	if err != nil {
		t.Fatal(err)
	}

	c.PutStatement("SELECT * FROM gotest;")
	rs, err := c.Execute(true)
	if err != nil {
		t.Fatal(err)
	}

	var id NullInt32
	var name NullString
	var points NullFloat64
	var data []byte
	var date NullString

	err = rs.Row().Read(&id, &name, &points, &data, &date)
	if err != nil {
		t.Fatal(err)
	}

	equal(t, id.Int32, int32(0))
	equal(t, name.String, "jane")
	equal(t, points.Float64, 3.22)
	equal(t, data, []byte("test"))
	equal(t, date.Valid, false)

	err = c.Delete(stmt)
	if err != nil {
		t.Fatal(err)
	}
}

func TestPreparedParamMulti(t *testing.T) {
	c.PutStatement("DROP TABLE IF EXISTS gotest;")
	c.PutStatement("CREATE TABLE gotest (id INTEGER PRIMARY KEY, name TEXT, " +
		"points FLOAT, data BLOB, date TEXT);")

	_, err := c.Execute(false)
	if err != nil {
		t.Fatal(err)
	}

	stmt, err := c.Prepare("INSERT INTO gotest " +
		"VALUES(:id, :name, :points, :data, :date);")
	if err != nil {
		t.Fatal(err)
	}

	equal(t, stmt.StatementSql(), "INSERT INTO gotest " +
		"VALUES(:id, :name, :points, :data, :date);")

	for i := 0; i < 1000; i++ {
		c.PutPrepared(stmt)
		c.BindParam(":id", i)
		c.BindParam(":name", "jane")
		c.BindParam(":points", 3.22)
		c.BindParam(":data", []byte("test"))
		c.BindParam(":date", nil)
	}

	_, err = c.Execute(false)
	if err != nil {
		t.Fatal(err)
	}

	c.PutStatement("SELECT * FROM gotest;")
	rs, err := c.Execute(true)
	if err != nil {
		t.Fatal(err)
	}

	var id NullInt32
	var name NullString
	var points NullFloat64
	var data []byte
	var date NullString

	for i, r := 0, rs.Row(); r != nil; i, r = i + 1, rs.Row()  {
		err := r.Read(&id, &name, &points, &data, &date)
		if err != nil {
			t.Fatal(err)
		}

		equal(t, id.Int32, int32(i))
		equal(t, name.String, "jane")
		equal(t, points.Float64, 3.22)
		equal(t, data, []byte("test"))
		equal(t, date.Valid, false)
	}

	err = c.Delete(stmt)
	if err != nil {
		t.Fatal(err)
	}

	c.PutStatement("SELECT * FROM gotest;")
	rs, err = c.Execute(true)
	if err != nil {
		t.Fatal(err)
	}

	var tmp NullString
	err = rs.Row().Read(&id, &name, &points, &data, &date, &tmp)
	if err == nil {
		t.Fatal("fail")
	}

	var tmp2 float32
	err = rs.Row().Read(&id, &name, &points, &data, &tmp2)
	if err == nil {
		t.Fatal("fail")
	}

}

func TestFail(t *testing.T) {
	c.PutStatement("x fail gotest;")

	_, err := c.Execute(false)
	if err == nil {
		t.Fatal("fail")
	}

	_, err = c.Prepare("x fail gotest;")
	if err == nil {
		t.Fatal("fail")
	}

	stmt, err := c.Prepare("SELECT '1';")
	if err != nil {
		t.Fatal(err)
	}

	err = c.Delete(stmt)
	if err != nil {
		t.Fatal(err)
	}

	err = c.Delete(stmt)
	if err == nil {
		t.Fatal("fail")
	}

	c.BindIndex(0, "x")
	_, err = c.Execute(false)
	if err == nil {
		t.Fatal("fail")
	}

	c.PutStatement("SELECT '1';")
	rs, err := c.Execute(true)
	if err != nil {
		t.Fatal("fail")
	}

	var value NullInt32
	var valstr NullString

	r := rs.Row()
	err = r.Read(&value)
	if err == nil {
		t.Fatal("fail")
	}

	err = r.Read(&valstr)
	if err != nil {
		t.Fatal("fail")
	}

	equal(t, valstr.String, "1")
}

func TestConnection(t *testing.T) {
	_, err := Create(&Config{
		Name:        "",
		ClusterName: "cluster",
		Timeout:     5000,
		SourceAddr:  "",
		SourcePort:  "",
		Urls:        []string{"tcx://127.0.0.1:7600"},
	})

	if err == nil {
		t.Fatal(err)
	}

	_, err = Create(&Config{
		Name:        "",
		ClusterName: "errorname",
		Timeout:     5000,
		SourceAddr:  "",
		SourcePort:  "",
		Urls:        []string{"tcp://127.0.0.1:7600"},
	})

	if err == nil {
		t.Fatal(err)
	}


	_, err = Create(&Config{
		Name:        "",
		ClusterName: "cluster",
		Timeout:     5000,
		SourceAddr:  "x",
		SourcePort:  "",
		Urls:        []string{"tcp://127.0.0.1:7600"},
	})

	if err == nil {
		t.Fatal(err)
	}

	_, err = Create(&Config{
		Name:        "",
		ClusterName: "cluster",
		Timeout:     5000,
		SourceAddr:  "",
		SourcePort:  "x",
		Urls:        []string{"tcp://127.0.0.1:7600"},
	})

	if err == nil {
		t.Fatal(err)
	}

	_, err = Create(&Config{
		Name:        "",
		ClusterName: "cluster",
		Timeout:     5000,
		SourceAddr:  "",
		SourcePort:  "",
		Urls:        []string{"@tcp:/1272-0.0.1:**7600"},
	})

	if err == nil {
		t.Fatal(err)
	}

	_, err = Create(&Config{
		Name:        "",
		ClusterName: "cluster",
		Timeout:     5000,
		SourceAddr:  "",
		SourcePort:  "",
		Urls:        nil,
	})

	if err == nil {
		t.Fatal(err)
	}

	_, err = Create(&Config{
		Name:        "",
		ClusterName: "cluster",
		Timeout:     5000,
		SourceAddr:  "",
		SourcePort:  "",
		Urls:        []string{"tcp://127.0.0.1:7600", "tcd://127.0.0.1:8000"},
	})

	if err == nil {
		t.Fatal(err)
	}

	_, err = Create(&Config{
		Name:        "",
		ClusterName: "cluster",
		Timeout:     5000,
		SourceAddr:  "",
		SourcePort:  "",
		Urls:        []string{"tcp://127.0.0.1"},
	})

	if err == nil {
		t.Fatal(err)
	}

	s, err := Create(&Config{
		Name:        "",
		ClusterName: "cluster",
		Timeout:     5000,
		SourceAddr:  "",
		SourcePort:  "",
		Urls:        []string{"tcp://127.0.0.1:7600", "tcp://127.0.0.1:7601"},
	})

	if err != nil {
		t.Fatal(err)
	}

	err = s.Shutdown()
	if err != nil {
		t.Fatal(err)
	}

	err = s.Shutdown()
	if err != nil {
		t.Fatal(err)
	}
}

func TestRow(t *testing.T) {

	c.PutStatement("DROP TABLE IF EXISTS gotest;")
	c.PutStatement("CREATE TABLE gotest (id INTEGER PRIMARY KEY, num INTEGER," +
		"name TEXT, points FLOAT, data BLOB, date TEXT);")

	_, err := c.Execute(false)
	if err != nil {
		t.Fatal(err)
	}

	c.PutStatement("INSERT INTO gotest VALUES(?, ?, ?, ?, ?, ?);")
	c.BindIndex(0, 0)
	c.BindIndex(1, int64(-1))
	c.BindIndex(2, "jane")
	c.BindIndex(3, 3.22)
	c.BindIndex(4, []byte("test"))
	c.BindIndex(5, nil)

	rs, err := c.Execute(false)
	if err != nil {
		t.Fatal(err)
	}

	equal(t, rs.LinesChanged(), 1)

	c.PutStatement("SELECT * FROM gotest;")
	rs, err = c.Execute(true)
	if err != nil {
		t.Fatal(err)
	}

	var id NullInt32
	var num NullInt64
	var name NullString
	var points NullFloat64
	var data []byte
	var date NullString

	err = rs.Row().Read(&id, &num, &name, &points, &data, &date)
	if err != nil {
		t.Fatal(err)
	}

	equal(t, id.Int32, int32(0))
	equal(t, num.Int64, int64(-1))
	equal(t, name.String, "jane")
	equal(t, points.Float64, 3.22)
	equal(t, data, []byte("test"))
	equal(t, date.Valid, false)

	r := rs.Row()
	if r != nil {
		t.Fatal(r)
	}

	c.PutStatement("INSERT INTO gotest VALUES(?, ?, ?, ?, ?, ?);")
	c.BindIndex(0, 1)
	c.BindIndex(1, int64(-1))
	c.BindIndex(2, "jane")
	c.BindIndex(3, 3.22)
	c.BindIndex(4, []byte("test"))
	c.BindIndex(5, nil)

	c.PutStatement("SELECT * FROM gotest;")
	rs, err = c.Execute(false)
	if err != nil {
		t.Fatal(err)
	}

	for i := 0; i < rs.RowCount(); i++ {
		r := rs.Row()
		equal(t, r.ColumnCount(), 6)

		col, err := r.ColumnName(0)
		equal(t, err, nil)
		equal(t, col, "id")

		col, err = r.ColumnName(1)
		equal(t, err, nil)
		equal(t, col, "num")

		col, err = r.ColumnName(2)
		equal(t, err, nil)
		equal(t, col, "name")

		col, err = r.ColumnName(3)
		equal(t, err, nil)
		equal(t, col, "points")

		col, err = r.ColumnName(4)
		equal(t, err, nil)
		equal(t, col, "data")

		col, err = r.ColumnName(5)
		equal(t, err, nil)
		equal(t, col, "date")
	}
}

func TestRow2(t *testing.T) {
	c.PutStatement("DROP TABLE IF EXISTS gotest;")
	c.PutStatement("CREATE TABLE gotest (id INTEGER PRIMARY KEY, num INTEGER," +
		"name TEXT, points FLOAT, data BLOB, date TEXT);")

	c.PutStatement("INSERT INTO gotest VALUES(?, ?, ?, ?, ?, ?);")
	c.BindIndex(0, 1)
	c.BindIndex(1, int64(-1))
	c.BindIndex(2, "jane")
	c.BindIndex(3, 3.22)
	c.BindIndex(4, []byte("test"))
	c.BindIndex(5, nil)
	rs, err := c.Execute(false)
	if err != nil {
		t.Fatal(err)
	}

	c.PutStatement("SELECT * FROM gotest;")
	rs, err = c.Execute(false)
	if err != nil {
		t.Fatal(err)
	}

	equal(t, rs.RowCount(), 1)

	r := rs.Row()
	if r == nil {
		t.Fatal("fail")
	}

	val, err := r.ValueByColumn("id")
	equal(t, int64(1), val.(int64))

	val, err = r.ValueByColumn("num")
	equal(t, int64(-1), val.(int64))

	val, err = r.ValueByColumn("name")
	equal(t, "jane", val.(string))

	val, err = r.ValueByColumn("points")
	equal(t, 3.22, val.(float64))

	val, err = r.ValueByColumn("data")
	equal(t, []byte("test"), val.([]byte))

	val, err = r.ValueByColumn("date")
	equal(t, val, nil)

	val, err = r.ValueByIndex(0)
	equal(t, int64(1), val.(int64))

	val, err = r.ValueByIndex(1)
	equal(t, int64(-1), val.(int64))

	val, err = r.ValueByIndex(2)
	equal(t, "jane", val.(string))

	val, err = r.ValueByIndex(3)
	equal(t, 3.22, val.(float64))

	val, err = r.ValueByIndex(4)
	equal(t, []byte("test"), val.([]byte))

	val, err = r.ValueByIndex(5)
	equal(t, val, nil)

	equal(t, r.ColumnCount(), 6)

	name, err := r.ColumnName(0)
	equal(t, name, "id")

	name, err = r.ColumnName(1)
	equal(t, name, "num")

	name, err = r.ColumnName(2)
	equal(t, name, "name")

	name, err = r.ColumnName(3)
	equal(t, name, "points")

	name, err = r.ColumnName(4)
	equal(t, name, "data")

	name, err = r.ColumnName(5)
	equal(t, name, "date")

	name, err = r.ColumnName(6)
	if err == nil {
		t.Fatal("fail")
	}

	val, err = r.ValueByColumn("wrong")
	if err == nil {
		t.Fatal("fail")
	}

	val, err = r.ValueByIndex(1111)
	if err == nil {
		t.Fatal("fail")
	}
}

func TestMultiResultset(t *testing.T) {

	c.PutStatement("DROP TABLE IF EXISTS gotest;")
	c.PutStatement("CREATE TABLE gotest (key TEXT PRIMARY KEY, value INTEGER)")

	_, err := c.Execute(false)
	if err != nil {
		t.Fatal(err)
	}

	c.PutStatement("INSERT INTO gotest VALUES(:key, :value)")
	c.BindParam(":key", "counter1")
	c.BindParam(":value", 400)
	rs, err := c.Execute(false)
	if err != nil {
		t.Fatal(err)
	}

	equal(t, rs.LinesChanged(), 1)

	c.PutStatement("SELECT * FROM gotest WHERE key = :key");
	c.BindParam(":key", "counter1");
	c.PutStatement("UPDATE gotest SET value = value + 1 WHERE key = :key");
	c.BindParam(":key", "counter1");
	c.PutStatement("SELECT * FROM gotest WHERE key = :key");
	c.BindParam(":key", "counter1");

	rs, err = c.Execute(false)
	if err != nil {
		t.Fatal(err)
	}

	equal(t, rs.RowCount(), 1)

	var key NullString
	var value NullInt64

	r := rs.Row()
	err = r.Read(&key, &value)
	equal(t, err, nil)
	equal(t, key.String, "counter1")
	equal(t, value.Int64, int64(400))

	b := rs.NextResultSet()
	equal(t, b, true)
	equal(t, rs.LinesChanged(), 1)

	b = rs.NextResultSet()
	equal(t, b, true)
	r = rs.Row()
	err = r.Read(&key, &value)
	equal(t, err, nil)
	equal(t, key.String, "counter1")
	equal(t, value.Int64, int64(401))

	b = rs.NextResultSet()
	equal(t, b, false)
}