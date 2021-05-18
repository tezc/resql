![Status](https://img.shields.io/badge/status-beta-blue)
[![License: BSD](https://img.shields.io/badge/License-BSD-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

### What is Resql?

Resql is a SQL database server that uses SQLite as its SQL engine and it provides replication and automatic failover capabilities. 

### Why ?

It's mostly about performance and consistency. Resql tries to be as fast as possible without giving up on consistency :

- **Performance** : A quick benchmark in wiki : [Benchmark](https://github.com/tezc/resql/wiki/Benchmark)
- **Consistency** : Resql implements RAFT consensus algorithm and provides exactly-once execution of operations. If you're not familiar with distributed systems, basically, Resql executes client operations one by one. If configured with multiple nodes, it replicates your operations to each node. Once you get an answer from the server for an operation, you can be sure your operation is persisted. If the client retries an operation on a server crash/network issue etc, it's guaranteed that the operation will be executed exactly once. 

### Features
* Can be used as a single node persistent database or in a cluster with replication and failover.
* Light on resources: ~1 MB executable size, can start with a few MB of RAM. Currently, it uses 2 threads only, it'll occupy 2 cores on your machine at most.
* [C](https://github.com/tezc/resql/wiki/C-Client), [Java](https://github.com/tezc/resql/wiki/Java-Client) and [Go](https://github.com/tezc/resql/wiki/Go-Client) clients are available.
* JSON, Full-Text Search, RTree and Geopoly SQLite extensions are included.
* Each operation is atomic. Operations can be batched together and sent to the server in one round-trip and executed atomically.
* Read and write operations can be combined together. e.g. SELECT and INSERT in a single batch.

### Limitations
* Explicit transactions are not supported. e.g. No BEGIN, COMMIT or ROLLBACK. Instead of explicit transactions, you can batch your operations and send them in one go. A batch is executed atomically.
* Not suitable for long-running analytical queries. e.g. Queries that last tens of seconds.

## Build

Resql can be compiled on Linux, macOS and FreeBSD. CI runs on multiple architectures. Big-little endian systems and 32 bit CPUs are supported.


#### Requirements
CMake, GCC or Clang.

These can be installed via the package manager of your OS.  
```
Ubuntu : apt install gcc cmake  
MacOS  : brew install cmake
```
#### Compile
```
git clone https://github.com/tezc/resql.git
cd resql
./build.sh
```
Build takes ~ 1 minute. It will create server, cli and benchmark executable under bin/ directory of the current directory.

After the build is completed : 
```
cd bin
./resql
```
Start command-line interface (CLI) to connect to the server
```
./resql-cli
```

## Documentation

#### Get Started
  - [Run locally](https://github.com/tezc/resql/wiki/Run-locally)
  - [Run in Docker](https://github.com/tezc/resql/wiki/Run-in-Docker) 
  - [Run as a systemd service](https://github.com/tezc/resql/wiki/Run-as-a-systemd-service)
#### Administration
  - [Configuration](https://github.com/tezc/resql/wiki/Configuration)
  - [Monitoring](https://github.com/tezc/resql/wiki/Monitoring)
  - [Backup](https://github.com/tezc/resql/wiki/Backup)
#### Clients
  - [Java client](https://github.com/tezc/resql/wiki/Java-Client)
  - [Go client](https://github.com/tezc/resql/wiki/Go-Client)
  - [C client](https://github.com/tezc/resql/wiki/C-Client)


## Project Layout 
```
bin/     : output dir, ./build.sh will generate server executable and cli here.  
cresql/  : c client
goresql/ : go client
jresql/  : java client
lib/     : dependencies
src/     : server source code  
test/    : server tests
util/    : cli, docker, benchmark tool
```

## Acknowledgements

This project contains code from other open source projects :

* **[Sqlite](http://sqlite.org/)** &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; : Resql uses sqlite as database engine.
* **[CRC32C](https://github.com/madler/brotli/blob/master/crc32c.c)** &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; : A modified version of CRC32C implementation.
* **[HdrHistogram](https://github.com/HdrHistogram/HdrHistogram_c)** : Used in resql_benchmark tool.
* **[Linenoise](https://github.com/antirez/linenoise)** &nbsp; &nbsp; &nbsp; &nbsp; : Used in resql_cli.
* **[Inih](https://github.com/benhoyt/inih)** &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; : A modified version of INI parser, used for the configuration file.
* **[Redis](https://github.com/redis/redis/blob/unstable/src/zmalloc.c)** &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; : Functions to get RAM capacity of the machine and RSS value.











