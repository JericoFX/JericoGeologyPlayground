# M8 Rockfall Design

Date: 2026-08-28

Slice: `M8-GEOLOGY-BLUEPRINT-P1-ROCKFALL`

Boundary: `OFFLINE_STATIC_FIELD_ROCKFALL`

## Goal

Add one typed rockfall recipe layer that deterministically removes unsupported
intact-rock components from the existing discrete geology field before Surface
Nets, preserving the already-published classified cook, collision and static
runtime path.

This design does not implement falling animation, debris, colluvium, deposition,
runtime destruction, imported-mesh structural analysis or a general topology
framework.

## Existing seams retained

- `GeologyStackRecipe` remains the only authoring recipe and canonical identity.
- The existing sparse Diligent/Vulkan session remains the only productive field
  evaluator.
- Existing Strata, FractureCells, DurabilityField and SurfaceWeathering stages
  keep their meanings and recipe order.
- Existing classified Surface Nets remains the only geology mesh extraction.
- Existing JRMS 1.1 `GEOL`, exact mesh collision, PackageTool and static runtime
  formats remain unchanged.
- No OpenVDB, editor UI, bespoke generator/preset or second mesh processor is
  introduced.

## Recipe contract

Advance the cumulative recipe to schema 19 and add `Rockfall = 23`.

```cpp
struct GeologyRockfallPayload
{
    Vec3d gravityDirectionWorld{0.0, -1.0, 0.0};
    double supportPlaneOffsetMeters{};
    double fractureCutoff{0.5};
    double activationCutoff{0.5};

    bool operator==(const GeologyRockfallPayload&) const = default;
};
```

Validation requires:

- schema 19 for kind 23;
- one finite non-zero gravity direction, canonically normalized;
- finite float-representable support-plane offset in metres;
- finite `fractureCutoff` and `activationCutoff` in `[0, 1]`;
- zero seed and exact identity layer transform;
- the normal `strength` and regional-mask contracts;
- Rockfall is the final enabled geometry/topology layer in this first slice;
- no new topology expansion beyond the already-planned field bounds.

The explicit terminal-layer ceiling keeps the offline readback/upload seam
bounded. A later authorized colluvium slice may deliberately extend it.

## Voxel topology contract

All rules below are `[JERICO SYNTHESIS]`.

For every sparse voxel sample in canonical Morton-leaf/local-index order:

```text
occupied = signedDistanceMeters < 0
up = -normalize(gravityDirectionWorld)
weight = clamp(layerStrength) * clamp(regionalMask(worldCentre))
active = occupied && weight >= activationCutoff
planeAnchor = occupied
           && dot(worldCentre, up)
              <= supportPlaneOffsetMeters + 0.5 * voxelSizeMeters
fixedAnchor = occupied && !active
anchor = planeAnchor || fixedAnchor
```

Two occupied face neighbours have a load-bearing bond iff:

```text
max(fractureA, fractureB) < fractureCutoff
```

Only the six axis-adjacent neighbours participate. Missing/positive-distance
neighbours are empty. No edge/corner bond exists.

Support is the deterministic flood from all anchors over load-bearing bonds.
An active occupied voxel not reached by that flood is detached. Detached
components are then labelled over the same bonds in canonical order; each
component identity is the hash of its ordered canonical voxel keys and its
stable witness is the smallest key.

Cells outside the active mask are retained anchors. This gives masks and
strength a useful non-destructive boundary and makes zero strength an exact
identity without cutting a component at the mask edge.

## Geometry and semantic output

For every detached voxel:

```text
signedDistanceMeters = max(abs(signedDistanceMeters),
                           0.5 * voxelSizeMeters)
```

The operation removes the complete detached component; it never interpolates a
partial displacement by `strength`.

Every retained occupied voxel sharing a face with a removed voxel is marked in
the already-existing classification as newly exposed/fresh. Existing strata,
fracture, erosion, cavity and other packed semantics remain unchanged. Removed
cells do not become colluvium or another material.

## Durability lineage

Durability is intentionally indirect:

```text
DurabilityField -> existing weathering rate -> current scalar occupancy
current scalar occupancy -> Rockfall support graph
```

Rockfall must not treat normalized durability as force, density, friction or a
physical strength threshold. Fracture is the only direct structural bond input.
This distinction catches an easy but unsupported coupling.

## Units and diagnostics

The productive result publishes bounded authoring diagnostics:

```cpp
std::uint64_t rockfallOccupiedVoxelCount;
std::uint64_t rockfallSupportedVoxelCount;
std::uint64_t rockfallDetachedVoxelCount;
std::uint64_t rockfallDetachedComponentCount;
std::uint64_t rockfallDetachedSetHash;
double rockfallDetachedVolumeCubicMeters;
std::uint64_t rockfallReadbackBytes;
std::uint64_t rockfallWorkItems;
```

`rockfallDetachedVolumeCubicMeters` is voxelized geometric volume, not kilograms.
Recipe hash, final field hash and mesh hash remain the durable product identity;
the detached-set hash is stage evidence, not a runtime asset schema.

## CPU/GPU execution seam

1. Vulkan evaluates all preceding layers in the existing resident buffers.
2. Rockfall checks cancellation and checked topology/readback budgets.
3. Existing scalar, packed structural classification and regional-mask inputs
   are read back once into bounded authoring memory.
4. One CPU reference/productive implementation performs canonical flood,
   component labelling, removal and fresh-surface marking.
5. Cancellation is checked before readback, between bounded scan/flood chunks,
   before upload and before surface extraction.
6. Updated scalar/classification data is uploaded into the existing ping-pong
   buffers.
7. Existing Vulkan Surface Nets, cook and collision continue unchanged.

The hard first-slice ceiling is 16,777,216 topology voxels and checked byte/work
arithmetic. Exceeding it fails visibly with
`procedural.geology.rockfall_work_exceeded`; allocation overflow fails before
readback. This bounded offline seam is not a CPU field evaluator or a parallel
mesher.

## Failure contracts

- `procedural.geology.recipe.rockfall_schema_unsupported`
- `procedural.geology.recipe.rockfall_gravity_invalid`
- `procedural.geology.recipe.rockfall_support_plane_invalid`
- `procedural.geology.recipe.rockfall_cutoff_invalid`
- `procedural.geology.recipe.rockfall_order_invalid`
- `procedural.geology.rockfall_field_invalid`
- `procedural.geology.rockfall_work_exceeded`
- existing cancellation error propagated unchanged

No failure silently falls back to mesh pruning, CPU source evaluation or runtime
destruction.

## Determinism

- no random candidate selection and no seed;
- canonical sparse voxel ordering;
- fixed six-neighbour order `-X,+X,-Y,+Y,-Z,+Z`;
- FIFO flood with stable insertion order;
- stable smallest-key component witnesses;
- checked exact cutoff comparisons;
- recipe hash includes every payload field;
- repeat evaluation must match detached-set, final-field, mesh, cook and package
  hashes.

## Acceptance examples

1. A bridge attached to the support plane by an unfractured neck remains.
2. Removing that neck through existing weathering detaches the same component.
3. A fracture value below the cutoff transmits support; the same literal value
   above the cutoff separates it.
4. Zero strength and a zero regional mask are exact identities.
5. A masked-out neighbour is retained support rather than silently deleted.
6. Detached count times `voxelSize^3` equals the reported cubic metres.
7. Permuting discovery/storage order cannot change component witnesses or hashes.
8. Cancellation and work/readback overflow fail before a partial product escapes.
9. Surface Nets, fresh classification, exact collision, cook, package and static
   runtime all consume the same retained field.

## End-to-end completeness audit before implementation

| Category | Applies now | Contract |
| --- | --- | --- |
| Source / recipe / serialization | Yes | schema 19, kind 23, canonical hash |
| Structural input | Yes | scalar occupancy plus existing fracture channel |
| Durability / weathering | Yes | indirect ordered lineage only |
| CPU | Yes | bounded deterministic global topology pass |
| GPU / shader | Yes | existing Vulkan field before/after the CPU seam; no second evaluator |
| Renderer / materials | Yes | existing classified Surface Nets; fresh bit on exposed faces |
| Collision | Yes | existing exact collision rebuilt from retained mesh |
| Cook / package / runtime | Yes | unchanged JRMS 1.1/static source-free route |
| Cancellation / failure / profiling | Yes | checked bounds, stable errors and diagnostics |
| Tests / debug evidence | Yes | CPU contract, schema, Vulkan product, cook/runtime lineage |
| Jolt dynamic bodies | No | `INTEGRATION REQUIRED` in a later runtime-destruction slice |
| Colluvium / deposition | No | separate later slice |
| Imported mesh structure | No | separate later slice |
| Runtime remesh / save / streaming / LOD | No | `INTEGRATION REQUIRED` |
| Gameplay/Luau/events/nav/VFX/audio | No | `INTEGRATION REQUIRED` |
| New editor UI | No | existing recipe and bounded inspection only |

## Completion boundary

Local `PASS` requires a witnessed RED, canonical recipe admission, deterministic
CPU topology behavior, productive Vulkan-to-CPU-to-Vulkan integration, classified
Surface Nets, exact collision/cook/package/static-runtime proof, repeated hashes,
cancellation and bound failures, updated audit/provenance and selective Git
publication.

It does not complete P1, the full geology blueprint or the Persistent Goal.
