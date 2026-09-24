# M8 RBrush Procedural Source Usage Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make an admitted RBrush tip filter normal PWAS procedural candidates without mouse/tablet stroke input.

**Architecture:** Add a schema-6 `RBrushMaskFilter` node to the existing PWAS graph and evaluate it inside the single PWAS placement evaluator. Tip pixels stay in an immutable authoring query record built by JericoEditor from the admitted RBrush library; PWAS stores only stable identity, projection and sampling settings.

**Tech Stack:** C++20, Jerico Core/Engine Painting types, ProceduralAuthoring PWAS graph/evaluator/TOML codec, JericoEditor RBrush Library, CMake/CTest, Windows Vulkan editor preset.

## Global Constraints

- Work only in the authoritative `F:\JericoEngine` checkout on `main`; do not create branches, worktrees or another task.
- Preserve every pre-existing tracked and untracked user file listed in `docs/superpowers/THREAD_HANDOFF.md`.
- RBrush and `.rbrush` remain editor-only; Shipping receives only later cooked static results.
- PWAS remains the only placement evaluator and graph authority.
- Do not add UI, Procedural 3D Layout, another workspace, static Surface Paint cook or static Sparse SDF cook.
- Do not add third-party dependencies.
- Follow RED before GREEN and do not claim PASS from compilation alone.

---

### Task 1: Product RED for a non-mouse RBrush mask source

**Files:**
- Modify: `Tests/ProceduralAuthoring/PwasPlacementEvaluatorTests.cpp`

**Interfaces:**
- Consumes: existing `houseRecipe()`, `makePwasHouseQuery()`, prefab fixtures and `evaluatePwasPlacements()` test helpers.
- Produces: a compile-time requirement for `Jerico/ProceduralAuthoring/PwasRBrushMaskSource.hpp` and a behavioral test named `rbrushTipFiltersProceduralCandidatesWithoutStroke()`.

- [ ] **Step 1: Add the missing product include and focused test**

Add:

```cpp
#include <Jerico/ProceduralAuthoring/PwasRBrushMaskSource.hpp>
```

The new test must construct a normal grid or Poisson graph, bind an asymmetric
R8 tip through `buildPwasRBrushMaskSourceGraph`, evaluate with one and four
workers, and assert:

```cpp
TEST_CHECK(!std::ranges::any_of(graph.nodes, [](const PwasNode& node) {
    return node.kind == PwasNodeKind::HandPaintedSeedScatter;
}));
TEST_CHECK(graph.rbrushDistributionBinding == std::nullopt);
TEST_CHECK(one.value() == four.value());
TEST_CHECK(one.value().statistics.rbrushMaskRejected > 0U);
TEST_CHECK(one.value().statistics.acceptedItems > 0U);
```

Also cover repeat equality, invert/change difference, missing/stale record,
invalid dimensions/frame and cancellation. Print one evidence marker containing
accepted, rejected, input hash, output hash and worker equality.

- [ ] **Step 2: Confirm scheduling remains unchanged**

Run:

```powershell
ctest --test-dir build/windows-vulkan-editor -C Debug -R "^repository.test_scheduling_contract$" --output-on-failure
```

Expected: PASS with the existing CTest inventory unchanged. Do not edit the
dirty `Tests/ProceduralAuthoring/CMakeLists.txt` or the scheduling manifest,
because the RED extends an already scheduled executable.

- [ ] **Step 3: Run the product RED**

Run:

```powershell
cmake --build build/windows-vulkan-editor --config Debug --target JericoPwasPlacementEvaluatorTests -- /m:2 /nr:false
```

Expected: FAIL with C1083 because
`Jerico/ProceduralAuthoring/PwasRBrushMaskSource.hpp` does not exist.

- [ ] **Step 4: Commit the authentic RED**

```powershell
git add -- Tests/ProceduralAuthoring/PwasPlacementEvaluatorTests.cpp
git diff --cached --check
git commit -m "test(rbrush): require procedural mask source"
git push origin main
```

---

### Task 2: Canonical graph node and schema-6 persistence

**Files:**
- Create: `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/PwasRBrushMaskSource.hpp`
- Create: `Tools/ProceduralAuthoring/Source/PwasRBrushMaskSource.cpp`
- Modify: `Tools/ProceduralAuthoring/CMakeLists.txt`
- Modify: `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/PwasRecipeGraph.hpp`
- Modify: `Tools/ProceduralAuthoring/Source/PwasRecipeGraph.cpp`
- Modify: `Tools/ProceduralAuthoring/Source/PwasRecipeIo.cpp`
- Modify: `Tests/ProceduralAuthoring/PwasRecipeGraphTests.cpp`
- Modify: `Tests/ProceduralAuthoring/PwasRecipeIoTests.cpp`

**Interfaces:**
- Consumes: `Painting::BrushPresetReference`, `PwasRecipeGraph`, existing graph validation/hash/TOML codec.
- Produces:

```cpp
struct PwasRBrushMaskProjection {
    Vec3d origin;
    Vec3d axisU{1.0, 0.0, 0.0};
    Vec3d axisV{0.0, 0.0, 1.0};
    double extentU{1.0};
    double extentV{1.0};
};

struct PwasRBrushMaskFilter {
    Painting::BrushPresetReference brush;
    std::uint64_t brushPackContentHash{};
    Guid tipGuid;
    PwasRBrushMaskProjection projection;
    double threshold{0.5};
    double gamma{1.0};
    bool invert{};
};

struct PwasRBrushMaskSourceRequest {
    const PwasRecipeGraph* baseRecipe{};
    PwasRBrushMaskFilter filter;
};

Result<PwasRecipeGraph> buildPwasRBrushMaskSourceGraph(
    const PwasRBrushMaskSourceRequest& request,
    std::stop_token stopToken = {});
```

- [ ] **Step 1: Add graph types and validation tests**

Add `PwasNodeKind::RBrushMaskFilter`, add `PwasRBrushMaskFilter` to
`PwasNodeParameters`, admit schema 6, and test zero identity/hash, invalid
projection, invalid threshold/gamma, duplicate node and invalid route.

- [ ] **Step 2: Implement the graph builder minimally**

`buildPwasRBrushMaskSourceGraph` must:

1. validate the immutable base graph and filter;
2. reject an existing RBrush mask node;
3. derive one stable node GUID from recipe, brush, tip, pack hash and settings;
4. insert the node after grid/Poisson/spline scatter and before any canonical
   region mask/SDF filter;
5. add/update exactly one brush dependency and sort nodes/edges/dependencies;
6. set schema 6 and validate the complete result;
7. return no graph when cancelled or invalid.

- [ ] **Step 3: Add exact schema-6 TOML read/write/hash**

Use one inline node table containing brush/preset/tip GUIDs, pack hash,
projection vectors/extents, threshold, gamma and invert. Preserve the exact
schema-5 golden fixture and add a schema-6 write/read/write equality fixture.

- [ ] **Step 4: Run graph and codec tests**

```powershell
cmake --build build/windows-vulkan-editor --config Debug --target JericoPwasRecipeGraphTests JericoPwasRecipeIoTests -- /m:2 /nr:false
ctest --test-dir build/windows-vulkan-editor -C Debug -R "^procedural\.pwas_(graph|recipe)$" --output-on-failure
```

Expected: `2/2 PASS` and the existing schema-5 output string unchanged.

- [ ] **Step 5: Commit graph/persistence**

```powershell
git add -- Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/PwasRBrushMaskSource.hpp Tools/ProceduralAuthoring/Source/PwasRBrushMaskSource.cpp Tools/ProceduralAuthoring/CMakeLists.txt Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/PwasRecipeGraph.hpp Tools/ProceduralAuthoring/Source/PwasRecipeGraph.cpp Tools/ProceduralAuthoring/Source/PwasRecipeIo.cpp Tests/ProceduralAuthoring/PwasRecipeGraphTests.cpp Tests/ProceduralAuthoring/PwasRecipeIoTests.cpp
git diff --cached --check
git commit -m "feat(pwas): persist rbrush mask sources"
git push origin main
```

---

### Task 3: Evaluate the real R8 tip inside PWAS

**Files:**
- Modify: `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/PwasPlacementEvaluator.hpp`
- Modify: `Tools/ProceduralAuthoring/Source/PwasPlacementEvaluator.cpp`
- Modify: `Tests/ProceduralAuthoring/PwasPlacementEvaluatorTests.cpp`

**Interfaces:**
- Consumes: schema-6 node from Task 2 and existing candidate/support pipeline.
- Produces:

```cpp
struct PwasRBrushMaskQueryRecord {
    Painting::BrushPresetReference brush;
    std::uint64_t brushPackContentHash{};
    Guid tipGuid;
    std::uint32_t width{};
    std::uint32_t height{};
    std::vector<std::byte> coverageR8;
};
```

Add `std::span<const PwasRBrushMaskQueryRecord> rbrushMasks` to
`PwasEvaluationContext`, `rbrushMaskCatalogHash` to provenance and
`rbrushMaskRejected` to statistics.

- [ ] **Step 1: Validate and hash the immutable query catalog**

Require sorted unique `(AssetGUID, PresetGUID, TipGUID)`, dimensions in
`1..4096`, at most `16,777,216` texels and exact `width * height` bytes. Derive
the catalog hash from identity, pack hash, dimensions and every coverage byte.
An empty catalog retains provenance hash zero so old graphs keep old input
hashes.

- [ ] **Step 2: Resolve the node and sample candidates**

Project each supported candidate position into the orthonormal U/V frame.
Outside `[0,1]^2` rejects. Inside, bilinearly sample R8, optionally invert,
apply `pow(coverage, gamma)` and compare to threshold. Reject before downstream
SDF/output work and increment `rbrushMaskRejected`.

The evaluator must require an exact matching record only when the graph has the
node. Absence or pack/tip mismatch returns
`procedural.pwas.rbrush_mask_dependency_stale`.

- [ ] **Step 3: Preserve deterministic worker behavior and hashing**

The filter reads immutable bytes only. Hash `rbrushMaskRejected` only when the
field is non-zero so schemas 1-5 retain historical hashes. Include the non-zero
mask catalog hash in schema-6 input provenance.

- [ ] **Step 4: Run the focal GREEN repeatedly**

```powershell
cmake --build build/windows-vulkan-editor --config Debug --target JericoPwasPlacementEvaluatorTests -- /m:2 /nr:false
ctest --test-dir build/windows-vulkan-editor -C Debug -R "^procedural\.pwas_evaluator$" --repeat until-fail:10 --output-on-failure
```

Expected: `10/10 PASS`; evidence marker repeats byte-identically.

- [ ] **Step 5: Commit evaluator GREEN**

```powershell
git add -- Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/PwasPlacementEvaluator.hpp Tools/ProceduralAuthoring/Source/PwasPlacementEvaluator.cpp Tests/ProceduralAuthoring/PwasPlacementEvaluatorTests.cpp
git diff --cached --check
git commit -m "feat(pwas): filter placement with rbrush masks"
git push origin main
```

---

### Task 4: Build the query from the admitted RBrush library

**Files:**
- Create: `Editor/JericoEditor/Include/Jerico/Editor/RBrushProceduralMaskSource.hpp`
- Create: `Editor/JericoEditor/Source/RBrushProceduralMaskSource.cpp`
- Modify: `Editor/JericoEditor/CMakeLists.txt`
- Modify: `Tests/Editor/RBrushProjectCatalogTests.cpp`

**Interfaces:**
- Consumes: `RBrushLibraryEditablePreset` from the existing admitted catalog and `PwasRBrushMaskQueryRecord` from Task 3.
- Produces:

```cpp
Result<Tools::ProceduralAuthoring::PwasRBrushMaskQueryRecord>
makePwasRBrushMaskQueryRecord(
    const RBrushLibraryEditablePreset& admitted);
```

- [ ] **Step 1: Write the adapter test against an admitted pack fixture**

Load the fixture with `loadProjectRBrushEditablePreset`, call the adapter, and
assert exact asset/preset/tip identity, pack hash, dimensions and R8 bytes.
Mutate or remove the primary tip and assert a typed failure.

- [ ] **Step 2: Implement the thin adapter**

Find exactly one `previewTips` entry matching `preset.primaryTipGuid`; copy its
bounded immutable coverage and the catalog-owned pack hash. Do not parse files,
resample pixels or create another catalog.

- [ ] **Step 3: Run editor/catalog and PWAS integration**

```powershell
cmake --build build/windows-vulkan-editor --config Debug --target JericoRBrushProjectCatalogTests JericoPwasPlacementEvaluatorTests -- /m:2 /nr:false
ctest --test-dir build/windows-vulkan-editor -C Debug -R "^(editor\.rbrush_project_catalog|procedural\.pwas_evaluator)$" --output-on-failure
```

Expected: `2/2 PASS`.

- [ ] **Step 4: Commit the admitted-source adapter**

```powershell
git add -- Editor/JericoEditor/Include/Jerico/Editor/RBrushProceduralMaskSource.hpp Editor/JericoEditor/Source/RBrushProceduralMaskSource.cpp Editor/JericoEditor/CMakeLists.txt Tests/Editor/RBrushProjectCatalogTests.cpp
git diff --cached --check
git commit -m "feat(rbrush): adapt admitted tips for pwas masks"
git push origin main
```

---

### Task 5: Verification, evidence and continuity checkpoint

**Files:**
- Create: `docs/evidence/M8_RBRUSH_PROCEDURAL_SOURCE_USAGE.md`
- Modify: `docs/CAPABILITY_MAP.md` only if its existing RBrush/PWAS row requires the new demonstrated fact
- Modify: `docs/superpowers/THREAD_HANDOFF.md`

**Interfaces:**
- Consumes: all product and test outputs from Tasks 1-4.
- Produces: durable Slice33 evidence and the single next legal action, Slice34.

- [ ] **Step 1: Run fresh Debug focal/regression gates**

```powershell
cmake --build build/windows-vulkan-editor --config Debug --target JericoPwasRecipeGraphTests JericoPwasRecipeIoTests JericoPwasPlacementEvaluatorTests JericoRBrushProjectCatalogTests -- /m:2 /nr:false
ctest --test-dir build/windows-vulkan-editor -C Debug -R "^(procedural\.pwas_(graph|recipe|evaluator)|editor\.rbrush_project_catalog|repository\.test_scheduling_contract)$" --output-on-failure
```

- [ ] **Step 2: Repeat the same focal product in Release**

```powershell
cmake --build build/windows-vulkan-editor --config Release --target JericoPwasRecipeGraphTests JericoPwasRecipeIoTests JericoPwasPlacementEvaluatorTests JericoRBrushProjectCatalogTests -- /m:2 /nr:false
ctest --test-dir build/windows-vulkan-editor -C Release -R "^(procedural\.pwas_(graph|recipe|evaluator)|editor\.rbrush_project_catalog)$" --output-on-failure
```

- [ ] **Step 3: Run boundaries and integrated build**

Run the exact existing boundaries:

```powershell
ctest --test-dir build/windows-vulkan-editor -C Debug -R "^(repository\.test_scheduling_contract|architecture\.|editor\.boundary_audit|repository\.runtime_only_target_discovery|repository\.shipping_source_exclusion)$" --output-on-failure
```

Then run:

```powershell
cmake --build build/windows-vulkan-editor --config Debug -- /m:2 /nr:false
git diff --check
```

Do not claim PASS for any command that times out or whose original process is
still running; read its final log/process state first.

- [ ] **Step 4: Record evidence and exact limitations**

The evidence must include RED error, schema-6 round-trip, accepted/rejected
counts, exact hashes, worker 1/4 equality, stale/cancel results, Debug/Release
commands and source-free boundary results. State explicitly that no UI,
runtime RBrush, static cook or visual PASS is claimed.

- [ ] **Step 5: Publish the checkpoint**

Update `THREAD_HANDOFF.md` with Slice33 `PASS`, exact HEAD/upstream/tree,
preserved user paths and the unique next action: Slice34 static Surface Paint
cook with RBrush dependency removed.

```powershell
git add -- docs/evidence/M8_RBRUSH_PROCEDURAL_SOURCE_USAGE.md docs/CAPABILITY_MAP.md docs/superpowers/THREAD_HANDOFF.md
git diff --cached --check
git commit -m "docs(rbrush): record procedural source checkpoint"
git push origin main
```

Do not stage `docs/CAPABILITY_MAP.md` when no row requires a factual update.
