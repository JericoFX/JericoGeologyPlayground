# JericoGeologyPlayground — Procedural Web Surface / Shader-Generated Spider Web

**Owner request:** 2026-09-27  
**Status:** ACTIVE EXPERIMENT / preferred spider-web rendering path  
**Repository:** `JericoFX/JericoGeologyPlayground`  
**Codename context:** CHOTO = JericoEngine 1.2 codename. It is NOT an algorithm name.

---

# 0. Direct instruction

Implement a specialized spider-web system for the Playground.

Do **not** render ordinary spider webs as hundreds or thousands of tiny spline tubes.

The preferred workflow is:

```text
User places a Plane / Area / Anchors
            ↓
     Web Recipe / Web Graph
            ↓
   2D procedural web generator
            ↓
 GPU/CPU bake of Web Field / Texture
            ↓
      1 card when planar
   or a few cards when needed
            ↓
       cheap web material
```

The web should normally exist as a procedural texture-backed surface.

Real strand geometry is optional for:

- debug;
- very close LOD;
- selected interactive strands;
- physics;
- special silhouette cases.

The source of truth remains the Web Recipe / Web Graph, not the generated texture.

Do not modify JericoEngine from this repository.

---

# 1. Owner-facing workflow

The owner should be able to do this in the normal Playground:

```text
Spider Web
-> Place Web Surface

Option A:
    place one plane / rectangular area

Option B:
    click 3+ anchor points on world surfaces

Option C:
    draw/select an irregular bounded area

-> Generate Web
-> change seed/density/rings/irregularity/thickness
-> immediately see the web
-> switch debug modes
-> break selected strand
-> test cheap Card representation
```

No source-code editing should be required.

---

# 2. Core concept

A spider web is represented as a lightweight graph:

```cpp
struct WebNode
{
    StableId id;
    Vec3 position;
};

struct WebEdge
{
    StableId id;
    StableId a;
    StableId b;
    float radius;
    WebEdgeState state;
};
```

Exact Playground types may differ.

Required edge states:

- Intact
- Missing
- Broken
- Disabled

The graph is small semantic data.

The card texture is a derived render representation.

---

# 3. Authoring modes

## 3.1 Plane Web

User places one oriented rectangular patch.

Inputs:

- transform
- width
- height
- local normal
- seed

This is the cheapest and simplest case.

---

## 3.2 Anchor Web

User clicks world-space anchors.

Examples:

```text
wall point
ceiling point
pipe point
rock point
corner point
```

At least three valid anchors define a candidate web region.

The system derives a local 2D basis for generation.

---

## 3.3 Area Web

User marks an irregular bounded area.

The region acts as a clipping mask for generated strands.

This can later be driven by:

- polygon selection;
- painted mask;
- BSP face region;
- SDF/cavity region;
- PWAS proposal.

For the Playground, a simple polygon/rectangular area is enough for the first vertical.

---

# 4. Local 2D frame

Given a planar web patch with center `C` and orthonormal axes `U,V,N`:

For a 3D point `P`:

```text
x = dot(P-C, U)
y = dot(P-C, V)
```

All procedural web generation happens in this local 2D space.

Generated 2D points map back to world:

```text
P_world = C + U*x + V*y
```

This keeps spider-web mathematics cheap and simple.

---

# 5. Best-fit plane for clicked anchors

When anchors are not perfectly coplanar, fit a plane.

Centroid:

```text
C = (1/N) * sum(P_i)
```

Covariance:

```text
M = sum((P_i-C)(P_i-C)^T)
```

The eigenvector corresponding to the smallest eigenvalue is the candidate normal `N`.

For each anchor:

```text
d_i = dot(P_i-C, N)
```

Measure:

```text
maxError = max(abs(d_i))
rmsError = sqrt(mean(d_i*d_i))
```

If the anchors are planar enough:

```text
-> one Web Surface / card
```

If not:

```text
-> split into 2-4 patches
or
-> keep a ribbon/geometry fallback
```

Do not flatten obviously non-planar webs into one plane.

---

# 6. Procedural web generation

The first web family should support radial/orb-style webs and messy corner webs.

## 6.1 Hub

Choose or derive a hub position `H` in local 2D.

For a rectangular region a useful default is the center plus deterministic offset:

```text
H = Center + irregularity * hash2(seed)
```

Clamp the hub inside the allowed region.

---

## 6.2 Spokes

For boundary/anchor point `A_i`:

```text
S_i(t) = H + t*(A_i-H)
```

where:

```text
0 <= t <= 1
```

Each spoke becomes one or more semantic WebEdges.

---

## 6.3 Rings / spiral-like connections

For ring level `k` among `R` rings:

```text
baseT_k = k / (R + 1)
```

Add deterministic irregularity:

```text
t_i,k = baseT_k + amplitude * noise(seed, i, k)
```

Clamp to preserve ordering:

```text
0 < t_i,k < 1
```

Point on spoke:

```text
P_i,k = H + t_i,k * (A_i-H)
```

Connect:

```text
P_i,k -> P_(i+1),k
```

Optionally omit selected connections by deterministic probability to create old/broken webs.

---

# 7. Messy cobweb generation

Not every web should look like a perfect orb web.

A messy cobweb preset may:

- create multiple local hubs;
- create random but deterministic cross-links;
- bias anchors toward corners;
- use fewer complete rings;
- create dangling partial strands;
- leave missing sectors;
- vary thickness;
- create secondary short connections.

Every generated edge must still receive a stable ID.

Do not use frame-time/random-device randomness.

Use recipe seed + stable logical edge keys.

---

# 8. Sag / hanging strand shape

A visual strand between endpoints `A` and `B` may sag.

Cheap authored approximation:

```text
P(t) = lerp(A,B,t) - G * sag * 4*t*(1-t)
```

where:

- `G` is normalized gravity direction;
- `sag` is authored amplitude;
- `t in [0,1]`.

This gives:

- zero displacement at endpoints;
- maximum sag at the center.

For physics-enabled strands, the actual Jolt result may later replace this resting approximation.

---

# 9. Main rendering path: Web Field, not strand meshes

The normal web representation should be generated directly into a texture/field.

For each projected strand segment `A -> B` and texture/sample point `X`:

```text
t = clamp(
    dot(X-A, B-A) / dot(B-A, B-A),
    0,
    1)

Q = A + t*(B-A)

d = length(X-Q)

r = lerp(radiusA, radiusB, t)

phi = d-r
```

Interpretation:

```text
phi < 0  -> inside strand
phi = 0  -> strand boundary
phi > 0  -> outside strand
```

The generator evaluates the minimum `phi` over relevant web edges.

This is the core field used to create coverage and shading.

---

# 10. Coverage

Do not use one binary sample per texel.

Use bounded supersampling during generation.

For `M` samples inside a texel:

```text
coverage =
    count(phi(sample_j) <= 0) / M
```

Recommended Playground comparisons:

- 1 sample
- 4x4
- 8x8 evidence mode

Measure generation cost.

---

# 11. Distance / centreline field

For thin strands, also preserve:

```text
distanceToCentreline
localRadius
```

or equivalent signed field:

```text
phi = distanceToCentreline - localRadius
```

The runtime material can reconstruct a smooth edge:

```text
aa = max(epsilon, edgeSoftness * fwidth(phi))

alpha =
    1 - smoothstep(-aa, +aa, phi)
```

Compare visually and numerically against pure coverage texture.

---

# 12. Texture generation timing

Do NOT recompute the whole web field every rendered frame.

Preferred flow:

```text
recipe/anchor edit
      ↓
mark web dirty
      ↓
generate texture once
      ↓
cache Texture2D
      ↓
normal rendering only samples it
```

For editor interaction:

```text
drag anchor
-> regenerate asynchronously/debounced
-> publish completed texture
```

For cook:

```text
Web Recipe
-> deterministic generation
-> baked/compressed texture
-> runtime material
```

---

# 13. GPU generation

The preferred experiment is a Diligent-compatible compute or offscreen GPU generation path if practical.

Inputs should be compact:

- WebEdge list
- local endpoints
- radius
- state
- patch bounds
- seed/material parameters

Output textures may include:

- coverage
- signed/centreline field
- thickness
- material variation
- normal helper
- edge contributor metadata

Do not expose third-party GPU types as semantic source data.

If CPU generation is simpler for the first correct vertical, implement CPU first and use it as a reference/oracle, then compare a GPU implementation byte/tolerance-wise where applicable.

Do not delay all functionality waiting for GPU compute.

---

# 14. Cheap runtime geometry

## 14.1 Planar Web

Use:

```text
1 quad = 2 triangles
```

or equivalent 2-triangle card.

## 14.2 Corner / non-planar Web

Use:

```text
2-4 cards
```

when that preserves the form.

## 14.3 Too spatial

Fallback:

```text
ribbon representation
```

Do not generate hundreds of cards.

---

# 15. Apparent 3D strand shading

A web card should not look like flat white paint.

From the signed/distance field `phi(x,y)`, estimate gradient:

```text
gx = d(phi)/dx
gy = d(phi)/dy
```

Construct a pseudo strand normal:

```text
N = normalize(
    -normalStrength * gx,
    -normalStrength * gy,
    1)
```

Better path when available:

- generate/bake actual normal from the procedural strand profile once;
- use gradient normal only as a cheaper fallback/reference.

Controls:

- normal strength
- roughness
- strand brightness
- thickness response
- wetness
- frost
- dirt/dust

---

# 16. Mipmaps

Spider silk can disappear with naive mip averaging.

Do not accept ordinary RGBA averaging as the only final result.

Because the original Web Graph remains available, test:

## Source-rasterized mips

For each mip resolution:

```text
WebGraph
-> rasterize directly at that resolution
```

Example:

```text
2048
1024
512
256
128
64
```

Each mip independently integrates source strand coverage.

Compare against ordinary averaged mips.

Measure:

- coverage;
- apparent strand width;
- lost strands;
- over-thick strands;
- temporal stability.

---

# 17. Irregular area clipping

Let `A(x,y)` be an area mask.

Then:

```text
finalCoverage(x,y)
    = webCoverage(x,y) * areaMask(x,y)
```

This lets the owner draw/place an area without creating custom mesh topology for every irregular boundary.

First vertical may use simple polygon clipping.

---

# 18. Stable Edge IDs and broken strands

Every procedural strand has a stable Edge ID.

If the owner breaks an edge:

```text
Edge.state = Broken
```

Then the Web Field is regenerated without that strand.

This is acceptable because web texture regeneration is event-driven, not per-frame.

### Preferred first damage model

When the web changes rarely:

```text
break strand
-> mark Web Surface dirty
-> regenerate card texture
-> publish replacement
```

This is simpler and cheaper than maintaining huge per-pixel dynamic Edge-ID contributor tables.

Measure regeneration latency.

### Optional later optimization

If runtime tear events are frequent and regeneration proves too expensive, evaluate contributor/EdgeID texture schemes separately.

Do NOT implement that complexity before measuring the simple dirty-regeneration path.

---

# 19. Physics representation

Visual web and physical web are different resolutions of the same Web Graph.

Example:

```text
700 visible strands
but
20-50 physical edges/nodes
```

Do not create one Jolt body/constraint per visual thread.

Modes:

## Static

No physics.

Only card texture.

## Hybrid

Most strands remain visual-only.

Selected important edges/nodes participate in a coarse physical graph.

## Dynamic experimental

Only for bounded hero webs.

Use a coarse physical graph and map visual deformation to the render surface.

JericoEngine's complete soft-body/Cosserat runtime integration is still partial, so do not claim Engine support merely because the Playground prototypes something locally.

No second physics engine.

---

# 20. Deformation of the cheap surface

For Hybrid mode, allow the card/patch to deform using a small control lattice.

Example:

```text
4 corner controls
+ optional center/hub control
```

or a low-resolution patch grid.

The coarse physical graph moves these controls.

The shader/card interpolates deformation.

Do not deform every strand independently.

This keeps the visual result reactive without turning the entire web into geometry.

---

# 21. Interactive Playground UI

Add a visible section approximately:

```text
Spider Web Surface
```

Authoring:

- Create Plane Web
- Place Anchor
- Remove Anchor
- Generate
- Reset

Presets:

- Orb Web
- Corner Cobweb
- Messy Cave Web
- Broken Old Web
- Dense Nest
- Trap Strands

Parameters:

- Seed
- Spokes
- Rings
- Density
- Secondary Strands
- Irregularity
- Missing %
- Strand Radius
- Sag
- Hub Offset
- Area Padding

Rendering:

- Resolution
- Supersampling
- Coverage mode
- Distance-field mode
- Normal strength
- Mip mode
- Card / Card Cluster / Ribbon
- Two-sided

Debug:

- Show anchors
- Show Web Graph
- Show Edge IDs
- Show card bounds
- Show coverage
- Show distance field
- Show normals
- Show mips
- Show physical graph

Damage:

- Select Edge
- Break Edge
- Restore Edge

Physics:

- Static
- Hybrid
- Dynamic Experimental
- physical edge budget
- debug physical graph

---

# 22. Performance metrics

Show in the UI:

- semantic nodes
- semantic edges
- visual strand count
- physical edge count
- card count
- triangles
- texture resolution
- total texture bytes
- mip bytes
- covered pixel ratio
- estimated transparent overdraw ratio
- texture generation CPU/GPU ms
- regeneration latency after one broken edge

Compare against a debug geometry implementation.

---

# 23. Required tests

## Determinism

Same recipe + seed:

- same nodes
- same edges
- same IDs
- same texture hash where deterministic path allows exact bytes

## Generation

- spokes end on valid anchors/boundary
- rings remain ordered
- no NaN/Inf
- missing-edge probability stable
- edge count bounded

## Field

For known line segment:

- center phi approximately -radius
- boundary phi approximately zero
- distant point positive
- coverage monotonic around boundary

## Planarity

- planar anchors -> one card
- corner anchors -> cluster/fallback
- no invalid flattening

## Mips

- thin reference strand survives until declared visibility threshold
- source-rasterized mips preserve coverage better than naive averaging in the fixture
- deterministic bytes/metrics

## Damage

- break EdgeID
- regenerate
- broken strand absent
- unrelated strands unchanged
- restore reproduces original accepted output

## Budgets

Reject/diagnose:

- too many anchors
- too many edges
- invalid radius
- zero-size area
- excessive texture resolution
- excessive physical edge count
- NaN/Inf

---

# 24. Evidence command

Add an automated command such as:

```text
JericoGeologyPlayground.exe --web-surface-test Evidence/web-surface
```

Retain:

1. Plane Orb Web
2. Messy Cobweb
3. Corner multi-card web
4. coverage view
5. distance-field view
6. normal-lit view
7. mip comparison
8. intact web
9. same web with one broken strand
10. physical coarse graph debug
11. metrics JSON
12. recipe and hashes

---

# 25. Acceptance gates

Not PASS because the shader draws white lines.

PASS requires:

- user can create web from plane/anchors;
- Web Graph is deterministic and stable;
- normal render path uses card/card cluster rather than per-strand mesh;
- texture/field generated only when recipe changes, not every frame;
- thin-strand-aware mip path exists;
- web looks convincingly filament-like under lighting;
- one broken strand can regenerate out without rebuilding thousands of meshes;
- one planar fixture renders with two triangles;
- non-planar fixture chooses a bounded card cluster or fallback;
- performance comparison against debug geometry retained;
- normal Playground UI exposes all core controls;
- Release build and CTest green;
- reproducible evidence retained;
- JericoEngine remains untouched.

---

# 26. What NOT to do

Do not:

- create a full Sweep tube for every ordinary visual strand;
- create one Jolt object per visual thread;
- evaluate every strand against every fragment every frame;
- regenerate the field every frame when nothing changed;
- force a 3D web into one plane;
- create hundreds of tiny cards;
- call a PNG screenshot a procedural web system;
- build a second physics engine;
- modify JericoEngine from the Playground.

---

# Owner-facing success

The owner should be able to:

1. place a plane or click anchors;
2. choose a web preset;
3. press Generate;
4. see a detailed spider web although the normal render representation is only one/few cards;
5. change seed/density/thickness/rings and see the cached field regenerate;
6. inspect the Web Graph as debug lines without making those lines the normal render path;
7. break one strand and see only that strand disappear after regeneration;
8. switch between Card / Graph Debug / Ribbon fallback;
9. see real performance metrics.

This is a specialized spider-web renderer built from the same semantic web data, not another generic spline-mesh system.

**CHOTO = JericoEngine 1.2 codename.**
