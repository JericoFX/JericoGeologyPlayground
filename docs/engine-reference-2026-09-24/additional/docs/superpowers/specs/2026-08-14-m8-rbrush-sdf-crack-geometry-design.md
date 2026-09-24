# M8 RBrush Slice 30 — Sparse SDF Crack Geometry Design

**Date:** 2026-08-14

**Status:** approved design under the user's autonomous 30-second decision rule

**Target:** JericoEngine 1.0 / M8 / RBrush Slice 30

**Scope:** one uploaded-style crack cuts actual Sparse SDF rock geometry and is
reversible through the existing Sparse SDF authoring transaction

## Product decision

Slice 30 extends `SparseSdfSession`. It does not introduce another edit
session, history stack, editor tab, field representation, projection path or
stamp implementation.

The accepted pipeline is:

```text
admitted RBrush crack mask
-> Slice 27 MaskSignedDistanceField2D
-> Slice 29 Surface Tangent projection over current replacementField
-> Slice 28 CrackCutter
-> validated SparseSdfField generation
-> Marching Cubes remesh
-> rebuilt BSP/SDF transition
-> existing SdfChangePreview
-> existing Apply / Undo / Redo
```

The mask, current field and accepted before-generation remain immutable.

## Alternatives rejected

### Separate brush-field history

A small `SdfBrushEditSession` would be easier to implement, but it would create
a second transaction authority beside `SparseSdfSession`. It is rejected.

### Test-only field swapping

Manually replacing before/after fields could make a visual test pass without
exercising production Apply/Undo/Redo. It is rejected as false evidence.

### UI-first integration

Adding picking or a new crack panel before a complete transaction would mix
World/Qt ownership with the missing product seam. Slice 30 remains Qt-free;
later UI must call this same production operation.

## Public contract

`SparseSdfSession.hpp` adds:

```cpp
struct SdfBrushCrackRequest
{
    Guid operationGuid;
    Guid stampRegionGuid;
    SdfBrushStampProvenance provenance;
    const MaskSignedDistanceField2D& mask;
    SdfBrushProjectionFrame frame;
    Bounds3d affectedBounds;
    std::span<const SdfBrickCoord> candidateBricks;
    SdfBrushProjectionSettings projection;
    SdfBrushStampSettings stamp;
};

struct SdfBrushCrackPreview
{
    SdfChangePreview change;
    std::uint64_t projectedPoints{};
    std::uint64_t changedSamples{};
    std::uint64_t projectionHash{};
    std::uint64_t stampOperationHash{};
    std::uint64_t beforeFieldHash{};
    std::uint64_t afterFieldHash{};
    std::uint64_t beforeMeshHash{};
    std::uint64_t afterMeshHash{};
};

Result<SdfBrushCrackPreview> previewBrushCrack(
    const SdfBrushCrackRequest& request,
    const CancellationToken& cancellation) const;
```

The method requires an already accepted `SdfAuthoringResult`. It injects the
session's current `replacementField` as the projection/stamp source. Callers
cannot provide another field or bypass current-generation ownership.

`stamp.operation` must be exactly `CrackCutter`. Other Slice 28 operations do
not enter the transaction through this Slice 30-specific method.

## Preview construction

1. Reject cancellation or a session with no accepted result.
2. Reject any operation other than `CrackCutter`.
3. Construct `SdfBrushProjectionRequest` from the current replacement field and
   the request's immutable mask/frame/bounds/candidates/settings.
4. Project through `projectSdfBrushToSparseField` using the cancellation
   token's `std::stop_token`.
5. Reject an empty projected point set.
6. Apply the points through `applySdfBrushStamp` using the same stop token.
7. Reject a zero-change stamp.
8. Mesh the returned field through `meshSparseSdf`; require a closed result.
9. Recompute the selected source face, selected replacement patch and
   `SdfTransitionResult` using the existing session helpers and unchanged
   recipe/locks/no-generate regions.
10. Recompute metrics, result hash and state hash through the existing
    `SdfAuthoringResult` rules.
11. Return an ordinary `SdfChangePreview` plus observable projection/stamp/mesh
    counters and hashes.

Preview does not mutate the session and returns no partial publication.

## Transaction behavior

`SparseSdfSession::apply`, `undo` and `redo` remain unchanged and authoritative.

- Apply admits the immutable crack result into the existing 16-entry,
  512-MiB-bounded history.
- Undo restores the exact prior recipe/result/content hashes and field/mesh
  generation.
- Redo restores the exact crack generation rather than recomputing it.
- A stale preview is rejected by the current revision/hash guards.
- A new accepted branch after Undo clears Redo through existing behavior.

The recipe hash remains stable because this slice applies a transient
authoring operation to the accepted SDF generation. Slice 35 owns static cook
and any durable recipe serialization needed to remove the RBrush dependency.

## Geometry proof

The normal automated fixture uses a deterministic branching crack mask and a
fine-cell rock-like SDF replacement produced through the existing BSP-to-SDF
session path. It proves:

- projected point count and changed sample count are non-zero;
- before and after field hashes differ;
- before and after mesh hashes differ;
- the crack converts selected near-surface solid samples to outside;
- both meshes are closed;
- Apply/Undo/Redo restore exact hashes and shared immutable generations;
- repeated preview produces identical projection/stamp/field/mesh hashes.

An optional evidence invocation consumes the retained local Photoshop crack
ABR at:

```text
build/local-fixtures/abr/Free Cracks Photoshop Brushes 10.abr
SHA-256 0D8875E5E19228A828F3A259348B8B94DB4584849F33BFC66DD8287FB65FEE9D
bytes 23,435,296
```

It decodes the real first R8 preset through the admitted ABR decoder, derives
the Slice 27 field, performs the same production transaction and writes a
retained BMP with source mask plus before/after orthographic SDF raycasts. The
after panel must show a spatially localized deeper surface while the before
panel does not. Mesh hashes/counters printed beside the artifact establish that
the visible result corresponds to remeshed geometry, not an overlay.

The ABR remains outside Git and is never copied to evidence or Shipping.

## Errors and cancellation

New session errors use stage `procedural.sdf.session`:

- `procedural.sdf.session_crack_unavailable` when no accepted SDF generation
  exists;
- `procedural.sdf.session_crack_invalid` for a non-crack operation, empty
  projection, zero-change result or invalid reconstructed geometry;
- existing projection/stamp/mesher/transition errors propagate unchanged;
- existing `procedural.sdf.cancelled` is returned when cancellation is already
  requested between pipeline stages.

No exception, validation failure or cancellation mutates session/history.

## Procedural 3D Layout boundary

This operation is a materialization/refinement consumer. It does not create or
modify `Procedural3DLayout`, route connections or generate a layout by editing
SDF bricks. Future PWAS Layout materialization may call the same SDF authority
after logical layout validation and Preview/Accept.

No tab, workspace, World session or procedural framework is added.

## Non-goals

- no combined material detail; Slice 31 owns it;
- no Rock Generator brush set; Slice 32 owns it;
- no Qt owner, picking or new controls;
- no persistent DDC;
- no `.rbrush` schema change;
- no static SDF cook or runtime RBrush;
- no claim that Sparse SDF 2.0, RBrush, M8 or JericoEngine 1.0 is complete.

## PASS criteria

Slice 30 is PASS only when all of these are freshly demonstrated:

1. a crack mask travels through Slice 27 -> Slice 29 -> Slice 28;
2. actual field samples change through `CrackCutter`;
3. the remeshed closed geometry changes;
4. the existing session Apply/Undo/Redo restores exact generations;
5. deterministic replay produces identical hashes;
6. invalid/stale/cancelled work publishes nothing;
7. the real local crack ABR produces retained, inspected before/after visual
   evidence;
8. Debug, Release, SDF neighbours, integrated build and runtime/Shipping source
   exclusion remain green;
9. RBrush authoring remains absent from Shipping;
10. M8 and the master Goal remain active.
