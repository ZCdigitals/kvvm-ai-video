#!/bin/bash

DST_DIR="dist"

# create dir
mkdir -p $DST_DIR
cd $DST_DIR

# configure
cmake -DBUILD_UNIT_V4L2_ENC_TEST=ON -DCMAKE_TOOLCHAIN_FILE=/workspace/toolchain.cmake ..

# make
make -j$(nproc)

# show result
echo "Binary: $(pwd)/video"
file video
size video
