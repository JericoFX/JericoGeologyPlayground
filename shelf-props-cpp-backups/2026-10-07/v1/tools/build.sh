#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT=${1:-"$ROOT/build-native"}
mkdir -p "$OUT"
CXX=${CXX:-g++}
SOURCES=(core industrial fracture furniture evaluate materials damage wood damp_wood localized_damage fine_wear interfaces png export_glb export_obj shelf_props shelf_props_materials shelf_props_placement)
OBJS=()
for source in "${SOURCES[@]}"; do
 object="$OUT/$source.o"
 if [[ ! -f "$object" || "$ROOT/src/$source.cpp" -nt "$object" ]]; then
  "$CXX" -std=c++17 -O2 -Wall -Wextra -Wpedantic -ffp-contract=off -fno-fast-math -I"$ROOT/include" -I"$ROOT/third_party" -c "$ROOT/src/$source.cpp" -o "$object"
 fi
 OBJS+=("$object")
done
for target in shelf_props_main shelf_props_tests placement_tests; do
 file="$ROOT/src/$target.cpp"; [[ "$target" != shelf_props_main ]] && file="$ROOT/tests/$target.cpp"
 name=$target; [[ "$target" == shelf_props_main ]] && name=jerico_shelf_props
 "$CXX" -std=c++17 -O2 -I"$ROOT/include" -I"$ROOT/third_party" "$file" "${OBJS[@]}" -lz -o "$OUT/$name"
done
"$OUT/shelf_props_tests" "$OUT/test-out"
"$OUT/placement_tests"
for name in png_tests evaluate_tests material_scope_tests export_tests export_obj_tests; do
 "$CXX" -std=c++17 -O2 -I"$ROOT/include" -I"$ROOT/third_party" "$ROOT/tests/$name.cpp" "${OBJS[@]}" -lz -o "$OUT/$name"
 "$OUT/$name" "$OUT/$name-out"
done
