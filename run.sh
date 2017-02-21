#!/bin/bash
cd build
cmake ..
make
./filter_proxy_C 1024 127.0.0.1
