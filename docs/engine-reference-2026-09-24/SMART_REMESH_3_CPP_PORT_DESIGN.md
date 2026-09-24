# Smart Remesh 3 C++ Research Port Design

## Status

Research-only. This design deliberately does not wire Smart Remesh-derived code into JericoEngine, JericoEditor, ProceduralAuthoring, AssetPipeline, or Shipping.

## Why it is isolated

The inspected Smart Remesh 3.0.0 Blender extension declares `GPL-3.0-or-later`. The bundled Instant Meshes executable is separately licensed under BSD-3-Clause. A source-derived C++ translation of the add-on must therefore be treated as GPL material unless a separate clean-room implementation or additional licence is obtained.

The research port lives under `Research/SmartRemesh3GPL/`, has no dependency from production CMake, and is meant only to validate the mathematics and produce black-box/reference tests for a future Jerico-native implementation.

## Goals

1. Preserve the most reusable mathematical ideas in an executable C++ reference.
2. Make the kernels deterministic, small, and independently testable.
3. Avoid Blender, NumPy, BMesh, Python, and Instant Meshes dependencies in the reference kernels.
4. Keep all source-derived work out of Jerico production targets.
5. Produce enough tests to let a future independent implementation match observable behavior without re-reading the GPL source.

## Implemented research kernels

- basic `Vec3` geometry helpers;
- tangent-plane projection and normalization;
- best representative selection for a 4-RoSy cross field;
- one-level Jacobi 4-RoSy smoothing with optional locked vertices;
- deterministic field combing over graph adjacency;
- signed-volume orientation test for closed triangle meshes;
- histogram-valley crease threshold selection;
- Otsu threshold selection;
- Bellman-Ford-style vector relaxation used as both a graph distance transform and a Lipschitz sizing limiter;
- face-count estimation from triangle area and local target edge length;
- uniform scaling of an adaptive sizing field to hit a requested face budget;
- centroid-bounded lattice-site collapse with disjoint sets;
- 3D Douglas-Peucker polyline simplification;
- polygon Newell normal/area;
- basic topology quality metrics for polygon meshes.

## Intentionally not linked into production

The following are documented but not linked into Jerico production by this research branch:

- the full position-field solver;
- multiresolution hierarchy construction;
- full lattice graph face extraction;
- hard-surface region reconstruction and chamfer-band generation;
- Blender-specific UV repair and bake orchestration;
- BMesh loop surgery and pole relaxation;
- calls into the bundled Instant Meshes binary.

Those are large enough that importing them directly would create unnecessary GPL coupling. Their behavior is specified in `SMART_REMESH_3_TECHNICAL_ANALYSIS.md` so a future Jerico implementation can be designed independently.

## Public API

`Research/SmartRemesh3GPL/SmartRemeshMath.hpp` exposes only value types and pure functions. It performs no allocation behind opaque engine handles, owns no global state, and depends only on the C++ standard library.

The research target builds as a standalone executable using the local `CMakeLists.txt`. No root `CMakeLists.txt` changes are made.

## Verification

The test executable covers:

- quarter-turn representative selection in the cross field;
- propagation with a locked orientation constraint;
- deterministic combing of a quarter-turned neighbor;
- positive signed volume for outward tetrahedron winding and negative for inverted winding;
- crease threshold selection on a synthetic two-population angle distribution;
- Otsu separation of a bimodal scalar distribution;
- graph-distance/Lipschitz relaxation on a chain;
- adaptive size rescaling to a requested face count;
- prevention of transitive chain-collapse in lattice clustering;
- Douglas-Peucker retention of a meaningful corner;
- Newell area on a unit quad;
- topology metric detection of a clean quad.

## Future Jerico-native route

A production implementation should be a new Jerico subsystem written from the public algorithm descriptions and the test vectors produced here, not by mechanically translating GPL source. The strongest candidates are:

1. adaptive sizing field;
2. 4-RoSy orientation field;
3. hard-surface feature-line detector;
4. panel extraction/rebuild;
5. pole relocation as a post-remesh topology optimization.

The Instant Meshes path can also be studied directly from its BSD-licensed upstream source if Jerico wants a permissively licensed implementation foundation.
