/*
 * MIT License
 *
 * Copyright (c) 2021 Resql Authors
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

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import java.util.ArrayList;
import java.util.List;

public class BasicTest {

    Resql client;

    @BeforeEach
    public void setup() {
        client = ResqlClient.create(new Config());
        client.put("DROP TABLE IF EXISTS basic;");
        client.put("CREATE TABLE basic (name TEXT, lastname TEXT);");
        client.execute(false);
    }

    @AfterEach
    public void tearDown() {
        if (client != null) {
            client.shutdown();
        }
    }

    @Test
    public void testSimple() {
        client.put("INSERT INTO basic VALUES('jane', 'doe');");
        ResultSet rs = client.execute(false);

        assert (rs.linesChanged() == 1);

        for (Row row : rs) {
            assert(false);
        }

        client.put("SELECT * FROM basic;");
        rs = client.execute(true);
        assert (rs.rowCount() == 1);

        for (Row row : rs) {
            assert(row.get("name").equals("jane"));
            assert (row.get("lastname").equals("doe"));
        }

        assert (!rs.nextResultSet());
    }

    @Test
    public void testLastRowId() {
        for (int i = 0; i < 100; i++) {
            client.put("INSERT INTO basic VALUES('jane', 'doe');");
            ResultSet rs = client.execute(false);
            assert (rs.linesChanged() == 1);
            assert (rs.lastRowId() == i + 1);
        }

        for (int i = 100; i < 200; i++) {
            client.put("INSERT INTO basic VALUES('jane', 'doe');");
            ResultSet rs = client.execute(false);
            assert (rs.linesChanged() == 1);
            assert (rs.lastRowId() == i + 1);
        }
    }

    @Test
    public void testConfigTimeout() {
        Config c = new Config();
        c.setTimeoutMillis(1);
        Assertions.assertThrows(ResqlException.class,
                                () -> ResqlClient.create(c));
    }

    @Test
    public void testConfigClusterName() {
        Config c = new Config();
        c.setClusterName(null);
        Assertions.assertThrows(ResqlException.class,
                                () -> ResqlClient.create(c));
    }

    @Test
    public void testConfigURI() {
        Config c = new Config();
        c.setUrls(new ArrayList<>());
        Assertions.assertThrows(ResqlException.class,
                                () -> ResqlClient.create(c));
    }

    @Test
    public void testConfigBadURI() {
        Config c = new Config();
        List<String> a = new ArrayList<>();
        a.add("127.0.0.1s");
        c.setUrls(a);
        Assertions.assertThrows(ResqlException.class,
                                () -> ResqlClient.create(c));
    }

    @Test
    public void testConfigBadURI2() {
        Config c = new Config();
        List<String> a = new ArrayList<>();
        a.add("127.0.0.1:900001");
        c.setUrls(a);
        Assertions.assertThrows(ResqlException.class,
                                () -> ResqlClient.create(c));
    }

    @Test
    public void testShutdown() {
        Config c = new Config();
        Resql s = ResqlClient.create(c);
        s.shutdown();
        s.shutdown();
    }



    @Test
    public void testEmptyWrite() {
        Assertions.assertThrows(ResqlSQLException.class,
                                () -> client.execute(false));
    }

    @Test
    public void testEmptyRead() {
        Assertions.assertThrows(ResqlSQLException.class,
                                () -> client.execute(true));
    }

    @Test
    public void testReadonly() {
        client.put("CREATE TABLE x (key TEXT, value TEXT);");
        Assertions.assertThrows(ResqlSQLException.class,
                                () -> client.execute(true));
    }

    @Test
    public void testEmptyPrepare() {
        Assertions.assertThrows(ResqlSQLException.class,
                                () -> client.prepare(null));
    }

    @Test
    public void testStatementAndPrepare() {
        client.put("CREATE TABLE x (key TEXT, value TEXT);");
        Assertions.assertThrows(ResqlSQLException.class,
                                () -> client.prepare("SELECT * FROM basic"));
    }

    @Test
    public void testParam() {
        client.put("INSERT INTO basic VALUES(:name, :lastname);");
        client.bind(":name", "janeparam");
        client.bind(":lastname", "doeparam");

        ResultSet rs = client.execute(false);
        assert (rs.linesChanged() == 1);

        client.put("SELECT * FROM basic;");
        rs = client.execute(true);

        for (Row row : rs) {
            assert (row.get("name").equals("janeparam"));
            assert (row.get("lastname").equals("doeparam"));
        }

        assert (!rs.nextResultSet());
    }

    @Test
    public void testIndex() {
        client.put("INSERT INTO basic VALUES(?, ?);");
        client.bind(0, "janeindex");
        client.bind(1, "doeindex");

        ResultSet rs = client.execute(false);
        assert (rs.linesChanged() == 1);

        client.put("SELECT * FROM basic;");
        rs = client.execute(true);

        for (Row row : rs) {
            assert (row.get(0).equals("janeindex"));
            assert (row.get(1).equals("doeindex"));
        }

        assert (!rs.nextResultSet());
    }

    @Test
    public void testPreparedParam() {
        PreparedStatement p = client.prepare(
                "INSERT INTO basic VALUES " + "(:name, :lastname);");
        assert (p.getStatement()
                .equals("INSERT INTO basic VALUES (:name, :lastname);"));

        client.put(p);
        client.bind(":name", "janeprepared0");
        client.bind(":lastname", "doeprepared0");

        ResultSet rs = client.execute(false);
        assert (rs.linesChanged() == 1);

        client.put("SELECT * FROM basic;");
        rs = client.execute(true);

        for (Row row : rs) {
            assert (row.get(0).equals("janeprepared0"));
            assert (row.get(1).equals("doeprepared0"));
        }

        assert (!rs.nextResultSet());

        client.put(p);
        client.bind(":name", "janeprepared1");
        client.bind(":lastname", "doeprepared1");

        rs = client.execute(false);
        assert (rs.linesChanged() == 1);

        client.put("SELECT * FROM basic;");
        rs = client.execute(true);

        int i = 0;
        for (Row row : rs) {
            assert (row.get(0).equals("janeprepared" + i));
            assert (row.get(1).equals("doeprepared" + i));
            i++;
        }

        assert (!rs.nextResultSet());
    }

    @Test
    public void testPreparedIndex() {
        PreparedStatement p =
                client.prepare("INSERT INTO basic VALUES " + "(?, ?);");

        assert (p.getStatement().equals("INSERT INTO basic VALUES (?, ?);"));

        client.put(p);
        client.bind(0, "janeindex0");
        client.bind(1, "doeindex0");

        ResultSet rs = client.execute(false);
        assert (rs.linesChanged() == 1);

        client.put("SELECT * FROM basic;");
        rs = client.execute(true);
        for (Row row : rs) {
            assert (row.get(0).equals("janeindex0"));
            assert (row.get(1).equals("doeindex0"));
        }

        assert (!rs.nextResultSet());

        client.put(p);
        client.bind(0, "janeindex1");
        client.bind(1, "doeindex1");
        rs = client.execute(false);
        assert (rs.linesChanged() == 1);

        client.put("SELECT * FROM basic;");
        rs = client.execute(true);

        int i = 0;
        for (Row row : rs) {
            assert (row.get(0).equals("janeindex" + i));
            assert (row.get(1).equals("doeindex" + i));
            i++;
        }

        assert (!rs.nextResultSet());

    }

    @Test
    public void testMulti() {
        PreparedStatement p =
                client.prepare("INSERT INTO basic VALUES " + "(?, ?);");

        assert (p.getStatement().equals("INSERT INTO basic VALUES (?, ?);"));

        client.put(p);
        client.bind(0, "janeindex1");
        client.bind(1, "doeindex1");
        client.put(p);
        client.bind(0, "janeindex2");
        client.bind(1, "doeindex2");
        client.put(p);
        client.bind(0, "janeindex3");
        client.bind(1, "doeindex3");

        ResultSet rs = client.execute(false);
        assert (rs.linesChanged() == 1);
        assert (rs.nextResultSet());
        assert (rs.linesChanged() == 1);
        assert (rs.nextResultSet());
        assert (rs.linesChanged() == 1);

        client.put("SELECT * FROM basic;");
        rs = client.execute(true);

        int i = 1;
        for (Row row : rs) {
            assert (row.get(0).equals("janeindex" + i));
            assert (row.get(1).equals("doeindex" + i));
            i++;
        }

        assert (!rs.nextResultSet());
    }

    @Test
    public void testDeletePrepared() {
        PreparedStatement p =
                client.prepare("INSERT INTO basic VALUES " + "(?, ?);");

        assert (p.getStatement().equals("INSERT INTO basic VALUES (?, ?);"));

        client.delete(p);

        client.put(p);
        client.bind(0, "janeindex1");
        client.bind(1, "doeindex1");

        Assertions.assertThrows(ResqlSQLException.class,
                                () -> client.execute(false));

        Assertions.assertThrows(ResqlSQLException.class,
                                () -> client.delete(p));

        client.put("select '1'");
        Assertions.assertThrows(ResqlSQLException.class,
                                () -> client.delete(p));
    }

    @Test
    public void testInvalidPrepare() {
        Assertions.assertThrows(ResqlSQLException.class, () -> client
                .prepare("INSERT INTO nonexisting VALUES " + "(?, ?);"));
    }

    @Test
    public void testMany() {
        PreparedStatement p =
                client.prepare("INSERT INTO basic VALUES " + "(?, ?);");

        assert (p.getStatement().equals("INSERT INTO basic VALUES (?, ?);"));

        for (int i = 0; i < 1000; i++) {
            client.put(p);
            client.bind(0, "janeindex" + i);
            client.bind(1, "doeindex" + i);
        }

        ResultSet rs = client.execute(false);

        for (int i = 0; i < 1000; i++) {
            assert(rs.linesChanged() == 1);
            rs.nextResultSet();
        }

        client.put("SELECT * FROM basic;");
        rs = client.execute(true);
        assert(rs.rowCount() == 1000);

        int i = 0;
        for (Row row : rs) {
            assert(row.get("name").equals("janeindex" + i));
            assert(row.get("lastname").equals("doeindex" + i));
            i++;
        }

        assert (i == 1000);

    }

    @Test
    public void testReturning() {
        client.put("INSERT INTO basic VALUES('jane', 'doe') RETURNING *;");
        ResultSet rs = client.execute(false);


        assert (rs.linesChanged() == 1);
        assert (rs.rowCount() == 1);

        for (Row row : rs) {
            assert(row.getColumnName(0).equals("name"));
            assert(row.getColumnName(1).equals("lastname"));
            assert(row.get(0).equals("jane"));
            assert(row.get(1).equals("doe"));
        }

        assert (!rs.nextResultSet());
    }
}
