# M8 Colluvium bounded offline/static design

Date: 2026-08-28

Slice: `M8-GEOLOGY-BLUEPRINT-P1-COLLUVIUM`

Design state: accepted by the repository 30-second recommended-choice rule

Selected boundary: `OFFLINE_STATIC_FIELD_COLLUVIUM`

## Local observable goal

Given an active Rockfall result over a classified sparse voxel field, place only
the material units that Rockfall already detached into deterministic stable
positions inside the existing topology, preserve exact integer-volume lineage,
mark the admitted deposition classification, and bake through the already
published Jerico mesh/material/collision/cook path.

The capability is successful only when the productive session demonstrates the
same identity on repeated execution and reports every detached voxel as either
deposited or explicitly unsettled.

## Considered boundaries

### A. Bounded offline/static voxel settling — selected

Consume Rockfall directly, settle detached voxel units inside the existing
sparse domain, then continue through classified Surface Nets and existing cook
consumers. P1 transport accepts axis-aligned gravity only.

This is the smallest coherent vertical slice because it preserves volumetric
Rockfall lineage, requires no alternate runtime system, and can state exact
conservation and failure contracts.

### B. Arbitrary-vector offline/static traversal — rejected for this slice

This would require a new contract for oblique voxel traversal, tie handling,
support contact, and stability. The accessible primary sources do not establish
those semantics, and the extra surface is unnecessary to prove colluvium.

### C. Heightfield erosion deposition adapter — rejected

The existing erosion path owns heightfield water and sediment state. Reusing it
would discard the three-dimensional Rockfall component lineage around caves and
overhangs and create a second transport authority.

## Recipe contract

- Add `GeologyLayerKind::Colluvium` after `Rockfall` and admit recipe schema
  version `20`.
- `Colluvium` may be the next active layer only after exactly one active
  `Rockfall`. Inactive layers do not interrupt that relationship.
- `Colluvium` is terminal among active layers. A recipe containing Rockfall
  without Colluvium remains valid.
- The payload contains only:
  - `maximumTransportDistanceMeters`: finite, non-negative distance;
  - `activationCutoff`: finite normalized destination-mask threshold.
- Gravity is inherited from the preceding Rockfall payload. It is not duplicated
  in Colluvium state or identity.
- The productive P1 contract accepts only an exact signed unit cardinal gravity
  vector. Oblique gravity fails validation visibly; it is not rounded or
  projected silently.
- No seed is present because the algorithm is intentionally deterministic and
  contains no stochastic choice.

Recipe identity continues through the existing canonical recipe serializer and
hash. RBrush payload, common layer strength, order, and active state therefore
remain part of the admitted identity.

## Inputs and ownership

Colluvium consumes these already-owned values from the immediately preceding
productive Rockfall stage:

- Rockfall reference result and diagnostics;
- Rockfall gravity;
- post-Rockfall signed-distance field;
- a snapshot of the pre-placement classification for every detached source;
- sparse leaf topology and voxel size;
- the Colluvium layer's common strength and RBrush evaluator;
- cancellation and a hard work-item budget.

Rockfall remains the sole authority for support discovery, component discovery,
detached membership, detached count, detached volume, and component identity.
Colluvium must reject inconsistent inputs rather than redetect those facts.

The valid placement domain is exactly the allocated sparse topology. The
topology planner reports zero expansion for Colluvium. A legal material unit
that cannot settle in that domain is reported as unsettled; the layer never
allocates hidden leaves.

## Reference result

The independent CPU reference result contains:

- canonical placements `{sourceKey, destinationKey}`;
- deposited and unsettled source counts;
- exact deposited and unsettled volumes;
- canonical deposited and unsettled hashes;
- a result identity hash binding Rockfall identity, payload, placements,
  remainder, topology, and voxel size;
- deterministic work-item diagnostics.

For every successful result:

```text
rockfall.detachedVoxelCount
  == colluvium.depositedVoxelCount + colluvium.unsettledVoxelCount

rockfall.detachedVolume
  == colluvium.depositedVolume + colluvium.unsettledVolume
```

The count equality is authoritative. Volumes are derived as count multiplied by
`voxelSize^3`, with checked finite arithmetic.

## Deterministic settling algorithm

Let `g` be the cardinal integer step in the Rockfall gravity direction.

1. Validate the Rockfall count/volume/result relationship, topology, payload,
   voxel size, gravity, cancellation object, and checked work budget before
   producing output.
2. Derive source keys only from Rockfall voxels changed to non-resident signed
   distance. Their count must equal Rockfall's detached count.
3. Sort sources by descending `dot(key, g)`, which processes the farthest cells
   along gravity first. Break ties by canonical voxel key.
4. Treat all unprocessed source keys as reserved placement targets. This keeps a
   prior material unit from occupying and overwriting another unit's source
   before that unit is processed.
5. For one source, perform a deterministic breadth-first traversal through
   in-domain empty cells. Allowed moves are `g` and the four cardinal directions
   orthogonal to `g`; movement against gravity is forbidden.
6. The maximum number of path steps is
   `floor(maximumTransportDistanceMeters / voxelSize)`, checked against a fixed
   P1 ceiling of `64` cells. Traversal order is canonical and every visited node
   consumes one work item. A successful productive operation may consume at
   most `67,108,864` visits across all sources; a smaller request budget may be
   injected by focused tests.
7. A destination is stable when it is empty, admitted by
   positive layer strength and mask weight with
   `layerStrength * RBrush(destinationCenter) >= activationCutoff`, and
   `destination + g` is occupied. Zero strength or zero mask never deposits,
   including when the cutoff is zero.
8. Rank reachable stable destinations by:
   - greatest progress along gravity;
   - least lateral Manhattan displacement;
   - least path length;
   - canonical destination key.
9. Mark the selected destination occupied before processing the next source, so
   deposits can support later deposits and form a pile. Release the processed
   source reservation.
10. If no admitted stable destination is reachable, add that source to the
    canonical unsettled remainder.

The algorithm models bounded static allocation, not a time integration,
particle trajectory, fluid, granular solver, or runtime simulation.

## Field and semantic-lineage update

The productive integration retains the one Rockfall readback and its CPU field
and classification snapshot for Colluvium; it does not perform a second
readback. After a successful reference result:

1. keep every Rockfall source cell non-resident;
2. write each destination as resident using the detached source's signed-distance
   magnitude with resident sign;
3. copy the source classification snapshot to its destination;
4. set the existing normalized `deposition` classification byte to `255` while
   preserving strata, fracture, erosion, cavity,
   curvature and exposure, and clear the deposited cell's `fresh` bit because
   freshly exposed resident surfaces remain owned by Rockfall;
5. update the normal classified-field identity path;
6. upload the changed field/classification buffers and capture the Colluvium
   stage through the existing productive session;
7. continue through the published classified Surface Nets, JRMS, PBR material,
   collision, cook, and source-free replay paths.

No new material schema, renderer path, shader family, collision representation,
or cook format is introduced.

The classified voxel field retains the exact deposition byte `255`. The existing
Surface Nets path trilinearly interpolates geological classification at generated
vertices, so a deposited boundary proves a positive normalized vertex deposition
value rather than requiring the uninterpolated byte `255` at a vertex.

## Transaction and failure contract

Invalid payload, absent/inconsistent Rockfall lineage, non-cardinal gravity,
topology overflow, coordinate overflow, non-finite volume, work-budget
exhaustion, and cancellation fail the Colluvium operation without publishing a
partial field or partial identity.

An unsettled material unit is not an operation failure. It is a successful,
explicitly measured bounded-domain result and participates in identity.

The existing GeologySession preview/cancel/apply/undo/redo transaction owns the
authoring lifecycle. Colluvium adds no parallel session or UI.

## Observable diagnostics

The productive diagnostics expose at least:

- Rockfall source component count and identity;
- source detached count and volume;
- deposited count, volume, and hash;
- unsettled count, volume, and hash;
- settling result identity;
- work items consumed;
- Colluvium stage capture/dispatch state.

These metrics must be actual typed measurements. A non-empty frame, compile,
parser round-trip, or debug view alone is not acceptance.

## Verification design

### RED 1 — CPU reference contract

Write a failing focused test before implementation that requires:

- detached material falls and rests on supported resident material;
- deposited material supports a later material unit and forms a pile;
- exact deposited/unsettled conservation;
- source permutation does not change placements or identity;
- RBrush-protected destinations remain unchanged;
- distance, cancellation, budget, overflow, and non-cardinal gravity fail or
  remain unsettled according to the contract.

### RED 2 — recipe and topology contract

Require schema-20 canonical round-trip and identity, legal
`Rockfall -> Colluvium` ordering, rejection without Rockfall or after another
active layer, Colluvium terminality, and zero topology expansion.

### RED 3 — productive integration

Require a real Vulkan/Diligent session to demonstrate changed classified field,
deposition classification, exact diagnostics, repeated identity, a classified
mesh/collision result, and compatibility with the existing cook route.

Focused tests precede broader integration and `git diff --check`.

## End-to-end system completeness audit — design pass

| Category | Applicability and selected integration |
| --- | --- |
| Source/input data | Applies: active recipe, RBrush, and preceding Rockfall result. No new source asset. |
| Asset pipeline | Applies: bake through existing classified mesh/JRMS/cook path. |
| Editor/authoring workflow | Applies: existing GeologySession preview/cancel/apply/undo/redo; no UI restoration. |
| ECS/runtime representation | No new mutable component. Result is existing cooked static geometry/classification. |
| CPU processing | Applies: deterministic bounded reference settling. |
| GPU processing | Applies only to the existing productive field stages, upload, capture, and downstream meshing; no new compute algorithm is required. |
| Renderer/shaders/materials | Applies downstream through existing classified mesh/PBR consumers and the admitted deposition byte. No parallel path. |
| Physics/collision | Applies downstream through existing cooked collision generated from final geometry. No runtime structural physics. |
| Animation | Not applicable to static geological authoring. |
| Luau/events/Sequencer | No new command/event or gameplay API is required for this static bake. |
| Serialization/save/load | Recipe schema and canonical serializer apply. Runtime uses existing cooked asset serialization; no save-game state. |
| Streaming/LOD | Applies through existing final JRMS/geometry consumers; no new residency representation. |
| Destruction/runtime modification | Explicitly outside this slice; see `INTEGRATION REQUIRED` below. |
| Navigation/gameplay queries | Final collision/geometry remains available to existing downstream systems; no Colluvium-specific query is introduced. |
| Particles/audio | Not applicable to an offline static operation. |
| Debugging/visualization | Applies through stage capture, typed diagnostics, and evidence; no UI panel is added. |
| Profiling/performance budgets | Applies through deterministic work-item count, hard budget, fixed cell-distance ceiling, and existing field bounds. |
| Testing/documentation/failure | Applies through the three RED gates, evidence, blueprint/audit update, and transactional failures above. |

## Semantic data lineage

```text
Rockfall fracture/support/component result
-> detached source keys + component identity + exact voxel volume
-> bounded deterministic Colluvium placements + explicit unsettled remainder
-> resident signed-distance field + copied geology classification + deposition byte
-> classified Surface Nets mesh
-> JRMS/PBR/collision/cook
-> static visual and physical result
```

Useful upstream information is preserved as follows:

- material classification follows the source voxel to the destination;
- Rockfall component identity remains bound into Colluvium diagnostics/identity;
- detached/deposited/unsettled volume remains explicitly measurable;
- freshly exposed resident surfaces remain owned by Rockfall and are not
  overwritten by deposited source classification;
- the existing deposition channel tells downstream consumers that a final
  surface originated as deposited material.

## INTEGRATION REQUIRED — deliberately deferred

### Runtime geological modification/destruction

- **Why required:** only a future feature that changes baked geology during live
  gameplay would need persistent mutable placement and remeshing.
- **Required input:** cooked geological field identity, modification commands,
  deterministic persistence, collision/nav/render publication boundaries.
- **Expected interface:** a separately admitted runtime geology capability behind
  Jerico commands/systems/events.
- **Owner:** a future dedicated runtime geology/destruction slice.
- **Dependency:** a project-level decision and tests for runtime modification.
- **Milestone:** not assigned or authorized by this P1 Colluvium slice.

### Arbitrary gravity transport

- **Why required:** only content that authors Colluvium under non-cardinal gravity
  needs an oblique traversal/stability contract.
- **Required input:** admitted grid traversal, support-contact, tie, and budget
  semantics.
- **Expected interface:** a later compatible extension of the reference request
  without changing conserved material lineage.
- **Owner:** a future explicitly scoped geology slice.
- **Dependency:** representative content and a justified production requirement.
- **Milestone:** not assigned or authorized here.

## Explicit exclusions

- Rockfall reimplementation;
- imported-mesh structural operators;
- general destruction, remesh, or save-state frameworks;
- runtime/Jolt transport;
- VDB or UI restoration;
- bespoke C++ generators or presets;
- reference-engine code, shader, format, or API ports;
- other P1/P2/M9/Task23 work;
- `docs/JERICO_1_0_PRODUCTION_EXPANSION_GEOLOGY_PWAS_EPLS_ANIMATION_DESTRUCTION.md`.

Completing this design or its local slice never completes the Persistent Goal.
