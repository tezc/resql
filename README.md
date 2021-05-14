# resql

## Overview
- bin/ : output dir, ./build.sh will generate server executable and cli here.  
- cresql/ : c client
- goresql/ : go client
- jresql/ : java client
- lib/ : dependencies
- src/ : server source code  
- test/ : server tests
- util/ : cli, docker, benchmark tool

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
./resql-cli
```

## Acknowledgements

This project contains code from other open source projects :

* **[Sqlite](http://sqlite.org/)** &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; : Resql uses sqlite as database engine.
* **[CRC32C](https://github.com/madler/brotli/blob/master/crc32c.c)** &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; : A modified version of CRC32C implementation.
* **[HdrHistogram](https://github.com/HdrHistogram/HdrHistogram_c)** : Used in resql_benchmark tool.
* **[Linenoise](https://github.com/antirez/linenoise)** &nbsp; &nbsp; &nbsp; &nbsp; : Used in resql_cli.
* **[Inih](https://github.com/benhoyt/inih)** &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; : A modified version of INI parser, used for the configuration file.
* **[Redis](https://github.com/redis/redis/blob/unstable/src/zmalloc.c)** &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; : Functions to get RAM capacity of the machine and RSS value.
  
