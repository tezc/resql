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
                                          .mode(Mode.SampleTime)
                                          .timeUnit(TimeUnit.MICROSECONDS)
                                          .warmupTime(TimeValue.seconds(5))
                                          .warmupIterations(1)
                                          .measurementTime(
                                                  TimeValue.seconds(5))
                                          .measurementIterations(1)
                                          .threads(4)
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