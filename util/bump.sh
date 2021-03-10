#!/bin/sh
set -e
cd "$(dirname "$0")"

if [ "$#" -lt 2 ]; then
    echo "Pass version string, e.g ./bump.sh 1.0.0-latest 1.0.1";
    exit 1
fi

(set -x; sed -i "s/$1/$2/g" ../c/resql.h)
(set -x; sed -i "s/$1/$2/g" ../go/client.go)
(set -x; sed -i "s/$1/$2/g" ../java/src/main/java/resql/Resql.java)
(set -x; sed -i "s/$1/$2/g" ../java/pom.xml)
(set -x; sed -i "s/$1/$2/g" ../src/rs.h)
(set -x; sed -i "s/$1/$2/g" ../util/cli/resql_cli.c)
(set -x; sed -i "s/$1/$2/g" ../util/docker/alpine/Dockerfile)
(set -x; sed -i "s/$1/$2/g" ../util/docker/debian/Dockerfile)
