# resql

## Overview
- bin/ : output dir, ./build.sh will generate server executable and cli here.  
- clients/ : c, cli, java and go clients, each directory is independent.  
- deps/ : c dependencies  
- main/ : main executable for server. tests build server as library to create multiple instance in the same process.  
- src/ : server source code  
- tests/ : server tests

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

## Wiki
[Check out Wiki](https://github.com/tezc/resql/wiki)
de
