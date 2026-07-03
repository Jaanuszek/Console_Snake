#!/bin/bash

BUILD_DIR="build/"

cmake -B ${BUILD_DIR} -S .
cmake --build ${BUILD_DIR} -j$(nproc) --verbose