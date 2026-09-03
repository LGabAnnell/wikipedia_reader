#! /bin/bash
rm -rf build
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build -j14
ctest --test-dir build/tests --output-on-failure