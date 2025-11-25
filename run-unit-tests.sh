#!/bin/sh

set -e

echo "Running emeraude-engine unit tests ..."

echo "======= CONFIGURING ======"
cmake -S ./ -B ./cmake-build-utest -DCMAKE_BUILD_TYPE=Release -DEMERAUDE_ENABLE_TESTS=ON -G "Ninja"
echo "======= COMPILING ======"
cmake --build ./cmake-build-utest --config Release
echo "======= RUNNING ======"
ctest --test-dir ./cmake-build-utest --verbose
echo "======= FINISHED ======"
