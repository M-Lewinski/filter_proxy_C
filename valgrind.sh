#!/bin/bash
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
valgrind --track-origins=yes --leak-check=yes  ./filter_proxy_C 1024 127.0.0.1
