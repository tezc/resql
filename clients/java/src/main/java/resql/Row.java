package resql;

public interface Row {

    /**
     * Get column value by name.
     *
     * Valid return types are Long, Double, String and byte[].
     * If column value is null, return value is null.
     * If column name does not exist, throws ResqlSqlException.
     *
     * @param columnName column name
     * @param <T>        return type
     * @return           column value
     */
    <T> T get(String columnName);

    /**
     * Get column value by index.
     *
     * Valid return types are Long, Double, String and byte[].
     * If column value is null, return value is null.
     * If index is out of range, throws ResqlSqlException.
     *
     * @param index column index
     * @param <T>   return type
     * @return      column value
     */
    <T> T get(int index);

    /**
     * Get column name by index.
     * If index is out of range, throws ResqlSqlException.
     *
     * @param index index
     * @return      column name
     */
    String getColumnName(int index);

    /**
     * Get column count
     * @return column count
     */
    int columnCount();
}
