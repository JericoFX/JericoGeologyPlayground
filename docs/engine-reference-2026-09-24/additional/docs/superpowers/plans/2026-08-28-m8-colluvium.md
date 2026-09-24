# M8 Colluvium implementation plan

> **Execution constraint:** Sol executes this plan in the current sole-owner
> task. Luna may only inspect READ_ONLY or run CMake/build/test/log analysis and
> may not edit or mutate Git.

**Goal:** Add the bounded offline/static Colluvium recipe operator that consumes
the immediately preceding Rockfall detached-volume lineage, deterministically
settles material inside the existing sparse topology, preserves exact
deposited/unsettled conservation, and continues through Jerico's published
classified mesh/material/collision/cook path.

**Architecture:** A header-only deterministic CPU reference owns static voxel
settling. Recipe schema 20 admits a terminal `Rockfall -> Colluvium` sequence.
The productive Diligent session reuses Rockfall's single CPU readback and
classification snapshot, applies the reference result transactionally, uploads
the modified classified field, and leaves all meshing, JRMS, material,
collision, and cook ownership in their existing paths.

**Technology:** C++20, existing Jerico procedural-authoring recipe/RBrush and
Diligent Vulkan session, existing focused CMake test executables, Markdown
authority/evidence.

---

## Task 1: Publish the research and frozen design checkpoint

**Files:**

- Add: `docs/evidence/M8_GEOLOGY_BLUEPRINT_P1_COLLUVIUM_RESEARCH.md`
- Add: `docs/superpowers/specs/2026-08-28-m8-colluvium-design.md`
- Add: `docs/superpowers/plans/2026-08-28-m8-colluvium.md`

**Step 1: Check document whitespace**

Run a no-index whitespace check over each new file.

Expected: no whitespace diagnostics; exit `1` is expected because each file is
new and differs from the null source.

**Step 2: Inspect only owned paths**

Run:

```powershell
git status --short -- docs/evidence/M8_GEOLOGY_BLUEPRINT_P1_COLLUVIUM_RESEARCH.md docs/superpowers/specs/2026-08-28-m8-colluvium-design.md docs/superpowers/plans/2026-08-28-m8-colluvium.md
```

Expected: exactly the three owned untracked files.

**Step 3: Commit and publish the checkpoint**

Stage only the three paths, inspect the cached diff, commit:

```text
docs(geology): design bounded colluvium deposition
```

Push `main`, then prove `HEAD == main == origin/main` and divergence `0/0`.

## Task 2: Freeze the CPU reference contract with observable RED

**Files:**

- Add: `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/Geology/GeologyColluvium.hpp`
- Modify: `Tests/ProceduralAuthoring/GeologyRecipeTests.cpp`

**Step 1: Write the failing tests first**

Add focused CPU tests that include the not-yet-existing Colluvium contract and
require:

- one detached unit to settle on supported material;
- a second unit to settle on the first deposit and form a pile;
- exact count and volume conservation;
- unchanged placements and identity under source permutation;
- RBrush/mask-protected destinations to remain untouched;
- explicit unsettled material at zero/insufficient distance;
- rejection of non-cardinal gravity, inconsistent Rockfall count, coordinate
  overflow, invalid budget, and non-finite inputs;
- cancellation and work-budget exhaustion to publish no partial result.

Invoke the new functions from the existing recipe/reference test main, adjacent
to the Rockfall reference tests.

**Step 2: Run RED and retain the failure**

Build only `JericoGeologyRecipeTests` in Debug.

Expected RED: compile failure because `GeologyColluvium.hpp` or its required
types/functions do not yet exist. Record the exact compiler failure in the
closure evidence checkpoint; do not weaken the test.

**Step 3: Implement the minimum reference contract**

In `GeologyColluvium.hpp`, add only:

- payload-independent request/result/placement/diagnostic types;
- cardinal-gravity and checked-bound validation;
- canonical source derivation from Rockfall output;
- reserved-source, bounded BFS settling;
- target-mask callback and cancellation;
- exact deposited/unsettled count, volume, and identity;
- the fixed `64`-cell transport ceiling and `67,108,864`-visit productive
  ceiling, with a smaller injectable test budget;
- no production field mutation and no new dependency.

Use the existing Rockfall key, hash, cancellation, and bounded-work conventions
where their contracts already fit; do not modify Rockfall semantics.

**Step 4: Run GREEN for the focused CPU contract**

Build the target and run:

```powershell
build/windows-vulkan-dev/Tests/ProceduralAuthoring/Debug/JericoGeologyRecipeTests.exe
```

Expected: all Colluvium CPU assertions pass and the executable exits `0`.

**Step 5: Inspect diff and commit**

Run focused `git diff --check`, inspect only the header and test hunks, then
commit:

```text
feat(geology): add conserved colluvium settling reference
```

Push and verify the published boundary.

## Task 3: Admit recipe schema 20 and terminal ordering with RED

**Files:**

- Modify: `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/Geology/GeologyTypes.hpp`
- Modify: `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/Geology/GeologyRecipe.hpp`
- Modify: `Tools/ProceduralAuthoring/Source/Geology/GeologyRecipe.cpp`
- Modify: `Tests/ProceduralAuthoring/GeologyRecipeTests.cpp`

**Step 1: Write recipe RED**

Add tests for:

- `GeologyLayerKind::Colluvium` and schema version `20`;
- canonical payload round-trip and recipe identity;
- accepted active `Rockfall -> Colluvium` sequence;
- accepted standalone Rockfall;
- rejection of Colluvium without preceding active Rockfall;
- rejection of another active layer between/after the pair;
- inactive layers not interrupting the active relationship;
- finite/non-negative distance, normalized cutoff, and cardinal inherited
  Rockfall gravity validation.

**Step 2: Run RED**

Build only `JericoGeologyRecipeTests`.

Expected RED: missing enum/payload/schema and ordering support.

**Step 3: Implement the minimum schema change**

Add enum value `24`, Colluvium payload, schema-20 parse/write/hash support, and
the narrow active-order state machine. Do not broaden any other layer contract.

**Step 4: Run GREEN**

Run the Debug recipe test executable directly and the focused recipe CTest if
registered.

Expected: all assertions pass.

**Step 5: Inspect and commit**

Run `git diff --check`, inspect the four owned files, commit:

```text
feat(geology): admit terminal colluvium recipe layer
```

Push and verify the published boundary.

## Task 4: Freeze zero-expansion topology with RED

**Files:**

- Modify: `Tools/ProceduralAuthoring/Source/Geology/Backends/GpuDiligent/GeologyGpuTopologyPlan.cpp`
- Modify: `Tests/ProceduralAuthoring/GeologyGpuTopologyPlanTests.cpp`

**Step 1: Write topology RED**

Add a schema-20 Colluvium test proving:

- no sparse leaf allocation or halo expansion;
- deterministic unchanged topology identity;
- the layer remains admitted after its Rockfall predecessor.

**Step 2: Run RED**

Build only `JericoGeologyGpuTopologyPlanTests` and run the executable.

Expected RED: missing explicit Colluvium handling or missing schema contract.

**Step 3: Implement explicit zero expansion**

Add the narrow planner case if required. Do not reuse heightfield erosion or add
a generic transport halo.

**Step 4: Run GREEN and commit**

Run the executable, `git diff --check`, inspect the two paths, commit:

```text
test(geology): freeze colluvium topology boundary
```

If the existing implementation already passes after the RED test is compiled,
retain the test and commit only the contract; do not add inert production code.

## Task 5: Integrate the productive Diligent session with RED

**Files:**

- Modify: `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/Geology/GeologyGpuSession.hpp`
- Modify: `Tools/ProceduralAuthoring/Source/Geology/Backends/GpuDiligent/GeologyGpuSessionDiligent.cpp`
- Modify: `Tests/ProceduralAuthoring/GeologyGpuSessionTests.cpp`

**Step 1: Write productive RED**

Add `--colluvium` focused Vulkan coverage requiring a real productive session
to expose:

- the preceding Rockfall component/count/volume identity;
- positive deposited count and exact deposited/unsettled conservation;
- deposited field cells and the existing deposition classification byte;
- unchanged protected cells;
- repeated Colluvium result/field identity;
- stage capture and typed work diagnostics;
- final classified mesh and collision compatibility;
- no second Rockfall-field readback for Colluvium.

Add a negative recipe/cancellation case proving no partial stage publication.

**Step 2: Run RED**

Build the Debug session test target and run `--colluvium`.

Expected RED: missing diagnostics and productive layer handling.

**Step 3: Implement the minimum productive integration**

- retain Rockfall's CPU field/classification/source snapshot only when an active
  Colluvium successor exists;
- invoke the reference operation with the inherited gravity, existing topology,
  RBrush evaluator, cancellation, and checked work budget;
- apply placements to a temporary CPU field/classification copy;
- preserve source classification, mark normalized deposition byte `255`, clear
  the deposited cell's fresh bit, and accumulate the existing field identity;
- publish diagnostics and upload/capture only after full success;
- release temporary state after the terminal layer;
- do not add runtime, physics, UI, generator, or alternate cook systems.

**Step 4: Run focused GREEN**

Run the Debug `--colluvium` test twice and compare emitted identities. Run the
existing `--rockfall` and `--rockfall-cook` focused modes to prove the published
Rockfall path remains intact.

**Step 5: Inspect and commit**

Run focused `git diff --check`, inspect the three owned files, commit:

```text
feat(geology): integrate productive colluvium bake
```

Push and verify the published boundary.

## Task 6: Prove cook/collision/runtime compatibility and performance bounds

**Files:**

- Modify only if the existing focused harness needs Colluvium selection:
  `Tests/ProceduralAuthoring/GeologyGpuSessionTests.cpp`
- Add stable evidence outputs under the slice-specific existing evidence
  convention; do not touch foreign capture inventory.

**Step 1: Run the focused Debug gates**

Build and run all three owned test executables:

- `JericoGeologyRecipeTests` for CPU reference and recipe/schema contracts;
- `JericoGeologyGpuTopologyPlanTests`
- `JericoGeologyGpuSessionTests --colluvium`
- existing `--rockfall` and `--rockfall-cook` regression modes.

**Step 2: Run Release productive proof**

Build the same targets in Release and execute Colluvium on the existing
representative productive Vulkan field/cook route. Capture actual typed counts,
volumes, hashes, work items, mesh/index counts, collision identity, and cook
identity. Repeat from a fresh process and compare.

**Step 3: Inspect visual/physical usefulness where the harness supports it**

Use the existing representative classified product/capture route. A debug mask
or non-empty image alone is not acceptance. Retain a visual only if it clearly
shows a static pile/deposit changing final geometry; otherwise report the exact
evidence limit and do not call the visual criterion PASS.

**Step 4: Run broader relevant CTest selection**

Run registered geology recipe, topology, session, Surface Nets/JRMS,
collision, and cook tests that consume the modified contracts. Do not run or
stage unrelated foreign suites merely to normalize the checkout.

**Step 5: Commit stable evidence only**

Stage only newly owned, reproducible evidence paths and any necessary focused
harness hunk; preserve the 182-entry foreign inventory exactly.

## Task 7: Repeat the architecture and lineage audit, then close the slice

**Files:**

- Modify: `docs/JERICO_PROCEDURAL_GEOLOGY_RESEARCH_IMPLEMENTATION_BLUEPRINT.md`
- Modify: `docs/evidence/M8_GEOLOGY_SYSTEM_COMPLETENESS_AUDIT.md`
- Add: `docs/evidence/M8_GEOLOGY_BLUEPRINT_P1_COLLUVIUM_CLOSURE.md`
- Modify: `docs/work/ACTIVE_SLICE.md`
- Modify: `docs/work/HANDOFF.md`

**Step 1: Reconcile implementation against design**

Record the actual schema/kind, algorithm ceiling, conservation equations,
identity, cancellation/bounds, productive metrics, material/collision/cook
lineage, and every honest evidence limitation. Update blueprint sections 30,
45, 52, and 88 only where the implemented facts change their status.

**Step 2: Repeat the system completeness audit**

Evaluate every category named by the root contract. Retain runtime geological
modification/destruction and arbitrary gravity as explicit
`INTEGRATION REQUIRED`; do not implement them.

**Step 3: Run final verification from the published candidate**

Run:

- focused Debug and Release tests;
- relevant registered CTests;
- `git diff --check`;
- owned-path diff review;
- status/inventory count and the canonical 182-entry foreign-inventory SHA-256;
- `git rev-parse HEAD`, local/remote refs, divergence, and relevant process
  inventory.

**Step 4: Publish closure**

Commit the truthful audit/closure/status checkpoint, push `main`, fetch, and
prove `HEAD == main == origin/main == remote main` with divergence `0/0`.

Colluvium may become local `PASS`; P1, the full Geology blueprint, and the
Persistent Goal remain incomplete.

**Step 5: Perform exactly one legal successor handoff**

Only after published closure:

1. set the next single `ACTIVE_SLICE` from existing durable authority without
   implementing it here;
2. create exactly one fresh top-level successor task, never a fork/worktree;
3. require explicit acceptance of exclusive checkout/main ownership, the exact
   active slice, preserved foreign inventory, and Persistent Goal continuity;
4. verify acceptance once, then renounce ownership immediately and permanently;
5. stop this task without executing the successor slice.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
