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

import java.util.concurrent.TimeUnit;


public class BenchmarkSimple {

    public void launchBenchmark() throws Exception {


        Options opt = new OptionsBuilder().include(
                this.getClass().getName() + ".*")
                                          .mode(Mode.AverageTime)
                                          .timeUnit(TimeUnit.MICROSECONDS)
                                          .warmupTime(TimeValue.seconds(2))
                                          .warmupIterations(1)
                                          .measurementTime(
                                                  TimeValue.seconds(5))
                                          .measurementIterations(1)
                                          .threads(1)
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
            client.put("CREATE TABLE IF NOT EXISTS multi (id INTEGER PRIMARY KEY, name TEXT, " +
                               "points FLOAT, data BLOB, date TEXT);");
            client.execute(false);
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
}
