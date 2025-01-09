#!/usr/bin/env sh

mkdir  build_debug
cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build_debug
cmake --build build_debug
