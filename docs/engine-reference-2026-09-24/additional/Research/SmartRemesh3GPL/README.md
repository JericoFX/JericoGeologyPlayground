# Smart Remesh 3 GPL Research Reference

This directory is an isolated research port of mathematical behavior inspected in the user-supplied Smart Remesh 3.0.0 Blender add-on.

**License boundary:** the add-on declares `GPL-3.0-or-later`. For that reason this code is intentionally not connected to JericoEngine production CMake targets and must not be copied mechanically into production code. It exists to validate equations, behavior, and test vectors for a later independent Jerico implementation.

The bundled Instant Meshes executable in the inspected add-on is separately BSD-3-Clause licensed; Jerico can evaluate the upstream permissive implementation independently if desired.

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build --output-on-failure -C Release
```

See `../../docs/research/SMART_REMESH_3_TECHNICAL_ANALYSIS.md` for the full algorithm inventory and `../../docs/research/SMART_REMESH_3_CPP_PORT_DESIGN.md` for the isolation strategy.
