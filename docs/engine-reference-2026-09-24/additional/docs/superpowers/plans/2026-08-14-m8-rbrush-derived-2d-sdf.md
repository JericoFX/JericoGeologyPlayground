# M8 RBrush Derived 2D SDF Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Convert an admitted canonical R8 or R16 coverage mask into a bounded, deterministic and valid 2D Euclidean signed-distance representation for RBrush Slice 27.

**Architecture:** Add one authoring-only `MaskSignedDistanceField2D` module to `Jerico::ProceduralAuthoring`. The module consumes immutable canonical coverage bytes, performs threshold/invert and isolated-texel cleanup, then runs a linear-time exact squared Euclidean distance transform with an exterior padding cell. It returns finite signed distances in texel units plus stable source, derivation and result hashes; it does not modify `.rbrush`, persist DDC bytes, invoke Sparse SDF operations or expose UI.

**Tech Stack:** C++20, `Jerico::Core::Result`, `std::span`, `std::stop_token`, existing ProceduralAuthoring CMake/test harness.

## Global Constraints

- `distance < 0` means inside, `distance = 0` means surface and `distance > 0` means outside.
- RBrush stores canonical mask bytes; the signed-distance representation is disposable derived data.
- Supported canonical inputs are R8 UNORM and little-endian R16 UNORM only.
- Width and height are each limited to `4,096`; total texels are limited to `16,777,216`.
- Threshold is finite and within `[0, 1]`; a sample is inside when its integer UNORM value is greater than or equal to `ceil(threshold * maximumCode)` before optional inversion.
- Cleanup is deterministic and limited to eight passes. A pass flips only a texel with no in-bounds neighbour of the same binary state, preserving connected one-texel cracks.
- The transform is linear in texel count, cooperatively cancelable and does not spawn work or publish state.
- Distances are quantized to `1/4096` texel before storage and hashing so identical input produces byte-identical derived data.
- Slice 27 does not implement DDC persistence, SDF stamping, projection, Boolean operations, geometry, UI, cook, runtime or Shipping behavior.

---

### Task 1: Canonical mask-to-SDF derivation

**Files:**
- Create: `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/MaskSignedDistanceField2D.hpp`
- Create: `Tools/ProceduralAuthoring/Source/MaskSignedDistanceField2D.cpp`
- Modify: `Tools/ProceduralAuthoring/CMakeLists.txt`
- Create: `Tests/ProceduralAuthoring/MaskSignedDistanceField2DTests.cpp`
- Modify: `Tests/ProceduralAuthoring/CMakeLists.txt`

**Interfaces:**
- Consumes: immutable R8/R16 canonical mask bytes, dimensions, `MaskSdf2DSettings` and optional `std::stop_token`.
- Produces: `Result<MaskSignedDistanceField2D> deriveMaskSignedDistanceField2D(const MaskSdf2DRequest&, std::stop_token = {})`.
- Produces: width, height, `std::vector<float> signedDistanceTexels`, `insideTexels`, `sourceHash`, `derivationKey` and `contentHash`.

- [ ] **Step 1: Write the failing product test**

Create a focused executable whose first case includes the missing product header and requires a 7x7 R8 mask containing a 3x3 solid island to produce:

```cpp
TEST_CHECK_EQ(field.width, 7U);
TEST_CHECK_EQ(field.height, 7U);
TEST_CHECK_EQ(field.insideTexels, 9U);
TEST_CHECK(std::abs(at(field, 3U, 3U) + 1.5F) <= 1.0F / 4096.0F);
TEST_CHECK(std::abs(at(field, 2U, 3U) + 0.5F) <= 1.0F / 4096.0F);
TEST_CHECK(std::abs(at(field, 1U, 3U) - 0.5F) <= 1.0F / 4096.0F);
TEST_CHECK(std::abs(
    0.5F * (at(field, 1U, 3U) + at(field, 2U, 3U)))
    <= 1.0F / 4096.0F);
```

The same test must cover repeatability, input immutability, finite all-inside/all-outside fields, R8/R16 field equivalence, threshold, inversion, isolated speck/hole cleanup, changed-input invalidation, exact error codes and pre-cancelled execution.

- [ ] **Step 2: Register and run the RED**

Add `JericoMaskSignedDistanceField2DTests` linked only to `Jerico::ProceduralAuthoring`, register it as `procedural.rbrush_derived_sdf2d`, configure the existing `windows-vulkan-editor` preset and build the target.

Run:

```powershell
cmake --preset windows-vulkan-editor
cmake --build --preset windows-vulkan-editor --target JericoMaskSignedDistanceField2DTests --parallel 4
```

Expected RED: compilation fails because `Jerico/ProceduralAuthoring/MaskSignedDistanceField2D.hpp` does not exist.

- [ ] **Step 3: Define the bounded public contract**

Add these public concepts without an AssetPipeline or Qt dependency:

```cpp
enum class MaskSdf2DPixelFormat : std::uint8_t
{
    R8Unorm = 1U,
    R16Unorm = 2U
};

struct MaskSdf2DSettings
{
    float threshold{0.5F};
    bool invert{};
    std::uint8_t isolatedTexelCleanupPasses{};
};

struct MaskSdf2DRequest
{
    std::uint32_t width{};
    std::uint32_t height{};
    MaskSdf2DPixelFormat format{MaskSdf2DPixelFormat::R8Unorm};
    std::span<const std::byte> coverage;
    MaskSdf2DSettings settings;
};

struct MaskSignedDistanceField2D
{
    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<float> signedDistanceTexels;
    std::uint64_t insideTexels{};
    std::uint64_t sourceHash{};
    std::uint64_t derivationKey{};
    std::uint64_t contentHash{};
};
```

Reject invalid dimensions/byte counts/settings with `procedural.sdf.mask2d_invalid`, resource-limit violations with `procedural.sdf.mask2d_budget` and cancellation with `procedural.sdf.mask2d_cancelled`; all diagnostics use stage `procedural.sdf.mask2d`.

- [ ] **Step 4: Implement threshold, inversion and cleanup**

Decode R16 explicitly as little-endian. Build one private binary mask, then run at most eight snapshot-based cleanup passes. For each texel, count only in-bounds eight-connected neighbours; flip the texel only when none share its state. Check cancellation at least once per processed row.

- [ ] **Step 5: Implement the exact linear-time transform**

Use the Felzenszwalb/Huttenlocher separable squared Euclidean transform over a `(width + 2) x (height + 2)` grid. The one-cell exterior border is outside. Reuse one full `int32_t` squared-distance buffer for the inside and outside transforms plus one-dimensional scratch arrays. Convert squared distance to signed linear distance with half-texel boundary correction:

```cpp
const double linear = std::sqrt(static_cast<double>(squared)) - 0.5;
const double signedValue = inside ? -linear : linear;
const float quantized = static_cast<float>(
    std::round(signedValue * 4096.0) / 4096.0);
```

An empty mask uses finite positive `hypot(width, height) + 0.5`; a full mask obtains finite negative distance from the padded exterior.

- [ ] **Step 6: Add deterministic provenance hashes**

Use a domain-separated canonical FNV-1a stream with explicit little-endian integers and float bit patterns:

```text
sourceHash    = hash(schema, width, height, format, canonical coverage bytes)
derivationKey = hash(schema, sourceHash, threshold bits, invert, cleanup passes)
contentHash   = hash(schema, width, height, insideTexels, quantized distance bits)
```

Reject any internally non-finite distance rather than publishing partial output.

- [ ] **Step 7: Run focal GREEN and neighbouring SDF regressions**

Run:

```powershell
cmake --build --preset windows-vulkan-editor --target JericoMaskSignedDistanceField2DTests --parallel 4
ctest --preset windows-vulkan-editor -R "^procedural\.rbrush_derived_sdf2d$" --output-on-failure
ctest --preset windows-vulkan-editor -R "^procedural\.sdf_(field|builder|mesher|transition|session|cook)$" --output-on-failure
```

Expected: the focal test and all selected neighbours pass without changing Sparse SDF product code.

- [ ] **Step 8: Verify architecture and source-free boundaries**

Run the existing scheduling/runtime-only/Shipping contract patterns selected by CTest inventory, then run:

```powershell
git diff --check
```

Expected: no Qt/Editor/runtime dependency is introduced, no `.rbrush` schema changes, no Shipping binary gains RBrush authoring symbols, and the diff check passes.

- [ ] **Step 9: Retain evidence and publish the checkpoint**

Record exact mask dimensions, sign samples, zero-crossing interpolation, hashes, cancellation/error results, focal/regression commands and non-claims in `docs/evidence/M8_RBRUSH_DERIVED_2D_SDF.md`. Update `docs/CAPABILITY_MAP.md` and `docs/superpowers/THREAD_HANDOFF.md` only after the full Slice 27 criterion is proven. Commit owned product/tests first, then evidence/continuity, and push `main` after confirming preserved user changes remain unstaged.
