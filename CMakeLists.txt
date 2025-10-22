cmake_minimum_required(VERSION 3.13)
project(video C)

# 设置工具链路径和前缀
set(TOOLCHAIN_PATH "/toolchain")
set(TOOLCHAIN_PREFIX "arm-rockchip830-linux-uclibcgnueabihf")

# 设置编译器
set(CMAKE_C_COMPILER "${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}-gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}-g++")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}-gcc")
set(CMAKE_AR "${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}-ar" CACHE FILEPATH "Archiver")
set(CMAKE_RANLIB "${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}-ranlib" CACHE FILEPATH "Ranlib")
set(CMAKE_STRIP "${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}-strip" CACHE FILEPATH "Strip")
set(CMAKE_OBJCOPY "${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}-objcopy" CACHE FILEPATH "Objcopy")
set(CMAKE_OBJDUMP "${TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}-objdump" CACHE FILEPATH "Objdump")

# 编译器标志
set(ARCH_FLAGS "-march=armv8-a -mtune=cortex-a53 -mfpu=neon-fp-armv8 -mfloat-abi=hard")
set(CMAKE_C_FLAGS "${ARCH_FLAGS} -Wall -Wextra" CACHE STRING "C flags")
set(CMAKE_CXX_FLAGS "${ARCH_FLAGS} -Wall -Wextra" CACHE STRING "C++ flags")

# 搜索路径
set(CMAKE_FIND_ROOT_PATH "${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# 源代码文件
set(SOURCES
    "src/main.c"
    "src/file.c"
    "src/video.c"
)

# 包含目录
include_directories(
    include
    ${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}/include
)

# 链接目录
link_directories(
    ${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}/lib
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES})

# 指定头文件搜索路径:cite[9]
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 查找并链接必要的库
set(V4L2_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(V4L2_LIBRARIES ${CMAKE_CURRENT_SOURCE_DIR}/lib/libv4l2.so)

# 添加头文件路径
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${V4L2_INCLUDE_DIRS}
)

# 链接库
target_link_options(${PROJECT_NAME} PRIVATE
    "-Wl,-rpath-link,${CMAKE_SOURCE_DIR}/lib"
)
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${V4L2_LIBRARIES}
    m
    pthread
    rt
)
