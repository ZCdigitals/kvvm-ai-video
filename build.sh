#!/bin/bash

DST_DIR="dist"

# create dir
mkdir -p $DST_DIR
cd $DST_DIR

# configure
cmake -DCMAKE_TOOLCHAIN_FILE=/workspace/toolchain.cmake ..

# make
make -j$(nproc)
