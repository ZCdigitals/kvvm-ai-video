#!/bin/bash

DST_DIR="dist"

# 创建构建目录
mkdir -p $DST_DIR
cd $DST_DIR

# 配置
cmake -DCMAKE_TOOLCHAIN_FILE=/workspace/toolchain.cmake ..

# 编译
make -j$(nproc)

# 显示构建结果
echo "Build completed!"
# echo "Binary: $(pwd)/video"

# 检查文件类型
# file video

# 显示文件大小
# size video
