# M8 Infinite Fracture Geometry-Nodes Technical Input

**Milestone:** M8 research input only  
**Source:** owner-supplied purchased `InfiniteFracture_MAIN.blend`  
**Status:** static extraction verified; algorithm/data-flow analysis only  
**Runtime dependency:** none authorized  
**Active-slice effect:** none; this document does not replace or reopen the legal `ACTIVE_SLICE`

## Purpose and source boundary

This document records reusable engineering ideas observed in the owner's purchased
Infinite Fracture Blender library. It deliberately does **not** reproduce the
proprietary Geometry Nodes graphs, fracture-piece meshes, collections, materials or
presets. The target is an independent Jerico problem definition and implementation
strategy.

The legal/architectural direction remains:

```text
purchased Geometry Nodes reference
  -> inspect behavior / math / data flow
  -> extract generic engineering contracts
  -> implement independently in Jerico-owned tooling
  -> bake/cook stable Jerico assets
  -> no purchased-reference dependency in Engine/Shipping
```

## Verified source reconstruction

The owner split the original file into four byte-preserving parts because the
single-file Drive download path cannot transfer a 667 MB object. The four parts were
materialized independently and concatenated in order with streaming I/O.

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `part001` | 209,715,200 | `4dba9a1b25aa343612d7a256abc70f0b9900469f9556fe7e591e81cb3baf8bfa` |
| `part002` | 209,715,200 | `97ec3f7b577cd89e867faf46bd49136693820755a3fa7f670051efc88e1068a7` |
| `part003` | 209,715,200 | `66a3d5b7d6ebda7fc71cb08c77b5fb3906781d756c95310a9185fe241ac47c83` |
| `part004` | 37,904,604 | `e9b734a4f0fe6cebd3b7632601d9baa876c89ab8c7a092238bc49e8663e48354` |
| reconstructed `InfiniteFracture_MAIN.blend` | 667,050,204 | `0c8f62d35a65a68fd4b17c19610dccb6d11ef3f5ea599d5a4b60121b95d62387` |

The reconstructed byte count exactly matches the owner-visible Drive metadata for
the unsplit object. As with the Damage Control source, this validates concatenation
length but is not an independent full-file hash comparison against the unsplit
Drive object because the connector did not expose that object's digest.

The header is valid `BLENDER-v405`: Blender 4.0.5, 64-bit pointer layout,
little-endian. Static scanning reached the exact 667,050,204-byte file boundary,
with one `DNA1` schema block and one terminal `ENDB` block.

The same read-only SDNA parser used for the Damage Control audit reconstructed all
**957/957** embedded struct layouts with zero size mismatches. No Blender Python,
text blocks, handlers, drivers, Geometry Nodes evaluation, materials or scene code
were executed.

## Extracted corpus

The static inventory contains:

- **41 node trees**;
- **3,975 nodes**;
- **5,144 links**;
- **792,015 Blender data blocks** overall;
- **1,183 object blocks**;
- **1,175 mesh blocks**;
- **43 image blocks**;
- **26 collection blocks**;
- **21 material blocks**.

The three principal authoring trees are large:

| Tree | Nodes | Links |
| --- | ---: | ---: |
| `Infinite Fracture (To Ground)` | 1,447 | 1,766 |
| `Infinite Fracture (To Curves)` | 1,192 | 1,436 |
| `Infinite Fracture (Pillars)` | 289 | 360 |

Important helpers include `.ScatterTiles`, `.BoundaryIndexes`, `.ChooseBounds`,
`.Curvature`, `.Is Edge Boundary`, `.TangentDot`, `.OffsetBounds.001`, `.Interior`,
`FracturePieces_Concrete`, `FracturePieces_Asphalt`, rebar/debris utilities and a
`Cracks` material group.

## Central finding: this is not a general mesh-fracture solver

The most important technical result is negative but highly useful: the inspected
library does **not** implement arbitrary physical fracture propagation or a generic
runtime mesh-boolean fracture solver.

Evidence from the complete 3,975-node corpus:

- there is no Geometry Nodes Mesh Boolean node in the extracted node-type set;
- `.ScatterTiles` uses `Collection Info -> Instance on Points` as its replacement
  mechanism;
- concrete and asphalt fracture wrappers each call `.ScatterTiles` six times with
  dedicated prebuilt chunk collections for the interior and boundary/end variants;
- the pillar path similarly consumes dedicated pillar-fracture, top, bottom and
  debris collections;
- damage is carried as scalar/ID attributes such as `damage_total`, `damage`,
  `chunk_id`, `islands`, `U`, `V`, `range`, `.curvature` and `ends`;
- random selection, deletion, rotation and scaling are applied per tile/island/chunk.

The inspected concrete family binds prebuilt collections corresponding to a general
interior fracture grid plus left, right, top and bottom end variants. The asphalt
family follows the same pattern with asphalt-specific collections. These exact
asset names are source evidence only; Jerico must define its own independent chunk
families and naming.

Therefore the generic mechanism is better described as:

```text
procedural base assembly
  -> regular semantic tiles/islands
  -> scalar damage field per tile/island
  -> classify interior/boundary role
  -> deterministic damaged/healthy choice
  -> replace damaged tile with a pre-authored fractured variant
  -> randomize transform within bounded rules
  -> decorate with interior material / rebar / debris
  -> cook the result
```

This is much cheaper and more controllable than arbitrary fracture of an unknown
continuous mesh. It also explains how the source can extend damage seemingly
“infinitely” along a path without solving a new fracture topology at every point.

## Finding 1: damage is a composable scalar field

The main curve/ground graphs explicitly store and reuse a `damage_total` field.
The visible authoring controls and extracted links show contributions from:

- range/distance along the generated strip;
- path curvature;
- distance from the middle;
- threshold/falloff controls;
- procedural noise scale/displacement/seed;
- start/end range and fade-by-length;
- distance to optional object/collection effectors;
- barrier/boundary exclusion;
- separate final-distance/final-damage shaping passes.

The internal frames are labeled `Total_DAMAGE`, `Final TotalDamage`, `Final Damage`
and `exclude boundary`, confirming that damage selection is a staged field pipeline
rather than a topological crack-propagation solve.

A reusable Jerico abstraction should therefore be an explicit **DamageField**:

```text
DamageField(x, semanticContext) -> [0,1]
```

with composable sources/operators, for example:

```text
DistanceToPath
DistanceAlongPath / Ends / Middle
Curvature
Noise(seed, scale)
DistanceToObject / DistanceToCollection
AuthoredMask / SemanticMask
BoundaryProtect
Threshold / SmoothFalloff
min / max / add / multiply / remap
```

The field should be deterministic and inspectable. The geometry consumer then
decides what a damage value means: replace a tile, remove it, offset it, expose
interior material, spawn debris, or pass the scalar to a material/decal layer.

This separation is much more useful for Jerico than a single “fracture” modifier.

## Finding 2: path curvature is a geometric mask source

The `.Curvature` helper samples neighboring points on a curve, builds the incoming
and outgoing segment vectors, normalizes them and takes their dot product before
remapping the result. Endpoint handling is explicit.

At a high level:

```text
v0 = normalize(P(i)   - P(i-1))
v1 = normalize(P(i+1) - P(i))
straightness = dot(v0, v1)
curvatureProxy = remap(straightness)
```

The exact remap/presentation is asset-specific; the reusable concept is not.
Jerico's canonical spline/path backend should expose a curvature/turning-angle field
so road damage, support density, cable attachment, banking, debris and material
wear can share the same source instead of recomputing it per tool.

## Finding 3: the base surface carries explicit longitudinal/lateral identity

The curve and ground variants construct/resample path geometry, build a mesh strip
and store explicit attributes named `U`, `V`, `range` and `.curvature`. The mesh
also receives a stable-ish island identifier used by later damage/replacement
stages.

That is a strong architecture pattern:

```text
canonical path
  -> resample
  -> cross-section/profile
  -> strip/tile mesh
  -> longitudinal U + lateral V + island/tile identity
  -> damage/deformation/decorators
```

Procedural assemblies know more about their topology than a generic mesh after the
fact. Jerico should preserve this semantic lineage during generation and cook. It
can drive UVs, boundary variants, materials, damage, scatter and later bake jobs
without rediscovering “which piece is where” from triangles.

## Finding 4: boundaries are classified, not left to random replacement

Two helpers make boundary intent explicit.

### `.Is Edge Boundary`

This helper reads mesh-edge neighbor face count and tests against one adjacent face,
then evaluates that boolean on the required domain. Conceptually it is the standard
mesh-boundary test:

```text
boundaryEdge = adjacentFaceCount == 1
```

### `.BoundaryIndexes`

This helper reads the stored integer `U` and `V` coordinates, computes min/max
statistics and compares each tile against those extrema. It builds a compact
boundary classification used to choose appropriate replacement variants.

That explains the dedicated left/right/top/bottom fracture collections observed in
the concrete and asphalt wrappers. A tile on an outer edge should not use the same
chunk family as a fully surrounded interior tile because it would expose the wrong
profile or leave obvious seams.

### Jerico relevance

Jerico should formalize **semantic tile role** instead of hard-coded collection
selection:

```text
Interior
BoundaryLeft
BoundaryRight
BoundaryTop
BoundaryBottom
Corner variants as required
```

A future `FractureTileFamily` can bind Jerico-owned healthy/damaged meshes per role.
The semantic role is stable metadata, not inferred repeatedly from rendered mesh.

## Finding 5: `.ScatterTiles` is deterministic damaged-variant selection

`.ScatterTiles` is the core replacement/deformation helper
(**131 nodes / 156 links**). It consumes:

- source geometry/points;
- a chunk collection;
- scale/rotation/randomization;
- seed and variant/index control;
- delete probability;
- chunk scale/rotation;
- `Preview Chunks`;
- `Stable For Animation`.

The graph reads `damage_total` and `islands`, computes per-island averages and
centers, uses seeded random values, records a `final_damage` attribute, instantiates
collection members with `Pick Instance`, then applies bounded rotation/scale and
optional deletion/preview switching.

The reusable contract is therefore:

```text
for each semantic tile/island:
    d = aggregate(DamageField over tile)
    id = StableTileId(tile)
    r = Hash(seed, id)
    state/variant = choose(d, r, tileRole)
    transform = boundedTransform(d, Hash(seed2, id))
    emit healthy or fractured Jerico-owned variant
```

This is precisely the sort of operation that should be deterministic and cacheable
in Procedural Studio/PWAS.

`Stable For Animation` is especially instructive: randomness tied to stable semantic
identity is preferable to randomness tied to transient evaluation order. Jerico's
version should make that guarantee explicit rather than expose it as a late switch.

## Finding 6: surface, interior, rebar and debris are separate semantic layers

The main graphs expose separate surface/interior material controls plus explicit
rebar and debris authoring paths. Rebar helpers create curve/profile geometry, while
debris is distributed/instanced separately from the core damaged tile.

This suggests the right Jerico decomposition:

```text
Structural shell / healthy surface
Fractured replacement piece
Interior fracture surface material
Rebar/reinforcement decorator
Debris decorator
Optional environment scatter
```

Do not bake all of these concepts into one mesh-generation algorithm. They have
different cost, material, collision and gameplay requirements.

For example, a concrete wall may use visible rebar, while asphalt should not. A
small crack may only need a material/decal mask and no replacement chunk at all.

## Finding 7: curve and ground variants are assembly tools, not only damage tools

The `To Curves` and `To Ground` groups contain substantial path/road construction
logic beyond replacement chunks. Their exposed controls include width, tile scale
/resolution and height, path normal, material override, damage field controls,
side barriers/railings, rebar/debris, path/around/below-road scatter and optional
surface snapping.

The ground route also contains road-shoulder, height/smoothing and road-mask logic.
Both routes use curve-to-mesh, resampling, subdivision, extrusion, nearest-surface,
raycast, scatter and UV/attribute storage.

Jerico should not reproduce this as one enormous node-equivalent object. Split the
problem into reusable authoring stages:

```text
Path Assembly
  -> semantic strip/tile generation
  -> DamageField
  -> damaged-variant resolver
  -> structural decorators (barrier/rebar)
  -> debris/environment scatter
  -> material/UV lineage
  -> preview/final cook
```

That decomposition aligns with existing Path Assembly, scatter and material/bake
research rather than creating an `InfiniteFracture` subsystem.

## Finding 8: pillar damage uses the same field + stable chunk-ID idea

`Infinite Fracture (Pillars)` exposes top, bottom, both-ends, middle and noise-based
damage controls, plus outer/interior delete probability, chunk transform controls,
debris and rebar.

The graph stores `damage` and `chunk_id`, creates a repeated/grid-like pillar section
along a vertical path and chooses from dedicated pillar fracture/top/bottom/debris
collections. It then randomizes/deletes/rotates/scales by chunk identity and damage.

This reinforces that the technique is general **semantic modular destruction**:
roads/bridges use `(U,V,island)` identity; pillars use `chunk_id`; both resolve a
damage field into prebuilt variants.

Jerico can generalize both through one stable element-ID contract.

## Finding 9: the `Cracks` tree is material damage, not topology fracture

The separate `Cracks` shader tree mixes image textures, normal maps, a damage scale,
roughness/gamma controls and displacement. It is an appearance layer.

Jerico should preserve an explicit distinction:

```text
DamageField -> Material/Decal crack       (cheapest)
DamageField -> local displacement/bake     (medium)
DamageField -> fractured replacement mesh  (higher)
DamageField -> dynamic rigid chunks        (gameplay/physics)
True arbitrary mesh fracture               (separate expensive capability)
```

The source library itself demonstrates the value of choosing the cheapest visual
representation that satisfies the shot/gameplay requirement.

## Recommended Jerico architecture

The source suggests the following independent Jerico-owned contracts.

### 1. `DamageField`

A deterministic scalar-field graph over authored geometry/path semantics. It can be
CPU-authored and optionally GPU-evaluated for preview, but its final result must be
cacheable/cookable.

Inputs may include:

- world/local position;
- path arc length and normalized range;
- U/V/tile role;
- curvature;
- distance to authored effectors;
- noise/hash;
- semantic/material mask;
- protected boundaries.

### 2. `ProceduralElementId`

Stable identity for a generated tile/chunk/region independent of transient mesh
ordering. All random choices use `Hash(globalSeed, stableId, stageSalt)`.

### 3. `VariantFamily`

A Jerico-owned family of compatible pieces:

```text
healthy/interior damaged
left/right/top/bottom boundary damaged
optional corners
material/interior tags
attachment sockets / rebar sockets
collision proxy
LOD/cook variants
```

The family is data; selection logic remains generic.

### 4. `DamageResolver`

Maps `DamageField + stableId + tileRole + seed` to state/variant/transform/deletion.
It should support preview and final quality without changing final determinism.

### 5. decorators

Rebar, rubble, dust, decals, edge dirt and environment scatter consume the same
DamageField and semantic metadata but remain independent passes.

## Direct relevance to spline destruction

This source strongly supports the planned spline-driven destruction workflow, but
with a cheaper and more controllable implementation than “boolean everything.”

For a Jerico destruction spline:

```text
spline
  -> distance/falloff DamageField
  -> local affected corridor only
  -> preserve/protect outside boundary
  -> subdivide/generate semantic cells only inside the corridor
  -> choose fractured variants or local procedural pieces
  -> generate crack/decal/material mask from the same field
  -> add edge-dirty/noise treatment so the affected boundary is not a clean box
  -> assign exterior/interior materials explicitly
  -> optional debris/rebar decorators
  -> preview -> accept -> cook
```

For authored modular walls/roads/bridges this prefractured-family route is likely the
best default. Use SDF/boolean/remesh processing only when the source must be cut at
arbitrary topology not representable by semantic modules.

## Runtime destruction boundary

This research should not be interpreted as a replacement for runtime physics.
A practical Jerico path is:

```text
AUTHORING:
DamageField + prefractured variants -> cooked dormant chunk metadata

RUNTIME:
trigger/gameplay event
  -> activate selected pre-cooked chunks
  -> Jolt bodies/constraints/impulses
  -> hide/replace healthy section
  -> debris/VFX/audio
```

That gives real gameplay destruction without performing a general mesh fracture
solve during the frame. A future arbitrary runtime fracture solver, if ever needed,
remains a separate capability with its own cost/acceptance gate.

## Performance guidance

| Technique | Jerico disposition | Reason |
| --- | --- | --- |
| Damage scalar field | interactive + cook | cheap, composable, cacheable |
| Path U/V + semantic tile IDs | generation-time metadata | removes repeated topology inference |
| Prefractured variant replacement | preferred default for modular destruction | predictable and much cheaper than arbitrary fracture |
| Stable hashed randomization | always | deterministic rebuild/animation |
| Boundary-specific variants | data-driven | prevents visible open seams |
| Curvature/noise/distance masks | interactive | inexpensive field sources |
| Raycast/proximity ground snapping | authoring quality tier | batch/cache; avoid massive runtime use |
| Rebar/debris scatter | authoring or event-time decoration | optional visual cost |
| Material/decal crack only | cheapest damage representation | use when silhouette/topology need not change |
| Arbitrary mesh boolean/fracture | last resort / offline | highest topology and validation cost |

This source is strong evidence **against** paying for a robust GPU mesh-boolean
fracture solver merely to reproduce the visual class demonstrated here.

## Independent RED/acceptance tests

When M8 legally reaches this capability, begin from Jerico-owned tests rather than
translating the source graphs.

### Damage field

- same geometry/settings/seed -> identical sampled field;
- path start/end/middle masks have explicit monotonic falloff;
- curvature contribution increases only where the chosen curvature proxy changes;
- protected boundaries remain zero-damage when enabled;
- effector/object distance combines deterministically with noise and thresholds.

### Stable identity

- reordering vertices/faces without changing generated semantic cells does not
  change element IDs;
- preview density/visibility does not change final variant choice;
- saved/cooked rebuild reproduces the same variants and transforms.

### Variant family

- every semantic role required by a generator has a compatible variant or explicit
  fallback;
- neighboring healthy/damaged pieces remain within gap/overlap tolerance;
- exterior/interior material lineage is preserved;
- collision proxy and rendered piece share stable identity.

### Path/road assembly

- U/V/tile identity is deterministic across rebuild;
- path resampling density does not move authored endpoints;
- curvature, width and shoulder changes preserve semantic lineage where possible;
- boundary roles are correct at every outer edge and corner.

### Runtime activation

- dormant cooked chunks cost no dynamic body until activated;
- activation swaps/hides the healthy section atomically;
- Jolt body/constraint creation follows the canonical physics authority;
- save/load reproduces activated state and stable chunk identity;
- no Blender or purchased source asset is required in Shipping.

## What this extraction does **not** prove

- It does not execute or visually render the Blender graphs.
- It does not benchmark the original library.
- It does not prove physical crack propagation, stress analysis or sediment/fracture
  mechanics.
- It does not prove a general mesh-boolean fracture path; the observed architecture
  instead relies on semantic tiles/chunks and prebuilt collections.
- It does not authorize copying any source collection, mesh, graph, material or
  texture into Jerico.
- It does not require the future Jerico implementation to have the same UI or
  parameter grouping.

## Disposition

**KEEP as M8 research input.** The highest-value result is architectural: the visual
class called “Infinite Fracture” can be achieved through a deterministic damage
field over semantic modular elements plus prefractured variant families, boundary
classification and independent decorators. This is substantially cheaper, safer and
more controllable than treating arbitrary real-time mesh fracture as the default.

Jerico should implement the generic contracts independently, reuse its canonical
path/spatial/material/physics authorities, and reserve true arbitrary fracture for
cases where the cheaper semantic-variant route demonstrably cannot satisfy the
asset or gameplay requirement.
