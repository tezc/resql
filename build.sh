#!/bin/sh
set -e
cd "$(dirname "$0")"
cwd=$(pwd)

cleanup () {
  if [ -n "$1" ]; then
    kill "$(jobs -p)" || true
    echo "Aborted by $1"
  elif [ $status -ne 0 ]; then
    kill "$(jobs -p)" || true
    echo "Failure (status $status)"
  else
    echo "Success"
  fi

  echo "cleanup"
  cd $cwd
  rm -rf bin/resql-trainer bin/build/ bin/pgo/
}

trap 'status=$?; cleanup; exit $status' EXIT
trap 'trap - HUP; cleanup SIGHUP; kill -HUP $$' HUP
trap 'trap - INT; cleanup SIGINT; kill -INT $$' INT
trap 'trap - TERM; cleanup SIGTERM; kill -TERM $$' TERM

echo $cwd
cd bin
rm -rf resql* build pgo
mkdir build pgo
cd build
cmake ../.. -DPGO=generate
make -j 1 && make install
cd ..

./resql -e >> /dev/null &
server_pid=$!
./resql-trainer >> /dev/null &
trainer_pid=$!

wait $trainer_pid
status=$?
if [ $status -ne 0 ];then
  echo "trainer failed"
  exit 1
fi

wait $server_pid
status=$?
if [ $status -ne 0 ];then
  echo "server failed"
  exit 1
fi

rm -rf resql* build/*
cd build
cmake ../.. -DPGO=use
make -j 1 && make install
cd ../..
