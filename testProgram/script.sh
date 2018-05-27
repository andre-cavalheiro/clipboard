#!/bin/bash

mkdir -p /tmp/tests
cd ..
curdir=$(pwd)

cd /tmp/tests


for i in 1 2 3 4 5 6 7 8 ; do
    mkdir -p $i
    cd $i
    ln -s "$curdir" .
    cd ..
done
