#!/bin/bash

# Install the Neo SDK

cd ../
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
cmake --build . --target install
ldconfig
