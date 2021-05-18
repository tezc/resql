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

public interface Resql extends AutoCloseable {

    String VERSION = "0.1.3";

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
