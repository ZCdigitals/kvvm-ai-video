#!/bin/bash

VERSION_PATH="src/version.h"
bump_build() {
    current=$(grep "VERSION_BUILD" $VERSION_PATH | grep -oE '[0-9]+')
    new=$((current + 1))
    sed -i "s/VERSION_BUILD $current/VERSION_BUILD $new/" $VERSION_PATH
}

bump_build

DST_DIR="dist"

# create dir
mkdir -p $DST_DIR
cd $DST_DIR

# configure
cmake -DCMAKE_TOOLCHAIN_FILE=/workspace/toolchain.cmake ..

# make
make -j$(nproc)
