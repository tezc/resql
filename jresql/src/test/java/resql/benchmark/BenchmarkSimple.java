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

package resql.benchmark;

import org.junit.jupiter.api.Test;
import org.openjdk.jmh.annotations.*;
import org.openjdk.jmh.infra.Blackhole;
import org.openjdk.jmh.runner.Runner;
import org.openjdk.jmh.runner.options.Options;
import org.openjdk.jmh.runner.options.OptionsBuilder;
import org.openjdk.jmh.runner.options.TimeValue;
import resql.Config;
import resql.PreparedStatement;
import resql.Resql;
import resql.ResqlClient;

import java.nio.charset.StandardCharsets;
import java.util.concurrent.TimeUnit;


public class BenchmarkSimple {

    @Test
    public void launchBenchmark() throws Exception {

        Resql client = ResqlClient.create(new Config());

        client.put("DROP TABLE IF EXISTS multi;");
        client.put("CREATE TABLE multi (id INTEGER PRIMARY KEY, name TEXT, " +
                           "points FLOAT, data BLOB, date TEXT);");
        client.execute(false);

        for (int i = 0; i < 1000; i++) {
            client.put("INSERT INTO multi VALUES(?, ?, ?, ?, ?);");
            client.bind(0, i);
            client.bind(1, "jane" + i);
            client.bind(2, 3.221 + i);
            client.bind(3, ("blob" + i).getBytes(StandardCharsets.UTF_8));
            client.bind(4, null);
        }
        client.execute(false);
        client.shutdown();


        Options opt = new OptionsBuilder().include(
                this.getClass().getName() + ".*")
                                          .mode(Mode.Throughput)
                                          .timeUnit(TimeUnit.SECONDS)
                                          .warmupTime(TimeValue.seconds(2))
                                          .warmupIterations(1)
                                          .measurementTime(
                                                  TimeValue.seconds(5))
                                          .measurementIterations(1)
                                          .threads(30)
                                          .forks(1)
                                          .shouldFailOnError(true)
                                          .build();

        new Runner(opt).run();
    }

    @State(Scope.Thread)
    public static class BenchmarkState {
        Resql client;
        PreparedStatement p;
        int i = 0;

        @Setup(Level.Trial)
        public void initialize() {
            client = ResqlClient.create(new Config());
            p = client.prepare("SELECT * FROM multi WHERE id = ?;");
        }

        @TearDown
        public void tearDown() {
            client.shutdown();
        }
    }

    @Benchmark
    public void benchmarkSQL(BenchmarkState state, Blackhole bh)
            throws InterruptedException {

        Resql client = state.client;

        client.put("SELECT * FROM multi WHERE id = '" + state.i + "';");
        state.i++;
        bh.consume(client.execute(true));
    }

    @Benchmark
    public void benchmarkPrepared(BenchmarkState state, Blackhole bh) {

        Resql client = state.client;
        PreparedStatement p = state.p;

        client.put(p);
        client.bind(0, (state.i));
        state.i++;
        bh.consume(client.execute(true));
    }

    @Benchmark
    public void benchmarkSelect(BenchmarkState state, Blackhole bh)
            throws InterruptedException {

        Resql client = state.client;

        client.put("SELECT '1';");
        bh.consume(client.execute(true));
    }
}
