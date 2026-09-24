# M8 Stair Builder 2.02 — Geometry-Nodes Mathematical Research Input

**Milestone:** M8  
**Category:** path-driven modular construction / curve deformation / repeated modules / railings / supports / parametric UV  
**Status:** `.blend` and Geometry Nodes structure inspected; research input ready; native Jerico implementation not started  
**Owner timing:** read and act on this only when M8 is the legal `ACTIVE_SLICE`  
**Runtime dependency:** none intended  
**Inspected archive SHA-256:** `cac94d44dcad7ef5c6768d6087b1f929a03b429a75b19fda29b2a198f4fade4f`

This document intentionally treats Stair Builder as a **mathematics and procedural
construction reference**, not as a request to add a staircase-only subsystem.
The useful abstractions are applicable to fences, catwalks, bridges, railings,
roads, pipes, cables, modular walls, platforms, cliff walkways, trims and other
assets distributed or deformed along a path.

It does not replace `ACTIVE_SLICE.md`, `HANDOFF.md`, the M8 closure plan, the M8
Blender authority, or Jerico's existing spline/path authority.

## 1. Package inspection

The supplied RAR contains primarily:

```text
Stair builder v2.blend       ~51.6 MB
Stair Builder documentation  HTML
```

There is no primary Python addon implementation to translate. The product is a
large Blender Geometry Nodes asset.

The `.blend` was inspected directly at Blender datablock/SDNA level. It identifies
itself as a Blender 3.6-era file and contains **33 Geometry Node trees**.

Representative node groups include:

```text
Stair Builder v2
SB Curve input
SB Step bases
SB Extrude
SB Railing
SB UV Mapping
SB whole process
SB Bases process
SB Decorate
SB Decorate process
Railing
Railing array
Curve safe extension
X_Curve_Deform
X Curve Deform Z Up
Stairs UV Skew
Sides UV mapping
Sort faces
Side selection
Side selection w bound
Capture Instance Dimensions
Random auto scale
```

Representative graph sizes found in the inspected file:

```text
main Stair Builder v2 : 197 nodes
SB Extrude            : 180 nodes
SB Railing            : 194 nodes
SB Step bases         : 145 nodes
SB UV Mapping         : 84 nodes
Sides UV mapping      : 106 nodes
Railing array         : 109 nodes
X_Curve_Deform        : 37 nodes
Stairs UV Skew        : 20 nodes
```

The exact proprietary graph must not be copied or redistributed. This research
extracts the **general mathematical problems and architectural patterns** Jerico
can reimplement independently.

---

# Part A — General architecture hidden inside a “stair” tool

## 2. The real abstraction

The product can be reduced to a much more general procedural system:

```text
path / polyline / curve
  -> sanitize + measure
  -> choose module count / spacing
  -> construct local frame along path
  -> distribute repeated modules
  -> scale/offset/orient modules
  -> optionally deform continuous parts along path
  -> generate secondary rails/supports/end pieces
  -> classify generated faces
  -> create parametric UVs
  -> publish geometry + semantic attributes
```

That is a **Path Modular Assembly** problem, not a staircase-specific problem.

Jerico should reuse its one canonical spline/path authority and build generic
consumers on top of it.

Potential assets:

- stairs;
- ladders;
- fences;
- handrails;
- guard rails;
- catwalks;
- bridges;
- modular walls;
- curbs;
- road barriers;
- pipes and cable trays;
- cliff walkways;
- mine tracks;
- tunnel supports;
- repeating decorative trims;
- chains/segment-based ropes where rigid modules are appropriate.

---

# Part B — Module count and distribution

## 3. Step/module quantization

The inspected `SB Step bases` graph measures the available line/path length and a
representative module dimension, then uses divide/floor-style operations to obtain
a stable module count.

The generic problem is:

```text
L = usable path length
w = preferred module footprint along path
n = max(1, floor(L / w))
```

But production placement needs a declared remainder policy.

### 3.1 Fit modes

Jerico should support explicit policies:

```text
KeepSizeAddGap
ScaleModulesToFit
TrimLastModule
DistributeRemainder
FixedCount
FixedSpacing
```

For `n` modules and target usable length `L`:

**Uniform scale-to-fit**

```text
w_fit = L / n
scale_x = w_fit / w_source
```

**Uniform gap distribution**

```text
gap = (L - n*w_source) / max(n-1, 1)
```

**Centered leftover margins**

```text
margin = (L - ((n-1)*spacing + w_source)) / 2
```

The tool should never hide which quantity it changed: size, gap, margin or count.

## 4. Arc-length placement

For a curve `C(s)` parameterized by arc length `s in [0,L]`, repeated module
centers can be evaluated as:

```text
s_i = start_margin + i*spacing + 0.5*module_length
p_i = C(s_i)
```

For curves whose native parameter `t` is not arc length, Jerico should use the
existing path arc-length lookup/table rather than pretending equal `t` increments
produce equal spacing.

Required properties:

- stable under curve tessellation changes;
- deterministic;
- no accumulating integration drift across many modules;
- supports open and closed paths;
- exposes residual length/error.

## 5. Height/run decomposition for stairs

A staircase-specific consumer can map total horizontal run `R` and elevation `H`
to `n` rises:

```text
rise = H / n
run  = R / n
slope_angle = atan2(H, R)
```

If authored constraints include maximum rise/minimum run, choose `n` from the
intersection of legal intervals:

```text
n >= ceil(H / max_rise)
n <= floor(H / min_rise)
```

Then optimize for preferred rise/run rather than hard-coding a building code into
the generic path system.

This same quantization pattern applies to:

- ladder rung spacing;
- support-column spacing;
- tunnel rib spacing;
- fence post spacing;
- road marker spacing.

---

# Part C — Path frames and deformation

## 6. Curve deformation discovered in the asset

The inspected `X_Curve_Deform` family uses curve sampling, tangent information,
vector cross products, vector scaling and addition to map local geometry onto a
curve.

The transferable mathematical form is:

```text
P_world(s,u,v) = C(s) + u*B(s) + v*N(s)
```

where:

- `C(s)` is path position;
- `T(s)` is tangent;
- `N(s)` and `B(s)` define the local transverse frame;
- `(u,v)` are local cross-section coordinates.

This is the core of sweeps, modular deformation and side-offset geometry.

## 7. Naive cross-product frame

A simple frame uses a reference up vector `U`:

```text
T = normalize(dC/ds)
B = normalize(cross(T, U))
N = cross(B, T)
```

This works until `T` becomes parallel/nearly parallel to `U`, where the cross
product becomes unstable.

Jerico must not use this as the sole production frame.

## 8. Rotation-minimizing frame

For general path assets use a rotation-minimizing/parallel-transport frame.

Given previous tangent/frame `(T0,N0,B0)` and new tangent `T1`, rotate the previous
frame by the minimal rotation that maps `T0` to `T1`.

One conceptual construction:

```text
axis = cross(T0, T1)
angle = atan2(length(axis), dot(T0,T1))
N1 = rotate(N0, normalize(axis), angle)
B1 = cross(T1, N1)
```

with robust special cases for parallel and anti-parallel tangents.

Benefits:

- minimizes unwanted roll;
- stable rail/fence orientation;
- useful for non-circular profiles;
- shared with pipes, cables, roads, camera rails and spline IK.

Jerico already has spline/path work. Audit and extend the canonical frame
implementation rather than adding a Stair Builder-specific frame system.

## 9. Authored up / roll

Some assets require explicit world-up behavior or authored roll. Define policy:

```text
FramePolicy =
    RotationMinimizing
    WorldUpProjected
    SurfaceNormalDriven
    AuthoredRoll
```

Authored roll `r(s)` applies around tangent:

```text
N' = rotate(N, T, r(s))
B' = cross(T, N')
```

## 10. Closed-loop correction

A transported frame around a closed curve can return with residual twist. Measure
closure rotation `theta` and distribute a correction along normalized arc length:

```text
r_correction(s) = -theta * (s/L)
```

This matters for closed railings, tracks, pipes and looped trim geometry.

## 11. Local-to-path deformation

For source local coordinate `x` used as longitudinal distance:

```text
s = s0 + x * longitudinal_scale
P = C(s) + y*B(s) + z*N(s)
```

For normalized source bounds `[xmin,xmax]`:

```text
u = (x - xmin) / max(xmax-xmin, eps)
s = lerp(s_start, s_end, u)
```

This generalizes the inspected curve-deform group without copying its node graph.

---

# Part D — Modular instances vs continuous deformation

## 12. Two path-construction modes

A robust Jerico system should separate:

### Discrete modules

Evaluate one transform per module:

```text
M_i = Frame(C(s_i)) * LocalOffset * LocalScale
```

Best for:

- steps;
- fence posts;
- bolts;
- lamps;
- tunnel braces;
- railing balusters;
- sleepers/ties.

### Continuous deformation/sweep

Map each source vertex to path coordinates.

Best for:

- handrails;
- pipes;
- curb strips;
- beams following curved routes;
- flexible trim profiles.

Do not convert everything to unique geometry if an instance transform is enough.

## 13. Instance dimension capture

The product has explicit helper groups for capturing instance dimensions and
scaling repeated parts automatically.

Jerico should represent module bounds explicitly:

```text
ModuleDescriptor {
    local_bounds
    primary_axis
    pivot_policy
    semantic_connectors[]
}
```

Then placement math never depends on an arbitrary imported object origin.

## 14. Randomized modules

The asset exposes random scale, rotation, flip/turn and collection selection.

Jerico must make randomness deterministic:

```text
seed_i = Hash(recipe_seed, module_index, semantic_slot, source_revision)
```

The random stream should survive rebuilds and LOD generation unless an input that
semantically changes placement also changes.

Avoid using global RNG state or evaluation order.

## 15. Sequential vs random collections

Useful generic policies:

```text
Sequential
RandomWeighted
Pattern
SemanticChoice
```

Example:

```text
choice_i = items[i mod item_count]
```

or deterministic weighted selection from `seed_i`.

This turns Stair Builder's decorative-step mechanism into a general modular
assembly system.

---

# Part E — offsets, widening and parallel paths

## 16. Lateral path offset

Railings/sides require offsetting from the center path.

For local side sign `q in {-1,+1}`:

```text
P_side(s) = C(s) + q * width(s)/2 * B(s)
```

For a 3D path this is frame-dependent and not equivalent to offsetting a 2D spline
in XY.

Potential uses:

- left/right rail;
- road edge;
- catwalk side beams;
- cable tray lanes;
- double tracks;
- cliff walkway barriers.

## 17. Corner behavior

At polyline corners, naive offsets can spike. A 2D planar miter example:

```text
m = normalize(n_prev + n_next)
miter_length = offset / dot(m, n_next)
```

Clamp the miter length and expose join policy:

```text
Miter
Bevel
Round
Break
```

For arbitrary 3D paths use the local path plane/frame and validate self-intersection.

## 18. Widening profile

Allow width as a scalar field `w(s)`:

```text
P_left  = C(s) - 0.5*w(s)*B(s)
P_right = C(s) + 0.5*w(s)*B(s)
```

This immediately generalizes from stair width to roads, platforms, riverside
walkways and variable-size cable trays.

---

# Part F — railings and secondary structures

## 19. Railing as a secondary path consumer

The inspected asset has separate railing and railing-array logic. The general
architecture should be:

```text
primary path
  -> derive side path(s)
  -> place posts/modules by spacing policy
  -> generate/deform continuous top rail
  -> optional end caps/end modules
```

Do not duplicate curve math in the railing subsystem.

## 20. Post spacing

For railing usable length `L` and maximum/preferred spacing `d`:

```text
segments = max(1, ceil(L / d))
actual_spacing = L / segments
post_count = segments + 1
```

If ends are hidden/overridden, adjust endpoint policy explicitly rather than
changing the interior distribution unpredictably.

## 21. Vertical vs path-normal rail orientation

Rail posts often want world vertical even when the path slopes:

```text
post_up = world_up
```

while the continuous rail follows path tangent. Therefore secondary assets need
independent orientation policies.

This distinction matters for:

- guardrails on ramps;
- light poles along roads;
- mine/tunnel support ribs;
- vegetation placement.

## 22. End pieces

The graph exposes upper/lower end controls and offsets. Jerico should model end
semantics generically:

```text
PathEndpoint {
    side: Start | End
    frame
    tangent
    connector
    end_module_policy
}
```

This can later drive joins between modular assemblies.

---

# Part G — support generation

## 23. Supports as constrained modules

A stair support is a special case of placing geometry between a path/platform and
a support surface.

Generalize to:

```text
anchor point A
support direction D
query target surface
hit point B
length = dot(B-A, D)
```

Then fit/deform/scale a support module between A and B.

Potential uses:

- bridge piers;
- platform supports;
- mine braces;
- pipes/cable supports;
- cliff walkway legs;
- procedural scaffolding.

## 24. Surface alignment

The inspected product exposes alignment-to-floor behavior. Jerico should use an
explicit spatial query:

```text
hit = Raycast(anchor, -support_up, max_distance, support_mask)
```

Then record:

- hit object/semantic surface;
- distance;
- normal;
- confidence/validity;
- source revision.

No support should silently snap to arbitrary geometry.

## 25. Scale-to-span

For source support length `L0` and target span `L`:

```text
scale_primary = L / max(L0, eps)
```

Pivot policy determines translation:

```text
StartAnchored
EndAnchored
Centered
```

Use authored connectors when available instead of bounding-box guesses.

---

# Part H — extrusion and semantic faces

## 26. Extrusion is not only geometry creation

The inspected `SB Extrude` graph heavily uses boolean selection masks after
extrusion. It distinguishes semantic face classes such as bottom, sides and inside.

This is important for Jerico: generated geometry should publish **semantic regions
at generation time** instead of reconstructing them later from normals/materials.

Suggested attributes:

```text
GeneratedRegion =
    Top
    Bottom
    SideLeft
    SideRight
    Front
    Back
    Interior
    Support
    Rail
    Module
```

Generic generators can use a broader stable semantic-ID mechanism rather than this
exact enum.

## 27. Extrude along vector

For region vertices `p_i` and extrusion vector `e`:

```text
p'_i = p_i + e
```

The nontrivial part is topology/region lineage:

```text
source face -> cap face
source boundary edge -> side face
```

Store lineage so materials, UVs, collision and selection can consume it.

## 28. Floor alignment

If the bottom should meet a support plane/surface, compute required offset from
spatial queries or known ground plane, not by destructive global transform.

For plane `(n,d)` and candidate bottom point `p` moving along direction `v`:

```text
t = -(dot(n,p) + d) / dot(n,v)
```

with parallel/invalid guards.

For arbitrary terrain use bounded raycasts/closest-point queries and a declared
alignment policy.

---

# Part I — parametric UV math

## 29. Why Stair Builder's UV work matters

Repeated/path-deformed assets frequently suffer from stretched or skewed UVs. The
inspected asset includes dedicated UV groups for sides, stair-line skew and
extrusion correction rather than relying only on a generic unwrap.

The general lesson:

> a procedural generator usually knows a better parameterization than a generic
> unwrap solver.

## 30. Path-distance UV

For a strip/sweep use arc length for U:

```text
U = s / texture_world_repeat
V = cross_section_coordinate / texture_world_repeat_v
```

This keeps texture scale stable through curve tessellation changes.

## 31. Bounds normalization

For coordinate `x` in authored bounds `[xmin,xmax]`:

```text
u = (x-xmin) / max(xmax-xmin, eps)
```

Then:

```text
uv = uv_offset + Rotate2D(uv_scale * base_uv, rotation)
```

## 32. Stair/run skew compensation

The inspected stair UV-skew group uses map-range, multiply/subtract/divide-style
operations tied to step X size and geometry bounds. The transferable model is an
affine compensation between the stair's stepped parameter space and its projected
surface/path parameterization.

One useful representation is:

```text
[u']   [1 k][u] + offset
[v'] = [0 1][v]
```

where `k` is derived from rise/run or path slope when continuous visual alignment
across stepped faces is desired.

Jerico should compute `k` from the generator's own known geometry rather than infer
it after the fact.

## 33. Semantic UV charts

Because generation knows top/side/front/back lineage, use that lineage to choose
chart axes consistently:

```text
Top    -> longitudinal/lateral
Side   -> longitudinal/vertical
Front  -> lateral/vertical
```

This is deterministic and gives better baking/trim behavior than a generic unwrap
for many procedural assets.

---

# Part J — safe curve extension and endpoint handling

## 34. Curve endpoint extension

The asset includes a `Curve safe extension` helper. Generic reason: deformation or
sampling near endpoints often requires a small domain beyond the visible source
range so caps/modules do not collapse or extrapolate unpredictably.

Jerico path queries should define endpoint behavior:

```text
Clamp
LinearExtrapolate
Wrap
Reject
```

For linear extension:

```text
C(-d)   = C(0) - d*T(0)
C(L+d)  = C(L) + d*T(L)
```

This is useful for rail caps, transition modules and tangent-stable endpoints.

---

# Part K — general reusable Jerico subsystem

## 35. Do not build a StairSystem first

The reusable backend should be closer to:

```text
PathAssemblyRecipe
PathModuleDistributor
PathFrameEvaluator
PathGeometryDeformer
ParallelPathBuilder
PathSupportBuilder
PathEndpointResolver
ProceduralRegionPublisher
ProceduralUvMapper
```

Then stairs are a recipe/consumer:

```text
StairRecipe =
  primary path
  + rise/run/count policy
  + step module
  + optional base/extrusion
  + optional rail/support recipes
```

Fences, roads and catwalks consume the same backends with different recipes.

## 36. Proposed PathAssemblyRecipe

Conceptual contract:

```text
PathAssemblyRecipe {
    path_asset
    longitudinal_range
    distribution_policy
    module_sources[]
    frame_policy
    lateral_offset
    width_profile
    scale_policy
    random_seed
    endpoint_policy
    support_policy
    semantic_output_policy
    uv_policy
}
```

Everything affecting output participates in deterministic hash/cache identity.

## 37. Source module contract

```text
PathModuleSource {
    asset_guid
    local_bounds
    primary_axis
    pivot
    start_connector
    end_connector
    allowed_scale_axes
    weight
}
```

This avoids fragile object-origin assumptions.

---

# Part L — CPU/GPU and performance

## 38. CPU-first stages

Prefer CPU/jobs initially for:

- curve sanitation;
- arc-length table generation;
- module-count decisions;
- endpoint/join policy;
- deterministic recipe publication;
- semantic topology construction.

## 39. GPU-friendly stages

Potential GPU work when scale justifies it:

- large batched path/frame evaluation;
- vertex deformation along a precomputed frame table;
- spatial support queries if Jerico already has GPU query infrastructure;
- preview-only mass instancing.

Do not move a few hundred stair/fence modules to bespoke GPU compute just because
it is possible. Instance transforms are already cheap; profile first.

## 40. Runtime vs cook

Default path assemblies should bake/cook to ordinary Jerico assets or instance
streams. Heavy Geometry-Nodes-like generation should not run every frame.

Runtime procedural regeneration is justified only where gameplay/editor behavior
requires it and must use normal Jerico jobs/budgets.

---

# Part M — integration with existing M8 work

## 41. Relation to the existing curve-asset M8 directive

M8 already requires hybrid curve assets for:

- tubes;
- cables/ropes;
- webs;
- rails;
- ladders;
- walkways.

Stair Builder reinforces that decision. It provides a concrete research example of
why one high-quality path/frame/distribution backend can serve many products.

This document does **not** authorize a second spline representation.

## 42. Shared services with other addon research

### Zen UV
Procedural semantic surfaces can produce deterministic UV charts and texel-density
inputs directly.

### SimpleBake
Path-generated high/low assets can use normal/AO/ID baking; generated semantic
regions become stable bake masks.

### Smart Segmentation
If imported modules lack semantics, segmentation can suggest regions, but generated
assets should publish semantics directly.

### Smart Remesh
Continuous deformed/generated surfaces may use remesh/cleanup later, preserving
path/region provenance where possible.

---

# Part N — proof matrix

## 43. Path/frame tests

- straight horizontal path;
- straight vertical path;
- arbitrary 3D sloped path;
- S-curve;
- tight bend;
- tangent nearly parallel to world up;
- anti-parallel tangent transition;
- closed loop with twist correction;
- repeated points / zero-length segment rejection.

Assertions:

- finite orthonormal frame;
- no sudden unexplained roll;
- deterministic output;
- bounded closure error;
- consistent handedness.

## 44. Distribution tests

- path shorter than one module;
- exact integer multiple;
- remainder;
- fixed count;
- fixed spacing;
- large module count;
- deterministic random collection;
- start/end margins.

Assert count, spacing, coverage and residual error.

## 45. Deformation tests

- known straight line equals affine transform;
- quarter circle preserves cross-section dimensions within tolerance;
- no fold on moderate curvature;
- declared behavior when bend radius is smaller than module/profile half width;
- normals/tangents valid after deformation.

## 46. Supports

- flat ground;
- sloped ground;
- missing ground;
- overhang/ambiguous hit;
- max-length rejection;
- stable connector/pivot behavior.

## 47. UV tests

- world-scale repeat invariant under curve tessellation;
- continuous side texture across module boundaries;
- known step skew correction;
- mirrored side policy;
- generated top/side/front region charts stable under rebuild.

## 48. Semantic lineage

For every generated triangle/instance, prove stable lineage to:

```text
recipe GUID
path segment/range
module semantic slot/index
source module asset
region class
```

LOD/cook must preserve the required semantic identity.

---

# Part O — license / clean-room boundary

## 49. Product asset boundary

The supplied Stair Builder file is a purchased/proprietary Geometry Nodes asset,
not Jerico source. Public product terms allow use of generated output but do not
make the node graph a Jerico code dependency.

Therefore:

- do not commit or redistribute the `.blend` in JericoEngine;
- do not serialize/copy the complete node graph into documentation;
- do not mechanically translate every node one-for-one to C++;
- use this document as a problem/behavior/math specification;
- implement generic algorithms independently under Jerico's chosen license;
- generated assets and external-license obligations must be handled by the normal
  asset provenance system.

The analysis intentionally records high-level equations, data flow, tests and
architecture rather than reproducing the proprietary node graph.

---

# 50. M8 decision

**HIGH-VALUE M8 INPUT, NOT BECAUSE IT MAKES STAIRS.**

The best reusable ideas are:

1. quantized modular distribution over arc length;
2. deterministic remainder/fit policies;
3. robust rotation-minimizing path frames;
4. source-geometry deformation along curves;
5. parallel/laterally offset paths;
6. secondary arrays such as posts/railings/supports;
7. connector/end-piece semantics;
8. generated-face semantic lineage;
9. parametric path-distance UVs and skew correction;
10. deterministic random/sequential module selection.

A native Jerico **Path Assembly** backend built from these principles can power
stairs, railings, fences, roads, catwalks, modular walls, bridge parts, tunnel
supports, pipes and many other procedural tools without depending on Stair Builder
or Blender at runtime.