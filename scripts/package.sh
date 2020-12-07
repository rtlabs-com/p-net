#!/bin/bash

PORT=$1

usage()
{
    echo "Usage: $0 <port>"
    exit
}

[ "$#" -ne 1 ] && usage

EXCLUDE=":(exclude)scripts"
EXCLUDE="$EXCLUDE :(exclude)src/ports"

files=$(git ls-files $EXCLUDE)
files="$files $(git ls-files src/ports/$PORT)"

rev=$(git describe --tag --always --dirty)

tar --transform 's,,p-net/,' -jcf p-net-$rev.tar.bz2 $files
