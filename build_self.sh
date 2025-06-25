#!/bin/bash
if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

cd `pwd`/build &&
    cmake -DCMAKE_BUILD_TYPE=Debug ..&&
    make clean&&
    make
cd ..