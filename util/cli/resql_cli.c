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

#include "resql.h"
#include "linenoise.h"
#include "sc_option.h"
#include "sc_uri.h"

#include <ctype.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define ANSI_RESET    "\x1b[0m"
#define COLUMN_HEADER "\x1b[0;35m"
#define HINT_COLOR    35

struct resql_cli
{
    struct sc_uri *uri;
    bool vertical;
    char **cmds;
    int count;
};

#define RESQL_CLI_VERSION "0.0.10-latest"

static struct resql_cli s_cli;
static struct resql *client;

static void resql_cli_cmdline_usage()
{
    printf("\n Resql CLI version : %s \n\n", RESQL_CLI_VERSION);
    printf(" -u=<uri>          --uri=<uri>              ex: --uri=127.0.0.1:7600 \n"
           " -c=<command>      --command=<command>      ex: ./resql-cli -c=\"SELECT * FROM resql_sessions\"\n"
           " -h                --help                   Print this help and exit \n"
           " -v,               --version                Print version and exit   \n"
           "\n\n");
}

void resql_cli_read_cmdline(struct resql_cli *cli, int argc, char *argv[])
{
    static struct sc_option_item options[] = {
            {'c', "command"},
            {'h', "help"},
            {'u', "uri"},
            {'v', "version"},
    };

    struct sc_option opt = {.argv = argv,
                            .count = sizeof(options) / sizeof(options[0]),
                            .options = options};
    char ch;
    char *value;

    for (int i = 1; i < argc; i++) {
        ch = sc_option_at(&opt, i, &value);

        switch (ch) {
        case 'c':
            if (value == NULL) {
                printf("Invalid -c option \n");
                exit(1);
            }

            s_cli.cmds = realloc(s_cli.cmds, (size_t) s_cli.count + 1);
            s_cli.cmds[s_cli.count] = strdup(value);
            s_cli.count++;
            break;
        case 'u': {
            struct sc_uri *uri;
            uri = sc_uri_create(value);
            if (uri == NULL) {
                printf("resql: Error parsing uri %s \n", optarg);
                exit(1);
            }

            sc_uri_destroy(cli->uri);
            cli->uri = uri;
        } break;

        case 'h':
        case 'v':
            resql_cli_cmdline_usage();
            exit(0);
        default:
            printf("resql-cli : Unknown option %s \n", argv[argc]);
            resql_cli_cmdline_usage();
            exit(1);
        }
    }
}

static int strlen30(const char *z)
{
    const char *z2 = z;
    while (*z2) {
        z2++;
    }
    return 0x3fffffff & (int) (z2 - z);
}

#define MAX(a, b) ((a) < (b) ? (b) : (a))

void resql_print_vertical(struct resql_result *rs)
{
    int wmax = 15;
    uint32_t columns;
    struct resql_column *row;

    columns = resql_column_count(rs);
    while ((row = resql_row(rs)) != NULL) {
        for (uint32_t i = 0; i < columns; i++) {
            wmax = MAX(wmax, (int) strlen(row[i].name));
        }
    }

    resql_reset_rows(rs);

    int rownum = 0;
    while ((row = resql_row(rs)) != NULL) {
        printf("\n%-*s : %d \n\n", wmax, "Row number", rownum);
        rownum++;

        columns = resql_column_count(rs);
        for (size_t i = 0; i < columns; i++) {
            switch (row[i].type) {
            case RESQL_INTEGER:
                printf("%-*s : %-15llu \n", wmax, row[i].name,
                       (unsigned long long) row[i].intval);
                break;
            case RESQL_FLOAT:
                printf("%-*s : %f \n", wmax, row[i].name, row[i].floatval);
                break;
            case RESQL_TEXT:
                printf("%-*s : %s \n", wmax, row[i].name, row[i].text);
                break;
            case RESQL_BLOB:
                printf("%-*s : %u bytes \n", wmax, row[i].name, row[i].len);
                break;
            case RESQL_NULL:
                printf("%-*s : %s \n", wmax, row[i].name, "null");
                break;
            default:
                printf("Error, result set corrupt! \n");
                return;
            }
        }
        printf("--------------------------\n");
    }
}

void resql_print_seperator(int total, const int *columns)
{
    int x = -1;
    int pos = 0;

    for (int i = 0; i < total; i++) {
        if (i == pos) {
            x++;
            pos += columns[x] + 3;
            printf("+");
        } else {
            printf("-");
        }
    }
    printf("\n");
}

int strncasecmp(const char *s1, const char *s2, size_t n)
{
    if (n == 0) {
        return 0;
    }

    do {
        unsigned char c1 = (unsigned char) *s1++;
        unsigned char c2 = (unsigned char) *s2++;

        if (c1 != c2) {
            if (c1 >= 'A' && c1 <= 'Z' && c2 >= 'a' && c2 <= 'z') {
                c1 += 'a' - 'A';
            } else if (c1 >= 'a' && c1 <= 'z' && c2 >= 'A' && c2 <= 'Z') {
                c2 += 'a' - 'A';
            }
            if (c1 != c2) {
                return c1 - c2;
            }
        }
        if (c1 == 0) {
            break;
        }
    } while (--n != 0);

    return 0;
}

int resql_cli_rep(struct resql_cli *cli, const char *buf)
{
    struct resql_column *row;
    struct resql_result *rs;
    int total = 1, rc, count;
    int col = 120;
    uint32_t columns;
    struct winsize w;
    int *p = NULL;
    char tmp[128];
    bool vertical = s_cli.vertical;

    (void) cli;

    resql_put_sql(client, buf);

    rc = resql_exec(client, false, &rs);
    if (rc == RESQL_ERROR) {
        printf("Disconnected : %s \n", resql_errstr(client));
        exit(EXIT_FAILURE);
    }

    if (rc == RESQL_SQL_ERROR) {
        printf("Error : %s \n", resql_errstr(client));
        return -1;
    }

    rc = ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    if (rc == 0 && w.ws_col != 0) {
        col = w.ws_col;
    }

    if (vertical) {
        resql_print_vertical(rs);
        return 0;
    }

    if (resql_row_count(rs) == -1) {
        printf("Done. No rows returned. \n");
        return 0;
    }

    count = resql_column_count(rs);
    p = calloc(sizeof(int), count);

    columns = resql_column_count(rs);
    while ((row = resql_row(rs)) != NULL) {
        for (size_t i = 0; i < columns; i++) {
            p[i] = MAX(p[i], (int) strlen(row[i].name));
        }
    }

    resql_reset_rows(rs);

    while ((row = resql_row(rs)) != NULL) {
        columns = resql_column_count(rs);
        for (uint32_t i = 0; i < columns; i++) {
            switch (row[i].type) {

            case RESQL_INTEGER:
                snprintf(tmp, 128, "%llu", (unsigned long long) row[i].intval);
                p[i] = MAX(p[i], (int) strlen(tmp));
                break;
            case RESQL_FLOAT:
                snprintf(tmp, 128, "%f", row[i].floatval);
                p[i] = MAX(p[i], (int) strlen(tmp));
                break;
            case RESQL_TEXT:
                p[i] = MAX(p[i], row[i].len);
                break;
            case RESQL_BLOB:
                snprintf(tmp, 128, "%lu bytes", (unsigned long) row[i].len);
                p[i] = MAX(p[i], (int) strlen(tmp));
                break;
            case RESQL_NULL:
                p[i] = MAX(p[i], (int) strlen("null"));
                break;
            default:
                printf("Error, result set corrupt! \n");
                return -1;
            }
        }
    }

    resql_reset_rows(rs);

    for (int i = 0; i < count; i++) {
        total += p[i] + 3;
    }

    if (total > col) {
        vertical = true;
    }

    if (vertical) {
        resql_print_vertical(rs);
    } else {
        resql_print_seperator(total, p);
        columns = resql_column_count(rs);
        row = resql_row(rs);
        for (uint32_t i = 0; i < columns; i++) {
            printf("|" COLUMN_HEADER " %-*s " ANSI_RESET, p[i], row[i].name);
        }
        printf("|\n");

        resql_print_seperator(total, p);

        do {
            columns = resql_column_count(rs);
            for (uint32_t i = 0; i < columns; i++) {
                switch (row[i].type) {

                case RESQL_INTEGER:
                    printf("| %-*llu ", p[i],
                           (unsigned long long) row[i].intval);
                    break;
                case RESQL_FLOAT:
                    printf("| %-*f ", p[i], row[i].floatval);
                    break;
                case RESQL_TEXT:
                    printf("| %-*s ", p[i], row[i].text);
                    break;
                case RESQL_BLOB:
                    snprintf(tmp, sizeof(tmp), "%lu bytes",
                             (unsigned long) row[i].len);
                    printf("| %-*s ", p[i], tmp);
                    break;
                case RESQL_NULL:
                    printf("| %-*s ", p[i], "null");
                    break;
                default:
                    printf("Error, result set corrupt! \n");
                    goto out;
                }
            }

            printf("|\n");

            resql_print_seperator(total, p);
        } while ((row = resql_row(rs)) != NULL);
    }

out:
    free(p);
    return 0;
}

static const char *commands[] = {".tables",  ".schema",    ".help",
                                 ".indexes", ".alltables", ".allindexes",
                                 ".vertical"};
static const size_t command_count = sizeof(commands) / sizeof(commands[0]);
const char *curr;

int sort(const void *c1, const void *c2)
{
    const char *s1 = *(const char **) c1;
    const char *s2 = *(const char **) c2;
    int s1_match = 0, s2_match = 0;
    int cmp;
    int s1_len = (int) strlen(s1);
    int s2_len = (int) strlen(s2);
    int curr_len = (int) strlen(curr);

    cmp = s1_len > curr_len ? curr_len : s1_len;
    for (int i = 0; i < cmp; i++) {
        if (s1[i] != curr[i]) {
            break;
        }
        s1_match++;
    }

    cmp = s2_len > curr_len ? curr_len : s2_len;
    for (int i = 0; i < cmp; i++) {
        if (s2[i] != curr[i]) {
            break;
        }
        s2_match++;
    }

    return s2_match - s1_match;
}

void completion(const char *buf, linenoiseCompletions *lc)
{
    int rc;
    size_t len = (size_t) strlen30(buf);
    size_t i, head;
    char line[1000];
    char *tmp[command_count];
    struct resql_column *row;
    struct resql_result *rs;

    if (len > sizeof(line) - 30) {
        return;
    }

    if (buf[0] == '.' && strncmp(".schema ", buf, strlen(".schema ")) != 0) {
        memcpy(tmp, commands, sizeof(commands));
        curr = buf;
        qsort(tmp, (size_t) command_count, sizeof(char *), sort);

        for (i = 0; i < command_count; i++) {
            linenoiseAddCompletion(lc, tmp[i]);
        }

        return;
    }


    for (i = len - 1; i >= 0 && (isalnum(buf[i]) || buf[i] == '_'); i--) {
    }

    if (i == len - 1) {
        return;
    }

    head = i + 1;
    memcpy(line, buf, (size_t) head);

    resql_put_sql(client, "SELECT DISTINCT candidate COLLATE nocase"
                          "  FROM completion(:head, :all) ORDER BY 1");
    resql_bind_param_text(client, ":head", &buf[head]);
    resql_bind_param_text(client, ":all", buf);

    rc = resql_exec(client, true, &rs);
    if (rc == RESQL_ERROR) {
        printf("Disconnected : %s \n", resql_errstr(client));
        exit(EXIT_FAILURE);
    }

    if (rs != NULL) {
        while ((row = resql_row(rs)) != NULL) {
            if (head + row[0].len < sizeof(line) - 1) {
                memcpy(line + head, row[0].text, (size_t) row[0].len + 1);
                linenoiseAddCompletion(lc, line);
            }
        }
    }
}

void free_hints(void *hint)
{
    free(hint);
}


char *hints(const char *buf, int *color, int *bold)
{
    int len = strlen30(buf);
    int i, head, rc;
    char *tmp[command_count];

    struct resql_column *row;
    struct resql_result *rs;

    if (buf[0] == '.' && strncmp(".schema ", buf, strlen(".schema ")) != 0) {
        if (len < 2) {
            *color = HINT_COLOR;
            *bold = 0;
            return strdup(".tables");
        }

        for (size_t j = 0; j < command_count; j++) {
            if (strncmp(commands[0], buf, strlen(commands[0])) == 0) {
                return NULL;
            }
        }

        memcpy(tmp, commands, sizeof(commands));
        curr = buf;
        qsort(tmp, command_count, sizeof(char *), sort);

        if (tmp[0][0] == buf[0]) {
            *color = HINT_COLOR;
            *bold = 0;
            return strdup(tmp[0]);
        }

        return NULL;
    }

    for (i = len - 1; i >= 0 && (isalnum(buf[i]) || buf[i] == '_'); i--) {
    }

    if (i == len - 1) {
        return NULL;
    }

    head = i + 1;

    resql_put_sql(client, "SELECT DISTINCT candidate COLLATE nocase"
                          "  FROM completion(:head, :all) ORDER BY 1");
    resql_bind_param_text(client, ":head", &buf[head]);
    resql_bind_param_text(client, ":all", buf);

    rc = resql_exec(client, true, &rs);
    if (rc == RESQL_ERROR) {
        printf("Disconnected : %s \n", resql_errstr(client));
        exit(EXIT_FAILURE);
    }

    if (rs == NULL) {
        return NULL;
    }

    row = resql_row(rs);
    if (row != NULL) {
        *color = HINT_COLOR;
        *bold = 0;
        return strdup(row[0].text);
    }

    return NULL;
}

void resql_print_user_tables()
{
    resql_cli_rep(&s_cli,
                  "SELECT name FROM sqlite_master WHERE type ='table' AND "
                  "name NOT LIKE 'sqlite_%' AND name NOT LIKE 'resql_%'");
}

void resql_print_all_tables()
{
    resql_cli_rep(&s_cli, "SELECT name FROM sqlite_master WHERE type ='table'");
}

void resql_print_user_indexes()
{
    resql_cli_rep(&s_cli,
                  "SELECT name FROM sqlite_master WHERE type ='index' AND "
                  "name NOT LIKE 'sqlite_%' AND name NOT LIKE 'resql_%'");
}

void resql_print_all_indexes()
{
    resql_cli_rep(&s_cli, "SELECT name FROM sqlite_master WHERE type ='index'");
}

void resql_print_schema(char *buf)
{
    char tmp[1024];
    char *p = strrchr(buf, ' ');
    if (!p || !(*(p + 1))) {
        printf("Syntax : .schema mytable");
        return;
    }

    snprintf(tmp, sizeof(tmp), "PRAGMA table_info([%s])", (p + 1));
    resql_cli_rep(&s_cli, tmp);
}

void resql_print_help()
{
    printf("\n");
    printf("You can type SQL queries. Commands starts with '.' \n"
           "character, they are not interpreted as SQL. \n\n");

    printf(" .tables                 Print user tables only                  \n"
           " .indexes                Print user indexes only                 \n"
           " .schema <table>         Print table schema                      \n"
           " .alltables              Print all tables                        \n"
           " .allindexes             Print all indexes                       \n"
           " .vertical               Flip vertical table print flag, default \n"
           "                         is automatic, if table does not fit the \n"
           "                         screen, it will be printed vertical \n");
    printf(" .help                   Print help screen \n");
}

void resql_cli_init(struct resql_cli *cli)
{
    cli->uri = sc_uri_create("tcp://127.0.0.1:7600");
    cli->vertical = false;
}

static void resql_signal(int sig)
{
    (void) sig;

    printf("Shutting down.. \n");
    exit(0);
}

int main(int argc, char **argv)
{
    int rc;
    char *line;
    struct sigaction action;

    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    action.sa_handler = resql_signal;

    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGINT, &action, NULL);

    resql_cli_init(&s_cli);
    resql_cli_read_cmdline(&s_cli, argc, argv);

    struct resql_config config = {
            .urls = s_cli.uri->str,
            .timeout_millis = 4000,
            .client_name = "cli",
            .cluster_name = "cluster",
    };

    printf("Trying to connect to server at %s \n", s_cli.uri->str);

    rc = resql_create(&client, &config);
    if (rc != RESQL_OK) {
        printf("Failed to connect to server at %s \n", s_cli.uri->str);
        exit(EXIT_FAILURE);
    }

    if (s_cli.count > 0) {
        for (int i = 0; i < s_cli.count; i++) {
            rc = resql_cli_rep(&s_cli, s_cli.cmds[i]);
            if (rc != 0) {
                return -1;
            }
        }

        return 0;
    }

    linenoiseSetCompletionCallback(completion);
    linenoiseSetHintsCallback(hints);
    linenoiseSetFreeHintsCallback(free_hints);
    linenoiseSetMultiLine(1);
    linenoiseHistoryLoad("resql-history.txt");

    printf("Connected \n");
    printf("\nType .help for usage. \n\n");

    while ((line = linenoise("resql> ")) != NULL) {
        if (line[0] != '\0' && line[0] != '.') {
            resql_cli_rep(&s_cli, line);
        } else if (!strncmp(line, ".help", 5)) {
            resql_print_help();
        } else if (!strncmp(line, ".vertical", 9)) {
            s_cli.vertical = !s_cli.vertical;
            printf("Vertical table : %s \n", s_cli.vertical ? "true" : "auto");
        } else if (!strncmp(line, ".alltables", 10)) {
            resql_print_all_tables();
        } else if (!strncmp(line, ".tables", 7)) {
            resql_print_user_tables();
        } else if (!strncmp(line, ".indexes", 8)) {
            resql_print_user_indexes();
        } else if (!strncmp(line, ".allindexes", 11)) {
            resql_print_all_indexes();
        } else if (!strncmp(line, ".schema", 7)) {
            resql_print_schema(line);
        } else if (line[0] == '.') {
            printf("Unrecognized command: %s\n\n", line);
            resql_print_help();
            continue;
        }

        linenoiseHistoryAdd(line);
        linenoiseHistorySave("resql-history.txt");
        printf("\n");
    }
    free(line);

    printf("Shutting down.. \n");
    resql_shutdown(client);

    return 0;
}
