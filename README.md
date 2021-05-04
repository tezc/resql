# resql

## Overview
- bin/ : output dir, ./build.sh will generate server executable and cli here.  
- c-client/ : c client
- goresql/ : go client
- java-client/ : java client
- src/ : server source code  
- tests/ : server tests
- util/ : cli, docker

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
  * Resql uses sqlite as database engine.  <br/><br/>
  
* **CRC32C** (https://github.com/madler/brotli/blob/master/crc32c.c) :  
  * A modified version of CRC32C implementation.  <br/><br/>
  
* **HdrHistogram** (https://github.com/HdrHistogram/HdrHistogram_c) :  
  * Pieces from HdrHistogram library in resql_benchmark tool. <br/><br/>
  
* **Linenoise** (https://github.com/antirez/linenoise)  
  * Used in resql_cli.  <br/><br/>
  
* **Inih** (https://github.com/benhoyt/inih)  
  * A modified version of INI parser, used for configuration file.  <br/><br/>
  
* **Redis** (https://github.com/redis/redis/blob/unstable/src/zmalloc.c)  
  * Functions to get RAM capacity of the machine and RSS value.  <br/><br/>
  
