# M8 Sparse SDF Geology Typed Controls Implementation Plan

> **Superseded:** archival plan only. Do not execute it. Current implementation
> authority is `docs/JERICO_PROCEDURAL_GEOLOGY_RESEARCH_IMPLEMENTATION_BLUEPRINT.md`.

> **For the sole checkout owner:** REQUIRED SUB-SKILL: use
> `superpowers:executing-plans` inline, task by task. Do not create a branch,
> worktree, fork, coordinator, successor or parallel writer while this slice is
> active.

**Goal:** Make Jerico's native Sparse SDF produce and author real closed rocks,
solid cliffs/massifs and spline/BSP-driven cave negative space from one shared,
typed geology recipe, then prove the generated surface through Jerico's real
mesh/collision/navigation/LOD/cook/Vulkan path.

**Architecture:** Keep the existing sparse bricks, `SparseSdfBuilder`,
`SparseSdfField`, `SparseSdfSession` semantics, Marching Cubes extractor, BVH,
RBrush, collision, navigation and cook/runtime paths. Split authoring data from
compiled evaluation: `SparseSdfGeologyRecipe` owns every behavior-affecting
control; `NativeSdfGeologyRecipe` is its deterministic compiled form;
`SdfGeologyFieldEvaluator` evaluates geometry and geology channels at arbitrary
3D positions; `SparseSdfBuilder` samples that final scalar field; the existing
extractor only represents it. `Rock`, `Cliff` and `Cave` showcases become recipe
consumers. Cave topology is explicit subtraction of spline/BSP/chamber fields
from a solid host mass.

**Tech Stack:** C++20, Jerico `Result<T>`, existing sparse SDF bricks and
Marching Cubes, Qt 6 inspector shell, Jolt-compatible cooked triangle collision,
Recast/Detour navigation, Diligent Vulkan, CMake/CTest. No OpenVDB, EmberGen,
FastNoise2 or new dependency in this slice.

**Binding authorities:**

- `docs/JERICO_PROCEDURAL_GEOLOGY_RESEARCH_IMPLEMENTATION_BLUEPRINT.md`
- `docs/SDF_GEOLOGY_EROSION_FRACTURE_MATH.md`
- `docs/superpowers/specs/2026-08-23-m8-sparse-sdf-geology-typed-controls-design.md` (archival only)
- `docs/work/ACTIVE_SLICE.md`

## Global constraints

- Work directly on `main` in `F:\JericoEngine`; Git remains authoritative.
- Preserve every unrelated tracked/untracked change and stage only owned
  geology/cave paths or owned hunks at final publication.
- `origin/main` currently contains the erosion/fracture directive one commit
  ahead; read it through Git until a non-destructive integration is explicitly
  legal. Do not overwrite local work to align it.
- Do not edit the extractor to compensate for a wrong raw field. The existing
  exact-zero tie correctness fix remains independently testable.
- `NegativeSpace=None` must mean no tunnel, chamber, arch or cut-through.
- `Floor`, `Wall`, `Ceiling`, `ExteriorRock`, `Strata` and `BrokenFractured`
  classify the final surface; they never generate its shape.
- A preset only changes visible typed recipe values.
- Preview may be extreme but resource-safe; publish validation rejects open
  masses, unresolved features, thin walls, excess gradient error and failed
  derivatives rather than silently changing intent.
- No intermediate commit or push. Run `git diff --check` at every checkpoint and
  perform one selective publication commit only after real Vulkan evidence.
- Do not reopen published JericoBSP, Rock Family, Cliff or M8 Render Foundation
  showcases except as regression consumers of the shared layer.
- Before any Vulkan run, confirm there is no other Jerico/build/Vulkan writer.

## File map

- `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/SparseSdfGeologyRecipe.hpp`
  — complete versioned public control model and validation/hash API.
- `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/SdfGeologyFieldEvaluator.hpp`
  and `Source/SdfGeologyFieldEvaluator.cpp` — geometry/geology sampling, 3D
  strata, stable fracture IDs, bounded volumetric erosion and debug stages.
- `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/SparseSdfRedistance.hpp`
  and `Source/SparseSdfRedistance.cpp` — distance-quality measurement and
  narrow-band fast-sweeping repair without changing intended sign.
- `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/SparseSdfGeologySession.hpp`
  and `Source/SparseSdfGeologySession.cpp` — draft preview/apply/diff/undo/redo
  semantics for the shared geology recipe.
- `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/SparseSdfGeologyGenerator.hpp`
  and `Source/SparseSdfGeologyGenerator.cpp` — compile/orchestrate/classify/cook;
  no cave-specific generation logic.
- `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/NativeSdfGeology.hpp`
  and `Source/NativeSdfGeology.cpp` — compiled primitives/features and builder
  sampling only; replace the provisional shallow-fracture implementation.
- `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/SparseSdfField.hpp`
  and `Source/SparseSdfField.cpp` — distance-quality metadata and deterministic
  reconstruction access required by re-distance.
- `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/SparseSdfMesher.hpp`
  and `Source/SparseSdfMesher.cpp` — explicit iso/topology settings and measured
  output only after the field stages are green.
- `Editor/JericoEditor/Source/SparseSdfGeologyInspector.hpp/.cpp` — Basic and
  Advanced controls bound to one draft recipe.
- `Editor/JericoEditor/Source/JericoBspWorkbench.hpp/.cpp` — host the inspector
  inside the existing Sparse SDF workbench and reuse its transaction/viewport
  path; do not add a second editor shell.
- `Editor/JericoEditor/Source/SparseSdfRockFamily.cpp`,
  `SparseSdfCliffShowcase.cpp`, `SparseSdfCaveShowcase.cpp` — consumers only.
- `Tests/ProceduralAuthoring/SparseSdfGeologyTests.cpp` — focused native
  recipe/evaluator/fracture/erosion/re-distance/topology tests.
- `Tests/ProceduralAuthoring/SparseSdfBuilderTests.cpp` — retained RAW A-H
  artifact mode and stage metrics.
- `Tests/Editor/SparseSdfGeologyInspectorTests.cpp` — Qt recipe/transaction/debug
  controls.
- Existing showcase tests/scripts — final semantic, Vulkan and source-free gates.
- `docs/evidence/M8_SPARSE_SDF_CAVE_TUNNEL_SHOWCASE.md` and owned artifacts —
  replace premature/incomplete evidence with retained final truth.

---

### Task 1: Typed recipe, complete controls and deterministic validation

**Files:**

- Create: `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/SparseSdfGeologyRecipe.hpp`
- Create: `Tests/ProceduralAuthoring/SparseSdfGeologyTests.cpp`
- Modify: `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/SparseSdfGeologyGenerator.hpp`
- Modify: `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/NativeSdfGeology.hpp`
- Modify: `Tools/ProceduralAuthoring/Source/NativeSdfGeology.cpp`
- Modify: `Tools/ProceduralAuthoring/CMakeLists.txt`
- Modify: `Tests/ProceduralAuthoring/CMakeLists.txt`
- Modify when the manifest requires it:
  `Tests/TestSchedulingPolicy.cmake` and
  `Tests/RepositoryContract/VerifyTestScheduling.cmake`

**Public contract:**

```cpp
struct SparseSdfGeologyRecipe
{
    std::uint32_t schemaVersion{1U};
    std::uint32_t algorithmVersion{1U};
    Guid id;
    SparseSdfGeometryType geometryType{SparseSdfGeometryType::Rock};
    SdfBaseFieldSettings base;
    SdfSamplingSettings sampling;
    SdfGeologicalModifierStack modifiers;
    std::vector<SdfNegativeSpaceFeature> negativeSpace;
    SdfSurfaceClassificationSettings classification;
    SdfMeshingSettings meshing;
    SdfLodSettings lod;
    SdfCollisionCookSettings cook;
    SdfDebugSettings debug;
    std::uint64_t seed{};
};

Result<void> validateSparseSdfGeologyRecipe(
    const SparseSdfGeologyRecipe&, SdfValidationLevel);
Result<std::uint64_t> hashSparseSdfGeologyRecipe(
    const SparseSdfGeologyRecipe&);
```

The typed groups contain every behavior-affecting field enumerated by the
approved design: base transform/superellipsoid exponent, voxel/bounds/budget,
anisotropy, decorrelated warp, macro, meso, 3D strata/lithology, fracture
families, erosion, contact, negative features, classification, meshing, LOD,
collision/cook and independent debug visibility. Future physical transport is
not represented as a working toggle.

- [ ] **Step 1: Add the recipe RED target**

Register `JericoSparseSdfGeologyTests` as `procedural.sdf_geology`. Add table
tests that clone one valid recipe, change each exposed scalar/enum/vector entry
one at a time and require a different recipe hash. Require explicit errors for
zero/negative spacing, width below resolution, invalid normalized directions,
excess resource budgets, `NegativeSpace=None` with enabled features and publish
wall-thickness violations.

- [ ] **Step 2: Witness RED**

```text
cmake --preset windows-vulkan-editor
cmake --build build/windows-vulkan-editor --config Release --target JericoSparseSdfGeologyTests
```

Expected: compile failure because the typed recipe header/API is absent. Retain
the exact diagnostic in the slice evidence log.

- [ ] **Step 3: Move the public model and implement canonical hashing**

Move the current public enums/structs out of `SparseSdfGeologyGenerator.hpp`;
preserve source compatibility by including the new header. Hash enums, enabled
states, values, vector lengths/order, GUIDs, seeds, schema/algorithm versions
and source revisions in declared order. Reject non-finite values; never hash
padding or private implementation constants.

- [ ] **Step 4: Implement preview and publish validation**

Preview validates finite/legal/resource-safe data. Publish additionally checks:

```text
dominant closed base mass
resolved feature width >= configured voxel requirement
maximum total displacement < local topology guardrail
minimum surviving wall thickness
distance-quality bound and admitted repair mode
valid collision/LOD/cook budgets
```

Return field-specific `EngineError` codes under
`procedural.sdf.geology.recipe.*`; do not silently clamp author intent.

- [ ] **Step 5: Run GREEN and repository scheduling regression**

```text
cmake --build build/windows-vulkan-editor --config Release --target JericoSparseSdfGeologyTests JericoRepositoryContractTests
ctest --test-dir build/windows-vulkan-editor -C Release -R "^(procedural.sdf_geology|repository.test_scheduling_contract)$" --output-on-failure
git diff --check
```

Inspect only owned hunks; do not commit.

---

### Task 2: One authoritative 3D base/geology field evaluator

**Files:**

- Create: `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/SdfGeologyFieldEvaluator.hpp`
- Create: `Tools/ProceduralAuthoring/Source/SdfGeologyFieldEvaluator.cpp`
- Modify: `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/NativeSdfGeology.hpp`
- Modify: `Tools/ProceduralAuthoring/Source/NativeSdfGeology.cpp`
- Modify: `Tools/ProceduralAuthoring/Source/SparseSdfGeologyGenerator.cpp`
- Modify: `Tools/ProceduralAuthoring/CMakeLists.txt`
- Modify: `Tests/ProceduralAuthoring/SparseSdfGeologyTests.cpp`

**Evaluation contract:**

```cpp
struct SdfGeologySample
{
    float hardness{1.0F};
    float erodibility{};
    float porosity{};
    float permeability{};
    float fractureResistance{1.0F};
    float sedimentAffinity{};
    std::uint16_t materialId{};
    double strataCoordinateMeters{};
    double fractureProximityMeters{};
    std::uint64_t nearestFractureCellId{};
    std::uint64_t secondFractureCellId{};
};

struct SdfFieldEvaluation
{
    double signedDistanceMeters{}; // negative inside
    SdfGeologySample geology;
    SdfDebugStageValues debug;
};

SdfFieldEvaluation evaluateSdfGeology(
    const NativeSdfGeologyRecipe&, const Vec3d& worldPosition) noexcept;
```

- [ ] **Step 1: Write RED base-field tests**

Require a sphere and ellipsoid to have negative centers, positive exterior and
expected axial zero crossings. Require Rock/Boulder to compile to one closed
solid base; Cliff/Massif to a directional solid; no geometry type may silently
enable negative space. Test arbitrary transform/orientation and all six sample
directions so the implementation cannot reduce to `z = h(x,y)`.

- [ ] **Step 2: Witness RED**

Build and run `procedural.sdf_geology`; expected missing evaluator symbols.

- [ ] **Step 3: Implement base primitives and explicit spaces**

Implement sphere, ellipsoid/superellipsoid, rounded box, capsule and composite
solid fields in local coordinates. Apply anisotropy/asymmetry and domain warp
to coordinates; evaluate displacement bands in the documented coordinate
space. Keep the operation order canonical:

```text
BaseField -> Anisotropy -> DomainWarp -> Macro -> Meso -> Strata
```

Use three independently seeded warp channels. Normalize bounded FBM/ridged
bands with explicit octave/lacunarity/gain settings and world-space amplitudes.

- [ ] **Step 4: Separate 3D strata geometry from lithology**

Compute `s = dot(direction, p) / spacing + phase + warp(p)`. One output may
apply bounded groove/displacement; a separate typed layer lookup populates
hardness/erodibility/porosity/permeability/fracture resistance/sediment affinity
and material ID. Tests sample top, wall, underside and rotated strata.

- [ ] **Step 5: Replace builder-local math with the evaluator**

`SparseSdfBuilder::buildGeology` creates one compiled evaluator and writes only
its final signed distance/ownership/flags into existing bricks. Do not enlarge
every legacy `SdfSample` with author-only channels; surface classification and
debug re-evaluate typed geology at final surface points.

- [ ] **Step 6: Run GREEN**

```text
cmake --build build/windows-vulkan-editor --config Release --target JericoSparseSdfGeologyTests JericoSparseSdfBuilderTests
ctest --test-dir build/windows-vulkan-editor -C Release -R "^(procedural.sdf_geology|procedural.sdf_builder)$" --output-on-failure
git diff --check
```

Do not tune the mesher if the raw field tests fail.

---

### Task 3: RAW A-F closed-rock progression

**Files:**

- Modify: `Tests/ProceduralAuthoring/SparseSdfBuilderTests.cpp`
- Modify: `Tools/ProceduralAuthoring/Source/SparseSdfGeologyGenerator.cpp`
- Modify: `docs/evidence/M8_SPARSE_SDF_CAVE_TUNNEL_SHOWCASE.md`
- Retain generated artifacts under the final owned evidence directory only
  after the results are accepted.

- [ ] **Step 1: Strengthen the existing RAW-stage rejection gate**

For stages A-F (`Base`, `Anisotropy`, `DomainWarp`, `Macro FBM`,
`Meso/Ridged`, `Strata`) require at each stage:

```text
NegativeSpace=None
non-empty real vertices/indices
one dominant closed component
no boundary edges
non-AABB silhouette
bounds hidden does not affect Generated Mesh state
different deterministic field/surface hash from the previous stage
bounded thickness and displacement
```

The test must reject a cave-like hollow shell, exterior recess presented as a
cave and a debug/bounds-only surface.

- [ ] **Step 2: Run the stronger RED against the current provisional field**

```text
cmake --build build/windows-vulkan-editor --config Release --target JericoSparseSdfBuilderTests
build\windows-vulkan-editor\Tests\ProceduralAuthoring\Release\JericoSparseSdfBuilderTests.exe --raw-rock-stages build\windows-vulkan-editor\evidence\raw-rock-stages build\windows-vulkan-editor\evidence\raw-rock-stages\metrics.txt
```

Expected: at least one new semantic/visibility requirement fails until the
typed stage/debug model is connected.

- [ ] **Step 3: Make A-F GREEN one layer at a time**

Fix the first failing field stage only. If a stage produces blob/cave topology,
repair that modifier's math or bound, never the extractor. Retain raw unshaded
iso-surface views for each accepted stage with `Generated Mesh=ON` and
`SDF Bounds=OFF`.

- [ ] **Step 4: Verify determinism and resolution response**

Run every stage twice byte-for-byte, then change voxel size and require changed
counts/fidelity while preserving semantic shape and closedness.

---

### Task 4: Finite 3D fractures, volumetric erosion and re-distance

**Files:**

- Modify: `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/SdfGeologyFieldEvaluator.hpp`
- Modify: `Tools/ProceduralAuthoring/Source/SdfGeologyFieldEvaluator.cpp`
- Create: `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/SparseSdfRedistance.hpp`
- Create: `Tools/ProceduralAuthoring/Source/SparseSdfRedistance.cpp`
- Modify: `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/SparseSdfField.hpp`
- Modify: `Tools/ProceduralAuthoring/Source/SparseSdfField.cpp`
- Modify: `Tools/ProceduralAuthoring/Source/NativeSdfGeology.cpp`
- Modify: `Tests/ProceduralAuthoring/SparseSdfGeologyTests.cpp`
- Modify: `Tests/ProceduralAuthoring/SparseSdfBuilderTests.cpp`

- [ ] **Step 1: Write fracture RED**

Require 3D Voronoi `F1`, `F2`, `F2-F1` plus nearest/second stable cell IDs to
remain deterministic across brick boundaries. Require directional joint
families at arbitrary orientation. A finite-width, finite-depth shallow cutter
must be visible on top/wall/underside samples, preserve the configured minimum
wall and remain topologically closed; `allowThroughCut=false` cannot create a
cave.

- [ ] **Step 2: Replace provisional `applyShallowFractures`**

Implement independent XYZ feature offsets, stable signed cell IDs, spacing,
width, maximum depth, blend, anisotropy, jitter, persistence, region mask,
surface-proximity mask, priority and exposed material. Represent the crack as
an explicit finite volumetric cutter field and combine it with CSG, rather than
adding an unbounded scalar groove.

- [ ] **Step 3: Write erosion RED**

On the same rotated solid, require different wear at equal geometric positions
when hardness/erodibility, fracture proximity, curvature/exposure, gravity and
strata weights differ. Require `phi_new = phi + E(p)` with bounded positive wear
and no `z=h(x,y)` shortcut.

- [ ] **Step 4: Implement deterministic volumetric erosion**

Calculate a normalized wear mask from the explicit geology/fracture inputs,
bounded noise and author mask. Clamp by `maximumWearMeters`, local thickness and
configured output clamp. Deposition/material classification is a separate
output; do not fake a transport solver.

- [ ] **Step 5: Write and implement re-distance RED/GREEN**

Add `SdfDistanceQuality { ExactAnalytic, Implicit, Redistanced }` metadata plus
gradient-error metrics. For a field with correct sign but scaled/bad distance,
require automatic repair only above threshold, preservation of every sampled
sign and a lower admitted `|length(gradient)-1|` error.

Implement a Jerico-native narrow-band fast sweep:

```text
deduplicate global brick-border samples
seed distance at sign-changing 6-neighbour edges by interpolation
perform configured directional Eikonal sweeps
restore original sign and clamp to narrow band
write identical values back to duplicate brick borders
recompute quality metrics/content hash
```

- [ ] **Step 6: Generate RAW G-H**

Extend the artifact mode with `ShallowFractures` and `Erosion`. Both must remain
closed rocks with no negative-space features and have distinct deterministic
hashes. Retain fracture proximity/cell-ID and wear debug captures separately
from shaded output.

- [ ] **Step 7: Run GREEN**

```text
cmake --build build/windows-vulkan-editor --config Release --target JericoSparseSdfGeologyTests JericoSparseSdfBuilderTests JericoSparseSdfMesherTests
ctest --test-dir build/windows-vulkan-editor -C Release -R "^(procedural.sdf_geology|procedural.sdf_builder|procedural.sdf_mesher)$" --output-on-failure
git diff --check
```

---

### Task 5: Explicit spline/chamber and exact convex BSP negative space

**Files:**

- Modify: `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/SparseSdfGeologyRecipe.hpp`
- Modify: `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/NativeSdfGeology.hpp`
- Modify: `Tools/ProceduralAuthoring/Source/SdfGeologyFieldEvaluator.cpp`
- Modify: `Tools/ProceduralAuthoring/Source/SparseSdfGeologyGenerator.cpp`
- Modify: `Tests/ProceduralAuthoring/SparseSdfGeologyTests.cpp`
- Modify: `Tests/ProceduralAuthoring/SparseSdfBuilderTests.cpp`

**Spline/BSP contract:**

```cpp
struct SdfTunnelControlPoint
{
    Vec3d position;
    Vec3d radiiMeters{1.0, 1.0, 1.0};
    double blendMeters{0.2};
    double localWarpAmplitudeMeters{};
};

struct SdfConvexBrushField
{
    Guid sourceBrush;
    std::uint64_t sourceRevision{};
    std::vector<SdfPlane> worldPlanes;
};
```

- [ ] **Step 1: Write the semantic RED**

Require three recipes from the same generator:

```text
Rock + NegativeSpace=None        -> independent closed rock
Cliff + NegativeSpace=None       -> solid directional massif
RockMass + Tunnel/Chamber        -> host remains solid around an explicit void
```

For the cave require simultaneously inspectable entrance, continuous curved
tunnel, chamber, natural ceiling and branch/curve/vertical change. Reject an
exterior recess, disconnected arch, debug-only path or circular straight tube.

- [ ] **Step 2: Implement spline tunnel evaluation**

Compile each path into deterministic variable-radius capsule segments with
smooth union at control points. Support elliptical cross-section, radius
variation, vertical change, branches and per-path bounded warp. Chambers remain
explicit ellipsoid/superellipsoid fields. Subtract the complete negative stack
from the already evaluated host mass.

- [ ] **Step 3: Implement convex BSP brush -> implicit opening**

Replace the current axis-aligned AABB-only opening. Transform every box/convex
brush face into an outward normalized world plane and evaluate:

```text
phi_brush(p) = max_i(dot(n_i, p) - h_i)
```

with the configured rounding/smooth subtraction. Require a rotated box and a
non-box convex brush to preserve their opening cross-sections. A BSP placed at
an entrance or inside the host becomes an opening/tunnel segment, not a circular
placeholder and not its AABB.

- [ ] **Step 4: Prove minimum wall and connectivity**

Sample the final field along the spline, around chamber walls and on the host
exterior. Require positive void clearance, negative surviving host walls,
connected entrance-to-branch route and closed generated host surface.

- [ ] **Step 5: Run GREEN**

Run `procedural.sdf_geology`, `procedural.sdf_builder` and existing BSP/SDF
regressions. Do not rerun or alter published showcase evidence yet.

---

### Task 6: Surface classification, topology controls, normals/tangents and derivatives

**Files:**

- Modify: `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/SparseSdfMesher.hpp`
- Modify: `Tools/ProceduralAuthoring/Source/SparseSdfMesher.cpp`
- Create: `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/SparseSdfTopology.hpp`
- Create: `Tools/ProceduralAuthoring/Source/SparseSdfTopology.cpp`
- Modify: `Tools/ProceduralAuthoring/Source/SparseSdfGeologyGenerator.cpp`
- Modify: `Tools/ProceduralAuthoring/Source/SdfCook.cpp` only if the existing
  derivative API cannot carry the new accepted result without duplication.
- Modify: `Tests/ProceduralAuthoring/SparseSdfGeologyTests.cpp`
- Modify: `Tests/ProceduralAuthoring/SparseSdfMesherTests.cpp`

- [ ] **Step 1: Write meshing/topology RED**

Require meaningful `isoValue`, minimum/maximum edge length, approximation error,
iteration budget, feature preservation, projection and triangle budget. A
smooth area must reduce density relative to a high-curvature/fracture/cavity
edge while closedness, silhouette and feature error stay within controls.

- [ ] **Step 2: Add explicit extraction settings without changing field shape**

`meshSparseSdf(field, settings, cancellation)` extracts the selected iso value
and reports generated counts/timing/edge/error metrics. Preserve the existing
overload as the schema-1 default. The earlier exact-zero tie handling remains a
standalone correctness rule, not a geology control.

- [ ] **Step 3: Implement the smallest deterministic Jerico topology pass**

Weld extractor duplicates, identify feature vertices from field-gradient/
normal/curvature discontinuity and perform bounded deterministic low-curvature
edge operations only when manifold/closedness and approximation checks pass.
Project accepted vertices back to the source iso-surface with bounded gradient
steps. Fail with metrics when a requested budget cannot be met; never damage a
cave opening to force a ratio.

- [ ] **Step 4: Classify after final geometry**

Use final gradient normal, slope, height, curvature, 3D strata coordinate,
fracture proximity and interior/exterior exposure to assign surface zones and
material IDs. Geometry must remain byte-identical when only material mappings
change.

- [ ] **Step 5: Verify normals, tangents, collision and LOD**

Require finite unit gradient normals at top/wall/underside/tunnel ceiling,
orthogonal finite tangents after topology, collision vertices/indices derived
from the accepted surface (never bounds), and a larger-voxel/controlled LOD that
preserves entrance, ceiling, branch and route within measured error.

- [ ] **Step 6: Run GREEN**

```text
cmake --build build/windows-vulkan-editor --config Release --target JericoSparseSdfGeologyTests JericoSparseSdfMesherTests JericoSdfCookTests
ctest --test-dir build/windows-vulkan-editor -C Release -R "^(procedural.sdf_geology|procedural.sdf_mesher|procedural.sdf_cook)$" --output-on-failure
git diff --check
```

---

### Task 7: Draft session and Basic/Advanced JericoEditor controls

**Files:**

- Create: `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/SparseSdfGeologySession.hpp`
- Create: `Tools/ProceduralAuthoring/Source/SparseSdfGeologySession.cpp`
- Create: `Editor/JericoEditor/Source/SparseSdfGeologyInspector.hpp`
- Create: `Editor/JericoEditor/Source/SparseSdfGeologyInspector.cpp`
- Create: `Tests/Editor/SparseSdfGeologyInspectorTests.cpp`
- Modify: `Editor/JericoEditor/Source/JericoBspWorkbench.hpp`
- Modify: `Editor/JericoEditor/Source/JericoBspWorkbench.cpp`
- Modify: `Editor/JericoEditor/CMakeLists.txt`
- Modify: `Tests/Editor/CMakeLists.txt`

- [ ] **Step 1: Write session RED**

Require immutable accepted recipe/result, draft diff, stale-preview rejection,
cancellation without state change, Apply, bounded undo/redo, deterministic
revision/hash and cook only from the exact accepted generation. Mirror
`SparseSdfSession` semantics without changing legacy `SdfRegionRecipe` v1.

- [ ] **Step 2: Implement the shared geology session**

Use `SparseSdfGeologyGenerator` for preview and keep `State { recipe,
generation, recipeHash, resultHash }`. The session owns no Qt and mutates only
after validated Apply.

- [ ] **Step 3: Write inspector RED**

In offscreen Qt, require Basic/Advanced to edit the same recipe, every
behavior-affecting field to have one widget/object name, presets to populate
ordinary visible values, invalid edits to preserve last valid preview, and
Preview/Cancel/Apply/Undo/Redo callbacks to carry exact hashes.

Require independent debug toggles:

```text
Bounds, Base Field, Warp, Macro, Meso, Strata/Material,
Fracture Proximity/Cell ID, Erosion, Negative Space,
Final Sign/Gradient Error, Generated Mesh, Classification, Collision/LOD
```

Specifically, turning Bounds off must leave Generated Mesh on and visible.

- [ ] **Step 4: Implement the inspector and host it in `JericoBspWorkbench`**

Add a `Region Sculpt / Geology Generator` choice inside the existing Sparse SDF
mode. The geology page edits `SparseSdfGeologyRecipe`, uses the existing
asynchronous/cancellable preview pattern and publishes its `rawSurface` into the
existing viewport display callback. Reuse selected BSP brushes as typed opening
features. Do not modify `EditorMainWindow.cpp` for this integration.

- [ ] **Step 5: Run GREEN**

```text
cmake --build build/windows-vulkan-editor --config Release --target JericoSparseSdfGeologyInspectorTests JericoEditor JericoRepositoryContractTests
ctest --test-dir build/windows-vulkan-editor -C Release -R "^(editor.sparse_sdf_geology_inspector|repository.test_scheduling_contract)$" --output-on-failure
git diff --check
```

---

### Task 8: Convert showcases to consumers and pass the real cave gate

**Files:**

- Modify only shared-layer consumer hunks in:
  `Editor/JericoEditor/Source/SparseSdfRockFamily.cpp`,
  `SparseSdfCliffShowcase.cpp`, `SparseSdfCaveShowcase.cpp`
- Modify: `Tests/Editor/SparseSdfCaveShowcaseTests.cpp`
- Modify: `Tests/Editor/VerifySparseSdfCaveShowcase.cmake`
- Modify only if the fixture contract requires the accepted products:
  `Tests/Editor/EditorFixtureBuilder.cpp`

- [ ] **Step 1: Assert consumer architecture**

Add source/behavior tests that the rock-family, cliff and cave showcases call
`SparseSdfGeologyGenerator`; no showcase owns independent FBM/strata/fracture/
erosion math. Cave recipe alone selects `CaveHostMass` and explicit negative
features. RBrush cracks/chips stay downstream/local and cannot define the host.

- [ ] **Step 2: Re-run the first cave RED before visual tuning**

The focal gate must reject unless entrance, continuous tunnel, chamber, natural
ceiling and a branch/curve/vertical change are simultaneously inspectable from
the final raw/generated mesh. It also rejects an AABB, exterior recess, arch,
debug-only field or straight circular tube.

- [ ] **Step 3: Replace cave-specific global generation with typed recipe data**

Keep camera/material/evidence composition in the showcase, but move every shape
parameter into the visible recipe. Build the cave as:

```text
RockMass host
  - spline entrance/main tunnel
  - chamber(s)
  - branch / vertical-change path
  - optional exact BSP opening
  + local bounded geology/RBrush detail
```

Tune fields, not mesher subdivision, until RAW acceptance passes.

- [ ] **Step 4: Run non-Vulkan focal regressions**

```text
cmake --build build/windows-vulkan-editor --config Release --target JericoSparseSdfRockFamilyTests JericoSparseSdfCliffShowcaseTests JericoSparseSdfCaveShowcaseTests
ctest --test-dir build/windows-vulkan-editor -C Release -R "^editor.sparse_sdf_(rock_family|cliff_showcase|cave_showcase)$" --output-on-failure
```

The published Rock Family/Cliff criteria must stay green, but do not replace
their evidence or reopen their slice status.

---

### Task 9: Vulkan, source-free proof and selective publication

**Files:**

- Replace owned final evidence under:
  `docs/evidence/M8_SPARSE_SDF_CAVE_TUNNEL_SHOWCASE.md`,
  `docs/evidence/data/`, `docs/evidence/images/`
- Modify: `docs/evidence/M8_GEOMETRY_TECHNOLOGY_SHOWCASE_EXIT_AUDIT.md`
- Modify: `docs/CAPABILITY_MAP.md`, `docs/IMPLEMENTATION_ROADMAP.md`
- Modify: `docs/work/ACTIVE_SLICE.md`, `docs/work/HANDOFF.md`
- Preserve all unrelated evidence and conceptual images.

- [ ] **Step 1: Audit Vulkan writer/process state**

Confirm no other JericoEditor, proof executable, compiler/linker or Vulkan test
writer is active. Stop and report if ownership is ambiguous.

- [ ] **Step 2: Generate retained RAW and final evidence**

Retain original-resolution captures for:

```text
A-H RAW rock iso-surfaces, Bounds OFF
final Rock, Bounds OFF
final Cliff/Massif, Bounds OFF
final Cave exterior/entrance and interior route, Bounds OFF
wireframe/topology
gradient normals/tangents
classification zones
fracture/erosion debug
collision derived from mesh
navigation route
LOD1 preservation
```

Record recipe/field/mesh/collision/navigation/cook hashes, vertex/triangle/brick
counts, voxel sizes, edge/error/gradient metrics, generation time and debug
visibility states.

- [ ] **Step 3: Run the real Vulkan and source-free package gates**

```text
ctest --test-dir build/windows-vulkan-editor -C Release -R "^editor.sparse_sdf_cave_showcase_vulkan$" --output-on-failure
```

Require two byte-identical source-free Shipping replays, full Jerico viewport,
real `PBR_Opaque` materials, exact generated triangle collision, navigation and
LOD/cook outputs. A concept image, parser, sparse field, bounds cube or package
metadata alone is not evidence.

- [ ] **Step 4: Run relevant integration and inspect the complete owned diff**

```text
ctest --test-dir build/windows-vulkan-editor -C Release -R "^(procedural.sdf_geology|procedural.sdf_builder|procedural.sdf_mesher|procedural.sdf_cook|editor.sparse_sdf_geology_inspector|editor.sparse_sdf_rock_family|editor.sparse_sdf_cliff_showcase|editor.sparse_sdf_cave_showcase|editor.sparse_sdf_cave_showcase_vulkan|repository.test_scheduling_contract)$" --output-on-failure
git diff --check
git status --short --branch
git diff --stat
```

Classify every dirty path/hunk as owned or unrelated before staging. Preserve
pre-existing staged intent; use a selective index, never broad `git add -A`.

- [ ] **Step 5: Reconcile truthful documentation**

Only after all gates pass, mark the cave/tunnel showcase PASS and update the
13-criterion matrix. Record measured limitations; do not claim physical
sediment transport, VDB, a general graph or future remesher behavior.

- [ ] **Step 6: Commit and push once**

Stage only verified owned files/hunks, inspect `git diff --cached`, and commit:

```text
feat(sdf): publish native geology cave and tunnel showcase
```

Push `main`, then verify:

```text
git fetch origin
git rev-parse HEAD
git rev-parse main
git rev-parse origin/main
git status --short --branch
```

All three revisions must match and unrelated dirty state must remain intact.

- [ ] **Step 7: Transfer exactly once and stop**

After publication only: update the sole `ACTIVE_SLICE` and compact checkpoint,
confirm no auxiliary agent/process is writing, create exactly one fresh
top-level successor with the durable bootstrap, transfer checkout ownership and
stop this task. The Persistent Goal remains active until JericoEngine 1.0.
