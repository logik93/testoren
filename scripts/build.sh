#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")/.."
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release "$@"
cmake --build build --config Release --parallel "${BUILD_JOBS:-3}"
ctest --test-dir build -C Release --output-on-failure
