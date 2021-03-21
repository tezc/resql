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

