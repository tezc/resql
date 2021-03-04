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


#include "settings.h"
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

enum settings_type
{
    SETTINGS_BOOL,
    SETTINGS_INTEGER,
    SETTINGS_STRING
};

enum settings_index
{
    SETTINGS_NODE_NODE_NAME,
    SETTINGS_NODE_BIND_URI,
    SETTINGS_NODE_ADVERTISE_URI,
    SETTINGS_NODE_SOURCE_ADDR,
    SETTINGS_NODE_SOURCE_PORT,
    SETTINGS_NODE_LOG_LEVEL,
    SETTINGS_NODE_LOG_DESTINATION,
    SETTINGS_NODE_DIRECTORY,
    SETTINGS_NODE_IN_MEMORY,

    SETTINGS_CLUSTER_NAME,
    SETTINGS_CLUSTER_NODE,

    SETTINGS_CMDLINE_EMPTY,
    SETTINGS_CMDLINE_SETTINGS_FILE,

    SETTINGS_INVALID
};

struct settings_item
{
    enum settings_type type;
    enum settings_index index;
    const char *section;
    const char *key;
};

// clang-format off
static struct settings_item settings_list[] = {
        {SETTINGS_STRING,  SETTINGS_NODE_NODE_NAME,        "node",     "node-name"       },
        {SETTINGS_STRING,  SETTINGS_NODE_BIND_URI,         "node",     "bind-uri"        },
        {SETTINGS_STRING,  SETTINGS_NODE_ADVERTISE_URI,    "node",     "advertise-uri"   },
        {SETTINGS_STRING,  SETTINGS_NODE_SOURCE_ADDR,      "node",     "source-addr"     },
        {SETTINGS_STRING,  SETTINGS_NODE_SOURCE_PORT,      "node",     "source-port"     },
        {SETTINGS_STRING,  SETTINGS_NODE_LOG_LEVEL,        "node",     "log-level"       },
        {SETTINGS_STRING,  SETTINGS_NODE_LOG_DESTINATION,  "node",     "log-destination" },
        {SETTINGS_STRING,  SETTINGS_NODE_DIRECTORY,        "node",     "directory"       },
        {SETTINGS_BOOL,    SETTINGS_NODE_IN_MEMORY,        "node",     "in-memory"       },

        {SETTINGS_STRING,  SETTINGS_CLUSTER_NAME,          "cluster",  "name"            },
        {SETTINGS_STRING,  SETTINGS_CLUSTER_NODE,          "cluster",  "node"            },

        {SETTINGS_BOOL,    SETTINGS_CMDLINE_EMPTY,         "cmd-line", "empty"           },
        {SETTINGS_STRING,  SETTINGS_CMDLINE_SETTINGS_FILE, "cmd-line", "settings-file"   },
};
// clang-format on

static const int settings_size = sizeof(settings_list) / sizeof(struct settings_item);

void settings_init(struct settings *c)
{
    *c = (struct settings){0};

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

    c->cmdline.empty = false;
    c->cmdline.settings_file = sc_str_create("settings.ini");
}

void settings_term(struct settings *c)
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
    sc_str_destroy(c->cmdline.settings_file);
}

static void settings_cmdline_usage()
{
    printf("\n\n reSQL version : %s \n\n", RS_VERSION_STR);
    printf(" -c=<file>    --settings=<file>       Config file path,         \n"
           "                                    default is './settings.ini' \n"
           " -e                                 Clear persistent data     \n"
           " -h           --help                Print this help and exit  \n"
           " -v,          --version             Print version and exit    \n"
           "                                                              \n"
           "\n\n");
}


void settings_read_cmdline(struct settings *c, int argc, char *argv[])
{
    char ch;
    char *value;

    struct sc_option_item options[] = {
            {.letter = 'c', .name = "settings"},
            {.letter = 'e', .name = "empty"},
            {.letter = 'h', .name = "help"},
            {.letter = 'v', .name = "version"},
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
            sc_str_set(&c->cmdline.settings_file, value);
            break;
        case 'e':
            c->cmdline.empty = true;
            break;
        case 'h':
        case 'v':
            settings_cmdline_usage();
            exit(0);
        case '?':
        default:
            printf("resql: " ANSI_RED "Unknown option '%s'.\n" ANSI_RESET,
                   argv[i]);
            settings_cmdline_usage();
            exit(1);
        }
    }
}

static int settings_add(void *arg, int line, const char *section, const char *key,
                      const char *value)
{
    (void) line;

    struct settings *c = arg;
    enum settings_index index = SETTINGS_INVALID;

    for (int i = 0; i < settings_size; i++) {
        if (strcasecmp(settings_list[i].section, section) == 0 &&
            strcasecmp(settings_list[i].key, key) == 0) {
            index = settings_list[i].index;
        }
    }

    switch (index) {
    case SETTINGS_NODE_NODE_NAME:
        sc_str_set(&c->node.name, value);
        break;
    case SETTINGS_NODE_BIND_URI:
        sc_str_set(&c->node.bind_uri, value);
        break;
    case SETTINGS_NODE_ADVERTISE_URI:
        sc_str_set(&c->node.ad_uri, value);
        break;
    case SETTINGS_NODE_SOURCE_ADDR:
        sc_str_set(&c->node.source_addr, value);
        break;
    case SETTINGS_NODE_SOURCE_PORT:
        sc_str_set(&c->node.source_port, value);
        break;
    case SETTINGS_NODE_LOG_DESTINATION:
        sc_str_set(&c->node.log_dest, value);
        break;
    case SETTINGS_NODE_LOG_LEVEL:
        sc_str_set(&c->node.log_level, value);
        break;
    case SETTINGS_NODE_DIRECTORY:
        sc_str_set(&c->node.dir, value);
        break;
    case SETTINGS_NODE_IN_MEMORY:
        if (strcasecmp(value, "true") != 0 && strcasecmp(value, "false") != 0) {
            snprintf(c->err, sizeof(c->err),
                     "Boolean value must be 'true' or 'false', "
                     "section=%s, key=%s, value=%s \n",
                     section, key, value);
            return -1;
        }
        c->node.in_memory = strcasecmp(value, "true") == 0 ? true : false;
        break;
    case SETTINGS_CLUSTER_NAME:
        sc_str_set(&c->cluster.name, value);
        break;
    case SETTINGS_CLUSTER_NODE:
        sc_str_set(&c->cluster.nodes, value);
        break;

    default:
        snprintf(c->err, sizeof(c->err),
                 "Unknown settings, section=%s, key=%s, value=%s \n", section,
                 key, value);
        return -1;
    }

    return 0;
}

void settings_read_file(struct settings *c, const char *path)
{
    int rc;

    rc = sc_ini_parse_file(c, settings_add, path);
    if (rc != 0) {
        fprintf(stderr, "Failed to find valid settings file at : %s \n", path);
        fprintf(stderr, "Reason : %s \n", c->err);
        exit(EXIT_SUCCESS);
    }
}

static void settings_to_buf(struct sc_buf *buf, enum settings_index i, void *val)
{
    struct settings_item item = settings_list[i];

    sc_buf_put_text(buf, "\t | %-10s | %-15s | ", item.section, item.key);

    switch (item.type) {
    case SETTINGS_BOOL:
        sc_buf_put_text(buf, "%s \n", ((bool) val) == true ? "true" : "false");
        break;
    case SETTINGS_INTEGER:
        sc_buf_put_text(buf, "%llu \n", (uint64_t *) val);
        break;
    case SETTINGS_STRING:
        sc_buf_put_text(buf, "%s \n", val);
        break;
    }
}

void settings_print(struct settings *c)
{
    char tmp[1024];
    struct sc_buf buf = sc_buf_wrap(tmp, sizeof(tmp), SC_BUF_REF);

    sc_buf_put_text(&buf, "\n\t | %-10s | %-15s | %-20s \n", "Section", "Key",
                    "Value");
    sc_buf_put_text(&buf, "\t %s \n",
                    "-------------------------------------------------");

    settings_to_buf(&buf, SETTINGS_NODE_NODE_NAME, c->node.name);
    settings_to_buf(&buf, SETTINGS_NODE_BIND_URI, c->node.bind_uri);
    settings_to_buf(&buf, SETTINGS_NODE_ADVERTISE_URI, c->node.ad_uri);
    settings_to_buf(&buf, SETTINGS_NODE_SOURCE_ADDR, c->node.source_addr);
    settings_to_buf(&buf, SETTINGS_NODE_SOURCE_PORT, c->node.source_port);
    settings_to_buf(&buf, SETTINGS_NODE_LOG_LEVEL, c->node.log_level);
    settings_to_buf(&buf, SETTINGS_NODE_LOG_DESTINATION, c->node.log_dest);
    settings_to_buf(&buf, SETTINGS_NODE_DIRECTORY, c->node.dir);
    settings_to_buf(&buf, SETTINGS_NODE_IN_MEMORY, (void *) c->node.in_memory);

    sc_buf_put_text(&buf, "\t %s \n",
                    "-------------------------------------------------");
    settings_to_buf(&buf, SETTINGS_CLUSTER_NAME, c->cluster.name);
    settings_to_buf(&buf, SETTINGS_CLUSTER_NODE, c->cluster.nodes);

    sc_buf_put_text(&buf, "\t %s \n",
                    "-------------------------------------------------");
    settings_to_buf(&buf, SETTINGS_CMDLINE_EMPTY, (void *) c->cmdline.empty);
    settings_to_buf(&buf, SETTINGS_CMDLINE_SETTINGS_FILE, c->cmdline.settings_file);

    sc_log_info("%s \n", tmp);
}
