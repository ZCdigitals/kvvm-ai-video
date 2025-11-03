cmake_minimum_required(VERSION 3.22)
project(video C)

option(BUILD_ENC_TEST "build enc test" OFF)
option(BUILD_V4L2_TEST "build V4L2 test" OFF)
option(BUILD_V4L2_ENC_TEST "build unit v4l2 enc test" OFF)
option(BUILD_V4L2_QUERY_SIZE_TEST "build v4l2 query size test" OFF)
option(BUILD_UNIT_SOCKET_TEST "build unit socket test" OFF)
option(BUILD_MAIN "build main" ON)

if(BUILD_V4L2_ENC_TEST OR BUILD_MAIN)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
endif()

if(BUILD_ENC_TEST OR BUILD_V4L2_ENC_TEST OR BUILD_MAIN)
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

include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

if(BUILD_ENC_TEST)
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

if(BUILD_V4L2_TEST)
    # build v4l2 test
    add_executable(
        v4l2_test
        "tests/v4l2_test.c"
    )
endif()

if(BUILD_V4L2_ENC_TEST)
    # build unit v4l2 enc test
    add_executable(
        unit_v4l2_enc_test
        "tests/unit_v4l2_enc_test.c"
        "src/video.c"
        "src/utils.c"
    )
    target_include_directories(unit_v4l2_enc_test PRIVATE
        ${PROJECT_SOURCE_DIR}/src
    )
    target_link_libraries(unit_v4l2_enc_test PRIVATE
        ${ROCKIT_LIB}
        ${ROCKCHIP_MPP_LIB}
        ${RGA_LIB}
    )

    add_executable(
        v4l2_enc_test
        "tests/v4l2_enc_test.c"
        "src/utils.c"
    )
    target_include_directories(v4l2_enc_test PRIVATE
        ${PROJECT_SOURCE_DIR}/src
    )
    target_link_libraries(v4l2_enc_test PRIVATE
        ${ROCKIT_LIB}
        ${ROCKCHIP_MPP_LIB}
        ${RGA_LIB}
    )
endif()

if(BUILD_V4L2_QUERY_SIZE_TEST)
    add_executable(
        v4l2_query_size_test
        "tests/v4l2_query_size_test.c"
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

if(BUILD_MAIN)
    # read version
    file(READ ${CMAKE_SOURCE_DIR}/version VERSION_STRING)
    string(STRIP ${VERSION_STRING} VERSION_STRING)

    # read git hash
    execute_process(
        COMMAND git config --global --add safe.directory ${CMAKE_SOURCE_DIR}
    )
    execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(NOT COMMIT_HASH)
        set(COMMIT_HASH "unknown")
    endif()

    # get time
    string(TIMESTAMP BUILD_TIMESTAMP "%Y-%m-%dT%H:%M:%SZ" UTC)

    # create exec
    add_executable(
        ${PROJECT_NAME}
        "src/args.c"
        "src/main.c"
        "src/socket.c"
        "src/utils.c"
        "src/video.c"
    )
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${PROJECT_SOURCE_DIR}/src
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        VERSION="${VERSION_STRING}"
        COMMIT="${COMMIT_HASH}"
        BUILD_TIME="${BUILD_TIMESTAMP}"
    )
    # link
    target_link_libraries(${PROJECT_NAME} PRIVATE
        ${ROCKIT_LIB}
        ${ROCKCHIP_MPP_LIB}
        ${RGA_LIB}
    )
endif()
