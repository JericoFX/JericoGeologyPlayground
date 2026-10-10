#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
checkpoint=../damage-checkpoint/JericoPipeCppCheckpoint01_20261007
engine=$checkpoint/original/build-chain/deps/engine-inputs
incs=(-Icore/include -I"$checkpoint/original/Source" -I"$engine/Engine/Core/Include" -I"$engine/Engine/Jobs/Include" -I"$engine/Tools/ProceduralAuthoring/Include" -I"$engine/Tools/ProceduralAuthoring/Source")
flags=(-O2 -std=c++20 -ffunction-sections -fdata-sections)
for file in DamageField RockAddonMath; do g++ "${flags[@]}" "${incs[@]}" -c "$checkpoint/original/Source/$file.cpp" -o "build/original_$file.o"; done
g++ "${flags[@]}" "${incs[@]}" -c "$engine/Tools/ProceduralAuthoring/Source/Geology/GeologyNoise.cpp" -o build/original_GeologyNoise.o
ar rcs build/liboriginal_damage.a build/original_*.o
for test in DamageField RockAddonMath; do g++ "${flags[@]}" "${incs[@]}" "$checkpoint/tests/original/${test}Tests.cpp" build/liboriginal_damage.a -Wl,--gc-sections -o "build/original_${test}_tests"; "build/original_${test}_tests"; done
for file in core/src/*.cpp; do g++ "${flags[@]}" "${incs[@]}" -Wall -Wextra -Wpedantic -c "$file" -o "build/$(basename "$file" .cpp).o"; done
ar rcs build/librocks_core.a build/alveolar_surface.o build/compare.o build/extract.o build/field.o build/heightfield.o build/polyhedral.o build/reference_studies.o build/surface_detail.o build/surface_families.o build/validate.o
for app in 'cli/main.cpp rocks_cli' 'tests/v3_erosion_tests.cpp v3_erosion_tests' 'tests/erosion_diagnose.cpp erosion_diagnose'; do read -r file name <<<"$app"; g++ "${flags[@]}" "${incs[@]}" "$file" build/librocks_core.a build/liboriginal_damage.a -Wl,--gc-sections -o "build/$name"; done
