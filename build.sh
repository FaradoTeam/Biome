#!/bin/sh

mkdir -p build && cd build

# Сделать один раз при необходимости:
# conan profile detect --force

conan install ../conanfile.py \
    --build=missing \
    -s compiler.libcxx=libstdc++11 \
    -of=conan/

cmake .. \
    -DBUILD_TESTS=ON \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=conan/conan_toolchain.cmake

make -j$(($(nproc) - 1))

ctest --output-on-failure
