#!/bin/bash
DST_DIR="dist"

# create dir
mkdir -p $DST_DIR
cd $DST_DIR

# configure
cmake -DBUILD_HOST_TESTS=ON -DBUILD_TARGET=OFF ..

# make
make -j$(nproc)
