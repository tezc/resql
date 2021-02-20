package resql;

class Prepared implements PreparedStatement {
    final long id;
    final String statement;

    Prepared(long id, String statement) {
        this.id = id;
        this.statement = statement;
    }

    public String getStatement() {
        return statement;
    }

    public long getId() {
        return id;
    }
}
