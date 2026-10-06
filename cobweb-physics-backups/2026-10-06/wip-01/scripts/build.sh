#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build
flags=(-std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -Iinclude)
core=(src/geometry.cpp src/fibers.cpp src/bake.cpp src/damage.cpp src/export.cpp)
physics=(src/physics_graph.cpp src/tear_binding.cpp src/xpbd_solver.cpp)
"${CXX:-g++}" "${flags[@]}" src/main.cpp "${core[@]}" -o build/cobweb_lab
"${CXX:-g++}" "${flags[@]}" tests/core_tests.cpp "${core[@]}" -o build/core_tests
"${CXX:-g++}" "${flags[@]}" src/sim_cli.cpp "${core[@]}" "${physics[@]}" -o build/cobweb_sim
"${CXX:-g++}" "${flags[@]}" tests/physics_binding_tests.cpp "${core[@]}" "${physics[@]}" -o build/physics_binding_tests
"${CXX:-g++}" "${flags[@]}" tests/physics_solver_tests.cpp src/xpbd_solver.cpp -o build/physics_solver_tests
