# M8 Blender Auxiliary Add-on Research Inputs

**Milestone:** M8  
**Status:** research index only; does not replace the legal `ACTIVE_SLICE`  
**Owner timing:** Astra reads these when M8 becomes the active slice and the related capability is reached.

The owner supplied purchased Blender add-ons/assets for technical analysis. They are
not all geology/geometry generators; together they expose supporting authoring,
mesh-analysis and asset-pipeline problems M8 should understand before implementing
Jerico-native tooling.

The long-term rule is:

```text
purchased addon / Geometry Nodes asset
  -> inspect behavior / algorithms / workflow
  -> durable research specification
  -> Jerico-native implementation + tests
  -> no purchased-addon dependency in Engine/Shipping
```

## Inputs

### Geometry/remesh

- `M8_SMART_REMESH_NATIVE_REIMPLEMENTATION_INPUT.md`
- `SMART_REMESH_3_TECHNICAL_ANALYSIS.md`
- `SMART_REMESH_3_CPP_PORT_DESIGN.md`

Purpose: remesh/topology/hard-surface reference.

### UV / chart partitioning / topology matching / texel density

- `M8_ZENUV_5_4_0_11_TECHNICAL_INPUT.md`

Purpose: surface-chart clustering, automatic seam generation, topology-safe island
cleanup, exact texel-density analysis, topology correspondence/stacks, relax solver
benchmarking, trims/hotspots, packing and UDIM.

**Source status:** the owner re-downloaded Zen UV and supplied a complete 5.4.0
source package. The prior incomplete-archive assumption is obsolete. The canonical
M8 document now contains direct source-inspected internals and the archive hash.

### Path-driven modular construction / curve deformation

- `M8_STAIR_BUILDER_2_02_TECHNICAL_INPUT.md`

Purpose: extract the generic mathematics hidden inside Stair Builder 2.02 rather
than create a staircase-only subsystem. The supplied `.blend` was inspected at
Geometry Nodes/datablock level and contains 33 node trees.

Primary reusable targets:

```text
arc-length module distribution
fit/remainder/count policies
rotation-minimizing path frames
curve deformation
parallel/lateral path offsets
posts/railings/support arrays
end connectors
procedural semantic face lineage
path-distance / skew-corrected UV mapping
```

The recommended Jerico abstraction is a general **Path Assembly** backend serving
stairs, fences, railings, catwalks, bridges, tunnel supports, modular walls,
pipes/cable trays and other path-driven assets. It must reuse the canonical Jerico
spline/path authority.

### Procedural dressing / scatter / cables / material masks

- `M8_DAMAGE_CONTROL_TECHNICAL_INPUT.md`

Purpose: extract reusable authoring mathematics and data-flow from the owner-supplied
Damage Control Geometry Nodes library without reproducing its proprietary graphs.
The verified static corpus contains 214 node trees, 10,344 nodes and 12,683 links.

Primary reusable targets:

```text
closed-form catenary endpoint solve
canonical-path cable/wire generation
random + Poisson surface candidate generation
distance/proximity rejection and projection
edge/corner surface-feature fields
volume occupancy sampling for piles
stable per-island procedural variation
orientation-aware cellular/noise masks
interactive-preview vs final-build quality tiers
```

The recommended direction is shared Jerico services for path math, mesh feature
fields, deterministic scatter and spatial rejection/projection. Damage Control is a
research reference only; no node tree, preset, model, texture or runtime dependency
is admitted.

### Modular damage / prefractured variants / spline destruction

- `M8_INFINITE_FRACTURE_TECHNICAL_INPUT.md`

Purpose: understand the actual mechanism behind the owner-supplied Infinite
Fracture Geometry Nodes library and identify the cheapest equivalent Jerico-native
architecture. The verified static corpus contains 41 node trees, 3,975 nodes and
5,144 links.

The central finding is that the inspected source is **not** a general mesh-boolean
or physical fracture-propagation solver. It builds semantic path/tile assemblies,
computes a deterministic scalar damage field, classifies interior/boundary roles and
selects prebuilt fractured variants by stable tile/island/chunk identity. Rebar,
debris, material cracks and environment scatter are independent decorators.

Primary reusable targets:

```text
DamageField composition: distance / range / curvature / noise / effectors
stable ProceduralElementId for deterministic variation
semantic tile role: interior / left / right / top / bottom
prefractured VariantFamily selection
boundary-safe modular damage
path U/V/tile lineage
spline-driven local destruction corridor
cooked dormant chunks -> Jolt activation at runtime
material/decal crack as cheaper damage tier
```

This is strong evidence against paying for an arbitrary GPU mesh-fracture solver
when modular/prefractured authoring satisfies the asset. True arbitrary fracture
remains a separate capability for cases where semantic variants are insufficient.

### Semantic mesh segmentation

- `M8_SMART_SEGMENTATION_1_0_TECHNICAL_INPUT.md`

Purpose: thickness/concavity-driven semantic partitioning, hierarchy and
crease-aware boundary refinement.

Two supplied ZIPs were compared. Their actual `Smart_Segmentation.py` payload is
byte-identical, so there is one technical source snapshot, not two implementations.

### Rig / retarget / skinning

- `M8_AUTO_RIG_PRO_3_78_50_TECHNICAL_INPUT.md`

Purpose: skeleton semantics, rest-pose retargeting, IK/FK, pole math, skin binding,
weight transfer and game/cook export normalization.

### Bake / material flattening

- `M8_SIMPLEBAKE_2_9_11_TECHNICAL_INPUT.md`

Purpose: high-to-low bake jobs, cage/projection, PBR flattening, specialist maps,
UDIM, postprocess, channel packing and deterministic texture cook.

### Layered Smart Materials / surface-feature fields / VDM

- `M8_UCUPAINT_2_4_9_TECHNICAL_INPUT.md`
- `M8_UCUPAINT_2_4_9_MATHEMATICAL_RECONSTRUCTION.md`

Purpose: use the owner-supplied Ucupaint 2.4.9 source as a reference for typed
material layers/masks and a shared Jerico `SurfaceFeatureFields` backend: curvature,
convexity/concavity, contact/cavity, edge strength, slope/orientation, height/exposure
and UV/directional flow. Smart Materials, Aged Wood/Metal, Damage, Geology, EPLS,
PWAS and remesh may consume those common fields rather than implementing independent
edge/cavity logic. The same fields can flatten through `MaterialBakeIR` when runtime
evaluation is unnecessary. Tangent-space VDM reuses Jerico's canonical tangent
authority. Ucupaint remains GPL research input only; no source, node library or
runtime dependency is admitted.

### Deferred Sequencer matchmove authoring reference

- `AUTOTRACK_4_2_8_DEFERRED_REFERENCE.md`

Purpose: retain the useful orchestration ideas from AutoTrack 4.2.8 for future
**Sequencer camera authoring**. The inspected add-on delegates feature detection,
tracking and camera reconstruction to Blender. Jerico's durable design is
`video -> tracks -> solve artifact -> Bake To Camera Track -> canonical Sequencer
CameraTrack`. There is no second camera/runtime path. Deterministic keyframe
selection, reconstruction/coverage diagnostics, robust MAD-based cleanup, staged
intrinsics refinement and degeneracy-aware acceptance remain useful inputs. The
capability stays deferred until the relevant Sequencer authoring slice is legal.

## Shared services Astra should look for before implementing

Do not implement duplicate math per addon. Audit existing Jerico capabilities first.

Likely shared services:

```text
MeshSpatialQuery / ray query
Mesh adjacency / half-edge topology
Signed dihedral / normal variation
Surface feature fields / boundary-distance queries
Deterministic surface + volume candidate generation
Spatial rejection / Poisson spacing
Stable mesh element / region / island identity
ProceduralElementId / deterministic staged hashing
DamageField / semantic tile-role classification
VariantFamily / damaged-state resolver
Surface region moments + graph clustering
Graph shortest path / union-find / priority queue
Mesh topology correspondence
UV island graph + metrics
Path arc-length tables
Catenary endpoint solve
Path curvature / turning-angle field
Rotation-minimizing frames
Path module distribution / deformation
Job/cancel/progress/cache framework
Semantic region streams
MaterialBakeIR / procedural mask IR
Rig/skeleton transform math
Asset lineage + deterministic cook
```

## Cross-input opportunities

### Zen UV + Smart Segmentation

Both reveal graph-based region generation. Share adjacency, signed feature metrics,
region statistics and topology-safe boundary cleanup rather than building separate
UV-only and segmentation-only graph infrastructures.

### Damage Control + Smart Segmentation

Damage Control's corner/edge scatter consumes edge-angle, normal and proximity
fields that overlap Smart Segmentation's concavity/crease analysis. Build one
surface-feature backend and let segmentation, scatter, grime, decal, wear and
material-mask tools consume it.

### Infinite Fracture + Smart Segmentation

A generic destructible mesh that lacks procedural tile semantics can use segmentation
only as an **offline authoring aid** to propose stable regions/chunks. Once accepted,
Jerico should persist those region IDs rather than recompute them at runtime.

### Stair Builder + existing M8 curve assets

The stair graph reinforces M8's existing requirement for tubes, cables, webs,
rails, ladders and walkways. A single robust path/frame/distribution backend should
serve all of them; no second spline representation is authorized.

### Damage Control + Stair Builder + existing M8 curve assets

Damage Control adds an explicit catenary solve, cable collision/projection and
arc-length attachments to the same path problem. Extend the canonical path authority
with reusable catenary/deformation policies rather than creating a wire-only curve
representation.

### Infinite Fracture + Path Assembly

Infinite Fracture's road/bridge paths reinforce the same semantic-path requirement:
store arc-length/U/V/tile role during generation so damage, UV, boundary variants,
scatter and materials consume known lineage. The destruction spline should be a
DamageField consumer of the canonical path representation, not a second spline type.

### Stair Builder + Zen UV

Procedural path generators know their own longitudinal/lateral coordinates and
semantic face classes, so they can emit deterministic UV parameterization directly.
Use a generic unwrap only when procedural coordinates are insufficient.

### Smart Remesh + SimpleBake

High-detail -> lighter topology -> normal/AO/material bake remains a canonical
asset-preparation workflow. Preserve semantic/UV lineage across the reduction.

### Ucupaint + SimpleBake + Smart Remesh

Ucupaint strengthens the shared surface-feature and material-bake direction rather
than creating another painter backend. Build signed curvature/contact/flow as reusable
fields, let Smart Materials and remesh consume the same analysis, and flatten through
the existing `MaterialBakeIR`/texture cook when runtime procedural evaluation is not
required. Tangent-space VDM must reuse Jerico's canonical tangent authority.

### Smart Segmentation + SimpleBake

Stable semantic regions can become material, color-ID and mask bake inputs.

### Damage Control + SimpleBake

Procedural grunge, cellular crack-like masks and per-region variation can remain
live while authoring, then be baked/packed when the final asset does not need a
runtime procedural evaluation. Preserve source parameters and deterministic seeds
for reproducible rebuilds.

### Infinite Fracture + Damage Control + SimpleBake

Use one damage field to drive the cheapest sufficient representation: material/decal
crack first, displacement/bake second, prefractured variant when silhouette/interior
must change, and dynamic Jolt chunk activation only when gameplay requires motion.

## Additional owner-supplied procedural references — 2026-09-20

- `M8_MIX_DAMAGE_LACES_FENCE_AGEDFX_PBG2_TECHNICAL_INPUT.md`
- `M8_MIX_DAMAGE_LACES_FENCE_AGEDFX_PBG2_MATHEMATICAL_RECONSTRUCTION.md` — implementation-level equations, independent kernels and acceptance tests.

This pass adds five static reference studies without changing the active slice:

- **Mix Damage V1.2** -> reuse the canonical `DamageField` and tiered damage representation; do not introduce a second destruction architecture.
- **Laces Generator 2.0** -> generalize to ordered anchors + path frames + profile sweep + weave policy for cables, bindings, ropes and threaded paths.
- **Fence/Gate/Railing/Wall Generator 1.1** -> extend the common Path Assembly backend with gate spans, slope adaptation, stepped spans and semantic architectural parts.
- **AgedFX 1.22** -> EPLS owns dust/dirt/scratches/environmental state; PWAS owns physical moss/cobweb instances; JericoParticles/volumetrics own airborne dust/fog.
- **Procedural Building Generator V2** -> define a Semantic Surface Assembly / Facade Tiler consuming BSP or guide geometry, with surface/edge roles, best-fit module tiling and role-based StyleSet/VariantFamily selection.

The cross-input rule is to converge on shared Jerico-native services (canonical paths,
stable frames, semantic roles, stable IDs, surface-feature fields, deterministic
variant selection, DamageField and cook provenance) rather than mirror add-on-specific
node graphs or Blender UI. Purchased meshes, textures, presets and serialized node
graphs remain outside the repository and Shipping.

## M8 scheduling rule

These documents do **not** authorize work during a different active slice.

When M8 is legal:

1. audit existing implementation first;
2. read only the research input relevant to the next capability;
3. identify common backends before addon-specific code;
4. write native tests/acceptance criteria;
5. implement independently;
6. preserve source-free Shipping and Blender-free runtime;
7. keep purchased add-ons/assets optional references, never required dependencies.

## Source/proprietary boundary

The repository should contain the **research documents**, not copies of purchased
addons or proprietary `.blend` graphs.

GPL sources such as inspected addon code may inform behavior, equations, test cases
and architecture but are not mechanically copied into Jerico production. Proprietary
Geometry Nodes assets such as Stair Builder, Damage Control or Infinite Fracture are
summarized at algorithm/data-flow level rather than serialized or reproduced
node-by-node.

Any future third-party library admission follows the normal dependency/license
process independently of this research.

## Open-source direction

The intended future public JericoEngine should expose its own implementations and
contracts. Research from purchased tools informs the problem definition and test
cases; it must not turn those tools into required build/runtime dependencies or
silently copy license-restricted source/assets.