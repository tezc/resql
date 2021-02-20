package resql;

/**
 * Exception for unsuccessful SQL operations.
 */
public class ResqlSQLException extends ResqlException {
    public ResqlSQLException(String s) {
        super(s);
    }

    public ResqlSQLException(Throwable t) {
        super(t);
    }
}
