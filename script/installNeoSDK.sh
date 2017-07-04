#!/bin/bash

# Install the Neo SDK

git clone https://github.com/micvision/neo-sdk.git

cd neo-sdk

mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
cmake --build . --target install
ldconfig
