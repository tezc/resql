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
* **Sqlite** (http://sqlite.org/) :  
  Resql uses sqlite as database engine.
  
* **CRC32C** (https://github.com/madler/brotli/blob/master/crc32c.c)  
 A modified version of CRC32C implementation.
  
* **HdrHistogram** (https://github.com/HdrHistogram/HdrHistogram_c)  
 Pieces from HdrHistogram library in resql_benchmark tool.
  
* **Linenoise** (https://github.com/antirez/linenoise)  
  Used in resql_cli.
  
* **Inih** (https://github.com/benhoyt/inih)  
 A modified version of INI parser, used for configuration file.
  
* **Redis** (https://github.com/redis/redis/blob/unstable/src/zmalloc.c)  
 Functions to get RAM capacity of the machine and RSS value.
  
