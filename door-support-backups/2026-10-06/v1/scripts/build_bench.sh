#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python generate_contract.py --output out/contract_examples
python physics_fixture.py
mkdir -p build output/physics
g++ -std=c++17 -O3 -Wall -Wextra -Werror -fno-rtti -fno-exceptions -ffp-contract=off -msse2 -mfpmath=sse -pthread -DJPH_ENABLE_ASSERTS -DJPH_OBJECT_LAYER_BITS=16 -DNDEBUG -I"${JOLT_SOURCE:-vendor/JoltPhysics-5.6.0}" -Ibuild src/door_jolt_bench.cpp "${JOLT_LIBRARY:-build-deps/jolt-release-sse2/libJolt.a}" -o build/door_jolt_bench
build/door_jolt_bench output/physics
