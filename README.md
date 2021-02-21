# resql

## Build
```
Requirements CMake, gcc or clang

git clone https://github.com/tezc/resql.git
cd resql
./build.sh
```

## Start Server
```
cd bin/
./resql
```

## CLI 

```
./resql-cli --help
```

In shell, type .help for commands.

```
resql> .help

You can type SQL queries. Commands starts with '.' 
character, they are not interpreted as SQL. 

 .tables                 Print user tables only                  
 .indexes                Print user indexes only                 
 .schema <table>         Print table schema                      
 .alltables              Print all tables                        
 .allindexes             Print all indexes                       
 .vertical               Flip vertical table print flag, default 
                         is automatic, if table does not fit the 
                         screen, it will be printed vertical 
 .help                   Print help screen
```

### Monitoring
Read only tables for monitoring :

resql_nodes    : nodes  
resql_sessions : clients  
resql_statements : prepared statements

e.g:

```
resql> SELECT * FROM resql_sessions
+-------------+-----------+----------+----------------+-----------------+------+
| client_name | client_id | sequence | local          | remote          | resp |
+-------------+-----------+----------+----------------+-----------------+------+
| cli         | 1039      | 0        | 127.0.0.1:7600 | 127.0.0.1:59284 | null |
+-------------+-----------+----------+----------------+-----------------+------+

```

### Management

Commands for adding a node, shutting down the cluster is done via resql()
SQL function.

Graceful shutdown a node : SELECT resql('shutdown', 'node0');  
Graceful shutdown all nodes : SELECT resql('shutdown', '*');

## Clients

## C client
Single .h and .c pair under clients/c/, just copy it to your project.

## Java client
Under clients/java

## Go client
Under clients/go