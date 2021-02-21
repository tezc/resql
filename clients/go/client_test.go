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
	"fmt"
	"os"
	"testing"
)

var c Resql

func TestMain(m *testing.M) {
	s, err := Create(&Config{
		Name:        "",
		ClusterName: "cluster",
		Timeout:     2000,
		SourceAddr:  "",
		SourcePort:  "",
		Urls:        []string{"tcp://127.0.0.1:7600"},
	})

	if err != nil {
		fmt.Println(err)
		os.Exit(-1)
	}

	c = s

	m.Run()
	err = s.Shutdown()

	if err != nil {
		fmt.Println(err)
		os.Exit(-1)
	}
}

func TestSingle(t *testing.T) {

	c.PutStatement("DROP TABLE IF EXISTS test;")
	c.PutStatement("CREATE TABLE test (id INTEGER, name TEXT, points FLOAT, data BLOB, date TEXT);")
	_, err := c.Execute(false)
	if err != nil {
		t.Fatal(err)
	}

	c.PutStatement("INSERT INTO TEST VALUES(?, ?, ?, ?, ?);")
	c.BindIndex(0, 300)
	c.BindIndex(1, "jane")
	c.BindIndex(2, 3.22)
	c.BindIndex(3, []byte("test"))
	c.BindIndex(4, nil)

	_, err = c.Execute(false)
	if err != nil {
		t.Fatal(err)
	}

	c.PutStatement("SELECT * FROM test;")
	rs, err := c.Execute(true)
	if err != nil {
		t.Fatal(err)
	}

	var id *int
	var name *string
	var points *float64
	var data []byte
	s := "d"
	var date *string = &s


	err = rs.NextRow().Read(&id, &name, &points, &data, &date)
	if err != nil {
		t.Fatal(err)
	}

	fmt.Println(*id, *name, *points, data, date)


}
