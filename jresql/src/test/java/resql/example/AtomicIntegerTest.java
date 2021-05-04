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
