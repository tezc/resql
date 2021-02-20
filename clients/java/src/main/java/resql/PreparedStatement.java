package resql;

public interface PreparedStatement {

    /**
     * Get prepared statement sql provided by user
     *
     * @return Prepared statement sql
     */
    String getStatement();
}
