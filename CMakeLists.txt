cmake_minimum_required(VERSION 3.6.1)
project(filter_proxy_C)
set(CMAKE_C_STANDARD 11)

aux_source_directory(./source SOURCE)
include_directories(${SOURCE})

aux_source_directory(. MAIN)

add_executable(${PROJECT_NAME} ${MAIN} ${SOURCE})