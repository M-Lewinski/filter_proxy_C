cmake_minimum_required(VERSION 3.6)
project(filter_proxy_C)

set(CMAKE_C_STANDARD 11)

set(SOURCE_FILES main.c)
add_executable(filter_proxy_C ${SOURCE_FILES})