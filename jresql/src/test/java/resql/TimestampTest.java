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
import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

public class TimestampTest {

    Resql client;

    @BeforeEach
    public void setup() {
        client = ResqlClient.create(new Config());
        client.put("DROP TABLE IF EXISTS ts_ms_table;");
        client.put("CREATE TABLE ts_ms_table (id INTEGER PRIMARY KEY AUTOINCREMENT, ts_ms TIMESTAMP DEFAULT (STRFTIME('%Y-%m-%d %H:%M:%f', 'NOW')));;");
        client.execute(false);
    }

    @AfterEach
    public void tearDown() {
        if (client != null) {
            client.clear();
            client.put("DROP TABLE IF EXISTS ts_ms_table;");
            client.execute(false);
            client.shutdown();
        }
    }

    @Test
    public void testTimestampIncrementFor10ms() throws InterruptedException {
        ResultSet rs;
        for (int i = 0; i < 100; i++) {
            client.put("INSERT INTO ts_ms_table DEFAULT VALUES;");
            rs = client.execute(false);
            assert (rs.linesChanged() == 1);
            assert (rs.lastRowId() == i + 1);
            Thread.sleep(10);
        }

        client.put("SELECT DISTINCT(ts_ms) FROM ts_ms_table;");
        rs = client.execute(true);
        assert (rs.rowCount() > 1);
    }
}
