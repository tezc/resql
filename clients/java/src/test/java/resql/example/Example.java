package resql.example;

import org.junit.jupiter.api.Test;
import resql.*;

import java.util.Arrays;

public class Example {

    @Test
    public void minimal() {
        try (Resql client = ResqlClient.create(new Config())) {
            client.put("SELECT 'Hello World!'");
            ResultSet rs = client.execute(true);
            for (Row row : rs) {
                System.out.println(row);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Test
    public void connect() {
        Config c = new Config().setClientName("test-client")
                               .setClusterName("cluster")
                               .setTimeoutMillis(10000)
                               .setUrls(Arrays.asList("tcp://127.0.0.1:7600",
                                                      "tcp://127.0.0.1:7601"));

        try (Resql client = ResqlClient.create(c)) {
            client.put("SELECT 'Hello World!'");
            client.execute(true);
        }  catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Test
    public void executingStatements() {
        try (Resql client = ResqlClient.create(new Config())) {
            client.put("CREATE TABLE test1 (key TEXT, value TEXT);");

            // 'true' for readonly operations only, e.g : SELECT.
            client.execute(false);

            // Drop table
            client.put("DROP TABLE test1;");
            client.execute(false);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Test
    public void bindingParameters() {
        try (Resql client = ResqlClient.create(new Config())) {
            client.put("CREATE TABLE test1 (key TEXT, value TEXT);");
            client.execute(false);

            // Option 1, with parameter name
            client.put("INSERT INTO test VALUES(:name,:lastname);");
            client.bind(":name", "jane");
            client.bind(":lastname", "doe");
            client.execute(false);

            // Option 2, with index
            client.put("INSERT INTO test VALUES(?, ?)");
            client.bind(0, "jack");
            client.bind(1, "does");
            client.execute(false);

            // Drop table
            client.put("DROP TABLE test1;");
            client.execute(false);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Test
    public void executingQueries() {

        try (Resql client = ResqlClient.create(new Config())) {
            client.put("CREATE TABLE test (name TEXT, lastname TEXT);");

            // 'true' for readonly operations only, e.g : SELECT.
            client.execute(false);

            // You can execute multiple operations in one round-trip
            client.put("INSERT INTO test VALUES('jane', 'doe');");
            client.put("INSERT INTO test VALUES('jack', 'doe');");
            client.put("INSERT INTO test VALUES('joe', 'doe');");
            client.execute(false);


            client.put("SELECT * FROM test;");

            // This is a readonly operation, we can pass 'true'
            ResultSet rs = client.execute(true);

            for (Row row : rs) {
                System.out.println(" name : " + row.get("name") +
                                  " last_name : " + row.get("lastname"));
            }

            // Cleanup
            client.put("DROP TABLE test;");
            client.execute(false);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Test
    public void preparedStatements() {
        try (Resql client = ResqlClient.create(new Config())) {
            client.put("CREATE TABLE test (name TEXT, lastname TEXT);");
            client.execute(false);

            PreparedStatement statement;

            //Option-1, with parameter index
            statement = client.prepare("INSERT INTO test VALUES(?,?)");
            client.put(statement);
            client.bind(0, "jane");
            client.bind(1, "doe");
            client.execute(false);

            // Clean-up when done.
            client.delete(statement);

            //Option-2, with parameter name
            statement = client.prepare("INSERT INTO test VALUES(:name,:lastname)");
            client.put(statement);
            client.bind(":name", "jane");
            client.bind(":lastname", "doe");
            client.execute(false);

            // Clean-up when done.
            client.delete(statement);

            // Cleanup
            client.put("DROP TABLE test;");
            client.execute(false);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Test
    public void getAndIncrementExample() {
        try (Resql client = ResqlClient.create(new Config())) {
            client.put(
                    "CREATE TABLE test (key TEXT PRIMARY KEY, value INTEGER)");
            client.put("INSERT INTO test VALUES('mykey', 0);");
            client.execute(false);


            // Demo for getAndIncrement atomically.
            client.put("SELECT * FROM test WHERE key = 'mykey';");
            client.put("UPDATE test SET value = value + 1 WHERE key = 'mykey';");
            client.put("SELECT * FROM test WHERE key = 'mykey';");

            // Pass false as we have an UPDATE in batch.
            ResultSet rs = client.execute(false);

            // rs has three result sets, each corresponds to operations
            // that we added into the batch.

            // First operation was SELECT
            for (Row row : rs) {
                System.out.println("Value was : " + row.get("value"));
            }

            // Advance to next result set which is for INSERT.
            rs.nextResultSet();
            System.out.println("Changes : " + rs.linesChanged());

            // Advance to next result set which is for SELECT again.
            rs.nextResultSet();
            for (Row row : rs) {
                System.out.println("Value is now : " + row.get("value"));
            }

            // Cleanup
            client.put("DROP TABLE test;");
            client.execute(false);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Test
    public void showcase() {
        Config config = new Config().setOutgoingAddr("127.0.0.1").setOutgoingPort(18000).setClientName("myclient");
        try (Resql client = ResqlClient.create(config)) {
            client.put("CREATE TABLE test (name TEXT, lastname TEXT);");
            client.execute(false);

            PreparedStatement statement;

            //Option-1, with parameter index
            statement = client.prepare("INSERT INTO test VALUES(?,?)");
            statement = client.prepare("SELECT * FROM test WHERE name=:name");

            Thread.sleep(100000);

            // Cleanup
            client.put("DROP TABLE test;");
            client.execute(false);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

}
