/*
 * BSD-3-Clause
 *
 * Copyright 2021 Ozan Tezcan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
            client.clear();
            client.put("DROP TABLE IF EXISTS multi;");
            client.execute(false);
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
