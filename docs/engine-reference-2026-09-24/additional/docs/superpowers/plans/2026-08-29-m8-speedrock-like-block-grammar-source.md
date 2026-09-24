# M8 SpeedRock-Like Block-Grammar Source Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Subagent-driven implementation is forbidden by the repository's single-writer contract.

**Goal:** Add one deterministic bounded block-grammar geology source that feeds
Jerico's existing Vulkan field, classified Surface Nets, cook, collision,
package and runtime consumers.

**Architecture:** Recipe schema 21 extends the existing first `Source` layer with
one fixed `BlockGrammar` source. CPU expands and compacts typed occupancy into
disjoint cuboids; those cuboids materialize as the existing additive source
geometry, so the admitted Vulkan scalar field and all downstream consumers stay
authoritative.

**Tech Stack:** C++20, toml++, Jerico Result/Guid contracts, Diligent Vulkan,
Surface Nets, JRMS/JRCL/EPAK and the existing Windows Vulkan editor build tree.

**Spec:** `docs/superpowers/specs/2026-08-29-m8-speedrock-like-block-grammar-source-design.md`

## Global Constraints

- Work only in `F:\JericoEngine` on `main`; no branch, worktree, fork or second writer.
- Preserve the 182-entry foreign inventory and SHA-256
  `2e996b1669df5ad1dd4e0cb9c3cbaf1b8bfc73ba6bc734abbad0a49419954030`.
- Execute only `M8-GEOLOGY-BLUEPRINT-P2-SPEEDROCK-LIKE-BLOCK-GRAMMAR-SOURCE`.
- Do not reopen P0/P1 or implement evolution/search, erosion, another P2, UI,
  VDB, presets, destruction/remesh/retopology/save-state or the production expansion.
- Sol is the only checkout writer and Git authority. No implementation subagents.
- Runtime formats remain unchanged; Shipping consumes only cooked Jerico assets.
- Every task begins with an observable RED and ends with focused verification.
- Persistent Goal remains `ACTIVE until full JericoEngine 1.0 closure`.

## File structure

- Create
  `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/Geology/GeologyBlockGrammarSource.hpp`:
  the bounded CPU evaluator, diagnostics, result hash and local cuboid output.
- Modify `GeologyTypes.hpp`: fixed grammar/source enums only.
- Modify `GeologyLayer.hpp`: rule/definition structs and the embedded source definition.
- Modify `GeologyRecipe.hpp/.cpp`: schema 21 validation, canonical TOML and hashing.
- Modify `GeologyGpuTopologyPlan.hpp/.cpp`: plan the existing sparse field from
  transformed grammar cuboid bounds without a BSP document.
- Modify `GeologyGpuSessionDiligent.cpp`: materialize grammar cuboids into the
  existing `SourceGeometry` and dispatch the unchanged source shader.
- Modify `GeologyRecipeTests.cpp`, `GeologyGpuTopologyPlanTests.cpp` and
  `GeologyGpuSessionTests.cpp`: CPU, topology, Vulkan, cook and artifact contracts.
- Modify `Tests/ProceduralAuthoring/CMakeLists.txt` only for the new focused
  Vulkan test registration; preserve its two foreign formatting hunks.
- Create/update only the slice's evidence, system audit, `ACTIVE_SLICE.md` and
  compact `HANDOFF.md` during final closure.

---

### Task 1: Freeze schema 21 and the bounded CPU grammar

**Files:**

- Create: `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/Geology/GeologyBlockGrammarSource.hpp`
- Modify: `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/Geology/GeologyTypes.hpp:47-52`
- Modify: `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/Geology/GeologyLayer.hpp:13-23`
- Modify: `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/Geology/GeologyRecipe.hpp:10-38`
- Modify: `Tools/ProceduralAuthoring/Source/Geology/GeologyRecipe.cpp:251-293,493-502,1584-2059,3669-3697,4646-4705`
- Test: `Tests/ProceduralAuthoring/GeologyRecipeTests.cpp`

**Interfaces:**

- Consumes: `GeologyLayerHeader::seed`, `GeologyTransform`, `Result<T>`, canonical recipe writer/hash.
- Produces:

```cpp
enum class GeologyBlockGrammarSymbol : std::uint8_t {
    Empty = 0U, Rule1 = 1U, Rule2 = 2U, Rule3 = 3U, Rule4 = 4U
};

struct GeologyBlockGrammarRule {
    std::array<GeologyBlockGrammarSymbol, 8U> children{};
    bool operator==(const GeologyBlockGrammarRule&) const = default;
};

struct GeologyBlockGrammarSourceDefinition {
    Vec3d rootHalfExtentMeters{1.0, 1.0, 1.0};
    GeologyBlockGrammarSymbol axiom{GeologyBlockGrammarSymbol::Rule1};
    std::array<GeologyBlockGrammarRule, 4U> rules{};
    std::uint32_t rewriteDepth{1U};
    bool implode{};
    bool operator==(const GeologyBlockGrammarSourceDefinition&) const = default;
};

struct GeologyBlockGrammarBox {
    std::array<std::uint32_t, 3U> minimumCell{};
    std::array<std::uint32_t, 3U> maximumCellExclusive{};
    bool operator==(const GeologyBlockGrammarBox&) const = default;
};

struct GeologyBlockGrammarSourceRequest {
    GeologyBlockGrammarSourceDefinition definition;
    std::uint64_t seed{};
    std::uint64_t maximumWorkItems{8'388'608U};
    std::function<bool()> cancellationRequested;
};

struct GeologyBlockGrammarSourceResult {
    std::uint32_t gridSide{};
    std::vector<std::uint8_t> occupied;
    std::vector<GeologyBlockGrammarBox> boxes;
    std::uint64_t workItems{};
    std::uint64_t occupiedCellCount{};
    std::uint64_t contentHash{};
};

Result<GeologyBlockGrammarSourceResult> evaluateGeologyBlockGrammarSource(
    const GeologyBlockGrammarSourceRequest& request);
std::uint64_t hashGeologyBlockGrammarSourceDefinition(
    const GeologyBlockGrammarSourceDefinition& definition) noexcept;
```

- [ ] **Step 1: Write CPU and recipe RED**

Add five focused functions and call them from `main()`:

```cpp
void blockGrammarSourceRewritesCanonicalOccupancyDeterministically();
void blockGrammarSourceCompactsAndCoalescesBoundedly();
void blockGrammarSourceRejectsInvalidCancelledAndExcessWork();
void roundTripsAndHashesBlockGrammarSourceCanonically();
void rejectsInvalidOrPreSchemaBlockGrammarSource();
```

Use one asymmetric four-rule fixture. Assert the exact occupied cell vector for
depth two, equal result/hash for repeated seed, unequal result/hash for a second
seed, exact X/Y/Z compaction coordinates, disjoint box coverage, and the stable
errors `procedural.geology.block_grammar_invalid`,
`procedural.geology.block_grammar_empty`,
`procedural.geology.block_grammar_work_budget_exceeded` and
`procedural.geology.cancelled`.

Schema RED constructs `GeologySourceKind::BlockGrammar`, schema 21, zero
`documentGuid`, non-zero source GUID/revision and a canonical definition hash.
It asserts byte-identical write/parse/write and recipe-hash changes for every
rule slot, axiom, depth, implosion flag, source revision, source hash, transform
and seed. Schema 20 and unknown symbols must fail.

- [ ] **Step 2: Build to capture authentic RED**

Run:

```powershell
cmake --build build/windows-vulkan-editor --config Debug --target JericoGeologyRecipeTests
```

Expected: compile failure because `BlockGrammar`, schema 21 and the evaluator do
not exist. Preserve the first relevant diagnostic for closure evidence.

- [ ] **Step 3: Implement the minimum CPU contract**

Add `BlockGrammar = 4U`, the fixed symbol/rule/definition types and
`geologyStackRecipeBlockGrammarSchemaVersion = 21U`.

In the new header, implement:

```cpp
constexpr std::uint32_t geologyBlockGrammarMaximumRewriteDepth = 6U;
constexpr std::uint64_t geologyBlockGrammarMaximumTerminalCells = 262'144U;
constexpr std::uint64_t geologyBlockGrammarMaximumChildWrites = 299'592U;
constexpr std::uint64_t geologyBlockGrammarMaximumBoxes = 65'536U;
constexpr std::uint64_t geologyBlockGrammarMaximumWorkItems = 8'388'608U;
```

Use `index = x + side * (y + side * z)`. Rewrite into a fresh vector each
generation. Derive `reflectionMask = mix64(seed, generation, parentIndex,
parentRule) & 7U`, then read `rule.children[childIndex ^ reflectionMask]`.
Check work/cancellation every 4096 operations.

For implosion, compact negative and positive halves toward `side/2` once per
axis in X, Y, Z order. For coalescing, scan Z/Y/X and extend X, then complete Y
rows, then complete Z slabs before marking cells consumed. Increment the same
checked work counter for every occupancy read/write.

Extend recipe validation and canonical TOML only for `BlockGrammar`; BSP cases
must remain byte-compatible. Serialize `axiom`, `rewriteDepth`, `implode`,
`rootHalfExtentMeters` and `rule1..rule4` as eight symbolic strings each.

- [ ] **Step 4: Run CPU GREEN and regression**

Run:

```powershell
cmake --build build/windows-vulkan-editor --config Debug --target JericoGeologyRecipeTests
ctest --test-dir build/windows-vulkan-editor -C Debug -R "^procedural.geology_recipe$" --output-on-failure
```

Expected: build succeeds and `procedural.geology_recipe` passes, including the
new exact occupancy, seed, validation, cancellation and round-trip contracts.

- [ ] **Step 5: Commit the CPU contract**

Stage only the new header, the four recipe/type files and the test file. Run
`git diff --cached --check`, inspect `git diff --cached --name-status`, then:

```powershell
git commit -m "feat(geology): add bounded block grammar source"
```

---

### Task 2: Feed grammar cuboids into the existing Vulkan source field

**Files:**

- Modify: `Tools/ProceduralAuthoring/Source/Geology/Backends/GpuDiligent/GeologyGpuTopologyPlan.hpp:35-38`
- Modify: `Tools/ProceduralAuthoring/Source/Geology/Backends/GpuDiligent/GeologyGpuTopologyPlan.cpp:481-705`
- Modify: `Tools/ProceduralAuthoring/Source/Geology/Backends/GpuDiligent/GeologyGpuSessionDiligent.cpp:445-779,1496-1940,2959-3005,3637-3945`
- Test: `Tests/ProceduralAuthoring/GeologyGpuTopologyPlanTests.cpp`
- Test: `Tests/ProceduralAuthoring/GeologyGpuSessionTests.cpp`
- Modify: `Tests/ProceduralAuthoring/CMakeLists.txt:616-660`

**Interfaces:**

- Consumes: `GeologyBlockGrammarSourceResult::boxes`, source-layer transform,
  existing `SourceGeometry`, `admitGeologyGpuWorkload` and source shader.
- Produces:

```cpp
Result<GeologyGpuTopologyPlan> planGeologyGpuTopology(
    const GeologyStackRecipe& recipe,
    const GeologyBlockGrammarSourceResult& source,
    const GeologyGpuTopologyLimits& limits = {});
```

`GeologyGpuSession::evaluate` accepts `document == nullptr` only when the first
source kind is `BlockGrammar`; BSP sources continue requiring and validating the
canonical document.

- [ ] **Step 1: Write topology and Vulkan RED**

Add a topology test that evaluates the CPU fixture, calls the new overload and
asserts transformed conservative leaf bounds, non-zero bounded leaves/bytes and
stable rejection of empty/stale/excess source results.

Add `evaluatesBlockGrammarSourceOnVulkanWithCpuParitySurfaceNetsAndCook()` and
CLI dispatch `--block-grammar`. It must assert:

```cpp
TEST_CHECK_EQ(stage.stageName, std::string{"BlockGrammarSourceField"});
TEST_CHECK_EQ(repeated.value().finalFieldHash, first.value().finalFieldHash);
TEST_CHECK(first.value().finalFieldHash != changedSeed.value().finalFieldHash);
TEST_CHECK(!first.value().denseMaster.mesh.positions.empty());
TEST_CHECK_EQ(first.value().denseMaster.vertexClassifications.size(),
              first.value().denseMaster.mesh.positions.size());
```

At bounded inspection points, compute CPU signed distance as the minimum box SDF
after applying the inverse layer transform and compare to Vulkan within
`1.0e-4` meters after narrow-band clamping. Verify cancellation before and
during grammar evaluation returns `procedural.geology.gpu_cancelled` without a
result.

Register exactly one new CTest:

```cmake
add_test(
    NAME procedural.geology_gpu_block_grammar
    COMMAND JericoGeologyGpuSessionTests --block-grammar)
```

Add it to the existing serial/resource property list without touching the
foreign lines 5 or 755.

- [ ] **Step 2: Build to capture topology/Vulkan RED**

Run:

```powershell
cmake --build build/windows-vulkan-editor --config Debug --target JericoGeologyGpuTopologyPlanTests JericoGeologyGpuSessionTests
```

Expected: compile/link failure for the missing topology overload and grammar
source materialization; CPU recipe tests remain green.

- [ ] **Step 3: Implement topology and materialization**

Refactor the topology planner only enough to share its existing bounds-to-leaf
calculation. For grammar, transform all eight corners of every local cuboid by
the source layer translation/quaternion/scale, include them in one finite world
bound, then reuse mask/layer expansion and all existing leaf/voxel/byte limits.

In the GPU session, evaluate the grammar once with the request cancellation
callback, plan from that exact result and materialize each box as one additive
`GpuBrush` with 12 triangles and 6 outward planes. Reuse
`appendBrushGeometry` through a local `BrushNode` or an equivalent focused box
helper; do not change the shader or add a grammar pipeline.

Populate existing source byte/count fields so `admitGeologyGpuWorkload` retains
its 536870912 primitive-test guard. Set only the source-stage diagnostic label
to `BlockGrammarSourceField`; dirty tracking, field hashes, Surface Nets and
classification remain shared.

- [ ] **Step 4: Run Vulkan GREEN and focused regressions**

Run:

```powershell
cmake --build build/windows-vulkan-editor --config Debug --target JericoGeologyGpuTopologyPlanTests JericoGeologyGpuSessionTests
ctest --test-dir build/windows-vulkan-editor -C Debug -R "^(procedural.geology_recipe|procedural.geology_gpu_topology|procedural.geology_gpu_block_grammar)$" --output-on-failure
```

Expected: CPU, topology and real Vulkan block-grammar tests pass. The Vulkan
diagnostics must report non-zero GPU time and no partial result on rejection.

- [ ] **Step 5: Commit productive Vulkan integration**

Stage only the topology/session/test paths and the owned CMake hunk. Use
`git diff --cached` to confirm the two pre-existing formatting hunks are absent,
then:

```powershell
git commit -m "feat(geology): evaluate block grammar source on Vulkan"
```

---

### Task 3: Prove cook, exact collision, source-free package and runtime

**Files:**

- Modify: `Tests/ProceduralAuthoring/GeologyGpuSessionTests.cpp`
- Create: `docs/evidence/images/m8-geology-blueprint-p2-block-grammar-source/`
- Create during evidence run under `build/windows-vulkan-editor/` only: cooked
  fixture, provenance text, `.epak` and BMP capture.

**Interfaces:**

- Consumes: `cookGeologySurface`, `writeMaterial`, `JericoPackageTool` and
  `JericoCookedAssetProof`.
- Produces: CLI
  `JericoGeologyGpuSessionTests --block-grammar-cook <directory>` containing
  `block-grammar.rmesh`, `block-grammar.rcoll` and one dependency-complete
  `block-grammar.rmat`.

- [ ] **Step 1: Write cook/artifact RED**

Extend the Vulkan test so an optional output directory calls
`cookGeologySurface` with non-zero stable mesh/collision/material GUIDs,
`sourceHash = grammar.contentHash`, the canonical recipe hash and the accepted
field/surface hashes. Write one PBR material:

```cpp
RuntimeFormats::PbrMaterialData pbr;
const auto material = RuntimeFormats::writeMaterial(
    RuntimeFormats::MaterialWriteRequest{
        .name = "Geology/BlockGrammarProof",
        .pipelineName = "Surface/PBR_Opaque",
        .guid = materialGuid,
        .backend = RuntimeFormats::MaterialBackend::Vulkan,
        .archiveContentVersion = 1U,
        .cookSchemaVersion = 1U,
        .pipelineType = RuntimeFormats::MaterialPipelineType::Graphics,
        .sourceHash = grammar.contentHash,
        .recipeHash = recipeHash,
        .archive = {std::byte{0x4A}, std::byte{0x54},
                    std::byte{0x50}, std::byte{0x42}},
        .pbr = pbr});
```

Assert reread `.rmesh` and `.rcoll` positions/indices are exact and source,
recipe, field, surface and classification descriptors match. The RED initially
fails because `--block-grammar-cook` is unknown.

- [ ] **Step 2: Run RED then implement the artifact writer**

Run the existing Debug executable against a new explicit directory under the
build tree. Expected RED: exit non-zero or no output directory because the CLI
branch is absent.

Add the CLI branch and minimal binary-file writer in the test executable only.
Do not add a production cooker or preset.

- [ ] **Step 3: Build Release and generate fresh cooked products**

Run:

```powershell
cmake --build build/windows-vulkan-editor --config Release --target JericoGeologyGpuSessionTests JericoPackageTool JericoCookedAssetProof
build/windows-vulkan-editor/Tests/ProceduralAuthoring/Release/JericoGeologyGpuSessionTests.exe --block-grammar-cook build/windows-vulkan-editor/m8-block-grammar-release/cooked
```

Expected: three cooked files, non-empty mesh, exact collision and zero source
files. Record SHA-256, vertex/index counts, field/surface/recipe hashes and CPU/GPU
diagnostics.

- [ ] **Step 4: Package and render only cooked Jerico assets**

Create a short provenance text in the build evidence directory that names the
Jerico-generated fixture, recipe/source hashes and official-paper citation but
contains no forbidden authoring marker. Use a fresh non-zero 32-hex build GUID:

```powershell
build/windows-vulkan-editor/Tools/PackageTool/Release/JericoPackageTool.exe --input-dir build/windows-vulkan-editor/m8-block-grammar-release/cooked --build-guid 9b4a5d3e7f214c18a6d809e2f3b4175c --provenance build/windows-vulkan-editor/m8-block-grammar-release/provenance.txt --output build/windows-vulkan-editor/m8-block-grammar-release/block-grammar.epak
build/windows-vulkan-editor/Samples/CookedAssetProof/Release/JericoCookedAssetProof.exe --package build/windows-vulkan-editor/m8-block-grammar-release/block-grammar.epak --mesh-guid 2b6772616d6d6172736f757263650001 --capture build/windows-vulkan-editor/m8-block-grammar-release/block-grammar-vulkan.bmp --resolution-width 1280 --resolution-height 720 --camera-z 5
```

If the fixed proof GUIDs differ in the implemented test fixture, use the exact
fixture GUID printed by the successful cook instead of silently editing runtime
assets. Expected: Vulkan, one non-empty draw, source-free package, material
resolved without fallback and a non-empty 1280x720 BMP.

- [ ] **Step 5: Inspect the original-resolution frame and run regressions**

Open the BMP at original resolution. Reject blank, clipped or debug-only proof;
adjust only test camera framing if needed. Copy the accepted BMP to the owned
evidence directory and record its SHA-256.

Run:

```powershell
ctest --test-dir build/windows-vulkan-editor -C Release -R "^(procedural.geology_recipe|procedural.geology_gpu_topology|procedural.geology_gpu_block_grammar|procedural.geology_cook|runtime_formats.(rmesh|rcoll)|physics.world_jolt)$" --output-on-failure
```

Expected: all available matching tests pass. Report any unmatched name honestly;
do not convert an absent test into PASS.

- [ ] **Step 6: Commit evidence-producing test changes**

Stage only the test file and accepted image, run `git diff --cached --check`, then:

```powershell
git commit -m "test(geology): prove block grammar runtime lineage"
```

---

### Task 4: Repeat completeness/data-lineage audit and close the slice

**Files:**

- Create: `docs/evidence/M8_GEOLOGY_BLUEPRINT_P2_BLOCK_GRAMMAR_SOURCE_CLOSURE.md`
- Modify: `docs/evidence/M8_GEOLOGY_SYSTEM_COMPLETENESS_AUDIT.md`
- Modify: `docs/work/ACTIVE_SLICE.md`
- Modify: `docs/work/HANDOFF.md`

**Interfaces:**

- Consumes: exact RED chronology, commit IDs, test output, hashes, diagnostics,
  package inventory and inspected image.
- Produces: one honest slice state, next single P2 mission and legal handoff data.

- [ ] **Step 1: Write closure evidence from observed facts only**

Record primary provenance, selected boundary, schema/source contracts, RED
diagnostics, test commands/results, CPU work, GPU timings, field/surface/recipe
hashes, cooked/package/capture hashes, exact collision and source-free runtime
result. State plainly that this closes only the first P2 source.

- [ ] **Step 2: Repeat the end-to-end audit**

Trace:

```text
typed grammar parameters + seed
-> bounded synchronous rewrite
-> optional X/Y/Z center compaction
-> exact coalesced cuboids
-> existing Vulkan negative-inside scalar field
-> existing Surface Nets + classification
-> existing .rmesh/.rcoll cook
-> source-free .epak
-> Vulkan renderer + static collision consumer
```

Evaluate every root `AGENTS.md` completeness category. Keep evolution/search,
aperiodic tiling, contact-preserving erosion, Dorsey weathering, moisture,
mineral dissolution and microstructure as ordered unstarted P2 work. Retain
runtime destruction/mutable collision/save, physical properties, classified
LOD/world cells, semantic editor views and downstream gameplay/Luau/navigation/
particles/audio as explicit `INTEGRATION REQUIRED` under existing owners.

- [ ] **Step 3: Verify diff and preservation inventory**

Run focused Debug/Release gates again, then:

```powershell
git diff --check
git status --short
git diff --cached --name-status
```

Before the final docs commit, verify the status minus owned staged paths still
contains exactly the original 182 lines and the same LF/no-final-newline SHA-256.

- [ ] **Step 4: Commit and publish closure**

Stage only closure evidence, audit, `ACTIVE_SLICE.md` and `HANDOFF.md`. Commit:

```powershell
git commit -m "docs(geology): close block grammar source"
git push origin main
git fetch --prune origin
git rev-parse HEAD
git rev-parse main
git rev-parse origin/main
git rev-list --left-right --count HEAD...origin/main
```

Expected: all refs equal, divergence `0 0`, 182 preserved foreign entries and
zero relevant processes.

- [ ] **Step 5: Perform exactly one legal successor handoff**

Update the sole `ACTIVE_SLICE` to the next ordered P2 feature only after this
slice is PASS and published. Create exactly one top-level successor, list tasks
before retrying any ambiguous result, require explicit acceptance of ownership,
the new slice, the 182-entry inventory and Persistent Goal continuity, then
renounce ownership and stop. Never implement that next slice here.
