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
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import java.nio.charset.StandardCharsets;

public class MultiTest {

    Resql client;

    @BeforeEach
    public void setup() {
        client = ResqlClient.create(new Config());
        client.put("DROP TABLE IF EXISTS multi;");
        client.put("CREATE TABLE multi (id INTEGER PRIMARY KEY, name TEXT, " +
                           "points FLOAT, data BLOB, date TEXT);");
        client.execute(false);
    }

    @AfterEach
    public void tearDown() {
        if (client != null) {
            client.shutdown();
        }
    }

    @Test
    public void testParam() {
        client.put(
                "INSERT INTO multi VALUES(:id, :name,  :points, :data, :date)" +
                        ";");
        client.bind(":id", 22L);
        client.bind(":name", "jane");
        client.bind(":points", 3.22);
        client.bind(":data", "blob".getBytes(StandardCharsets.UTF_8));
        client.bind(":date", null);

        ResultSet rs = client.execute(false);
        assert (rs.linesChanged() == 1);

        client.put("SELECT * FROM multi;");
        rs = client.execute(true);

        for (Row row : rs) {
            assert (row.get("id") == Long.valueOf(22));
            assert (row.get("name").equals("jane"));
            assert (row.get("points").equals(3.22));
            assert (new String((byte[]) row.get("data")).equals("blob"));
            assert (row.get("date") == null);
        }

        assert (!rs.nextResultSet());
    }

    @Test
    public void testIndex() {
        client.put("INSERT INTO multi VALUES(?, ?, ?, ?, ?);");
        client.bind(0, 22L);
        client.bind(1, "jane");
        client.bind(2, 3.22);
        client.bind(3, "blob".getBytes(StandardCharsets.UTF_8));
        client.bind(4, null);

        ResultSet rs = client.execute(false);
        assert (rs.linesChanged() == 1);

        client.put("SELECT * FROM multi;");
        rs = client.execute(true);

        for (Row row : rs) {
            assert (row.get(0) == Long.valueOf(22));
            assert (row.get(1).equals("jane"));
            assert (row.get(2).equals(3.22));
            assert (new String((byte[]) row.get(3)).equals("blob"));
            assert (row.get(4) == null);
        }
        assert (!rs.nextResultSet());
    }

    @Test
    public void testMany() {
        int j = 0;
        for (int k = 0; k < 100; k++) {
            for (int i = 0; i < 1000; i++) {
                client.put("INSERT INTO multi VALUES(?, ?, ?, ?, ?);");
                client.bind(0, j);
                client.bind(1, "jane" + j);
                client.bind(2, 3.22 + j);
                client.bind(3, ("blob" + j).getBytes(StandardCharsets.UTF_8));
                client.bind(4, null);
                j++;
            }

            client.execute(false);
        }

        client.put("SELECT * FROM multi;");
        ResultSet rs = client.execute(true);

        int i = 0;
        for (Row row : rs) {
            Long id = row.get(0);
            String name = row.get(1);
            Double points = row.get(2);
            byte[] b = row.get(3);
            String s = row.get(4);

            assert (id == i);
            assert (name.equals("jane" + i));
            assert (points == 3.22 + i);
            assert (new String(b).equals("blob" + i));
            assert (s == null);
            i++;
        }

        assert (i == 100000);
        assert (!rs.nextResultSet());
    }

    @Test
    public void testMulti() {
        for (int i = 0; i < 1000; i++) {
            client.put("INSERT INTO multi VALUES(?, ?, ?, ?, ?);");
            client.bind(0, i);
            client.bind(1, "jane" + i);
            client.bind(2, 3.221 + i);
            client.bind(3, ("blob" + i).getBytes(StandardCharsets.UTF_8));
            client.bind(4, null);
        }

        client.execute(false);

        client.put("SELECT name FROM multi;");
        client.put("SELECT points FROM multi");

        ResultSet rs = client.execute(true);

        int i = 0;
        for (Row row : rs) {
            assert (row.get(0).equals("jane" + i));
            i++;
        }

        assert (i == 1000);
        assert (rs.nextResultSet());

        i = 0;
        for (Row row : rs) {
            assert (row.get(0).equals(3.221 + i));
            i++;
        }

        assert (i == 1000);
        assert (!rs.nextResultSet());

    }
}
