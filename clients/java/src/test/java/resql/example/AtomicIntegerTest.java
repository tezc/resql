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
