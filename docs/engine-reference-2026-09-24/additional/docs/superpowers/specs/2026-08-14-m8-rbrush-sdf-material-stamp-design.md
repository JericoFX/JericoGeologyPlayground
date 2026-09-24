# M8 RBrush Sparse SDF + Material Stamp — Design

**Date:** 2026-08-14

**Status:** accepted under the standing autonomous-decision directive

**Milestone:** M8

**Slice:** 31 — Combined Geometry/Material Stamp

## 1. Observable result

One accepted RBrush action creates both parts of a crack:

- a real Sparse SDF subtraction that changes the closed replacement mesh; and
- coordinated PBR surface detail using the same brush, seed and spatial frame.

The action previews without mutation and applies, undoes and redoes atomically.
Its dirty region invalidates the bounded geometry, collision, navigation,
surface, shadow, GI and World Cell dependants required by the Virtualized World
contract.

This slice does not add UI, a new procedural tab, a cook format or a runtime
RBrush dependency. RBrush remains editor authoring input. Static derived output,
cook/package and source-free Shipping proof remain the later Slice 34/35 gate.

## 2. Reused authorities

No geometry or paint algorithm is duplicated:

- `SparseSdfSession::previewBrushCrack` owns projection, stamp application,
  closed mesh regeneration and Sparse SDF history validation.
- `SurfacePaint::evaluateMaterialPaint` owns atomic multi-texture PBR evaluation.
- `DirtySpatialRegion` remains the shared invalidation contract.
- `RBrush` remains a brush/preset reference. Material payload does not enter the
  `.rbrush` format.
- Procedural 3D Layout remains a future PWAS mode in the existing World shell.
  It may later consume this dressing action after materialization, but Slice 31
  does not implement or fork the layout generator.

## 3. Ownership decision

Add a Qt-free `RBrushSdfMaterialStampSession` to
`JericoEditorProjectAuthority`.

This editor-owned coordinator is the shallow policy layer allowed to combine two
deep modules. It owns one moved `SparseSdfSession` and one immutable resolved
material generation. It never exposes mutable access to the inner session.

Rejected alternatives:

1. Putting PBR material state in `SparseSdfSession` would couple the canonical
   geometry authority to Surface Paint and runtime material formats.
2. Dispatching two independent transactions would permit partial apply and two
   unrelated undo histories for what the designer sees as one action.
3. Introducing a generic cross-tool transaction framework in this slice would
   be speculative and larger than the required behavior.

## 4. Coordinated request

`RBrushSdfMaterialStampRequest` contains:

- one action GUID;
- one non-zero seed;
- one `SdfBrushCrackRequest`;
- one `MaterialPaintEvaluationRequest`.

The coordinator rejects the request unless:

- action GUID equals the SDF operation GUID and material layer GUID;
- SDF RBrush asset/preset identity equals the resolved material brush reference;
- the material layer contains exactly one stroke and it references the same
  brush and seed;
- material projection axes match the SDF tangent/bitangent frame;
- material origin is the derived lower-left corner of the centered SDF mask and
  its extents equal the SDF mask metric in world meters;
- both subsystem requests pass their existing validation and produce a change.

The projection comparison uses a small fixed numerical tolerance only for the
equivalent `double` representations. It does not silently remap mismatched
authoring input.

## 5. Preview and immutable generations

`preview` evaluates both existing backends against the current immutable state
and returns:

- the ordinary `SdfBrushCrackPreview`;
- the ordinary `MaterialPaintEvaluationResult`;
- an immutable resolved material result assembled from the returned textures;
- before/after geometry, material and combined state hashes;
- expected session revision;
- the common action GUID and seed;
- one combined dirty spatial region.

The current material GUID and canonical `.rmat` content hash stay stable: the
paint operation creates new resolved texture generations, not a different
material asset identity. Each changed texture receives a deterministic content
hash over its canonical identity, format metadata, dimensions and bytes. A
material-state hash covers the material identity, PBR data and textures in
canonical binding order.

The combined state hash contains the accepted geometry state hash and resolved
material state hash. Action GUID, seed and dirty-region metadata are validated
separately as transaction provenance; they do not make identical accepted bytes
hash differently merely because the edit arrived through another history path.

Preview owns its immutable snapshots. A later preview or caller lifetime cannot
alter an earlier result.

## 6. Atomic apply and history

Before mutation, `apply` validates:

- expected revision;
- exact before geometry/material/combined hashes;
- exact current identities;
- the retained preview result and dirty region;
- the inner SDF preview through the existing `SparseSdfSession` validation.

Only after all fallible coordinator checks succeed does it apply the inner SDF
change and swap the material snapshot. The coordinator exclusively owns the
inner SDF session, and the material pointer swap cannot fail, so no partially
visible result exists.

One history entry stores immutable before/after material snapshots plus the
coordinated metadata. `undo` and `redo` invoke the matching inner SDF history
operation and then restore the exact material snapshot. Stale previews are
rejected without changing either side. A new apply clears redo.

History is bounded to 16 entries and 512 MiB of uniquely retained material
texture bytes, matching the existing Sparse SDF session scale. When necessary,
the oldest coordinated history entry is discarded; the live state is never
discarded.

## 7. Dirty-region contract

The combined bounds are the component-wise union of the SDF affected bounds and
Surface Paint dirty bounds. Dependencies are exactly:

```text
GeometryPages
| Collision
| Navigation
| SurfacePages
| ShadowPages
| GiBricks
| WorldCellDependencies
```

The result must pass `validateDirtySpatialRegion`. The coordinator reports one
bounded change rather than parallel invalidation scanners.

## 8. Error, cancellation and threading behavior

Preview receives the existing cancellation token and performs no accepted-state
mutation. Cancellation, invalid provenance, mismatched transforms, no-effect
paint, no-effect geometry or backend validation failure returns an explicit
error.

This slice adds no worker or Qt callback. A later UI/job owner may run preview on
immutable inputs and perform the short accepted commit on the owning thread under
the existing C04 generation-guard contract.

## 9. RED/GREEN proof

The focal product test will use a real accepted Sparse SDF session plus resolved
multi-channel material fixtures and prove:

1. one action changes the closed SDF mesh and coordinated BaseColor,
   Roughness/ORM, Normal and Material Height data;
2. the same brush identity, seed and spatial frame govern both outputs;
3. repeated preview is deterministic and non-mutating;
4. apply, undo and redo restore exact combined hashes and material bytes;
5. a stale preview and every coordination mismatch produce zero partial change;
6. the combined dirty region is bounded and carries all seven dependency bits;
7. cancellation leaves the session unchanged;
8. retained evidence shows the geometry and PBR detail produced by the same
   action.

The focal CTest is `editor.rbrush_sdf_material_stamp_session`. Compilation or a
log line alone is not PASS.

## 10. Completion boundary

Slice 31 is PASS only after the focal RED becomes GREEN, relevant SDF and Surface
Paint regressions remain GREEN, a retained deterministic visual/result artifact
exists, and the product code is committed and published.

It does not complete M8 or the JericoEngine 1.0 Goal.
