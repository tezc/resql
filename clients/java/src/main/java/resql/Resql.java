package resql;

public interface Resql {

    /**
     * Prepare Statement
     *
     * @param sql Statement sql
     * @return Prepared statement
     */
    PreparedStatement prepare(String sql);

    /**
     * Delete prepared statement.
     *
     * @param statement Prepared statement
     */
    void delete(PreparedStatement statement);

    /**
     * Add statement to the current operation batch.
     *
     * @param statement Statement
     */
    void put(String statement);

    /**
     * Add statement to current operation batch.
     *
     * @param statement Statement
     */
    void put(PreparedStatement statement);

    /**
     * Bind value by index. 'put' must be called before binding value.
     * Otherwise, throws ResqlSQLException.
     *
     * Valid 'value' types are Long, Double, String, byte[] and null.
     *
     * e.g :
     * client.put("SELECT * FROM test WHERE key = '?');
     * client.bind(0, "jane");
     *
     * @param index index
     * @param value value
     */
    void bind(int index, Object value);

    /**
     * Bind value by index. 'put' must be called before binding value.
     * Otherwise, throws ResqlSQLException.
     *
     * Valid 'value' types are Long, Double, String, byte[] and null.
     *
     * e.g :
     * client.put("SELECT * FROM test WHERE key = :key");
     * client.bind(":key", "jane");
     *
     * @param param param
     * @param value value
     */
    void bind(String param, Object value);

    /**
     * Execute statements. Set readonly to 'true' if all the statements are
     * readonly. This is a performance optimization. If readonly is set even
     * statements are not, operation will fail and ResultSet return value will
     * indicate the error.
     *
     * @param readonly true if all the statements are readonly.
     * @return Operation result list. ResultSet will contain multiple
     * results if multiple statements are executed.
     */
    ResultSet execute(boolean readonly);


    /**
     * Clear current operation batch. e.g You add a few statements and before
     * calling execute() if you want to cancel it and start all over again.
     */
    void clear();

    /**
     * Shutdown client.
     */
    void shutdown();
}
