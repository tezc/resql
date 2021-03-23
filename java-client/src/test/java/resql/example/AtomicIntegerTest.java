/*
 * MIT License
 *
 * Copyright (c) 2021 Ozan Tezcan
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

package resql.example;

import org.junit.jupiter.api.Test;
import resql.*;

public class AtomicIntegerTest {

    @Test
    public void getAndIncrement() {
        Resql client = ResqlClient.create(new Config());

        client.put("DROP TABLE IF EXISTS key_value;");
        client.put(
                "CREATE TABLE key_value (key TEXT PRIMARY KEY, value INTEGER)");
        client.execute(false);

        client.put("INSERT INTO key_value VALUES(:key, :value)");
        client.bind(":key", "counter1");
        client.bind(":value", 0);
        ResultSet rs = client.execute(false);
        System.out.println("Changes : " + rs.linesChanged());

        client.put("SELECT * FROM key_value WHERE key = :key");
        client.bind(":key", "counter1");
        client.put("UPDATE key_value SET value = value + 1 WHERE key = :key");
        client.bind(":key", "counter1");

        rs = client.execute(false);

        for (Row row : rs) {
            String key = row.get("key");
            Long value = row.get("value");

            System.out.println("Previous = " + key + " : " + value);
        }

        client.put("SELECT * FROM key_value");
        rs = client.execute(true);

        for (Row row : rs) {
            String key = row.get("key");
            Long value = row.get("value");

            System.out.println("Current = " + key + " : " + value);
        }

        client.shutdown();
    }

    @Test
    public void getAndIncrementPrepared() {
        Resql client = ResqlClient.create(new Config());

        client.put("DROP TABLE IF EXISTS key_value;");
        client.put(
                "CREATE TABLE key_value (key TEXT PRIMARY KEY, value INTEGER)");
        client.execute(false);

        client.put("INSERT INTO key_value VALUES(:key, :value)");
        client.bind(":key", "counter1");
        client.bind(":value", 1000);
        ResultSet rs = client.execute(false);
        System.out.println("Changes : " + rs.linesChanged());


        PreparedStatement select = client.prepare("SELECT * FROM key_value WHERE key = :key");
        PreparedStatement update = client.prepare("UPDATE key_value SET value = value + 1 WHERE key = :key");

        for (int i = 0; i < 10; i++) {
            client.put(select);
            client.bind(":key", "counter1");
            client.put(update);
            client.bind(":key", "counter1");
            rs = client.execute(false);

            Row row = rs.iterator().next();
            System.out.println("Current = " + row.get("key") + " : " + row.get("value"));
        }
        client.shutdown();
    }
}
