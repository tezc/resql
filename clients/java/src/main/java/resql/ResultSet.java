package resql;

public interface ResultSet extends Iterable<Row> {

    /**
     * Advance to the next result set.
     *
     * @return 'true' if next result set exists.
     */
    boolean nextResultSet();

    /**
     * @return Number of lines changed for the current INSERT, UPDATE or DELETE
     *         statement. For other statements, returned value is unspecified.
     */
    int linesChanged();

    /**
     * Get row count for the current result set, -1 if not applicable
     *
     * @return row count.
     */
    int rowCount();
}
