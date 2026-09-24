# M8 RBrush SDF Stamp Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Apply all ten Slice 28 RBrush operations to a real immutable `SparseSdfField`, producing a validated deterministic field or mask result without implementing Slice 29 projections.

**Architecture:** Add one authoring-only `SdfBrushStamp` deep module to `Jerico::ProceduralAuthoring`. It consumes canonical, already projected sample points with RBrush/derived-mask provenance, evaluates the operation against the immutable source field, remaps region ownership when required and reopens the result through `SparseSdfField::create`. Projection and 2D-mask sampling remain a later producer of the same point contract.

**Tech Stack:** C++20, existing `SparseSdfField`, `Jerico::Core::Result`, `std::span`, `std::stop_token`, CMake/CTest and `TestSupport`.

## Global Constraints

- Implement exactly `Add`, `Subtract`, `SmoothAdd`, `SmoothSubtract`, `SurfaceDisplace`, `CrackCutter`, `ChipCutter`, `Smooth`, `Flatten` and `MaskOnly`.
- Do not implement or imply Surface Tangent, Planar, Triplanar, Spherical, Cylindrical, Radial, Object Local or World projection in this slice.
- Source `SparseSdfField` is immutable; no partial field may escape on error or cancellation.
- Maximum request size is `1,048,576` projected points.
- Input point order is not semantic; sort a private copy by brick/sample and reject duplicate keys.
- Positive-influence `Locked` or `NoGenerate` samples reject the complete request.
- Geometry changes receive `stampRegionGuid`; existing owner GUIDs remain exact after sorted region-table insertion/remap.
- `MaskOnly` preserves field data/hashes and returns sorted positive mask values.
- The module is Tools/authoring-only and does not link Qt, Editor, AssetPipeline, renderer, runtime or Shipping.
- C04 is preserved through cooperative cancellation; no thread or job owner is added here.

---

### Task 1: Sparse field stamp operations

**Files:**
- Create: `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/SdfBrushStamp.hpp`
- Create: `Tools/ProceduralAuthoring/Source/SdfBrushStamp.cpp`
- Modify: `Tools/ProceduralAuthoring/CMakeLists.txt`
- Create: `Tests/ProceduralAuthoring/SdfBrushStampTests.cpp`
- Modify: `Tests/ProceduralAuthoring/CMakeLists.txt`
- Modify: `Tests/TestSchedulingPolicy.cmake`
- Modify: `Tests/RepositoryContract/VerifyTestScheduling.cmake`

**Interfaces:**
- Consumes: `SdfBrushStampRequest` with immutable source field, operation/provenance/settings and projected points.
- Produces: `Result<SdfBrushStampResult> applySdfBrushStamp(const SdfBrushStampRequest&, std::stop_token = {})`.
- Produces: validated derived `SparseSdfField`, optional sorted mask samples, changed count, operation hash and result content hash.

- [x] **Step 1: Write the complete failing test**

Create `JericoSdfBrushStampTests` with a one-brick plane fixture whose regions,
owners, flags, cell size, narrow band and hashes already pass
`SparseSdfField::create`. Require exact behavior for all operations:

```cpp
Add             min(source, stamp)
Subtract        max(source, -stamp)
SmoothAdd       smoothMin(source, stamp, smoothK)
SmoothSubtract  smoothMax(source, -stamp, smoothK)
SurfaceDisplace source - displacement * influence
CrackCutter     max(source, -stamp)
ChipCutter      max(source, -stamp)
Smooth          lerp(source, seven-sample axis-neighbour average, influence)
Flatten         lerp(source, flattenTarget, influence)
MaskOnly        unchanged field plus canonical mask sample
```

The test must also require source immutability, field reopen, distinct operation
hashes, order independence, inserted-region owner remap, half-strength blending,
narrow-band clamp, protected/no-generate failure, invalid/missing/duplicate/
budget/provenance/settings failure and pre-cancellation.

- [x] **Step 2: Register scheduling and observe the causal RED**

Register `procedural.sdf_brush_stamp` without a scheduling label. Update both
scheduling authorities to the exact precomputed inventory:

```text
test_count=287
test_inventory_sha256=c098fbb95ccc321ad4b2bfd6713322c1afb4f7a4fc754f56cd9b800f543480dc
scheduled_count=67
run_serial=21
```

Run:

```powershell
cmake --preset windows-vulkan-editor
cmake --build --preset windows-vulkan-editor --target JericoSdfBrushStampTests --parallel 4
```

Expected RED: `C1083` because
`Jerico/ProceduralAuthoring/SdfBrushStamp.hpp` does not exist.

- [x] **Step 3: Define the public operation and point contract**

Implement the exact enum and structs from
`docs/superpowers/specs/2026-08-14-m8-rbrush-sdf-stamp-design.md`, plus:

```cpp
inline constexpr std::uint64_t sdfBrushStampMaximumPoints = 1'048'576U;

[[nodiscard]] Result<SdfBrushStampResult> applySdfBrushStamp(
    const SdfBrushStampRequest& request,
    std::stop_token stopToken = {});
```

Use stage `procedural.sdf.stamp` and stable error codes:

```text
procedural.sdf.stamp_invalid
procedural.sdf.stamp_budget
procedural.sdf.stamp_protected
procedural.sdf.stamp_cancelled
```

- [x] **Step 4: Validate and canonicalize before mutation**

Require non-zero operation/RBrush/preset identities and non-zero pack/mask
hashes. Geometry operations require a non-zero stamp region. Validate enum,
finite operation settings and every point. Reject sample indices `>=729`,
missing bricks, duplicate keys and positive influence outside `[0,1]`.
Sort a private point copy lexicographically and perform the complete
Locked/NoGenerate preflight before building any result field.

- [x] **Step 5: Evaluate all ten operations from immutable samples**

Use polynomial smooth min/max from the design. For Boolean/smooth candidates,
blend `source -> candidate` by influence. Surface displacement subtracts the
configured displacement times influence. Flatten blends to the point target.
Smooth converts brick/sample index to exact world position, queries center and
available six-axis neighbours through `SparseSdfField::sampleDistance`, then
blends to their average. Clamp every geometry result to
`cellSizeMeters * narrowBandCells`.

For `MaskOnly`, emit only positive-influence samples and leave the copied field,
source hash, recipe hash and content hash exact.

- [x] **Step 6: Preserve ownership and publish a valid field**

Collect changes before editing. If at least one geometry sample changed, insert
`stampRegionGuid` into a sorted region copy, remap every existing non-null owner
index by GUID, apply changed distances and assign the stamp owner. Derive a new
recipe hash and field content hash from the immutable source plus canonical
operation/change streams. Reopen through `SparseSdfField::create`; propagate
failure rather than returning unchecked data.

- [x] **Step 7: Prove deterministic hashes and cancellation**

Domain-separate canonical little-endian FNV-1a streams for operation, field and
result. Hash float bit patterns after narrow-band clamp. Hash sorted point keys,
settings and provenance. Check cancellation before allocation and throughout
validation, field copy/remap, application and hash traversal.

- [x] **Step 8: Run focal GREEN and neighbours**

Run:

```powershell
cmake --build --preset windows-vulkan-editor --target JericoSdfBrushStampTests --parallel 4
ctest --preset windows-vulkan-editor -R "^procedural\.sdf_brush_stamp$" --output-on-failure
ctest --preset windows-vulkan-editor -R "^procedural\.sdf_(field|builder|mesher|transition|session|cook)$" --output-on-failure
```

Expected: `1/1` focal and `6/6` neighbouring SDF tests pass.

- [x] **Step 9: Run Release, integrated build and boundaries**

Run:

```powershell
cmake --build --preset windows-vulkan-editor --config Release --target JericoSdfBrushStampTests --parallel 4
ctest --test-dir build/windows-vulkan-editor -C Release -R "^procedural\.sdf_brush_stamp$" --output-on-failure
cmake --build --preset windows-vulkan-editor --parallel 4
ctest --preset windows-vulkan-editor -R "(runtime_only|shipping|source_exclusion|procedural_authority|scheduling)" --output-on-failure
git diff --check
```

Expected: Release focal, integrated Debug build, the 12-test boundary matrix and
diff check pass; runtime-only/Shipping do not acquire stamp/RBrush authoring
symbols.

- [x] **Step 10: Retain and publish Slice 28 evidence**

Create `docs/evidence/M8_RBRUSH_SDF_STAMP.md` with exact per-operation input and
output distances, hashes, owner remap, RED, all commands/results and explicit
non-claims. Update `docs/CAPABILITY_MAP.md` and
`docs/superpowers/THREAD_HANDOFF.md` only after all ten operations and gates are
green. Commit product/tests separately from evidence/continuity, preserve the
user's mixed CMake hunks unstaged, push `main`, and confirm
`HEAD == origin/main`.
