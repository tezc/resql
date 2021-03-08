#!/bin/sh
set -e
cd "$(dirname "$0")"
cwd=$(pwd)

if [ "$#" -ne 0 ]; then
  if [ "$1" = --install ]; then
    if [ ! -f bin/resql ] || [ ! -f bin/resql-cli ]; then
      echo "Executables are missing, run ./build.sh first."
      exit 1
    fi

    (set -x; cp bin/resql bin/resql-cli /usr/local/bin/)
    (set -x; cp bin/resql.ini /etc/)
    exit 0
  elif [ "$1" = --uninstall ]; then
    (set -x; rm -rf /usr/local/bin/resql /usr/local/bin/resql-cli)
    (set -x; rm -rf /etc/resql.ini)
    exit 0
  else
    echo "Unknown option : $1";
    exit 1
  fi
fi

cleanup() {
  cd "$cwd"

  if [ -n "$1" ]; then
    rm -rf bin/resql bin/resql-cli
    kill "$(jobs -p)" || true
    echo "Aborted by $1"
  elif [ "$status" -ne 0 ]; then
    rm -rf bin/resql bin/resql-cli
    kill "$(jobs -p)" || true
    echo "Failure (status $status)"
  else
    echo "Success"
  fi

  echo "Clean-up done"
  rm -rf bin/resql-trainer bin/build/ bin/pgo/
}

trap 'status=$?; cleanup; exit $status' EXIT
trap 'trap - HUP; cleanup SIGHUP; kill -HUP $$' HUP
trap 'trap - INT; cleanup SIGINT; kill -INT $$' INT
trap 'trap - TERM; cleanup SIGTERM; kill -TERM $$' TERM

echo "$cwd"
cd bin
rm -rf resql resql-cli build pgo
mkdir build pgo
cd build
cmake ../.. -DPGO=generate
make -j 1 && make install
echo "First step has been completed successfully."
cd ..

./resql -e > /dev/null &
server_pid=$!
./resql-trainer > /dev/null &
trainer_pid=$!

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

./resql -w
rm -rf resql resql-cli build/*
cd build
cmake ../.. -DPGO=use
make -j 1 && make install
cd ../..
