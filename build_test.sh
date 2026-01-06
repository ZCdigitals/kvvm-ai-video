#!/bin/bash

DST_DIR="dist"

# create dir
mkdir -p $DST_DIR
cd $DST_DIR

# configure
cmake \
    -DBUILD_DMA_TEST=ON \
    -DBUILD_MAIN=OFF \
    -DCMAKE_TOOLCHAIN_FILE=/workspace/toolchain.cmake ..

# make
make -j$(nproc)
