/*
 * MIT License
 *
 * Copyright (c) 2021 Ozan Tezcan
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

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
