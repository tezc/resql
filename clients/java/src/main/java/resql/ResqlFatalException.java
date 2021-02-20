package resql;

public class ResqlFatalException extends ResqlException {
    public ResqlFatalException(String s) {
        super(s);
    }

    public ResqlFatalException(Throwable t) {
        super(t);
    }
}
