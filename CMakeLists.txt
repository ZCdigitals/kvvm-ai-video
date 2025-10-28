cmake_minimum_required(VERSION 3.22)
project(video C)

option(BUILD_HOST_TESTS "build host tests" OFF)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

if (BUILD_HOST_TESTS)
    # build socket test
    add_executable(
        socket_test
        "tests/socket_test.c"
        "src/socket.c"
    )
    target_include_directories(socket_test PRIVATE
        ${PROJECT_SOURCE_DIR}/src
    )
endif()

if (!BUILD_HOST_TESTS)
    # find lib
    find_library(ROCKIT_LIB
        NAMES rockit librockit.so librockit.a
        REQUIRED
    )
    find_library(ROCKCHIP_MPP_LIB
        NAMES  mpp librockchip_mpp.so librockchip_mpp.a librockchip_mpp.so.0 librockchip_mpp.1
        REQUIRED
    )
    find_library(RGA_LIB
        NAMES  rga librga.so librga.a
        REQUIRED
    )

    # source
    set(SOURCES
        "src/main.c"
        "src/file.c"
        "src/video.c"
    )

    # create exec
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()
