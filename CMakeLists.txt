cmake_minimum_required(VERSION 3.22)
project(video C)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

# 查找库文件
# find_library(ROCKIT_LIB
#     NAMES rockit librockit.so librockit.a
#     REQUIRED
# )
# find_library(ROCKCHIP_MPP_LIB
#     NAMES  mpp librockchip_mpp.so librockchip_mpp.a librockchip_mpp.so.0 librockchip_mpp.1
#     REQUIRED
# )
# find_library(RGA_LIB
#     NAMES  rga librga.so librga.a
#     REQUIRED
# )

# 源代码文件
# set(SOURCES
#     "src/main.c"
#     "src/file.c"
#     "src/video.c"
# )

# 创建可执行文件
# add_executable(${PROJECT_NAME} ${SOURCES})

# add_executable(
#     enc_file
#     "src/enc_file.c"
# )
# 链接 Rockchip 库
# target_link_libraries(enc_file PRIVATE
#     ${ROCKIT_LIB}
#     ${ROCKCHIP_MPP_LIB}
#     ${RGA_LIB}
# )

add_executable(
    socket_test
    "tests/socket_test.c"
    "src/socket.c"
    "src/time.c"
)
target_include_directories(socket_test PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)
