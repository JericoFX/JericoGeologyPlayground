# M8 RBrush Slice 29 — SDF Projection Implementation Plan

> **Execution:** Continue in the single authoritative task and checkout. Apply
> TDD and verification-before-completion. Do not create a task/worktree/branch.

**Goal:** Convert the Slice 27 derived 2D SDF into bounded canonical Slice 28
stamp points through all eight required projection modes.

**Architecture:** Add `SdfBrushProjection` to the existing authoring-only
`Jerico::ProceduralAuthoring` library. It validates/canonicalizes a frame,
bounds and candidate bricks, samples the existing 2D SDF continuously, builds a
bounded extruded 3D operand and returns sorted `SdfBrushStampPoint` values. A
focal test feeds those points to `applySdfBrushStamp`; no UI or second SDF
implementation is introduced.

**Tech stack:** C++20, existing `MaskSignedDistanceField2D`, `SparseSdfField`,
`SdfBrushStamp`, `Jerico::Core::Result`, CMake/CTest.

## Files

- Create:
  `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/SdfBrushProjection.hpp`
- Create: `Tools/ProceduralAuthoring/Source/SdfBrushProjection.cpp`
- Modify: `Tools/ProceduralAuthoring/CMakeLists.txt`
- Create: `Tests/ProceduralAuthoring/SdfBrushProjectionTests.cpp`
- Modify selectively: `Tests/ProceduralAuthoring/CMakeLists.txt`
- Modify: `Tests/TestSchedulingPolicy.cmake`
- Modify: `Tests/RepositoryContract/VerifyTestScheduling.cmake`

## Steps

- [x] **1. Write and register the complete failing test.**

  Require every projection mode to emit a finite, bounded point from a real
  mask/field fixture and require Slice 28 to change/reopen the field. Add exact
  asymmetric-mask mapping, order, bounds, budget, validation and cancellation
  cases.

- [x] **2. Observe scheduling RED, then causal product RED.**

  Register `procedural.sdf_brush_projection` without a scheduling label. Run
  `cmake --preset windows-vulkan-editor`, capture the exact 287→288 inventory
  hash, update both scheduling authorities, rerun configure, then build target
  `JericoSdfBrushProjectionTests`. Expected causal failure: C1083 for the
  missing `SdfBrushProjection.hpp`.

- [x] **3. Implement the public projection contract.**

  Define the eight-mode enum, canonical frame/settings/request/result structs,
  hard budget reuse and `projectSdfBrushToSparseField` with stable errors.

- [x] **4. Validate immutable inputs and canonicalize candidates.**

  Validate the complete mask shape/hashes/finiteness, frame orthonormality and
  handedness, bounds, settings, budget and candidate-brick inventory. Sort a
  private copy and fail on duplicates or missing bricks.

- [x] **5. Implement metric bilinear mask sampling.**

  Preserve Slice 27 aspect and signed-distance units. Support bounded and
  wrapped axes without reading outside the mask.

- [x] **6. Implement all eight coordinate mappings.**

  Implement Surface Tangent, Planar, Triplanar, Spherical, Cylindrical, Radial,
  Object Local and World exactly as the accepted design defines. Apply rotation
  before sampling and fail closed when Triplanar cannot resolve a normal.

- [x] **7. Build the bounded extruded operand.**

  Enumerate samples inside affected bounds, combine lateral 2D SDF and one-sided
  depth slab, omit values outside the source narrow band, clamp emitted values
  and enforce the caller/hard output budget.

- [x] **8. Hash and cancel canonically.**

  Hash source/mask/frame/settings/candidates and emitted points with canonical
  little-endian FNV-1a. Check cancellation during every potentially long loop;
  return no partial points.

- [x] **9. Run focal and neighbour GREEN.**

  Build/run the new focal and rerun Slice 27/28 plus all six existing Sparse
  SDF neighbours. Expected: projection `1/1`, RBrush SDF chain `3/3`, Sparse SDF
  neighbours `6/6`.

- [x] **10. Run Release, integrated and boundary gates.**

  Run Release focal, the complete Debug build, the established 12-test
  runtime-only/Shipping/source-exclusion/authority/scheduling matrix and
  `git diff --check`.

- [x] **11. Retain and publish the checkpoint.**

  Record exact per-mode points/hashes and RED/GREEN commands in
  `docs/evidence/M8_RBRUSH_SDF_PROJECTION.md`; update Capability Map and
  `THREAD_HANDOFF.md`; commit product separately from evidence; preserve the
  user's mixed CMake hunks; push `main`; confirm `HEAD == origin/main`.

## Stop condition

Stop Slice 29 after all eight projections feed Slice 28 and every stated gate is
green. Do not start Slice 30 crack geometry, editor/Undo, DDC, runtime RBrush or
Procedural 3D Layout in the same checkpoint.
