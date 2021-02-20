package resql.benchmark;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import resql.Config;
import resql.Resql;
import resql.ResqlClient;

public class LatencyTest {
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
    public void latency() {
        for (int i = 0; i < 10000000; i++) {
            long ts = System.nanoTime();
            client.put("Select '1';");
            client.execute(true);
            System.out.println("Took " + (System.nanoTime() - ts));
        }

    }
}
