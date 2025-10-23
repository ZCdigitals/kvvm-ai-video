cmake_minimum_required(VERSION 3.13)
project(video C)

# 源代码文件
set(SOURCES
    "src/main.c"
    "src/file.c"
    "src/video.c"
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})
