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
		Urls:        []string{"tcp://127.0.0.1:8080"},
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
