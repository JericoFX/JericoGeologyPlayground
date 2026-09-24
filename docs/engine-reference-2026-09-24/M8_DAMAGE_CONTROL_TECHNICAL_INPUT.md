# M8 Damage Control Geometry-Nodes Technical Input

**Milestone:** M8 research input only  
**Source:** owner-supplied purchased `DamageControl_MAIN.blend`  
**Status:** static extraction verified; algorithm/data-flow analysis only  
**Runtime dependency:** none authorized  
**Active-slice effect:** none; this document does not replace or reopen the legal `ACTIVE_SLICE`

## Purpose and source boundary

This document records reusable technical ideas observed in the owner's purchased
Damage Control Blender library. It is an engineering research input, not a port of
the asset and not a dependency proposal.

The required long-term direction remains:

```text
purchased Geometry Nodes / shader reference
  -> inspect behavior, math and authoring workflow
  -> describe independent Jerico problem/contract
  -> implement/test Jerico-native or Jerico-owned Blender-worker tooling
  -> cook stable Jerico assets
  -> no purchased asset dependency in Engine/Shipping
```

No `.blend`, texture, model, node-tree JSON, serialized graph, proprietary preset,
or mechanically translated node graph is committed by this research. The findings
below intentionally stay at algorithm/data-flow level, consistent with the existing
M8 Blender auxiliary research boundary.

## Verified source reconstruction

The Drive copy exceeded the connector's single-file download limit, so the owner
split it into three byte-preserving parts. The parts were materialized separately
and concatenated in order with streaming I/O.

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `part001` | 209,715,200 | `fe39fe66c42cca6350969c07430f8aaca6a0a72187d65a0013a9d25403acc096` |
| `part002` | 209,715,200 | `a57f6e45ffb1d9cf843bb5e4cbe0d4b3c967e58b2b02f8f8136365fced7d5059` |
| `part003` | 181,092,202 | `bfe2004530bd69fa14fdfd8cc3877ae4de3f378f3647bfefea805f1401ebc044` |
| reconstructed `DamageControl_MAIN.blend` | 600,522,602 | `53f752d98d4fe65ca54f52e5bd40128791ee8319cc96034b387a578a374d0270` |

The reconstructed byte count exactly matches the owner-visible original Drive
metadata. That validates the concatenation length, but it is **not** an independent
hash comparison against the unsplit Drive object because that object's digest was
not available through the connector.

The file header is valid `BLENDER-v405`: Blender 4.0.5, 64-bit pointer layout,
little-endian. Static block scanning reached the exact file boundary and found one
`DNA1` schema block and one terminal `ENDB` block.

The extraction path deliberately did not execute Blender content. The current
analysis environment has no Blender executable or `bpy`, so a read-only static
parser was used instead. It decoded the embedded SDNA schema, reconstructed struct
layout/alignment, indexed Blender block-address ranges and traversed node-tree,
node, socket and link records. Reconstructed SDNA sizes matched **957/957** embedded
struct definitions (zero layout mismatches), which is a strong structural check for
the fields used below.

No Python text blocks, handlers, drivers, operators, modifiers, Geometry Nodes,
shaders or scene evaluation were executed.

## Extracted corpus

The verified static inventory contains:

- **214 node trees**;
- **10,344 nodes**;
- **12,683 links**;
- **628,264 Blender data blocks** in the file overall;
- **3,012 object blocks**, **1,219 mesh blocks**, **129 image blocks** and
  **94 material blocks** in the broader library.

The most frequent node classes in the node-tree corpus include 2,706 reroutes,
979 group inputs, 734 scalar Math nodes, 527 Map Range nodes, 441 Mix nodes,
434 Vector Math nodes, 215 Geometry Node group instances, 206 Color Ramps,
172 Compare nodes, 151 Random Value nodes, 146 Geometry Switch nodes,
130 image textures, 121 Separate Geometry nodes, 106 Boolean Math nodes,
93 Collection Info nodes, 84 Position nodes, 78 Store Named Attribute nodes,
72 Set Position nodes, 64 Join Geometry nodes, 62 Noise Textures and
61 Instance on Points nodes.

This distribution matters more than the raw size of the `.blend`: the source is a
large procedural authoring library with repeated utilities and presets around a
smaller set of reusable geometry/shading ideas.

Representative high-value trees observed directly include:

```text
.Catenary / .Catenary2D / .CatenaryOffsets / .asinh
.DistributePoints In Corners
.EdgeCracks
.OuterEdges_MaskFIX
.UberNoiseFIX
.UberVoronoi
Pile Generator
Scatter Around
Scatter Around for Ground
Scatter Corners
Scatter Curve
Scatter Ivy / Scatter Ivy 2
Scatter Moss
Scatter Vines
Viewport LOD
Wires Generated (to Object)
Wires Manual (to Curve or Edges)
```

There are also many Grunge material/mask groups, scatter/biome presets and wrapper
presets. Those preset families are evidence of workflow coverage, not a reason to
reproduce their proprietary graphs in Jerico.

## Finding 1: exact catenary math is explicit and reusable

The cable/wire stack contains a small mathematical core that can be described
independently of the asset.

### `asinh`

The extracted helper graph evaluates the standard identity:

```text
asinh(x) = ln(x + sqrt(x*x + 1))
```

Its logarithm base socket is the stored float `2.7182817459106445`, so the node is
using the natural logarithm identity rather than a base-10 approximation.

### 2D catenary

The `.Catenary2D` data flow is exactly:

```text
y(x) = a * cosh((x - xc) / a) + yc
```

The operation sequence was decoded from Blender 4.0's `NodeMathOperation` enum and
verified against the extracted links: subtract -> divide -> hyperbolic cosine ->
multiply -> add.

### Endpoint offsets

For endpoint heights `y1`, `y2`, horizontal endpoint distance `d` and catenary
parameter `a`, the extracted offset helper reduces to the standard closed form:

```text
q  = (y1 - y2) / (2*a*sinh(d/(2*a)))
xc = d/2 + a*asinh(q)
yc = y1 - a*cosh(xc/a)
```

The graph stores the equivalent sign-flipped intermediate using
`sinh(-d/(2*a))`; simplifying the odd/even hyperbolic identities yields the form
above.

The parent catenary group measures endpoint distance, resamples the input curve,
evaluates the catenary along spline length and writes the solved vertical value
while retaining the existing horizontal coordinates. It also exposes a collision
selection path and a resampling resolution control.

### Jerico relevance

This should **not** become a Damage-Control-specific feature. It reinforces the
existing M8 path/curve authority:

```text
Canonical spline/path
  -> arc-length parameterization
  -> catenary endpoint solve
  -> sampled/deformed curve
  -> optional collision/projection relaxation
  -> profile sweep / cable mesh
  -> stable cooked asset
```

A Jerico-owned catenary utility can serve wires, hanging cables, ropes, pipes,
web strands and environmental dressing. The closed form is cheap enough for
interactive authoring; collision relaxation is a separate cost tier and should be
optional.

## Finding 2: `Scatter Around` is a candidate-generation + rejection pipeline

`Scatter Around` is one of the largest observed trees (**534 nodes / 647 links**),
but its reusable architecture can be reduced to a small number of stages.

The group exposes authoring controls for collection selection, deterministic seed,
density, scale/random scale, alignment, distance from emitter geometry, distance
from existing instances, culling, masks/noise, ground projection, optional raycast,
minimum scale, collision chaining and realization policy.

Two distinct surface-candidate generators are present:

- ordinary random point distribution with density and seed;
- Poisson-disk distribution with minimum distance, maximum density and seed.

Those candidates then pass through proximity/nearest-surface, instance-to-point,
comparison/switch, position and optional raycast stages. The important reusable
idea is therefore not a specific Blender node graph but this contract:

```text
source surface
  -> deterministic candidate generation
  -> feature/mask eligibility
  -> distance/proximity rejection
  -> surface/ground projection
  -> orientation + scale randomization
  -> optional previous-instance exclusion
  -> instances / cooked realization
```

The observed graph uses multiple Geometry Proximity and Sample Nearest Surface
operations plus a raycast path, so spacing and projection are first-class parts of
the authoring workflow rather than a final cosmetic pass.

### Jerico relevance

Jerico should expose a common scatter backend rather than separate implementations
for rocks, debris, ivy, biome props and wall dressing. Candidate generation and
rejection should be deterministic and cacheable. A spatial acceleration structure
(grid/BVH depending on source) should replace repeated high-level proximity queries
in a native implementation.

Recommended policies:

- `RandomSurface` for cheap broad fill;
- `PoissonSurface` when minimum spacing matters;
- deterministic seed/hash streams per stage;
- material/semantic/vertex-mask filters;
- slope/normal filters;
- emitter and already-placed-instance exclusion radii;
- optional ray/surface projection;
- preview density separate from final density;
- batch/cancel/progress and stable cooked output.

Do not pay Poisson/proximity/raycast cost when a simple random fill is visually
sufficient.

## Finding 3: corner scatter is a reusable surface-feature field

`Scatter Corners` delegates its feature selection to a lower-level
`.DistributePoints In Corners` utility (**119 nodes / 130 links**).

The observed utility combines:

- three Mesh Edge Angle queries;
- multiple normal fields;
- dot products, vector subtraction, normalization and distance operations;
- three Geometry Proximity queries;
- comparison/switch masks;
- random surface candidate distribution;
- selection, maximum-slope and edge-distance controls.

Its public controls include corner distance, edge-detection distance, slope limit,
seed, fill-voids behavior and a hard-limit switch. `Scatter Corners` layers asset
selection, scaling, masking, colorization and authoring options on top.

The generic lesson is:

```text
mesh feature measurements
  -> scalar eligibility field
  -> seeded surface candidates
  -> threshold/proximity rejection
  -> scatter instances
```

This is broader than “scatter in corners.” The same feature-field service can
support edge grime, rubble along wall/floor contacts, moss, decals, fracture edge
accent, wear, snow/dust accumulation and semantic selection tools.

For Jerico, feature extraction should be a shared mesh-analysis backend with
explicit quantities such as signed dihedral/edge angle, normal variation,
curvature proxy, boundary distance, material boundary and semantic-region distance.
Consumers should compose fields rather than each reimplementing topology scans.

## Finding 4: `Pile Generator` uses volume occupancy, not a destruction solver

`Pile Generator` (**147 nodes / 181 links**) contains an explicit
`Mesh to Volume -> Distribute Points in Volume` route. Its extracted controls
include pile size/height, displacement, density, instance scale, seed and a ground
subdivision/material path.

Observed high-level flow:

```text
pile envelope / deformed source
  -> mesh-to-volume occupancy
  -> seeded random points in volume
  -> nearest-surface/orientation adjustment
  -> instance placement
  -> optional preview/ground treatment
```

This is a useful authoring approach for rubble heaps, trash, stones, clutter and
vegetation volume fill. It is **not evidence of rigid-body fracture, physical
sediment transport or a destruction solver**.

Jerico should treat this as an offline/authoring generator. A cheaper native
implementation could use analytical or voxel/SDF occupancy and rejection sampling,
then cook instances or merged geometry. Use volumetric sampling only where a
surface scatter cannot produce the desired pile depth.

## Finding 5: the wire generator composes path math with collision/projection

`Wires Generated (to Object)` is another large authoring graph
(**545 nodes / 673 links**). It exposes emitter/target selection, deterministic
spawn controls, min/max length, catenary strength/randomness, radius and profile
resolution, geometry/ground collision, hanging/ground softening, noise, optional
hanging assets/plugs and spacing.

The tree reuses the catenary helper, curve-to-mesh utilities and several
raycast/nearest-surface/curve sampling operations.

The reusable architecture is:

```text
spawn/endpoint policy
  -> path initialization
  -> catenary solve
  -> optional collision/project/soften iterations
  -> cable profile sweep
  -> optional attachments along arc length
  -> material/UV attributes
  -> cook
```

Jerico already needs a canonical curve/path backend. This source reinforces that
cables and wires should consume that backend rather than create a second spline
representation. The expensive collision/softening pass belongs in authoring and
can have quality tiers; the game runtime normally consumes the cooked result.

## Finding 6: grunge/crack utilities are procedural masks, not physical fracture

The shader/mask side uses repeated low-cost procedural primitives rather than one
monolithic material algorithm.

### Edge crack mask

`.EdgeCracks` combines multiple 3D Voronoi evaluations, including distance between
Voronoi position outputs, with noise/white-noise perturbation and threshold ramps.
The useful concept is to estimate distance to cellular boundaries at multiple
scales, decorrelate instances/islands, then threshold/remap that scalar into a
crack-like mask.

A Jerico-independent version can be expressed as:

```text
cellular feature position A/B
  -> boundary-distance proxy
  -> multi-scale modulation/jitter
  -> remap/threshold
  -> semantic crack mask
```

This is appropriate for material masks, decal proposal and offline displacement.
It must not be confused with topology-changing fracture propagation.

### Per-island noise

`.UberNoiseFIX` uses several normalized 3D noise layers and a per-island random
translation path based on an `island_id` attribute and white noise. This avoids
obvious repetition between disconnected pieces while keeping a common procedural
material family.

Jerico can generalize this as a stable `island/region -> hash -> transform/noise
seed` service. The hash should come from stable authored/cooked region identity,
not transient mesh ordering.

### Orientation-aware Voronoi

`.UberVoronoi` combines a normalized 4D Voronoi field with normal/tangent basis
math, vector rotation, object-space noise and seeded offsets. The generic lesson is
to evaluate pattern fields in a surface-aware local frame when world/object axes
would visibly stretch or misorient the pattern.

### Outer-edge mask

`.OuterEdges_MaskFIX` consumes stored geometry attributes and blends min/max/
subtractive/smoothed responses according to sharpness-like signals. The important
architectural point is that geometry analysis can be performed once during asset
preparation and then exposed as compact attributes to materials, rather than asking
every shader to rediscover topology.

## Finding 7: explicit viewport-density logic is an authoring concern

The `Viewport LOD` group (**40 nodes / 48 links**) contains Blender's viewport
context test plus switch/separation/sampling logic. It exposes distinct viewport
and render density controls.

Jerico should preserve the same *policy*, not the graph:

```text
interactive preview budget != final build budget
```

Procedural Studio/PWAS tools need deterministic preview proxies and final-quality
rebuilds. Runtime LOD/meshlet policy is a separate system and should not be inferred
from this Blender authoring graph.

## Consolidated Jerico services suggested by this source

The observations reinforce a small set of shared services instead of one feature
per asset preset:

1. **Path arc-length + catenary math** integrated with the canonical spline/path
   representation.
2. **Surface feature field** from adjacency/edge angle/normals/boundaries and
   distance transforms.
3. **Deterministic scatter candidate generators**: random surface, Poisson surface
   and volume occupancy.
4. **Spatial rejection/projection service** using the appropriate grid/BVH/SDF
   accelerator rather than repeated consumer-specific scans.
5. **Stable region/island hashing** for deterministic variation independent of
   transient topology order.
6. **Procedural mask IR** supporting noise/cellular fields, remap/threshold,
   orientation frames and precomputed mesh attributes.
7. **Authoring quality tiers** separating cheap interactive preview from final
   cook.
8. **Stable lineage/semantic attributes** so procedural geometry can feed material,
   bake, scatter and later cook stages without rediscovering topology.

These services overlap existing M8 research from Smart Remesh, Smart Segmentation,
Stair Builder and SimpleBake. They should be audited together when M8 is legally
active to avoid duplicate topology, path, spatial-query or bake infrastructure.

## Performance guidance

The observed library is designed for authoring convenience, not Jerico runtime
cost. A Jerico reimplementation should explicitly tier expensive work:

| Technique | Recommended placement | Cost rule |
| --- | --- | --- |
| Catenary closed-form evaluation | interactive authoring; optional runtime for tiny dynamic sets | cheap; cache arc-length tables |
| Random surface scatter | interactive/offline | default broad-fill path |
| Poisson scatter | interactive/offline | use only when spacing is visually important |
| Mesh proximity / nearest surface | authoring/offline | batch through shared acceleration structure |
| Repeated ray projection | authoring/offline | quality tier; avoid per-frame mass use |
| Mesh-to-volume scatter | offline/final authoring | use only when real 3D occupancy is needed |
| Cellular/noise masks | shader or bake depending frequency | bake when stable and expensive/repeated |
| Collision/softening for cables | offline/interactive tool | never assume full-quality solve every frame |
| Viewport density reduction | authoring | mandatory for heavy generators |

The cheapest visually sufficient route remains the default.

## Independent implementation targets and RED tests

When this research becomes actionable under the M8 authority, implementation
should begin from Jerico contracts/tests, not from the proprietary graph layout.
Suggested acceptance tests:

### Catenary

- equal-height endpoints are symmetric about `d/2`;
- unequal-height endpoints hit both endpoint heights within tolerance;
- reducing/increasing the chosen sag parameter changes curvature monotonically
  under the documented Jerico convention;
- resampling density changes tessellation, not the solved endpoints;
- degenerate/near-zero `a` and distance have explicit guarded behavior.

### Surface feature field

- planar interior is rejected as a corner/edge feature;
- known convex/concave test wedges produce stable signed feature values;
- topology/vertex ordering does not change stable semantic results;
- slope and boundary-distance thresholds are independently testable.

### Scatter

- same source + settings + seed -> byte/stable-ID equivalent placement;
- changing preview density never changes final-build determinism;
- Poisson minimum separation is measurable in the final set;
- exclusion against previous instances is order-independent under the chosen
  deterministic policy;
- masks/material/semantic filters compose without destroying source geometry.

### Volume pile

- no point is accepted outside the occupancy field;
- fixed seed is deterministic;
- density scales count without changing the source asset;
- surface-only fallback remains available when volume cost is unjustified.

### Procedural masks

- disconnected regions receive deterministic but decorrelated variations;
- feature fields are invariant under stable-ID-preserving mesh reorder;
- material evaluation can consume precomputed semantic attributes without access
  to Blender-specific state.

## What this extraction does **not** prove

- It does not execute the Blender graphs or visually compare their evaluated output.
- It does not prove performance characteristics of the original Blender library.
- It does not authorize copying node groups, presets, textures, meshes or materials.
- It does not establish physical fracture, hydraulic erosion, sediment transport or
  a fracture-propagation solver merely because some visual masks resemble cracks.
- It does not convert Blender's viewport-preview system into Jerico runtime LOD.
- It does not make Blender or Damage Control a Shipping dependency.
- It does not yet analyze the separate `InfiniteFracture_MAIN.blend`; that source is
  intentionally handled after this Damage Control report is published and verified.

## Disposition

**KEEP as M8 research input.** The strongest reusable findings are the explicit
catenary solver, generic candidate/rejection scatter architecture, corner/edge
feature-field composition, volumetric pile sampling, stable per-region procedural
variation and the strict preview-vs-final authoring split.

Implementation should remain independent, compositional and source-free in
Shipping. Presets are examples of desired authoring coverage, not APIs to clone.
