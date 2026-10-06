#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p vendor/build
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -Ivendor/include vendor/src/*.cpp -o vendor/build/cobweb_lab
