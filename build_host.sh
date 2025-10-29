#!/bin/bash
DST_DIR="dist"

# create dir
mkdir -p $DST_DIR
cd $DST_DIR

# configure
cmake -DBUILD_UNIT_SOCKET_TEST=ON -DBUILD_MAIN=OFF ..

# make
make -j$(nproc)
