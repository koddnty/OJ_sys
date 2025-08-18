cmake_minimum_required(VERSION 3.10)
project(OJ_Judge)

set(CMAKE_CXX_STANDARD 17)
set(SRC_DIR "/home/shared/OJ_sys")
file(GLOB_RECURSE SOURCES "${PROJECT_SOURCE_DIR}/*.cpp")
find_package(OpenSSL REQUIRED)



include_directories(
    "${SRC_DIR}/workflow/workflow-master/_include/workflow"
    "${SRC_DIR}/tools/wfrest-main/_include/wfrest"
    "${SRC_DIR}/tools/json-develop/include"
)

add_executable(run ${SOURCES})


target_link_libraries( run
    crypt
    wfrest
    redis++
    workflow
    OpenSSL::SSL
    OpenSSL::Crypto
)