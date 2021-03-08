/*
 *  Resql
 *
 *  Copyright (C) 2021 Resql Authors
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "conf.h"
#include "file.h"

#include "sc/sc_buf.h"
#include "sc/sc_ini.h"
#include "sc/sc_log.h"
#include "sc/sc_option.h"
#include "sc/sc_str.h"

#include <string.h>

#define ANSI_RESET "\x1b[0m"
#define ANSI_RED                                                               \
    "\x1b[1m"                                                                  \
    "\x1b[31m"

enum conf_type
{
    CONF_BOOL,
    CONF_INTEGER,
    CONF_STRING
};

enum conf_index
{
    CONF_NODE_NODE_NAME,
    CONF_NODE_BIND_URI,
    CONF_NODE_ADVERTISE_URI,
    CONF_NODE_SOURCE_ADDR,
    CONF_NODE_SOURCE_PORT,
    CONF_NODE_LOG_LEVEL,
    CONF_NODE_LOG_DESTINATION,
    CONF_NODE_DIRECTORY,
    CONF_NODE_IN_MEMORY,

    CONF_CLUSTER_NAME,
    CONF_CLUSTER_NODE,

    CONF_CMDLINE_CONF_FILE,
    CONF_CMDLINE_EMPTY,
    CONF_CMDLINE_SYSTEMD,
    CONF_CMDLINE_WIPE,

    CONF_INVALID
};

struct conf_item
{
    enum conf_type type;
    enum conf_index index;
    const char *section;
    const char *key;
};

// clang-format off
static struct conf_item conf_list[] = {
        {CONF_STRING,  CONF_NODE_NODE_NAME,        "node",     "node-name"       },
        {CONF_STRING,  CONF_NODE_BIND_URI,         "node",     "bind-uri"        },
        {CONF_STRING,  CONF_NODE_ADVERTISE_URI,    "node",     "advertise-uri"   },
        {CONF_STRING,  CONF_NODE_SOURCE_ADDR,      "node",     "source-addr"     },
        {CONF_STRING,  CONF_NODE_SOURCE_PORT,      "node",     "source-port"     },
        {CONF_STRING,  CONF_NODE_LOG_LEVEL,        "node",     "log-level"       },
        {CONF_STRING,  CONF_NODE_LOG_DESTINATION,  "node",     "log-destination" },
        {CONF_STRING,  CONF_NODE_DIRECTORY,        "node",     "directory"       },
        {CONF_BOOL,    CONF_NODE_IN_MEMORY,        "node",     "in-memory"       },

        {CONF_STRING,  CONF_CLUSTER_NAME,          "cluster",  "name"            },
        {CONF_STRING,  CONF_CLUSTER_NODE,          "cluster",  "node"            },

        {CONF_STRING,  CONF_CMDLINE_CONF_FILE,     "cmd-line", "config"          },
        {CONF_BOOL,    CONF_CMDLINE_EMPTY,         "cmd-line", "empty"           },
        {CONF_BOOL,    CONF_CMDLINE_SYSTEMD,       "cmd-line", "systemd"         },
        {CONF_BOOL,    CONF_CMDLINE_WIPE,          "cmd-line", "wipe"            },
};
// clang-format on

static const int conf_size = sizeof(conf_list) / sizeof(struct conf_item);

void conf_init(struct conf *c)
{
    *c = (struct conf){0};

    c->node.name = sc_str_create("");
    c->node.bind_uri = sc_str_create("");
    c->node.ad_uri = sc_str_create("");
    c->node.source_addr = sc_str_create("");
    c->node.source_port = sc_str_create("");
    c->node.log_dest = sc_str_create("stdout");
    c->node.log_level = sc_str_create("INFO");
    c->node.dir = sc_str_create("./");
    c->node.in_memory = true;
    c->cluster.name = sc_str_create("cluster");
    c->cluster.nodes = sc_str_create("");

    c->cmdline.config_file = sc_str_create("resql.ini");
    c->cmdline.empty = false;
    c->cmdline.systemd = false;
    c->cmdline.wipe = false;
}

void conf_term(struct conf *c)
{
    sc_str_destroy(c->node.name);
    sc_str_destroy(c->node.bind_uri);
    sc_str_destroy(c->node.ad_uri);
    sc_str_destroy(c->node.source_addr);
    sc_str_destroy(c->node.source_port);
    sc_str_destroy(c->node.log_dest);
    sc_str_destroy(c->node.log_level);
    sc_str_destroy(c->node.dir);
    sc_str_destroy(c->cluster.name);
    sc_str_destroy(c->cluster.nodes);
    sc_str_destroy(c->cmdline.config_file);
}

static void conf_cmdline_usage()
{
    printf("\n\n resql version : %s \n\n", RS_VERSION_STR);
    printf(" -c=<file>    --config=<file>       Config file path, default is './resql.ini'                                       \n"
           " -e           --empty               Clear persistent data and start. Be warned!, this option will wipe out database. \n"
           " -h           --help                Print this help and exit                                                         \n"
           " -s           --systemd             Run as systemd daemon, requires NOTIFY_SOCKET option.                            \n"
           " -v,          --version             Print version and exit                                                           \n"
           " -w,          --wipe                Clear persistent data and exit. Be warned!, this option will wipe out database.  \n"
           "                                                                                                                     \n"
           "\n\n");
}


void conf_read_cmdline(struct conf *c, int argc, char **argv)
{
    char ch;
    char *value;

    struct sc_option_item options[] = {
            {.letter = 'c', .name = "config"},
            {.letter = 'e', .name = "empty"},
            {.letter = 'h', .name = "help"},
            {.letter = 's', .name = "systemd"},
            {.letter = 'v', .name = "version"},
            {.letter = 'w', .name = "wipe"},
    };

    struct sc_option opt = {
            .count = sizeof(options) / sizeof(options[0]),
            .options = options,
            .argv = argv,
    };

    for (int i = 1; i < argc; i++) {
        ch = sc_option_at(&opt, i, &value);
        switch (ch) {
        case 'c':
            sc_str_set(&c->cmdline.config_file, value);
            break;
        case 'e':
            c->cmdline.empty = true;
            break;
        case 's':
            c->cmdline.systemd = true;
            break;
        case 'h':
        case 'v':
            conf_cmdline_usage();
            exit(0);
        case 'w':
            c->cmdline.wipe = true;
            break;
        case '?':
        default:
            printf("resql: " ANSI_RED "Unknown option '%s'.\n" ANSI_RESET,
                   argv[i]);
            conf_cmdline_usage();
            exit(1);
        }
    }
}

static int conf_add(void *arg, int line, const char *section, const char *key,
                    const char *value)
{
    (void) line;

    struct conf *c = arg;
    enum conf_index index = CONF_INVALID;

    for (int i = 0; i < conf_size; i++) {
        if (strcasecmp(conf_list[i].section, section) == 0 &&
            strcasecmp(conf_list[i].key, key) == 0) {
            index = conf_list[i].index;
        }
    }

    switch (index) {
    case CONF_NODE_NODE_NAME:
        sc_str_set(&c->node.name, value);
        break;
    case CONF_NODE_BIND_URI:
        sc_str_set(&c->node.bind_uri, value);
        break;
    case CONF_NODE_ADVERTISE_URI:
        sc_str_set(&c->node.ad_uri, value);
        break;
    case CONF_NODE_SOURCE_ADDR:
        sc_str_set(&c->node.source_addr, value);
        break;
    case CONF_NODE_SOURCE_PORT:
        sc_str_set(&c->node.source_port, value);
        break;
    case CONF_NODE_LOG_DESTINATION:
        sc_str_set(&c->node.log_dest, value);
        break;
    case CONF_NODE_LOG_LEVEL:
        sc_str_set(&c->node.log_level, value);
        break;
    case CONF_NODE_DIRECTORY:
        sc_str_set(&c->node.dir, value);
        break;
    case CONF_NODE_IN_MEMORY:
        if (strcasecmp(value, "true") != 0 && strcasecmp(value, "false") != 0) {
            snprintf(c->err, sizeof(c->err),
                     "Boolean value must be 'true' or 'false', "
                     "section=%s, key=%s, value=%s \n",
                     section, key, value);
            return -1;
        }
        c->node.in_memory = strcasecmp(value, "true") == 0 ? true : false;
        break;
    case CONF_CLUSTER_NAME:
        sc_str_set(&c->cluster.name, value);
        break;
    case CONF_CLUSTER_NODE:
        sc_str_set(&c->cluster.nodes, value);
        break;

    default:
        snprintf(c->err, sizeof(c->err),
                 "Unknown config, section=%s, key=%s, value=%s \n", section,
                 key, value);
        return -1;
    }

    return 0;
}

void conf_read_file(struct conf *c, const char *path)
{
    int rc;

    rc = sc_ini_parse_file(c, conf_add, path);
    if (rc != 0) {
        fprintf(stderr, "Failed to find valid config file at : %s \n", path);
        fprintf(stderr, "Reason : %s \n", c->err);
        exit(EXIT_SUCCESS);
    }
}

static void conf_to_buf(struct sc_buf *buf, enum conf_index i, void *val)
{
    struct conf_item item = conf_list[i];

    sc_buf_put_text(buf, "\t | %-10s | %-15s | ", item.section, item.key);

    switch (item.type) {
    case CONF_BOOL:
        sc_buf_put_text(buf, "%s \n", ((bool) val) == true ? "true" : "false");
        break;
    case CONF_INTEGER:
        sc_buf_put_text(buf, "%llu \n", (uint64_t *) val);
        break;
    case CONF_STRING:
        sc_buf_put_text(buf, "%s \n", val);
        break;
    }
}

void conf_print(struct conf *c)
{
    char tmp[1024];
    struct sc_buf buf = sc_buf_wrap(tmp, sizeof(tmp), SC_BUF_REF);

    sc_buf_put_text(&buf, "\n\t | %-10s | %-15s | %-20s \n", "Section", "Key",
                    "Value");
    sc_buf_put_text(&buf, "\t %s \n",
                    "-------------------------------------------------");

    conf_to_buf(&buf, CONF_NODE_NODE_NAME, c->node.name);
    conf_to_buf(&buf, CONF_NODE_BIND_URI, c->node.bind_uri);
    conf_to_buf(&buf, CONF_NODE_ADVERTISE_URI, c->node.ad_uri);
    conf_to_buf(&buf, CONF_NODE_SOURCE_ADDR, c->node.source_addr);
    conf_to_buf(&buf, CONF_NODE_SOURCE_PORT, c->node.source_port);
    conf_to_buf(&buf, CONF_NODE_LOG_LEVEL, c->node.log_level);
    conf_to_buf(&buf, CONF_NODE_LOG_DESTINATION, c->node.log_dest);
    conf_to_buf(&buf, CONF_NODE_DIRECTORY, c->node.dir);
    conf_to_buf(&buf, CONF_NODE_IN_MEMORY, (void *) c->node.in_memory);

    sc_buf_put_text(&buf, "\t %s \n",
                    "-------------------------------------------------");
    conf_to_buf(&buf, CONF_CLUSTER_NAME, c->cluster.name);
    conf_to_buf(&buf, CONF_CLUSTER_NODE, c->cluster.nodes);

    sc_buf_put_text(&buf, "\t %s \n",
                    "-------------------------------------------------");
    conf_to_buf(&buf, CONF_CMDLINE_CONF_FILE, c->cmdline.config_file);
    conf_to_buf(&buf, CONF_CMDLINE_EMPTY, (void *) c->cmdline.empty);
    conf_to_buf(&buf, CONF_CMDLINE_SYSTEMD, (void*) c->cmdline.systemd);
    conf_to_buf(&buf, CONF_CMDLINE_WIPE, (void*) c->cmdline.wipe);

    sc_log_info("%s \n", tmp);
}
