package resql;

/**
 * Exception for connection related errors.
 */
public class ResqlException extends RuntimeException {
    public ResqlException(String s) {
        super(s);
    }

    public ResqlException(Throwable t) {
        super(t);
    }
}
