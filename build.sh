#!/bin/sh
set -e
cd "$(dirname "$0")"
cwd=$(pwd)

if [ "$#" -ne 0 ]; then
  if [ "$1" = --install ]; then
    if [ ! -f bin/resql ] || [ ! -f bin/resql-cli ] || [ ! -f bin/resql-benchmark ]; then
      echo "Executables are missing, run ./build.sh first."
      exit 1
    fi

    (set -x; cp bin/resql bin/resql-cli bin/resql-benchmark /usr/local/bin/)
    (set -x; cp bin/resql.ini /etc/)
    exit 0
  elif [ "$1" = --uninstall ]; then
    (set -x; rm -rf /usr/local/bin/resql /usr/local/bin/resql-cli /usr/local/bin/resql-benchmark)
    (set -x; rm -rf /etc/resql.ini)
    exit 0
  else
    echo "Unknown option : $1";
    exit 1
  fi
fi

cleanup() {
  cd "$cwd"

  (kill -9 "$trainer_pid" || true) > /dev/null 2>&1
  (kill -9 "$server_pid" || true) > /dev/null 2>&1
  (kill -9 "$(jobs -p)" || true) > /dev/null 2>&1
  rm -rf bin/resql-trainer bin/build/ bin/pgo/

  echo "Clean-up done"

  if [ -n "$1" ]; then
    rm -rf bin/resql bin/resql-cli bin/resql-benchmark
    echo "Aborted by $1"
  elif [ "$status" -ne 0 ]; then
    rm -rf bin/resql bin/resql-cli bin/resql-benchmark
    echo "Failure (status $status)"
  else
    echo "Success"
  fi
}

trap 'status=$?; cleanup; exit $status' EXIT
trap 'trap - HUP; cleanup SIGHUP; kill -HUP $$' HUP
trap 'trap - INT; cleanup SIGINT; kill -INT $$' INT
trap 'trap - TERM; cleanup SIGTERM; kill -TERM $$' TERM

echo "$cwd"
cd bin
rm -rf resql resql-cli resql-benchmark build pgo
rm -rf *.resql
mkdir build pgo
cd build
cmake ../.. -DPGO=generate
make -j 1 && make install
echo "First step has been completed successfully."
cd ..

./resql --node-bind-url=tcp://127.0.0.1:9717 &
server_pid=$!
echo "Server has been started successfully."

./resql-trainer &
trainer_pid=$!
echo "Trainer has been started successfully."

wait $trainer_pid
status=$?
if [ $status -ne 0 ]; then
  echo "trainer failed"
  exit 1
fi

wait $server_pid
status=$?
if [ $status -ne 0 ]; then
  echo "server failed"
  exit 1
fi

echo "Tests has been completed successfully."

rm -rf *.resql
rm -rf resql resql-cli resql-benchmark build/*
cd build
cmake ../.. -DPGO=use
make -j 1 && make install
cd ../..
