package resql.example;

import org.junit.jupiter.api.Test;
import resql.*;

public class KeyValueTest {

    @Test
    public void simple() {
        Resql client = ResqlClient.create(new Config());

        client.put("SELECT * FROM resql_info");
        ResultSet rs = client.execute(true);

        for (Row row : rs) {
            for (int i = 0; i < row.columnCount(); i++) {
                System.out.printf("%-20s : %-20s \n", row.get(i), row.get(i));
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
