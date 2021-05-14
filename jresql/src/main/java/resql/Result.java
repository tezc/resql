/*
 * BSD-3-Clause
 *
 * Copyright 2021 Ozan Tezcan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

package resql;

import java.util.*;

class Result implements ResultSet, Iterator<Row> {

    private RawBuffer buf;
    private long lastRowId;
    private int linesChanged;
    private int nextResultSet;
    private int rowCount;
    private int remainingRows;
    private int columnCount;
    final Map<String, Integer> columnMap = new HashMap<>();
    final List<String> columnNames = new ArrayList<>();
    final List<Object> columnValues = new ArrayList<>();
    private final ResultRow row = new ResultRow();

    Result() {
    }

    void setBuf(RawBuffer buf) {
        columnMap.clear();
        columnNames.clear();
        columnValues.clear();

        this.buf = buf;
        nextResultSet = this.buf.position();

        nextResultSet();
    }

    @Override
    public int linesChanged() {
        return linesChanged;
    }

    @Override
    public long lastRowId() {
        return lastRowId;
    }

    @Override
    public int rowCount() {
        return rowCount;
    }

    @Override
    public boolean hasNext() {
        return remainingRows > 0;
    }

    @Override
    public Row next() {
        if (remainingRows-- <= 0) {
            return null;
        }

        columnValues.clear();

        for (int i = 0; i < columnCount; i++) {
            int param = buf.get();
            switch (param) {
                case Msg.PARAM_INTEGER:
                    columnValues.add(buf.getLong());
                    break;
                case Msg.PARAM_FLOAT:
                    columnValues.add(buf.getDouble());
                    break;
                case Msg.PARAM_TEXT:
                    columnValues.add(buf.getString());
                    break;
                case Msg.PARAM_BLOB:
                    columnValues.add(buf.getBlob());
                    break;
                case Msg.PARAM_NULL:
                    columnValues.add(null);
                    break;
                default:
                    throw new ResqlSQLException(
                            "Unexpected column type : " + param);
            }
        }

        return row;
    }

    @Override
    public boolean nextResultSet() {
        linesChanged = 0;
        rowCount = -1;
        columnCount = -1;
        remainingRows = -1;

        buf.position(nextResultSet);

        if (buf.get() != Msg.FLAG_OP) {
            return false;
        }

        nextResultSet = buf.position() + buf.getInt();
        linesChanged = buf.getInt();
        lastRowId = buf.getLong();

        int flag = buf.get();
        if (flag == Msg.FLAG_ROW) {
            columnCount = buf.getInt();

            for (int i = 0; i < columnCount; i++) {
                String columnName = buf.getString();
                columnMap.put(columnName, i);
                columnNames.add(columnName);
            }
            rowCount = buf.getInt();
            remainingRows = rowCount;
        } else if (flag != Msg.FLAG_OP_END) {
            throw new ResqlException("Unexpected value : " + flag);
        }

        return true;
    }

    @Override
    public Iterator<Row> iterator() {
        return this;
    }


    class ResultRow implements Row {

        ResultRow() {

        }

        public int columnCount() {
            return columnMap.size();
        }


        @Override
        @SuppressWarnings("unchecked")
        public <T> T get(String name)
        {
            Integer index = columnMap.get(name);
            if (index == null) {
                throw new ResqlSQLException(
                        "Column does not exists : " + name);
            }

            return (T) columnValues.get(index);
        }

        @Override
        @SuppressWarnings("unchecked")
        public <T> T get(int index) {
            try {
                return (T) columnValues.get(index);
            } catch (IndexOutOfBoundsException e) {
                throw new ResqlSQLException(e);
            }
        }

        @Override
        public String getColumnName(int index) {
            try {
                return columnNames.get(index);
            } catch (IndexOutOfBoundsException e) {
                throw new ResqlSQLException(e);
            }
        }

        @Override
        public String toString() {
            StringBuilder s = new StringBuilder();
            for (Object o : columnValues) {
                if (s.length() > 0) {
                    s.append(", ");
                }
                s.append(o);
            }

            return s.toString();
        }
    }

}
