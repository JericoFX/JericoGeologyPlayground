# M8 RBrush Procedural Source Usage Design

## Status and scope

This design covers RBrush Slice 33 only:

> RBrush functions as a non-mouse mask source for procedural placement.

It does not add editor UI, another World/PWAS workspace, Procedural 3D Layout,
runtime RBrush loading, static cook, Surface Paint cook or Sparse SDF cook.
RBrush remains editor-only authoring data. PWAS remains the placement authority.

## Observable acceptance

An admitted RBrush tip can be bound to a normal procedural PWAS route such as
grid, Poisson or spline scatter. Candidate placement is filtered by the actual
tip coverage projected into World space, without any mouse/tablet stroke or
`HandPaintedSeedScatter` node.

The same recipe, World query, RBrush identity/content and worker count inputs
produce the same placements and bytes. One and four workers produce identical
results. Missing or stale RBrush content fails closed before a result is
accepted.

## Decision

Add one explicit `RBrushMaskFilter` node to the canonical PWAS graph.

The alternatives are rejected:

1. Converting the brush to boxes or pre-expanded seeds would discard grayscale
   detail, duplicate derived data and weaken source traceability.
2. Synthesizing a fake mouse stroke would reuse the hand-painted path, but the
   real tip coverage would not control upstream procedural candidates. That is
   not the Slice 33 behavior.
3. A second procedural evaluator owned by RBrush would duplicate PWAS and break
   the single-authority rule.

The chosen node makes RBrush the mask source while the existing scatter node
remains the candidate source and the existing PWAS evaluator remains the only
placement implementation.

## Canonical data

### Graph node

`PwasRBrushMaskFilter` stores only durable authoring identity and projection:

- `BrushPresetReference` (`AssetGUID + PresetGUID`);
- exact RBrush pack content hash;
- exact `TipGUID` selected by the preset;
- World projection origin and orthonormal U/V axes;
- positive U/V extents;
- threshold in `[0, 1]`;
- gamma in an admitted positive range;
- invert flag.

The graph moves to schema 6 when this node exists. Schema 1-5 readers and
writers retain their existing behavior; schema 5 RBrush stroke bindings remain
valid and byte-stable. An older writer must reject a schema-6 graph rather than
silently remove the mask source.

The graph adds the RBrush asset/content hash to its existing dependency list.
It does not embed tip pixels, `.rbrush` bytes or source-image paths.

### Evaluation query record

`PwasRBrushMaskQueryRecord` is an immutable authoring-time input containing:

- the same brush reference, pack hash and tip GUID;
- bounded tip dimensions;
- canonical R8 coverage bytes.

The Editor adapter builds this record from
`loadProjectRBrushEditablePreset`; ProceduralAuthoring does not depend on
AssetPipeline or parse `.rbrush`.

## Data flow

```text
Admitted .rbrush catalog entry
  -> load exact preset and primary tip
  -> immutable PwasRBrushMaskQueryRecord

PWAS grid / Poisson / spline candidates
  -> project supported World position into the RBrush U/V frame
  -> bilinear sample canonical R8 coverage
  -> invert, gamma and threshold
  -> pass/reject candidate
  -> existing SDF/region filters
  -> existing prefab output
```

Outside the projection rectangle is no coverage and therefore rejects. This
slice uses one clamped stamp; tiling/address modes are not introduced without a
separate requirement.

The node is inserted between the scatter node and existing downstream mask/SDF
filters. It may coexist with the canonical `ProceduralMask`; neither replaces
the other.

## Determinism and identity

Node hashing and recipe serialization include every identity, projection and
sampling parameter. Evaluation provenance includes the exact mask-query
catalog only when a graph uses the new node, preserving historical input hashes
for graphs that do not use RBrush mask sources.

Candidate acceptance is a deterministic threshold test against the sampled
coverage. It does not depend on thread order, wall-clock time or UI state.

The result exposes an `rbrushMaskRejected` statistic. Zero is omitted from the
historical result-hash contribution so existing graph hashes remain stable;
non-zero values participate in new schema-6 result identity.

## Validation and failure behavior

Validation rejects:

- zero GUIDs or zero pack hash;
- absent/stale brush, preset, pack or tip identity;
- duplicate query records;
- zero/oversized dimensions or mismatched byte count;
- non-finite, non-orthonormal or left-handed projection frames;
- invalid extents, threshold or gamma;
- more than one RBrush mask node or an invalid graph route;
- cancellation before a complete result.

All failures return typed `procedural.pwas.rbrush_mask_*` errors and publish no
partial placements or graph mutation.

## Ownership and C04

The adapter and evaluator are Qt-free and accept immutable snapshots plus the
existing cancellation token. Slice 33 adds no synchronous Qt call. A future UI
consumer must run the existing PWAS evaluation through the established C04 job
owner; this slice does not claim that future owner or UI.

## Test strategy

The authentic RED is a new product test that includes the absent
`PwasRBrushMaskSource.hpp` API.

GREEN must demonstrate:

1. a grid/Poisson graph uses the RBrush tip without stroke samples or a
   `HandPaintedSeedScatter` node;
2. an asymmetric real R8 tip accepts the expected side of the World projection
   and rejects the other side;
3. same inputs repeat exactly and workers 1/4 match;
4. invert or changed admitted content produces a different valid result;
5. missing/stale/invalid query records and cancellation fail closed;
6. schema-6 write/read/write and recipe hash are exact;
7. schema-5 RBrush stroke fixtures retain their current bytes and results;
8. scheduling, architecture, runtime-only and Shipping source-exclusion gates
   remain green.

No screenshot is required because this slice is a deterministic authoring
backend contract, not a visual/editor PASS. The retained evidence records exact
accepted/rejected counts, hashes and worker equality.

## Exit and next slice

Slice 33 passes only after the backend, persistence and a real PWAS placement
evaluation are demonstrated. Slice 34 then owns removal of RBrush dependencies
from cooked static Surface Paint output. Slice 35 owns the equivalent static
Sparse SDF result. Neither cook path is part of this design.
