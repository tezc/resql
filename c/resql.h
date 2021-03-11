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
#include <stdlib.h>

#define RESQL_VERSION "0.0.10"

#ifdef RESQL_HAVE_CONFIG_H
    #include "config.h"
#else
    #define resql_malloc  malloc
    #define resql_calloc  calloc
    #define resql_realloc realloc
    #define resql_free    free
#endif

#ifdef __cplusplus
extern "C" {
#endif

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
    enum resql_type type; // Column type
    const char *name;     // Column name
    int32_t len;          // Column value length, applicable to text and blob

    union
    {
        const void *blob;
        const char *text;
        int64_t intval;
        double floatval;
    };
};

struct resql_config
{
    // client name, if passed null, random string will be assigned.
    const char *client_name;

    // cluster name, cannot be null or incorrect.
    const char *cluster_name;

    /**
     * Node urls, it's okay to give one url,
     * client will discover other nodes later.
     *
     * e.g "tcp://127.0.0.1:7600 tcp://127.0.0.1:7601 tcp://127.0.0.1:7602"
     */
    const char *urls;

    /**
     * Outgoing bind address for the client.
     */
    const char *outgoing_addr;

    /**
     * Outgoing bind port for client.
     */
    const char *outgoing_port;

    /**
     * Operation timeout, this applies to initial connection, re-connection if
     * necessary or to statement executions. On timeout, client will return
     * RESQL_ERROR. Error message can be acquired by calling
     * resql_errstr();
     */
    uint32_t timeout_millis;
};

/**
 * Initialize library once when your application starts and terminate once
 * before shutting down.
 *
 * @return 'RESQL_OK' on success, negative on failure.
 */
int resql_init();
int resql_term();

/**
 * Create client and connect to the server.
 *
 * @param c      client
 * @param conf config
 * @return       RESQL_OK           : on success, connection established.
 *               RESQL_OOM          : failed to allocate memory for
 *                                        resql object.
 *               RESQL_CONFIG_ERROR : misconfiguration, call
 *                                        resql_errstr() for error message.
 */
int resql_create(resql **c, struct resql_config *conf);

/**
 * Shutdown client, it will try to close connection gracefully so server can
 * free allocated resources. It will deallocate resql object, so it's not
 * safe to use client object after this call.
 *
 * @param c client
 * @return  RESQL_OK    : on success
 *          RESQL_ERROR : on error, e.g connection problem.
 */
int resql_shutdown(resql *c);

/**
 *  Get last error string.
 *
 * @param c client
 * @return  error string
 */
const char *resql_errstr(resql *c);

/**
 * Prepared statement. If client exists gracefully by calling
 * resql_shutdown(), client's prepared statements are deallocated on the
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
 * resql_shutdown(), client's prepared statements are deallocated on the
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
 * @param c    client
 * @param stmt stmt
 */
void resql_put_prepared(resql *c, const resql_stmt *stmt);

/**
 * Add statement to the current operation batch.
 *
 * resql_stmt *stmt;
 * resql_result *rs;
 *
 * resql_put_sql(client, "SELECT * FROM TEST");
 * resql_put_sql(client, "SELECT * FROM ANOTHER");
 * resql_put_prepared(client, stmt);
 * resql_bind_index_int(client, 0, id);
 *
 * int rc = resql_exec(client, readonly, &rs);
 *
 * @param c   client
 * @param sql sql
 */
void resql_put_sql(resql *c, const char *sql);

/**
 * Bind values, either :
 * resql_bind_param_int(c, ":param", "value");
 * resql_bind_index_int(c, 0, "value");
 */
void resql_bind_param_int(resql *c, const char *param, int64_t val);
void resql_bind_param_float(resql *c, const char *param, double val);
void resql_bind_param_text(resql *c, const char *param, const char *val);
void resql_bind_param_blob(resql *c, const char *param, int len, void *data);
void resql_bind_param_null(resql *c, const char *param);

void resql_bind_index_int(resql *c, int index, int64_t val);
void resql_bind_index_float(resql *c, int index, double val);
void resql_bind_index_text(resql *c, int index, const char *val);
void resql_bind_index_blob(resql *c, int index, int len, void *data);
void resql_bind_index_null(resql *c, int index);

/**
 * Execute added statements.
 *
 * resql_stmt *stmt;
 * resql_result *rs;
 *
 * resql_put_sql(client, "SELECT * FROM TEST");
 * resql_put_sql(client, "SELECT * FROM ANOTHER");
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
 * resql_put_sql(client, "SELECT * FROM TEST");
 * resql_put_sql(client, "SELECT * FROM ANOTHER");
 * resql_put_prepared(client, stmt);
 * // Want to dismiss these statements.
 *
 * resql_clear(client);
 *
 *
 * @param client client
 */
void resql_clear(resql *client);

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

/**
 * Get row. Returns struct resql_column list, call resql_column_count() to
 * get column count;
 *
 * @param rs result
 * @return   column array.
 */
struct resql_column *resql_row(resql_result *rs);

#ifdef __cplusplus
}
#endif

#endif
