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

package resql.example;

import org.junit.jupiter.api.Test;
import resql.*;

public class KeyValueTest {

    @Test
    public void simple() {
        Resql client = ResqlClient.create(new Config());

        client.put("SELECT * FROM resql_nodes");
        ResultSet rs = client.execute(true);

        for (Row row : rs) {
            for (int i = 0; i < row.columnCount(); i++) {
                System.out.printf("%-20s : %-20s \n",
                                  row.getColumnName(i), row.get(i));
            }
        }

        client.shutdown();
    }

    @Test
    public void putAndGetNamed() {
        Resql client = ResqlClient.create(new Config());

        client.put("DROP TABLE IF EXISTS key_value;");
        client.put("CREATE TABLE key_value (key TEXT PRIMARY KEY, value TEXT)");
        client.execute(false);

        client.put("INSERT INTO key_value VALUES(:key, :value)");
        client.bind(":key", "userkey1");
        client.bind(":value", "uservalue1");
        ResultSet rs = client.execute(false);
        System.out.println("Changes : " + rs.linesChanged());

        client.put("SELECT * FROM key_value");
        rs = client.execute(true);

        for (Row row : rs) {
            String key = row.get("key");
            String value = row.get("value");

            System.out.println(key + " " + value);
        }

        client.shutdown();
    }

    @Test
    public void putAndGetIndex() {
        Resql client = ResqlClient.create(new Config());

        client.put("DROP TABLE IF EXISTS key_value;");
        client.put("CREATE TABLE key_value (key TEXT PRIMARY KEY, value TEXT)");
        client.execute(false);

        client.put("INSERT INTO key_value VALUES(?, ?)");
        client.bind(0, "userkey1");
        client.bind(1, "uservalue1");
        ResultSet rs = client.execute(false);
        System.out.println("Changes : " + rs.linesChanged());

        client.put("SELECT * FROM key_value");
        rs = client.execute(true);

        for (Row row : rs) {
            String key = row.get(0);
            String value = row.get(1);

            System.out.println(key + " " + value);
        }

        client.shutdown();
    }


    @Test
    public void preparedNamed() {
        Resql client = ResqlClient.create(new Config());

        client.put("DROP TABLE IF EXISTS key_value;");
        client.put("CREATE TABLE key_value (key TEXT PRIMARY KEY, value TEXT)");
        client.execute(false);

        PreparedStatement stmt = client.prepare(
                "INSERT INTO key_value VALUES(:key, :value)");

        for (int i = 0; i < 10; i++) {
            client.put(stmt);
            client.bind(":key", "userkey" + i);
            client.bind(":value", "uservalue" + i);
        }

        client.execute(false);

        client.put("SELECT * FROM key_value");
        ResultSet rs = client.execute(true);

        for (Row row : rs) {
            String key = row.get("key");
            String value = row.get("value");

            System.out.println(key + " " + value);
        }

        client.shutdown();
    }

    @Test
    public void preparedIndex() {
        Resql client = ResqlClient.create(new Config());

        client.put("DROP TABLE IF EXISTS key_value;");
        client.put("CREATE TABLE key_value (key TEXT PRIMARY KEY, value TEXT)");
        client.execute(false);

        PreparedStatement stmt = client.prepare(
                "INSERT INTO key_value VALUES(?, ?)");

        for (int i = 0; i < 10; i++) {
            client.put(stmt);
            client.bind(0, "userkey" + i);
            client.bind(1, "uservalue" + i);
        }

        client.execute(false);

        client.put("SELECT * FROM key_value");
        ResultSet rs = client.execute(true);

        for (Row row : rs) {
            String key = row.get("key");
            String value = row.get("value");

            System.out.println(key + " " + value);
        }

        client.shutdown();
    }

    @Test
    public void deletePrepared() {
        Resql client = ResqlClient.create(new Config());

        client.put("DROP TABLE IF EXISTS key_value;");
        client.put("CREATE TABLE key_value (key TEXT PRIMARY KEY, value TEXT)");
        client.execute(false);

        PreparedStatement stmt = client.prepare(
                "INSERT INTO key_value VALUES(?, ?)");

        for (int i = 0; i < 10; i++) {
            client.put(stmt);
            client.bind(0, "userkey" + i);
            client.bind(1, "uservalue" + i);
        }

        client.execute(false);
        client.delete(stmt);

        client.shutdown();
    }

    @Test
    public void atomicGetAndSet() {
        Resql client = ResqlClient.create(new Config());

        client.put("DROP TABLE IF EXISTS key_value;");
        client.put("CREATE TABLE key_value (key TEXT PRIMARY KEY, value TEXT)");
        client.execute(false);

        client.put("INSERT INTO key_value VALUES(?, ?)");
        client.bind(0, "userkey1");
        client.bind(1, "uservalue1");
        ResultSet rs = client.execute(false);
        System.out.println("Changes : " + rs.linesChanged());


        client.put("SELECT * FROM key_value WHERE key = :key");
        client.bind(":key", "userkey1");
        client.put("INSERT OR REPLACE INTO key_value VALUES('userkey1', :value)");
        client.bind(":value", "newvalue");

        rs = client.execute(false);
        for (Row row : rs) {
            String key = row.get("key");
            String value = row.get("value");

            System.out.println("Previous : " + key + " " + value);
        }

        client.put("SELECT * FROM key_value WHERE key = :key");
        client.bind(":key", "userkey1");

        rs = client.execute(false);
        for (Row row : rs) {
            String key = row.get("key");
            String value = row.get("value");

            System.out.println("Current : " + key + " " + value);
        }

        client.shutdown();
    }

    @Test
    public void atomicSetAndGet() {
        Resql client = ResqlClient.create(new Config());

        client.put("DROP TABLE IF EXISTS key_value;");
        client.put("CREATE TABLE key_value (key TEXT PRIMARY KEY, value TEXT)");
        client.execute(false);

        client.put("INSERT INTO key_value VALUES(?, ?)");
        client.bind(0, "userkey1");
        client.bind(1, "uservalue1");
        ResultSet rs = client.execute(false);
        System.out.println("Changes : " + rs.linesChanged());

        client.put("INSERT OR REPLACE INTO key_value VALUES('userkey1', :value)");
        client.bind(":value", "newvalue");
        client.put("SELECT * FROM key_value WHERE key = :key");
        client.bind(":key", "userkey1");

        rs = client.execute(false);
        System.out.println("Changes : " + rs.linesChanged());

        rs.nextResultSet();

        for (Row row : rs) {
            String key = row.get("key");
            String value = row.get("value");

            System.out.println("Current : " + key + " " + value);
        }

        client.shutdown();
    }
}
