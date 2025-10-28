cmake_minimum_required(VERSION 3.22)
project(video C)

option(BUILD_HOST_TESTS "build host tests" OFF)
option(BUILD_TARGET_TESTS "build target tests" OFF)
option(BUILD_TARGET "build target" ON)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

if(BUILD_HOST_TESTS)
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

if(BUILD_TARGET_TESTS OR BUILD_TARGET)
    # find lib
    find_library(ROCKIT_LIB
        NAMES rockit librockit.so librockit.a
        REQUIRED
    )
    find_library(ROCKCHIP_MPP_LIB
        NAMES mpp librockchip_mpp.so librockchip_mpp.a librockchip_mpp.so.0 librockchip_mpp.1
        REQUIRED
    )
    find_library(RGA_LIB
        NAMES rga librga.so librga.a
        REQUIRED
    )
endif()

if(BUILD_TARGET_TESTS)
    # build v4l2 test
    add_executable(
        v4l2_test
        "tests/v4l2_test.c"
        "src/video.c"
    )
    target_include_directories(v4l2_test PRIVATE
        ${PROJECT_SOURCE_DIR}/src
    )

    # build enc test
    add_executable(
        enc_test
        "tests/enc_test.c"
    )
    # link
    target_link_libraries(enc_test PRIVATE
        ${ROCKIT_LIB}
        ${ROCKCHIP_MPP_LIB}
        ${RGA_LIB}
    )
endif()

if(BUILD_TARGET)
    # create exec
    add_executable(
        ${PROJECT_NAME}
        "src/main.c"
    )
    # link
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${ROCKIT_LIB}
        ${ROCKCHIP_MPP_LIB}
        ${RGA_LIB}
    )
endif()
