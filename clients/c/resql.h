/*
 * MIT License
 *
 * Copyright (c) 2021 Resql Authors
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

#ifndef RESQL_H
#define RESQL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


typedef struct resql_result resql_result;
typedef struct resql resql;
typedef uint64_t resql_stmt;

// clang-format off
enum resql_type
{
    RESQL_INTEGER = 0,
    RESQL_FLOAT   = 1,
    RESQL_TEXT    = 2,
    RESQL_BLOB    = 3,
    RESQL_NULL    = 4,
};

enum resql_rc
{
    RESQL_OK           = 0,  // Success return value
    RESQL_ERROR        = -1, // Used for network failures, disconnections.
    RESQL_SQL_ERROR    = -2, // User error, e.g SQL syntax issue,
    RESQL_CONFIG_ERROR = -5, // Returns only from resql_create()
    RESQL_OOM          = -6, // Returns only from resql_create()
    RESQL_PARTIAL      = -7, // Internal
    RESQL_INVALID      = -8, // Internal
    RESQL_FATAL        = -9  // Internal
};

// clang-format on

struct resql_column
{
    enum resql_type type;
    const char *name;
    int32_t len;

    union
    {
        const void *blob;
        const char *text;
        uint64_t num;
        double real;
    };
};

struct resql_config
{
    // client name, if passed null, random string will be assigned.
    const char *client_name;

    // cluster name, cannot be null or incorrect.
    const char *cluster_name;

    /**
     * Node uris, it's okay to give one uri,
     * client will discover other nodes later.
     *
     * e.g "tcp://127.0.0.1:7600 tcp://127.0.0.1:7601 tcp://127.0.0.1:7602"
     */
    const char *uris;

    /**
     * Outgoing bind address for the client.
     */
    const char *source_addr;

    /**
     * Outgoing bind port for client.
     */
    const char *source_port;

    /**
     * Operation timeout, this applies to initial connection, re-connection if
     * necessary or to statement executions. On timeout, client will return
     * RESQL_ERROR. Error message can be acquired by calling
     * resql_errstr();
     */
    uint32_t timeout;
};

/**
 * Create client and connect to the server.
 *
 * @param c      client
 * @param config config
 * @return       RESQL_OK           : on success, connection established.
 *               RESQL_OOM          : failed to allocate memory for
 *                                        resql object.
 *               RESQL_CONFIG_ERROR : misconfiguration, call
 *                                        resql_errstr() for error message.
 */
int resql_create(resql **c, struct resql_config *config);

/**
 * Destroy client, it will try to close connection gracefully so server can
 * free allocated resources. It will deallocate resql object, so it's not
 * safe to use client object after this call.
 *
 * @param c client
 * @return  RESQL_OK    : on success
 *          RESQL_ERROR : on error, e.g connection problem.
 */
int resql_destroy(resql *c);

/**
 *  Get last error string.
 *
 * @param c client
 * @return  error string
 */
const char *resql_errstr(resql *c);

/**
 * Prepared statement. If client exists gracefully by calling
 * resql_destroy(), client's prepared statements are deallocated on the
 * server automatically. This operation cannot be pipelined with others.
 *
 * parametered statements and indexed statements are supported.
 *
 * resql_stmt stmt;
 *
 * e.g
 * resql_prepare(client, "SELECT * FROM test WHERE name = ?", &stmt);
 * resql_prepare(client, "SELECT * FROM test WHERE name = :name", &stmt);
 *
 * @param c   client
 * @param sql sql statement.
 * @return    RESQL_OK        : on success,
 *            RESQL_ERROR     : on connection failure or on out of memory.
 *            RESQL_SQL_ERROR : on misuse, e.g syntax error in sql string.
 */
int resql_prepare(resql *c, const char *sql, resql_stmt *stmt);

/**
 * Delete prepared statement. If client exists gracefully by calling
 * resql_destroy(), client's prepared statements are deallocated on the
 * server automatically. This operation cannot be pipelined with others.
 *
 * @param c    client
 * @param stmt prepared statement
 * @return     RESQL_OK        : on success,
 *             RESQL_ERROR     : on connection failure or on out of memory.
 *             RESQL_SQL_ERROR : on misuse, e.g trying to delete non
 *                                   existing prepared statement.
 */
int resql_del_prepared(resql *c, resql_stmt *stmt);

/**
 * Add statement to the current operation batch.
 *
 * resql_stmt *stmt;
 * resql_result *rs;
 *
 * resql_put_statement(client, "SELECT * FROM TEST");
 * resql_put_statement(client, "SELECT * FROM ANOTHER");
 * resql_put_prepared(client, stmt);
 * resql_bind_index_int(client, 0, id);
 *
 * int rc = resql_exec(client, readonly, &rs);
 *
 * @param c
 * @param sql
 * @return
 */
int resql_put_prepared(resql *c, const resql_stmt *stmt);

/**
 * Add statement to the current operation batch.
 *
 * resql_stmt *stmt;
 * resql_result *rs;
 *
 * resql_put_statement(client, "SELECT * FROM TEST");
 * resql_put_statement(client, "SELECT * FROM ANOTHER");
 * resql_put_prepared(client, stmt);
 * resql_bind_index_int(client, 0, id);
 *
 * int rc = resql_exec(client, readonly, &rs);
 *
 * @param c
 * @param sql
 * @return
 */
int resql_put_sql(resql *c, const char *sql);

/**
 * Bind with parameter. String and blob is copied
 * into internal buffer, reSQL won't call free() for them.
 *
 * resql_bind_param(c, ":intcolumn", "%d", 3);
 * resql_bind_param(c, ":realcolumn", "%f", 3.11);
 * resql_bind_param(c, ":textcolumn", "%s", "value");
 * resql_bind_param(c, ":blobcolumn", "%b", 10, "blobvalue");
 * resql_bind_param(c, ":anyother", "%n"); // NULL binding
 *
 * @param c      client
 * @param param  param
 * @param fmt    fmt, valid fmts are '%d' : int64_t
 *                                   '%f' : double
 *                                   '%s' : const char*
 *                                   '%b' : int, const void*
 *                                   '%n' : bind null, no value needed
 * @param ...
 * @return
 */
int resql_bind_param(resql *c, const char *param, const char *fmt, ...);

/**
 * Bind with index, indexes starts from '0'. String and blob is copied
 * into internal buffer, Resql won't call free() for them.
 *
 * resql_bind_index(c, 0, "%d", 3);
 * resql_bind_index(c, 1, "%f", 3.11);
 * resql_bind_index(c, 2, "%s", "value");
 * resql_bind_index(c, 3, "%b", 10, "blobvalue");
 * resql_bind_index(c, 4, "%n"); // NULL binding
 *
 * @param c      client
 * @param param  param
 * @param fmt    fmt, valid fmts are '%d' : int64_t
 *                                   '%f' : double
 *                                   '%s' : const char*
 *                                   '%b' : int, const void*
 *                                   '%n' : bind null, no value needed
 * @param ...
 * @return
 */
int resql_bind_index(resql *c, int index, const char *fmt, ...);

/**
 * Execute added statements.
 *
 * resql_stmt *stmt;
 * resql_result *rs;
 *
 * resql_put_statement(client, "SELECT * FROM TEST");
 * resql_put_statement(client, "SELECT * FROM ANOTHER");
 * resql_put_prepared(client, stmt);
 * resql_bind_index_int(client, 0, id);
 *
 * int rc = resql_exec(client, readonly, &rs);
 *
 *  * while (resql_next(rs)) {
 *     while(resql_row(rs)) {
 *          ...
 *     }
 * }
 *
 * @param client
 * @param readonly
 * @param rs
 * @return
 */
int resql_exec(resql *client, bool readonly, resql_result **rs);

/**
 * After adding statements, cancel operation without executing.
 * e.g
 * resql_stmt *stmt;
 *
 * resql_put_statement(client, "SELECT * FROM TEST");
 * resql_put_statement(client, "SELECT * FROM ANOTHER");
 * resql_put_prepared(client, stmt);
 * // Want to dismiss these statements.
 *
 * resql_clear(client);
 *
 *
 * @param client client
 * @return       return value, RESQL_OK or RESQL_ERROR.
 */
int resql_clear(resql *client);

/**
 * Resets row iterator, so you can go over the rows again.
 *
 * @param rs result
 */
void resql_reset_rows(resql_result *rs);

/**
 * Advance to the next result. Typical usage :
 *
 * resql_result *rs;
 * resql_exec(client, readonly, &rs);
 * while (resql_next(rs)) {
 *     while(resql_row(rs)) {
 *          ...
 *     }
 * }
 *
 *
 * @param rs result
 * @return   if next row exists.
 */
bool resql_next(resql_result *rs);

/**
 * Get row count for the current statement. Only meaningful for SELECT
 * statements. If does not apply to current statement (e.g statement is UPDATE),
 * returned value will be '-1'
 *
 * @param rs result
 * @return   row count
 */
int resql_row_count(resql_result *rs);

/**
 * Get count the number of rows modified for the current statement.
 * Return value is only meaningful for INSERT, UPDATE or DELETE statements.
 * Otherwise, its value is unspecified.
 *
 * @param rs result
 * @return   rows modified
 */
int resql_changes(resql_result *rs);

/**
 * Column count for the current result. Meaningful only for statements which
 * return rows. e.g SELECT. Must be called after resql_next() call.
 *
 * @param rs result
 * @return   column count
 */
int resql_column_count(resql_result *rs);

struct resql_column* resql_row(resql_result *rs);

#endif
