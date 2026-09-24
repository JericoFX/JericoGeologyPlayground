# M8 RBrush Rock Generator Brush Sets Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task in the current authoritative task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Resolve canonical RBrush tag queries and explicit weighted overrides into deterministic crack/chip brush choices for the future Rock Generator.

**Architecture:** Extend the existing in-memory RBrush project catalog with its canonical SDF defaults, then add one Qt-free editor-authority resolver that delegates tag semantics to `queryProjectRBrushLibrary`. The resolver normalizes candidates into stable GUID order, applies explicit-over-query precedence, and emits fixed-domain deterministic choices using the existing SDF operation vocabulary.

**Tech Stack:** C++20, Jerico `Result`/`Guid`, RBrush canonical IR and project catalog, JericoEditor project authority, Sparse SDF brush operation types, CMake/CTest.

## Global Constraints

- Work only in `F:\JericoEngine` on the current authoritative `main`; do not create a task, fork, branch, worktree or alternate checkout.
- Preserve all existing tracked and untracked user files and stage only Slice32 paths.
- Reuse `queryProjectRBrushLibrary`; do not create a scanner, catalog or second tag matcher.
- RBrush remains editor-only authoring input. Runtime and Shipping receive only later cooked static results.
- Do not add Rock Generator geometry, distribution, UI, Procedural 3D Layout, cook/package or a new dependency.
- Resolve in stable `(AssetGUID, PresetGUID)` order and use Jerico-owned integer sampling rather than standard-library distributions.
- Compilation alone is not PASS; exact deterministic choices, negative behavior and relevant regressions are required.

---

## File map

- Modify `Editor/JericoEditor/Include/Jerico/Editor/RBrushProjectCatalog.hpp`: project the canonical SDF defaults already stored in every preset IR.
- Modify `Editor/JericoEditor/Source/RBrushProjectCatalog.cpp`: copy SDF defaults into the existing admitted catalog row.
- Modify `Editor/JericoEditor/Source/RBrushLibrary.cpp`: validate the projected SDF metadata together with the existing catalog snapshot.
- Modify `Tests/Editor/RBrushProjectCatalogTests.cpp`: prove the projection survives discovery/summarization.
- Create `Editor/JericoEditor/Include/Jerico/Editor/RBrushRockGeneratorBrushSet.hpp`: public set/query/range/request/result contract.
- Create `Editor/JericoEditor/Source/RBrushRockGeneratorBrushSet.cpp`: validation, canonical query delegation, explicit precedence, stable fixed-point weighting, deterministic sampling and recipe hashing.
- Modify `Editor/JericoEditor/CMakeLists.txt`: compile the resolver into `JericoEditorProjectAuthority`.
- Create `Tests/Editor/RBrushRockGeneratorBrushSetTests.cpp`: focal product and rejection proof.
- Modify `Tests/Editor/CMakeLists.txt`: build/register the focal test.
- Modify `Tests/TestSchedulingPolicy.cmake` and `Tests/RepositoryContract/VerifyTestScheduling.cmake`: admit exactly the new test count/hash after the authentic scheduling RED.
- Create `docs/evidence/M8_RBRUSH_ROCK_GENERATOR_BRUSH_SETS.md`: retain exact recipes, hashes, commands and nonclaims.
- Modify `docs/superpowers/THREAD_HANDOFF.md`: publish the Slice32 checkpoint only after all verification is fresh.

---

### Task 1: Create the authentic brush-set RED

**Files:**

- Create: `Tests/Editor/RBrushRockGeneratorBrushSetTests.cpp`
- Modify: `Tests/Editor/CMakeLists.txt`
- Modify after the scheduling failure: `Tests/TestSchedulingPolicy.cmake`
- Modify after the scheduling failure: `Tests/RepositoryContract/VerifyTestScheduling.cmake`

**Interfaces:**

- Consumes: `ProjectRBrushAsset`, `RBrushPresetReference`, canonical tag metadata and `SdfBrushOperation`.
- Produces: the public API below, intentionally absent at RED time.

```cpp
enum class RBrushRockFeature : std::uint8_t { Crack = 1, Chip = 2 };

struct RBrushTagQuery
{
    std::vector<std::string> require;
    std::vector<std::string> any;
    std::vector<std::string> exclude;
};

struct RBrushFloatRange
{
    float minimum{};
    float maximum{};
};

struct WeightedRBrushReference
{
    RBrushPresetReference brush;
    float weight{1.0F};
    RBrushFloatRange scale{1.0F, 1.0F};
    RBrushFloatRange depthMeters;
    RBrushFloatRange rotationRadians;
    float threshold{0.5F};
    float gamma{1.0F};
    bool invert{};
};

struct RBrushSet
{
    std::vector<WeightedRBrushReference> entries;
    RBrushTagQuery query;
    std::uint64_t seed{};
};

struct RBrushRockGeneratorBrushSetRequest
{
    RBrushRockFeature feature{RBrushRockFeature::Crack};
    RBrushSet brushes;
    std::uint32_t count{};
};

Result<RBrushRockGeneratorBrushSetResult>
resolveRBrushRockGeneratorBrushSet(
    std::span<const ProjectRBrushAsset> assets,
    const RBrushRockGeneratorBrushSetRequest& request);
```

- [ ] **Step 1: Register the focal test**

Add `JericoRBrushRockGeneratorBrushSetTests`, link
`Jerico::EditorProjectAuthority`, and register:

```cmake
add_test(
    NAME editor.rbrush_rock_generator_brush_sets
    COMMAND JericoRBrushRockGeneratorBrushSetTests)
set_tests_properties(
    editor.rbrush_rock_generator_brush_sets
    PROPERTIES LABELS "jerico.schedule.serial" RUN_SERIAL TRUE TIMEOUT 120)
```

- [ ] **Step 2: Write the deterministic product fixture**

Build two admitted `ProjectRBrushAsset` snapshots containing compatible
`crack/sdf`, `chip/sdf`, `large` and unrelated presets. Use non-zero tip hashes,
content hashes and canonical SDF defaults. Supply the assets in forward and
reverse order and request twelve choices with:

```cpp
request.feature = RBrushRockFeature::Crack;
request.brushes.seed = 0xC0FFEE1234ULL;
request.brushes.query.require = {"sdf"};
request.brushes.query.any = {"crack", "chip"};
request.brushes.query.exclude = {"large"};
request.count = 12U;
```

Assert identical result and recipe hash for both catalog orders, exactly twelve
choices, no excluded preset, and `SdfBrushOperation::CrackCutter` on every
choice.

- [ ] **Step 3: Prove explicit precedence and chip output**

Add one explicit entry for a query candidate with weight `8`, scale `0.75..1.5`,
depth `0.02..0.08`, rotation `-pi..pi`, threshold `0.35`, gamma `1.7` and
invert true. Assert only one normalized candidate exists for that GUID and every
emitted value lies within its configured range. Repeat with feature `Chip` and
assert deterministic `ChipCutter` output differs from the crack recipe.

- [ ] **Step 4: Add complete rejection cases**

Assert failure and no returned partial result for zero seed/count, count above
4,096, conflicting tags, duplicate explicit references, missing references,
non-SDF-compatible presets, zero/non-finite weights, unordered/non-finite ranges,
invalid threshold/gamma and an empty final candidate set.

- [ ] **Step 5: Run and retain the authentic RED**

Run:

```powershell
cmake --preset windows-vulkan-editor
cmake --build --preset windows-vulkan-editor --target JericoRBrushRockGeneratorBrushSetTests
```

Expected first failure: scheduling inventory changes from 290 to 291. Update
only the reported expected count/hash in the two scheduling authority files.
Reconfigure and rebuild. Expected product failure: C1083 because
`Jerico/Editor/RBrushRockGeneratorBrushSet.hpp` does not exist.

- [ ] **Step 6: Commit the RED**

Stage only the focal test, test CMake and exact scheduling-policy changes. Run
`git diff --cached --check` and commit:

```text
test(rbrush): require deterministic rock brush sets
```

---

### Task 2: Project canonical SDF metadata through the existing catalog

**Files:**

- Modify: `Editor/JericoEditor/Include/Jerico/Editor/RBrushProjectCatalog.hpp`
- Modify: `Editor/JericoEditor/Source/RBrushProjectCatalog.cpp`
- Modify: `Editor/JericoEditor/Source/RBrushLibrary.cpp`
- Modify: `Tests/Editor/RBrushProjectCatalogTests.cpp`

**Interfaces:**

- Consumes: `Tools::AssetPipeline::RBrushPresetIR::sdfDefaults`.
- Produces: `ProjectRBrushPresetCatalogEntry::sdfDefaults` with the exact same
  `Tools::AssetPipeline::RBrushSdfDefaults` type.

- [ ] **Step 1: Extend the existing catalog assertion**

Set a non-default compatible SDF profile in the existing project-catalog
fixture and require:

```cpp
TEST_CHECK_EQ(preset.sdfDefaults, sourcePack.presets.front().sdfDefaults);
```

- [ ] **Step 2: Run the catalog test and verify RED**

Run:

```powershell
cmake --build --preset windows-vulkan-editor --target JericoRBrushProjectCatalogTests
```

Expected: compile failure because the projected row has no `sdfDefaults` member.

- [ ] **Step 3: Add the canonical projection**

Add this member directly after the tip metadata:

```cpp
Tools::AssetPipeline::RBrushSdfDefaults sdfDefaults;
```

Initialize it from `preset.sdfDefaults` in `summarizeProjectRBrushCatalog`.
Extend `RBrushLibrary.cpp::validateCatalog` to reject non-finite/out-of-range
threshold, depth, edge width and gamma. Compatibility false remains valid; it
means the resolver must not admit the preset as an SDF candidate.

- [ ] **Step 4: Run catalog and library regressions**

Run:

```powershell
cmake --build --preset windows-vulkan-editor --target JericoRBrushProjectCatalogTests
ctest --preset windows-vulkan-editor -R "^editor\.rbrush_(project_catalog|library)" --output-on-failure
```

Expected: all selected tests PASS and the exact SDF profile survives catalog
summarization/discovery.

- [ ] **Step 5: Commit the catalog projection**

Run `git diff --check`, stage only these four paths, and commit:

```text
feat(rbrush): project sdf defaults into catalog
```

---

### Task 3: Implement the deterministic resolver GREEN

**Files:**

- Create: `Editor/JericoEditor/Include/Jerico/Editor/RBrushRockGeneratorBrushSet.hpp`
- Create: `Editor/JericoEditor/Source/RBrushRockGeneratorBrushSet.cpp`
- Modify: `Editor/JericoEditor/CMakeLists.txt`
- Modify as needed for corrected assertions: `Tests/Editor/RBrushRockGeneratorBrushSetTests.cpp`

**Interfaces:**

- Consumes: the exact request types from Task 1, `queryProjectRBrushLibrary`,
  `resolveProjectRBrushPreset`, projected SDF defaults and
  `Tools::ProceduralAuthoring::SdfBrushOperation`.
- Produces:

```cpp
struct ResolvedRBrushRockChoice
{
    RBrushPresetReference brush;
    std::uint64_t packContentHash{};
    Tools::AssetPipeline::RBrushDigest canonicalTipHash{};
    Tools::ProceduralAuthoring::SdfBrushOperation operation{
        Tools::ProceduralAuthoring::SdfBrushOperation::CrackCutter};
    float scale{1.0F};
    float depthMeters{};
    float rotationRadians{};
    float threshold{0.5F};
    float gamma{1.0F};
    bool invert{};
    std::uint64_t seed{};

    bool operator==(const ResolvedRBrushRockChoice&) const = default;
};

struct RBrushRockGeneratorBrushSetResult
{
    std::vector<ResolvedRBrushRockChoice> choices;
    std::uint64_t recipeHash{};

    bool operator==(const RBrushRockGeneratorBrushSetResult&) const = default;
};
```

- [ ] **Step 1: Declare the focused public API**

Place only data types, the resolver declaration and public limits in the header.
Keep normalization, fixed-point weights, hashing and sampling private to the
source file.

- [ ] **Step 2: Delegate query semantics to the canonical library**

Build one `RBrushLibraryQuery` from `require/any/exclude`, call
`queryProjectRBrushLibrary`, flatten the returned presets and discard candidates
whose projected SDF defaults are not compatible. Do not reproduce tag matching.

- [ ] **Step 3: Normalize implicit and explicit candidates**

Create implicit entries using weight `1`, scale `1..1`, canonical default depth,
rotation `0..2*pi`, and canonical threshold/gamma/invert. Resolve every explicit
reference through `resolveProjectRBrushPreset`; reject duplicates and
incompatible presets. Replace an implicit candidate when its identity matches an
explicit entry, then sort by `(AssetGUID, PresetGUID)`.

- [ ] **Step 4: Implement fixed-domain deterministic selection**

Convert validated weights to positive 32-bit fixed-point units with a fixed
scale of `1,000,000`, rejecting overflow or a zero aggregate. For each choice,
mix set seed, index and a selection-domain constant, take modulo aggregate
weight, and select through cumulative weights. Sample scale/depth/rotation with
separate constants and a 24-bit fraction, quantizing each result to `1/4096`.
Derive the emitted operation and per-choice seed from feature plus index.

- [ ] **Step 5: Hash full recipe authority**

Use an explicit byte-wise stable hash helper over normalized query strings,
feature, seed, count, every sorted candidate configuration, pack content hash,
canonical tip hash and all emitted choices. Do not hash object padding or native
container addresses.

- [ ] **Step 6: Build and run focal GREEN**

Run:

```powershell
cmake --build --preset windows-vulkan-editor --target JericoRBrushRockGeneratorBrushSetTests
ctest --preset windows-vulkan-editor -R "^editor\.rbrush_rock_generator_brush_sets$" --output-on-failure
```

Expected: 1/1 PASS with all deterministic and rejection assertions executed.

- [ ] **Step 7: Commit the minimum GREEN**

Run `git diff --check`, review only resolver/CMake/test corrections, and commit:

```text
feat(rbrush): resolve deterministic rock brush sets
```

---

### Task 4: Verify and retain the Slice32 checkpoint

**Files:**

- Create: `docs/evidence/M8_RBRUSH_ROCK_GENERATOR_BRUSH_SETS.md`
- Modify: `docs/superpowers/THREAD_HANDOFF.md`

**Interfaces:**

- Consumes: committed focal behavior and fresh regression output.
- Produces: exact durable metrics, honest nonclaims and one next legal action.

- [ ] **Step 1: Run deterministic proof twice**

Run the focal executable twice and record the exact crack/chip recipe hashes,
choice identities and sampled values. The two runs must match byte-for-byte in
their deterministic metric output.

- [ ] **Step 2: Run focused regressions**

Run:

```powershell
ctest --preset windows-vulkan-editor -R "^editor\.rbrush_(project_catalog|library|rock_generator_brush_sets|sdf_material_stamp_session)$|^procedural\.sdf_brush" --output-on-failure
cmake --build --preset windows-vulkan-editor
ctest --preset windows-vulkan-editor -R "^(repository\.test_scheduling|editor\.boundary_audit)$" --output-on-failure
```

Classify every failure; do not weaken policy or call the slice PASS with a
failing regression.

- [ ] **Step 3: Record durable evidence**

Document HEAD, exact commands/pass counts/timings, normalized candidate count,
forward/reverse catalog recipe hashes, crack/chip/changed-seed hashes, exact
sampled choices and every rejection class. State explicitly that no rock mesh,
distribution, UI, cook/package, Layout or runtime RBrush was implemented.

- [ ] **Step 4: Commit evidence**

Run `git diff --check`, stage only the evidence file, and commit:

```text
docs(rbrush): retain rock brush set proof
```

- [ ] **Step 5: Publish the continuity checkpoint**

Reconcile Git without reset/restore/clean, enumerate all preserved user paths,
and update `THREAD_HANDOFF.md` only if every observable criterion is fresh.
Commit:

```text
docs(rbrush): record rock brush set checkpoint
```

Push current `main`, fetch, and verify `HEAD == origin/main`. Keep M8 and the
master Goal `ACTIVE / NOT COMPLETE`, and continue to the next undemonstrated
RBrush slice in the same task.
