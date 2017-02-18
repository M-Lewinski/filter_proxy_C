cmake_minimum_required(VERSION 2.8)
project(filter_proxy_C)
set(CMAKE_C_STANDARD 11)

aux_source_directory(./source SOURCE)
include_directories(${SOURCE})

set(CMAKE_CXX_FLAGS_DEBUG  "-g")

aux_source_directory(. MAIN)
find_package( Threads REQUIRED )

add_executable(${PROJECT_NAME} ${MAIN} ${SOURCE} source/rule.h source/rule.c source/request.h source/request.c)
target_link_libraries(${PROJECT_NAME} pthread)
