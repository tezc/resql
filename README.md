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

## Usage
1. [Monitoring](https://github.com/tezc/resql/wiki/Monitoring)
2. [Command Line Interface (CLI) Guide](https://github.com/tezc/resql/wiki/Command-Line-Interface-(CLI)-Guide)
3. [C Client Guide](https://github.com/tezc/resql/wiki/C-Client-Guide)
4. [Go Client Guide](https://github.com/tezc/resql/wiki/Go-Client-Guide)
5. [Java Client Guide](https://github.com/tezc/resql/wiki/Java-Client-Guide)

