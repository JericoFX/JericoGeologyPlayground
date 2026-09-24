# M8 DirectedCrack Swept-Volume Implementation Plan

> **For the active Sol owner:** REQUIRED SUB-SKILL: use
> `superpowers:executing-plans` to execute this plan serially. Jerico authority
> forbids subagent writers, branches and worktrees; Luna may only perform
> READ_ONLY inspection or CMake/build/test/log/review assistance.

**Goal:** Add one bounded artist-directed crack graph that maps canonical Jerico
splines through an implicit surface chart, subtracts a true profiled swept volume
from the resident negative-inside field and proves the existing classified
Surface Nets/cook/collision/material/runtime vertical.

**Architecture:** Advance the cumulative typed geology recipe to schema 17 and
add one `DirectedCrack` payload referencing the existing canonical spline
inventory. Prepare a bounded deterministic 2D branch graph, evaluate one
Jerico-owned implicit swept-union formula in double and Vulkan `float`, and reuse
the resident ping-pong field, masks, dirty tracking, classification, Surface
Nets and downstream product path.

**Tech Stack:** C++20, existing Jerico `Result`/GUID/cancellation/spline/geology
contracts, Diligent compute on Vulkan, HLSL, configured CMake/MSBuild targets,
classified Surface Nets, JRMS 1.1 `GEOL`, PackageTool and AssetDump.

**Spec:**
`docs/superpowers/specs/2026-08-28-m8-directed-crack-swept-volume-design.md`

## Global Constraints

- Execute only `M8-GEOLOGY-BLUEPRINT-P1-DIRECTED-CRACK-SWEPT-VOLUME`.
- Sol is the sole checkout writer, architect, integrator and Git authority.
- Do not create a branch, worktree, fork, coordinator, alternate checkout or
  parallel owner.
- Preserve the 182-entry foreign inventory with SHA-256
  `2e996b1669df5ad1dd4e0cb9c3cbaf1b8bfc73ba6bc734abbad0a49419954030`;
  stage only owned paths/hunks.
- Use the existing `ProceduralSpline`, recipe, negative-inside field, masks,
  resident Vulkan ping-pong evaluator, Surface Nets, JRMS/collision and PBR
  paths. Do not create parallel systems.
- Hard bounds are 32 branches, 128 prepared segments, 8 profile keys, positive
  uniform scale and a checked voxel-segment distance-test budget.
- No copied reference implementation, named generator/preset, bespoke proof
  mesh, durability, rockfall, colluvium, imported structural mesh, later P1/P2,
  OpenVDB/UI restoration, M9, Task23 or production-expansion work.
- Every production change follows a witnessed RED before GREEN.
- A compile or synthetic field test is not final acceptance; real Vulkan,
  classified cook/collision, source-free package and original-resolution Beauty
  are required.

---

## File Structure

- Create
  `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/Geology/GeologyDirectedCrack.hpp`:
  bounded preparation helpers and the independent double-precision reference.
  Keep it header-only because the authoritative configured build cannot safely
  regenerate global CMake while preserved Qt targets are missing.
- Modify `GeologyTypes.hpp`, `GeologyLayer.hpp`, `GeologyRecipe.hpp` and
  `GeologyRecipe.cpp`: schema 17, kind/payload, validation, canonical text,
  parsing and hashing.
- Modify `GeologyGpuTopologyPlan.cpp`: zero occupancy expansion and physical
  band admission through existing topology validation.
- Modify `GeologyGpuSessionDiligent.cpp` and `GeologyGpuShaders.hpp`: bounded
  prepared buffers and one layer kind in the resident evaluator.
- Modify `GeologyRecipeTests.cpp`, `GeologyGpuTopologyPlanTests.cpp` and
  `GeologyGpuSessionTests.cpp`: CPU/schema/preparation/topology/Vulkan/product
  contracts.
- Modify only the owned registration hunk in
  `Tests/ProceduralAuthoring/CMakeLists.txt`: focused Vulkan CTest registration.
- Add closure evidence and update the existing system audit, active slice and
  compact checkpoint only after the productive gates pass.

---

### Task 1: Freeze the independent scalar contract with an authentic RED

**Files:**

- Create:
  `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/Geology/GeologyDirectedCrack.hpp`
- Modify:
  `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/Geology/GeologyTypes.hpp`
- Modify:
  `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/Geology/GeologyLayer.hpp`
- Test: `Tests/ProceduralAuthoring/GeologyRecipeTests.cpp`

**Interfaces:**

- Produces:
  `GeologyDirectedCrackProfileKey`, `GeologyDirectedCrackBranch`,
  `GeologyDirectedCrackPayload`, `GeologyDirectedCrackPreparedSegment`, and
  `evaluateGeologyDirectedCrackReference(...)`.
- The reference consumes a literal source distance/gradient/world point,
  transform, prepared segments/profile, outset, strength and mask; it returns
  signed distance plus selected segment/profile diagnostics.

- [ ] **Step 1: Write the failing CPU behavior test**

Add the absent include and one focused witness to `GeologyRecipeTests.cpp`:

```cpp
void directedCrackReferenceMapsChartAndSubtractsNegativeInside()
{
    const std::array segments{GeologyDirectedCrackPreparedSegment{
        .chartStartMeters = {-0.5, 0.0},
        .chartEndMeters = {0.5, 0.0},
        .startWidthMeters = 0.20,
        .endWidthMeters = 0.20,
        .depthMeters = 0.30}};
    const std::array profile{
        GeologyDirectedCrackProfileKey{0.0, 1.0},
        GeologyDirectedCrackProfileKey{1.0, 0.0}};
    const auto sample = evaluateGeologyDirectedCrackReference(
        -0.10, {0.0, 0.0, 1.0}, {0.0, 0.0, -0.10}, {},
        segments, profile, 0.01, 0.30, 1.0, 1.0);
    TEST_CHECK_NEAR(sample.signedDistanceMeters, 1.0 / 15.0, 1.0e-12);
    TEST_CHECK(sample.carved);
}
```

Add separate literal assertions for: a point outside width, a point deeper than
the branch, zero gradient, zero strength and zero mask. The production mutation
these tests catch is changing chart mapping, half-width, depth interval,
subtraction sign or exact identity.

- [ ] **Step 2: Build and retain RED**

Run:

```powershell
cmake --build build/windows-vulkan-dev --config Debug --target JericoGeologyRecipeTests
```

Expected RED: compiler failure because
`GeologyDirectedCrack.hpp`/`GeologyDirectedCrackPreparedSegment` does not exist.
Record the exact diagnostic in the eventual closure evidence.

- [ ] **Step 3: Add the minimum types and reference**

Add `DirectedCrack = 21U` to `GeologyLayerKind`. Add the payload records with
defaulted equality to `GeologyLayer.hpp`, but do not add schema serialization
yet. Implement in the new header:

```cpp
struct GeologyDirectedCrackPreparedSegment
{
    std::array<double, 2U> chartStartMeters;
    std::array<double, 2U> chartEndMeters;
    double startWidthMeters{};
    double endWidthMeters{};
    double depthMeters{};
    Guid branchGuid;
    Guid segmentStartPointGuid;
    std::uint32_t segmentSlot{};
};

[[nodiscard]] GeologyDirectedCrackReferenceSample
evaluateGeologyDirectedCrackReference(
    double sourceDistanceMeters,
    Vec3d sourceGradient,
    Vec3d worldPosition,
    const GeologyTransform& transform,
    std::span<const GeologyDirectedCrackPreparedSegment> segments,
    std::span<const GeologyDirectedCrackProfileKey> profile,
    double openingOutsetMeters,
    double surfaceBandMeters,
    double strength,
    double maskWeight) noexcept;
```

Use the exact chart/volume/subtraction equations in the spec. Do not add a
generic graph or SDF abstraction. Return exact identity before projection when
`abs(sourceDistanceMeters) > surfaceBandMeters`.

- [ ] **Step 4: Verify GREEN and mutation coverage**

Build and run:

```powershell
cmake --build build/windows-vulkan-dev --config Debug --target JericoGeologyRecipeTests
build\windows-vulkan-dev\Tests\ProceduralAuthoring\Debug\JericoGeologyRecipeTests.exe
```

Expected: build exit 0 and all existing plus DirectedCrack reference checks pass.
Mentally invert `max(source, -volume)` or full-width-to-half-width conversion;
the new literal assertions must fail under either mutation.

- [ ] **Step 5: Commit the reference contract**

Stage only the three production paths and the owned test hunks, verify
`git diff --cached --check`, and commit:

```text
feat(geology): define directed crack field contract
```

---

### Task 2: Admit schema 17, graph identity and canonical validation

**Files:**

- Modify:
  `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/Geology/GeologyRecipe.hpp`
- Modify:
  `Tools/ProceduralAuthoring/Source/Geology/GeologyRecipe.cpp`
- Modify: `Tests/ProceduralAuthoring/GeologyRecipeTests.cpp`

**Interfaces:**

- Consumes the Task 1 payload and `GeologyStackRecipe::splines`.
- Produces `geologyStackRecipeDirectedCrackSchemaVersion = 17U`, canonical
  `DirectedCrack` text, parser output, deterministic recipe hash and stable
  validation errors.

- [ ] **Step 1: Write failing round-trip and validation tests**

Create `directedCrackRecipe()` with two open local-XY splines, one root and one
declared child. Freeze literal canonical text for schema 17, kind name, branch
GUIDs, parent/attachment IDs, depth/probability/propagation, profile keys,
spacing/outset/band, header transform/seed/mask and referenced spline data.

Add hash mutations for every new control. Add one-case-at-a-time invalid tests
for duplicate/cyclic/forward parents, missing spline/attachment, closed or
non-coplanar spline, noncoincident join, non-uniform scale, invalid width/depth,
probability/propagation, profile endpoints/order/count, band/outset/spacing and
GPU-float overflow.

- [ ] **Step 2: Run schema RED**

Run the Debug recipe target/executable from Task 1.

Expected RED: schema 17 is unsupported and `DirectedCrack` has no canonical
writer/parser/validation path.

- [ ] **Step 3: Implement the smallest cumulative schema extension**

Update the admitted-schema checks, `admitted(GeologyLayerKind)`,
`payloadMatchesKind`, `layerKindText`, per-layer validation, writer enum branch,
parser name map and parser branch. Require exactly one root and validate ordered
parent references so acyclicity follows without a separate graph engine.

Canonical payload shape must remain nested under the existing layer:

```text
projection = "ImplicitSurfacePlanarChart"
maximumResampleSpacingMeters = <real>
openingOutsetMeters = <real>
surfaceBandMeters = <real>
[[layers.directedCrack.profile]]
normalizedDepth = <real>
widthScale = <real>
[[layers.directedCrack.branches]]
branchGuid = "..."
splineGuid = "..."
parentBranchGuid = "..."
attachmentPointGuid = "..."
depthMeters = <real>
activationProbability = <real>
propagation = <real>
```

- [ ] **Step 4: Run Debug and Release recipe gates**

```powershell
cmake --build build/windows-vulkan-dev --config Debug --target JericoGeologyRecipeTests
cmake --build build/windows-vulkan-dev --config Release --target JericoGeologyRecipeTests
build\windows-vulkan-dev\Tests\ProceduralAuthoring\Debug\JericoGeologyRecipeTests.exe
build\windows-vulkan-dev\Tests\ProceduralAuthoring\Release\JericoGeologyRecipeTests.exe
```

Expected: all variants pass with exact canonical round trip and stable errors.

- [ ] **Step 5: Commit schema 17**

Inspect only owned hunks and commit:

```text
feat(geology): admit directed crack recipe schema
```

---

### Task 3: Prepare the bounded branch graph and freeze topology admission

**Files:**

- Modify:
  `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/Geology/GeologyDirectedCrack.hpp`
- Modify:
  `Tools/ProceduralAuthoring/Source/Geology/Backends/GpuDiligent/GeologyGpuTopologyPlan.cpp`
- Test: `Tests/ProceduralAuthoring/GeologyRecipeTests.cpp`
- Test: `Tests/ProceduralAuthoring/GeologyGpuTopologyPlanTests.cpp`

**Interfaces:**

- Produces:

```cpp
struct GeologyDirectedCrackPreparedGraph
{
    std::uint64_t identityHash{};
    std::vector<GeologyDirectedCrackPreparedSegment> segments;
};

[[nodiscard]] Result<GeologyDirectedCrackPreparedGraph>
prepareGeologyDirectedCrackGraph(
    const GeologyStackRecipe& recipe,
    const GeologyLayer& layer,
    const CancellationToken& cancellation);
```

- [ ] **Step 1: Write preparation RED tests**

Require deterministic `SegmentStable` IDs, physical spacing equal to the minimum
of authored/voxel/width/depth limits, exact propagation clipping, deterministic
seeded child activation, parent suppression, endpoint width interpolation and
repeat hash identity. Add failure witnesses for >128 segments, degeneracy,
undeclared crossing, T-junction, collinear overlap and cancellation.

- [ ] **Step 2: Write topology RED tests**

Require `DirectedCrack` to add exactly zero occupancy expansion when enabled,
disabled or zero strength; require invalid depth/outset beyond the finite field
band to fail before any device allocation. The mutation caught is treating a
subtractive carve like occupancy-adding displacement.

- [ ] **Step 3: Run RED**

Build/run `JericoGeologyRecipeTests` and
`JericoGeologyGpuTopologyPlanTests` Debug. Expected failures: preparation API
absent and topology planner does not recognize kind 21.

- [ ] **Step 4: Implement bounded preparation**

Find referenced splines without copying them, compute effective spacing, call
`sampleProceduralSpline(..., SplineSamplingPhase::SegmentStable, ...)`, clip by
cumulative propagation and transform into chart-local prepared segments. Fold
seed and GUID bytes through a Jerico-owned deterministic hash. Check cancellation
between branches and after sampling.

Use robust orientation tests in double for 2D crossings. Allow only adjacent
same-spline endpoints and the exact declared parent/child attachment. Return
stable errors before partial output.

- [ ] **Step 5: Implement zero-expansion topology handling**

Add `GeologyDirectedCrackPayload` to `layerExpansion` with result `0.0`. Keep the
existing source four-voxel exterior band and validate outset `<= voxelSize` plus
depth/band representability through the canonical recipe validator. Do not add a
second bounds planner.

- [ ] **Step 6: Verify GREEN in Debug and Release**

Build/run both test targets in both configurations. Expected: exact repeat
identity, cancellation/failure cases and zero expansion pass.

- [ ] **Step 7: Commit preparation and topology**

```text
feat(geology): prepare bounded directed crack graphs
```

---

### Task 4: Integrate the resident Vulkan evaluator with explicit work guards

**Files:**

- Modify:
  `Tools/ProceduralAuthoring/Source/Geology/Backends/GpuDiligent/GeologyGpuSessionDiligent.cpp`
- Modify:
  `Tools/ProceduralAuthoring/Source/Geology/Backends/GpuDiligent/GeologyGpuShaders.hpp`
- Modify: `Tests/ProceduralAuthoring/GeologyGpuSessionTests.cpp`
- Modify owned hunk only: `Tests/ProceduralAuthoring/CMakeLists.txt`

**Interfaces:**

- GPU segment record: two `float4` values
  `(startX,startY,endX,endY)` and
  `(startWidth,endWidth,depth,padding)`.
- GPU profile record: one `float4`
  `(normalizedDepth,widthScale,0,0)`.
- Layer constants store transform translation/quaternion/inverse uniform scale,
  outset/band, segment count and profile count; existing identity stores kind,
  folded seed and mask flags.

- [ ] **Step 1: Write the productive Vulkan RED**

Add `directedCrackLayerRecipe(document)` using the accepted existing geological
source/product. Add `evaluatesDirectedCrackOnProductiveVulkan()` and CLI route
`--directed-crack`. Require field inspections for source and DirectedCrack,
literal CPU reference parity samples, repeat hash identity, source/classification
identity, measured width/depth, profile/branch/seed/propagation changes, exact
disabled/zero-mask identity, localized dirty voxels and cancellation.

Register `procedural.geology_gpu_directed_crack` in only the owned CMake hunk and
include it in the existing Vulkan label/resource lock/timeout list.

- [ ] **Step 2: Run Vulkan RED**

Build the existing configured Debug target and run:

```powershell
build\windows-vulkan-dev\Tests\ProceduralAuthoring\Debug\JericoGeologyGpuSessionTests.exe --directed-crack
```

Expected RED: stable `procedural.geology.gpu_layer_unsupported` for kind 21.

- [ ] **Step 3: Add bounded CPU-to-GPU packing**

Prepare the graph before resource allocation. Validate all packed floats, checked
segment/profile byte arithmetic and:

```cpp
constexpr std::uint64_t maximumDirectedCrackDistanceTests = 268'435'456ULL;
checkedMultiply(activeVoxelCount, segmentCount, distanceTests);
```

Reject overflow/excess with stable
`procedural.geology.gpu_directed_crack_work_exceeded`. Upload one non-empty
default record for other layer kinds so shader bindings remain total. Check
cancellation before preparation, after preparation, after upload and before
dispatch.

- [ ] **Step 4: Add one HLSL kind-21 branch**

Declare the two structured buffers alongside `g_JointPlanes`. Implement the
spec's centered gradient, closest-surface chart, closest 2D segment, profile-key
interpolation, axial interval, implicit union and `max(source,-volume)`.
Preserve classification and common hash writes. Do not add another field or
pipeline.

- [ ] **Step 5: Verify Debug Vulkan GREEN**

Build and run the focused CLI. Expected: exit 0, CPU/GPU maximum deviation at or
below `1.0e-4 m`, repeat hashes exact, disabled field identity exact, non-zero
bounded dirty voxels for one profile edit, and cancellation/work errors stable.

- [ ] **Step 6: Verify Release Vulkan GREEN and focused CTest**

```powershell
cmake --build build/windows-vulkan-dev --config Release --target JericoGeologyGpuSessionTests
build\windows-vulkan-dev\Tests\ProceduralAuthoring\Release\JericoGeologyGpuSessionTests.exe --directed-crack
ctest --test-dir build/windows-vulkan-dev -C Release -R "procedural.geology_gpu_directed_crack" --output-on-failure
```

If the configured tree does not yet know the new CTest registration because
global regeneration remains blocked, record that limitation and use the focused
executable as the authoritative gate; do not edit or absorb the foreign Qt/CMake
defects.

- [ ] **Step 7: Commit resident Vulkan integration**

```text
feat(geology): evaluate directed cracks on Vulkan
```

---

### Task 5: Prove real topology and preserve the classified product lineage

**Files:**

- Modify: `Tests/ProceduralAuthoring/GeologyGpuSessionTests.cpp`
- Modify only if a directly observed compatibility defect requires it:
  `Tests/ProceduralAuthoring/GeologyCookTests.cpp`
- Add evidence under:
  `docs/evidence/images/m8-geology-blueprint-p1-directed-crack/`

**Interfaces:**

- Consumes the existing classified `GeologySurfaceMesh`, cook, collision and PBR
  paths. Produces no new runtime format or material contract.

- [ ] **Step 1: Add topology assertions before changing production**

On one existing accepted implicit geological product, assert literal opening
width/depth bands, at least one topological crack mouth, no disconnected empty
component under exterior flood fill, no unintended disconnected solid component,
and undirected edge incidence two for the remaining closed surface.

- [ ] **Step 2: Run and retain the expected topology RED**

Run `--directed-crack` before tuning the product graph. Expected RED must be one
specific physical/topology assertion, not a compile error or empty frame.

- [ ] **Step 3: Tune only typed fixture data**

Adjust only spline/profile/branch inputs inside the existing test product until
the admitted graph produces a thin visible carve. Do not add a named generator,
preset, dedicated mesh or new material.

- [ ] **Step 4: Prove classification, cook and collision**

Cook the cracked classified product through the existing Geology cook path;
reread JRMS 1.1 `GEOL`; prove positions/indices match exact triangle collision
and the packed material classification remains byte-identical for retained
vertices/samples. Record the missing fresh-surface semantic as
`INTEGRATION REQUIRED` rather than modifying the published material contract.

- [ ] **Step 5: Capture real Vulkan evidence**

Use the existing package-only renderer to capture the input and cracked product
at original resolution. Inspect the images visually for a real open crack,
expected profile taper, retained block form/materials, no fallback and no
disconnected holes. Repeat the accepted cracked capture and require byte-identical
SHA-256.

- [ ] **Step 6: Commit productive topology/evidence inputs**

Commit only stable test/evidence artifacts that are required for the final gate:

```text
test(geology): prove directed crack product topology
```

---

### Task 6: Close the source-free vertical and publish selectively

**Files:**

- Add:
  `docs/evidence/M8_GEOLOGY_BLUEPRINT_P1_DIRECTED_CRACK_CLOSURE.md`
- Modify: `docs/evidence/M8_GEOLOGY_SYSTEM_COMPLETENESS_AUDIT.md`
- Modify: `docs/work/ACTIVE_SLICE.md`
- Modify: `docs/work/HANDOFF.md`
- Add only accepted image/data/package evidence required by executed gates.

**Interfaces:**

- Produces durable hashes, measurements, negative boundaries, completeness
  matrix and one next `ACTIVE_SLICE`. It does not complete P1, the blueprint or
  the Persistent Goal.

- [ ] **Step 1: Run the fresh focused matrix**

Run Debug and Release recipe, topology and `--directed-crack` Vulkan gates from
`build/windows-vulkan-dev`. Run the existing classified cook/reread and exact
collision gate. Build the authoritative PackageTool target and use AssetDump
`--shipping-validate` on the isolated package/product.

- [ ] **Step 2: Replay source-free package-only Vulkan twice**

Require no source recipe, editor payload, fallback/error marker or external
asset lookup. Capture two consecutive accepted Beauty frames and compare exact
SHA-256. Record adapter, field/mesh/package hashes, vertex/triangle counts,
maximum parity deviation, GPU nanoseconds, prepared segment/distance-test counts
and dirty voxels.

- [ ] **Step 3: Re-run the completeness/data-lineage audit**

Mark applicable source/serialization/CPU/GPU/renderer/shader/material/collision/
debug/profiling/testing/failure-handling categories with executed evidence.
Retain explicit `INTEGRATION REQUIRED` entries for fresh crack semantics, Jolt
properties, LOD/streaming, runtime remesh/destruction/durability/save and
navigation/gameplay/Luau/events/particles/audio.

- [ ] **Step 4: Verify the preservation boundary**

Run `git diff --check`. Recompute default-status inventory after excluding owned
paths and require exactly 182 entries with the original SHA-256. Inspect
`git diff --cached --name-status` and `git diff --cached` before every commit.

- [ ] **Step 5: Publish the closure and next single slice**

Advance `ACTIVE_SLICE.md` only to the next applicable blueprint P1 item after
DirectedCrack (expected `durability graph`, reconciled against current authority).
Update the compact `HANDOFF.md`, commit owned closure paths, push `main`, fetch
and verify:

```text
HEAD == main == origin/main == remote main
divergence == 0/0
```

- [ ] **Step 6: Transfer exactly one successor and stop**

Create exactly one new top-level task only after publication. Immediately list
tasks before any retry. Require explicit acceptance of exclusive ownership, the
new single `ACTIVE_SLICE`, foreign-inventory preservation and
`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`. Renounce ownership,
archive predecessor/old chats reversibly without archiving the current owner,
mark this Codex Goal complete and stop without starting the successor slice.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`.
