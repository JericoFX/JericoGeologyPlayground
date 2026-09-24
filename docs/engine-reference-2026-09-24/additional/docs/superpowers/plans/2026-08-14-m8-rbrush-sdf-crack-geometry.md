# M8 RBrush Slice 30 — Sparse SDF Crack Geometry Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use
> superpowers:executing-plans to implement this plan task-by-task in the single
> authoritative task and checkout. Do not create a task, branch or worktree.

**Goal:** Make an uploaded-style RBrush crack change actual closed Sparse SDF
rock geometry and pass through the existing Apply/Undo/Redo transaction.

**Architecture:** Extend `SparseSdfSession` with one crack-specific Preview
operation. It composes the already accepted Slice 27 mask, Slice 29 projection,
Slice 28 stamp, existing mesher and existing BSP/SDF transition into an ordinary
`SdfChangePreview`; current Apply/Undo/Redo remain the only history authority.
The focal proof uses a deterministic embedded crack for CTest and optionally
decodes the retained local Photoshop ABR to produce inspected before/after BMP
evidence from the actual fields.

**Tech Stack:** C++20, `Jerico::ProceduralAuthoring`, admitted ABR decoder,
`SparseSdfField`, Marching Cubes, CMake/CTest, existing transaction hashes.

## Global Constraints

- `HEAD` and `origin/main` start equal at `ef9f97a`; design commit is
  `a5e4b79` until published.
- Preserve all twelve tracked user modifications, `.codex-remote-attachments/`,
  the untracked RBrush capture, JericoBSP JPG and concept images `01..14`.
- Modify the mixed `Tests/ProceduralAuthoring/CMakeLists.txt` only by selective
  staging of the new target/test hunk.
- No UI, picking, material companion, DDC, cook, Shipping RBrush or Layout 3D.
- The operation must be `CrackCutter`; do not expose every Slice 28 operation
  through the transaction in this slice.
- The local ABR is evidence input only and never enters Git or Shipping.
- The master Goal, M8, Sparse SDF 2.0 and the RBrush family remain incomplete.

---

### Task 1: Register the real transaction/geometry RED

**Files:**

- Create: `Tests/ProceduralAuthoring/SdfBrushCrackGeometryTests.cpp`
- Modify selectively: `Tests/ProceduralAuthoring/CMakeLists.txt`
- Modify: `Tests/TestSchedulingPolicy.cmake`
- Modify: `Tests/RepositoryContract/VerifyTestScheduling.cmake`

**Interfaces:**

- Consumes: existing `SparseSdfSession`, `MaskSignedDistanceField2D`,
  `SdfBrushProjection`, `SdfBrushStamp`, `meshSparseSdf` and ABR decoder.
- Produces: test target `JericoSdfBrushCrackGeometryTests` and CTest
  `procedural.sdf_brush_crack_geometry`.

- [x] **Step 1: Write the failing session transaction test**

  Build a real fine-cell `SparseSdfSession`, accept its initial preview, derive
  a hand-authored 33x33 branching R8 crack and call the wished-for API:

  ```cpp
  const auto crack = session.previewBrushCrack(
      SdfBrushCrackRequest{
          .operationGuid = guid(0xA0U),
          .stampRegionGuid = guid(0xA1U),
          .provenance = provenance(mask),
          .mask = mask,
          .frame = frontFaceFrame(),
          .affectedBounds = frontFaceBounds(),
          .candidateBricks = candidates,
          .projection = crackProjection(),
          .stamp = SdfBrushStampSettings{
              .operation = SdfBrushOperation::CrackCutter,
              .smoothK = 0.01F}},
      cancellation.token());
  TEST_CHECK(crack.hasValue());
  TEST_CHECK(crack.value().changedSamples > 0U);
  TEST_CHECK(crack.value().beforeMeshHash !=
             crack.value().afterMeshHash);
  ```

  The mutation caught is removal of the session composition seam: compilation
  must fail because `previewBrushCrack` and its request/result types do not
  exist.

- [x] **Step 2: Require real Apply/Undo/Redo behavior**

  Assert Preview is immutable, Apply installs the exact after hashes, Undo
  restores the exact before result/field/mesh hashes, and Redo restores the
  identical accepted after generation without recomputation.

- [x] **Step 3: Require deterministic geometry and failure behavior**

  Repeat Preview and require identical projection/stamp/field/mesh hashes.
  Require no accepted result, non-`CrackCutter`, empty/outside projection,
  zero-change, pre-cancellation and stale accepted Preview to fail without
  changing revision/content/history.

- [x] **Step 4: Register the unscheduled test and observe scheduling RED**

  Run:

  ```powershell
  cmake --preset windows-vulkan-editor
  ```

  Expected: `repository.test_scheduling.full_inventory_changed`, count
  `288 -> 289`, with the exact new SHA-256 printed by CMake.

- [x] **Step 5: Admit the exact inventory and observe product RED**

  Update both scheduling authorities with count `289` and the exact printed
  hash, reconfigure, then run:

  ```powershell
  cmake --build --preset windows-vulkan-editor `
    --target JericoSdfBrushCrackGeometryTests --parallel 4
  ```

  Expected: C++ compile failure naming missing
  `SdfBrushCrackRequest`/`previewBrushCrack`, not a fixture or syntax error.

---

### Task 2: Add the crack-specific session Preview contract

**Files:**

- Modify:
  `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/SparseSdfSession.hpp`
- Modify: `Tools/ProceduralAuthoring/Source/SparseSdfSession.cpp`
- Test: `Tests/ProceduralAuthoring/SdfBrushCrackGeometryTests.cpp`

**Interfaces:**

- Produces: `SdfBrushCrackRequest`, `SdfBrushCrackPreview` and
  `SparseSdfSession::previewBrushCrack` exactly as frozen in the design.
- Preserves: `SparseSdfSession::apply`, `undo`, `redo`, history limits and
  accepted-preview validation.

- [x] **Step 1: Declare the public types and method**

  Include `SdfBrushProjection.hpp`; add the crack request/result structs and
  const Preview method without exposing a caller-provided source field.

- [x] **Step 2: Add only crack-specific entry validation**

  Return `procedural.sdf.session_crack_unavailable` when `current_.result` is
  null. Return `procedural.sdf.session_crack_invalid` unless the operation is
  exactly `CrackCutter`. Check cancellation before each heavy stage.

- [x] **Step 3: Compose projection and stamp**

  Construct the projection request with
  `current_.result->replacementField`, call
  `projectSdfBrushToSparseField(..., cancellation.stopToken())`, reject empty
  points, then call `applySdfBrushStamp` with the same current field and stop
  token. Reject `changedSamples == 0`.

- [x] **Step 4: Remesh and rebuild the transition**

  Mesh the stamped field, require `validateClosedMesh`, reuse
  `selectedFacePlane`, `selectedPatch` and `buildSdfTransition` with the current
  recipe's transition/protection settings. Do not copy those algorithms into a
  second module.

- [x] **Step 5: Build the immutable accepted result**

  Copy the unchanged environment field, move the stamped replacement field and
  rebuilt geometry, recompute `authoringMetrics`, `authoringHash`, `stateHash`,
  `impactFor` and `affectedFor`, and return both `SdfChangePreview` and the
  observable counters/hashes.

- [x] **Step 6: Run the focal GREEN**

  ```powershell
  cmake --build --preset windows-vulkan-editor `
    --target JericoSdfBrushCrackGeometryTests --parallel 4
  ctest --test-dir build/windows-vulkan-editor -C Debug `
    -R "^procedural\.sdf_brush_crack_geometry$" --output-on-failure
  ```

  Expected: `1/1 PASS`, non-zero changed samples, different closed mesh hash,
  exact Apply/Undo/Redo hashes.

---

### Task 3: Produce actual local-ABR visual evidence

**Files:**

- Modify: `Tests/ProceduralAuthoring/SdfBrushCrackGeometryTests.cpp`
- Create from product execution:
  `docs/evidence/images/m8-rbrush-sdf-crack-geometry.bmp`

**Interfaces:**

- Consumes optional command line arguments `<abr-path> <evidence-bmp>`.
- Produces a fixed-layout BMP from the decoded mask and before/after SDF fields;
  no source ABR bytes are copied.

- [x] **Step 1: Decode the real first crack preset**

  Read the admitted local fixture, call `decodeAbrSamples`, require 15 samples,
  select sample 0, require R8 and derive its field through Slice 27.

- [x] **Step 2: Add deterministic SDF raycast panels**

  For each pixel, cast along the selected face normal through the actual before
  and after fields, locate the first sign crossing and shade it from the field
  normal plus depth. Write three labelled-by-layout panels: mask, before rock,
  after crack. The test asserts a minimum count of pixels whose hit depth moved
  inward in the after panel.

- [x] **Step 3: Execute the real ABR proof**

  ```powershell
  & build/windows-vulkan-editor/Tests/ProceduralAuthoring/Release/JericoSdfBrushCrackGeometryTests.exe `
    'build/local-fixtures/abr/Free Cracks Photoshop Brushes 10.abr' `
    'docs/evidence/images/m8-rbrush-sdf-crack-geometry.bmp'
  ```

  Expected: exit `0`, fifteen decoded presets, non-zero projected/changed
  samples, distinct field/mesh hashes, non-zero depth-changed pixels and a valid
  retained BMP.

- [x] **Step 4: Inspect the retained image**

  Open the BMP and require that the localized crack is visibly absent before
  and present after. A mask-only or identical before/after image is RED.

---

### Task 4: Verify integration and publish the checkpoint

**Files:**

- Create: `docs/evidence/M8_RBRUSH_SDF_CRACK_GEOMETRY.md`
- Modify: `docs/CAPABILITY_MAP.md`
- Modify: `docs/superpowers/THREAD_HANDOFF.md`
- Modify this plan to mark completed steps.

**Interfaces:**

- Evidence records RED, exact hashes/counters, image SHA-256 and commands.
- Handoff leaves Slice 31 Combined Geometry/Material Stamp as the only next
  action.

- [x] **Step 1: Run Debug/Release focal and SDF neighbours**

  Build/run the focal in both configurations and run the Slice 27/28/29 plus
  existing Sparse SDF recipe/field/builder/mesher/transition/session/cook
  tests. Require zero failures.

- [x] **Step 2: Run integrated and boundary gates**

  Run the complete Debug preset build, scheduling contract and the established
  runtime-only/source-exclusion/authority/Editor-boundary/JericoBSP/PWAS/
  Spline/Sparse-SDF Shipping matrix. Use a timeout above the known 270-second
  Editor boundary rebuild so the final command itself returns a 12/12 summary.

- [x] **Step 3: Review and commit product selectively**

  Inspect the entire diff, stage only the new hunk from the mixed test CMake,
  run `git diff --cached --check`, and commit:

  ```text
  feat(procedural): transact rbrush cracks on sparse sdf
  ```

- [x] **Step 4: Retain documentation and image**

  Record non-claims: no UI, material detail, DDC, cook/runtime RBrush, Layout
  3D, Sparse SDF 2.0, M8 or master Goal completion. Commit:

  ```text
  docs(rbrush): retain sparse sdf crack geometry checkpoint
  ```

- [x] **Step 5: Publish and reconcile**

  Push `main`, verify `HEAD == origin/main`, verify the index is empty and list
  every preserved user-owned path class still outside Git.

## Stop condition

Stop Slice 30 after the real ABR crack is visibly and structurally present in
closed remeshed SDF geometry and exact Apply/Undo/Redo is proven. Do not begin
Slice 31 combined material detail in the same checkpoint.
