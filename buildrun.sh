#!/usr/bin/env bash
set -exuo pipefail
trash -v *.log || true
cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DPhysX_ROOT=$HOME/git/physx4 -B build 2>&1 | tee cmake.log
ninja -C build 2>&1 | tee ninja_build.log
./build/big_example/big_example 2>&1 | tee run_big_example.log
