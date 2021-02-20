package resql;

public final class ResqlClient {

    private ResqlClient() {

    }

    public static Resql create(Config config) {
        try {
            return new Client(config);
        } catch (Exception e) {
            if (e instanceof ResqlException) {
                throw e;
            }

            throw new ResqlException(e);
        }
    }
}
