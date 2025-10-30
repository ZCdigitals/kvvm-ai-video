cmake_minimum_required(VERSION 3.22)
project(video C)

option(BUILD_ENC_TEST "build enc test" OFF)
option(BUILD_V4L2_TEST "build V4L2 test" OFF)
option(BUILD_UNIT_SOCKET_TEST "build unit socket test" OFF)
option(BUILD_UNIT_V4L2_ENC_TEST "build unit v4l2 enc test" OFF)
option(BUILD_MAIN "build main" ON)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

if(BUILD_ENC_TEST OR BUILD_UNIT_V4L2_ENC_TEST OR BUILD_MAIN)
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

if(BUILD_ENC_TEST)
    # build enc test
    add_executable(
        enc_test
        "tests/enc_test2.c"
    )
    # link
    target_link_libraries(enc_test PRIVATE
        ${ROCKIT_LIB}
        ${ROCKCHIP_MPP_LIB}
        ${RGA_LIB}
    )
endif()

if(BUILD_V4L2_TEST)
    # build v4l2 test
    add_executable(
        v4l2_test
        "tests/v4l2_test.c"
    )
endif()

if(BUILD_UNIT_SOCKET_TEST)
    # build unit socket test
    add_executable(
        unit_socket_test
        "tests/unit_socket_test.c"
        "src/socket.c"
        "src/utils.c"
    )
    target_include_directories(unit_socket_test PRIVATE
        ${PROJECT_SOURCE_DIR}/src
    )
endif()

if(BUILD_UNIT_V4L2_ENC_TEST)
    #build unit v4l2 enc test
    # add_executable(
    #     unit_v4l2_enc_test
    #     "tests/unit_v4l2_enc_test.c"
    #     "src/enc.c"
    #     "src/video.c"
    #     "src/utils.c"
    # )
    # target_include_directories(unit_v4l2_enc_test PRIVATE
    #     ${PROJECT_SOURCE_DIR}/src
    # )
    # target_link_libraries(unit_v4l2_enc_test PRIVATE
    #     ${ROCKIT_LIB}
    #     ${ROCKCHIP_MPP_LIB}
    #     ${RGA_LIB}
    # )


    add_executable(
        v4l2_enc_test
        "tests/v4l2_enc_test.c"
    )
    # target_include_directories(unit_v4l2_enc_test PRIVATE
    #     ${PROJECT_SOURCE_DIR}/src
    # )
    target_link_libraries(v4l2_enc_test PRIVATE
        ${ROCKIT_LIB}
        ${ROCKCHIP_MPP_LIB}
        ${RGA_LIB}
    )
endif()

if(BUILD_MAIN)
    # create exec
    add_executable(
        ${PROJECT_NAME}
        "src/enc.c"
        "src/main.c"
        "src/socket.c"
        "src/utils.c"
        "src/video.c"
    )
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${PROJECT_SOURCE_DIR}/src
    )
    # link
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${ROCKIT_LIB}
        ${ROCKCHIP_MPP_LIB}
        ${RGA_LIB}
    )
endif()
