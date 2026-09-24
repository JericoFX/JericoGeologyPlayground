# JericoEngine M8 — Mathematical Reconstruction: Mix Damage, Laces, Fence, AgedFX and PBG2

**Date:** 2026-09-20  
**Status:** M8 research / mathematical implementation input; not authorization to modify the active M9 slice.  
**Companion:** `docs/research/M8_MIX_DAMAGE_LACES_FENCE_AGEDFX_PBG2_TECHNICAL_INPUT.md`.

## 0. Purpose and evidence discipline

The previous analysis mapped the five purchased Blender references into Jerico's architecture, but it was intentionally too high-level. This appendix records the mathematical contracts needed to implement the useful behavior independently in JericoEngine.

Notation in this document:

- **[OBSERVED]**: directly established from the supplied Python or static `.blend` node data: operation type, connection, constant, socket or data flow.
- **[DERIVED]**: algebraic rewriting of observed operations, without copying the graph topology.
- **[JERICO]**: independent formulation recommended for JericoEngine when the reference only establishes behavior or when a more robust native formulation is preferable.
- **[DO NOT INFER]**: behavior that was not proven by the static pass and must not be treated as exact source equivalence.

The goal is **behavioral/mathematical reimplementation**, not graph reproduction. Purchased node graphs, presets, meshes, textures and style libraries remain outside Shipping and outside the repository.

---

# 1. Shared mathematical primitives

These primitives recur across all five references and should exist once in Jerico.

## 1.1 Saturation and remap

For `x` mapped from `[a,b]` to `[c,d]`:

```text
t = (x - a) / (b - a)
Remap(x,a,b,c,d) = c + t (d-c)
RemapClamped = c + clamp(t,0,1)(d-c)
```

Reversed ranges are legal and useful for threshold inversion.

## 1.2 Deterministic hash / random stream

Never use global RNG evaluation order as procedural identity.

```text
h = Hash64(assetGuid, proceduralElementId, stageTag, localIndex, userSeed)
u01 = Float01(h)
```

Use separate `stageTag`s for placement, variant, scale, rotation, damage and material so adding one random decision does not reshuffle all subsequent decisions.

## 1.3 Local frame

Given unit normal `n` and a preferred tangent direction `a`:

```text
t = normalize(a - n * dot(a,n))
b = normalize(cross(n,t))
R = [t b n]
q = transpose(R) * (x - center)
```

If `a` is nearly parallel to `n`, choose the world axis least aligned with `n`.

## 1.4 Rotation-minimizing path frame

For path samples `p_i`:

```text
t_i = normalize(p_{i+1} - p_i)
```

Transport the previous normal to the new tangent using shortest-arc rotation. A robust quaternion form is:

```text
v = cross(t_{i-1}, t_i)
c = dot(t_{i-1}, t_i)
q = normalize([1 + c, v.x, v.y, v.z])
n_i = rotate(q, n_{i-1})
b_i = normalize(cross(t_i, n_i))
n_i = normalize(cross(b_i, t_i))
```

Handle the anti-parallel case with a stable fallback axis.

This is preferred over Frenet frames because straight/low-curvature segments do not randomly roll.

## 1.5 Arc-length parameterization

For polyline/path samples:

```text
s_0 = 0
s_i = s_{i-1} + |p_i - p_{i-1}|
L = s_last
u_i = s_i / L
```

A module position at physical distance `d` is evaluated from the segment satisfying `s_i <= d <= s_{i+1}`.

## 1.6 SDF boolean algebra

For signed fields where negative is inside:

```text
Union(A,B)        = min(A,B)
Intersection(A,B) = max(A,B)
Difference(A,B)   = max(A,-B)
```

Useful smooth union:

```text
h = clamp(0.5 + 0.5*(B-A)/k, 0, 1)
SmoothMin(A,B,k) = lerp(B,A,h) - k*h*(1-h)
```

Do not use a smooth operation where a gameplay cut must remain dimensionally exact.

---

# 2. Mix Damage V1.2 — full mathematical interpretation

## 2.1 What is directly established

**[OBSERVED]** The primary `DamagePro2` graph contains:

- 96 scalar Math nodes;
- 38 Vector Math nodes;
- 10 Noise Texture nodes;
- 19 deterministic/random value nodes;
- 36 Set Position nodes;
- 13 Delete Geometry nodes;
- 7 Curve-to-Mesh and 5 Mesh-to-Curve paths;
- 5 Mesh-to-Volume nodes;
- 4 Volume-to-Mesh nodes;
- 5 Mesh Boolean nodes;
- explicit controls for two surface-damage layers plus Internal Damage, Sphere, Crack, Steel/Grid and Brick/Rock families.

This proves that the reference combines **surface fields, geometric displacement, curve sweeps, voxel/volume reconstruction and boolean topology change**. It does not justify copying the graph.

## 2.2 Damage projector / Decal3D coordinate system

This is the key integration with the existing Jerico `JRC_GN_Decal3D` work.

For impact/projector center `c`, projector forward axis `n`, and tangent basis `(t,b)`:

```text
q = (u,v,w) = transpose([t b n]) * (x-c)
```

All decal/cavity/hole math occurs in `q` space, so the same damage recipe works on a wall, floor, ceiling, rock face or arbitrary mesh orientation.

Optional non-uniform scale and rotation:

```text
[u';v'] = R2D(theta) * [u/sx; v/sy]
w' = w/sz
```

with

```text
R2D(theta) = [ cos(theta) -sin(theta)
               sin(theta)  cos(theta) ]
```

## 2.3 Two-layer surface damage field

The reference exposes `Texture 1/2`, Scale, Power, Angle and Mix.

A Jerico-native equivalent is:

```text
q1 = R2D(theta1) * (uv * scale1) + offset1
q2 = R2D(theta2) * (uv * scale2) + offset2

m1 = saturate(NoiseOrPattern1(q1)) ^ power1
m2 = saturate(NoiseOrPattern2(q2)) ^ power2
```

Then a recipe selects a combination operator:

```text
BlendLerp = lerp(m1,m2,mixWeight)
BlendMax  = max(m1,m2)
BlendMin  = min(m1,m2)
BlendMul  = m1*m2
BlendAdd  = saturate(m1+m2)
```

**[DO NOT INFER]** The static pass does not establish that every Blender `Mix` switch maps to one fixed Jerico operator. Persist the operator explicitly in the Jerico recipe.

## 2.4 Normal displacement / chipped surface

**[OBSERVED]** Multiple noise fields feed Set Position nodes.

Jerico formulation:

```text
eta(x) = 2*N(scale*x + offset, seed) - 1
A(x) = damageMask(x) * amplitude
x' = x + n(x) * A(x) * eta(x)
```

For directionally broken material:

```text
x' = x + [ n*A_n + t*A_t + b*A_b ] * eta
```

Use tangential components sparingly; normal displacement is more stable for UV/material lineage.

## 2.5 Volume reconstruction

**[OBSERVED]** Mesh→Volume→Mesh occurs repeatedly.

The independent mathematical model is:

1. Define a local voxel domain `Omega` around the damage AABB.
2. Sample a signed/occupancy field `F(x)` on grid spacing `h`.
3. Modify the field.
4. Extract the `F=0` isosurface using Jerico's native meshing path.

Recommended bounded resolution:

```text
h(q) = max(h_min, baseVoxelSize / 2^qualityTier)
Nx = ceil(extentX/h)
Ny = ceil(extentY/h)
Nz = ceil(extentZ/h)
```

Clamp total voxels by an authoring budget. Quality is an **authoring/cook** parameter first, not a runtime license to rebuild an entire wall every frame.

## 2.6 3D damage decal: cosmetic mark → real cavity → through-hole

This is a concrete Jerico capability, not merely an analogy.

### 2.6.1 Elliptical footprint

```text
rho = sqrt((u/a)^2 + (v/b)^2)
```

Use noise to break the perfect ellipse:

```text
rhoN = rho + edgeNoiseAmp * (2*N(edgeFreq*[u,v],seed)-1)
```

### 2.6.2 Soft visual mask

```text
M_decal = 1 - smoothstep(rInner, rOuter, rhoN)
```

This is the cheapest tier: no geometry change.

### 2.6.3 Indentation profile

One useful crater depth profile:

```text
zDent(rhoN) = -depth * (1 - saturate(rhoN)^p)^q
```

and geometric displacement is

```text
x' = x + n * zDent(rhoN) * M_decal
```

### 2.6.4 Bounded cavity cutter

Define a radial field:

```text
Fr = rhoN - 1
```

and an axial slab covering `w in [-D, 0]`:

```text
Fw = max(w, -w-D)
```

A finite cavity cutter is:

```text
F_cavity(q) = max(Fr, Fw)
```

Optional irregular depth:

```text
D(u,v) = D0 * [1 + kD*(2*N_d(u,v)-1)]
Fw = max(w, -w-D(u,v))
```

Subtract from host SDF:

```text
F_out = max(F_host, -F_cavity)
```

### 2.6.5 Through-hole

Measure host thickness along the projector axis using two-sided ray/field intersection:

```text
T = distance(entryPoint, exitPoint)
```

A through-hole requires:

```text
D >= T + exitMargin
```

The exit footprint may differ from entry:

```text
a_exit = a * (1 + spallX)
b_exit = b * (1 + spallY)
```

Blend a front cutter and an exit spall cutter along depth rather than forcing a straight cylinder.

### 2.6.6 Fresh interior provenance

The new cut surface must be identified at generation time.

A field-space predicate is:

```text
FreshInterior = abs(F_out) < eps
             && F_host < -eps
             && abs(F_cavity) < epsCut
```

Persist this as a semantic face/vertex/corner stream when extracting the mesh.

This allows immediate material assignment:

```text
plaster surface -> broken gypsum interior
concrete surface -> aggregate interior
brick wall      -> mortar/brick interior
metal panel     -> bare fractured metal
rock            -> fresh rock
```

No later triangle-angle heuristic should be needed.

### 2.6.7 Lip and spall ring

Use two radial masks:

```text
M_core = 1 - smoothstep(r0,r1,rhoN)
M_lip  = smoothstep(r0,r1,rhoN) * (1-smoothstep(r1,r2,rhoN))
```

The lip may displace outward:

```text
x' += n * lipHeight * M_lip * (0.5 + 0.5*N_lip)
```

while `M_core` drives inward cavity subtraction.

## 2.7 Internal spherical damage

The reference exposes Internal Damage/Sphere with count, scale, power, quality, offset and seed.

For deterministic cavity centers `c_i`:

```text
c_i = SampleInside(damageRegion, Hash(seed,i)) + offset
r_i = baseRadius * RadiusVariation(Hash(seed,i,"r"))
F_i(x) = |x-c_i| - r_i
```

Union the internal cavities:

```text
F_internal = min_i F_i
```

and subtract from the host:

```text
F = max(F_host, -F_internal)
```

`power` should shape radius/distribution rather than being an unexplained scalar:

```text
r_i = rMin + (rMax-rMin) * u_i^power
```

## 2.8 Crack field

**[OBSERVED]** Crack uses curve/mesh paths and exposes scale, thickness, quality, offset and seed.

Represent crack centerline as `c(s)` and local radius `r(s)`:

```text
F_crack(x) = min_s ( |x-c(s)| - r(s) )
```

For a shallow crack, bound it to a surface-normal slab:

```text
F_depth = max(w, -w-crackDepth)
F_cut = max(F_crack, F_depth)
```

A noisy crack path can be built from a tangent walk:

```text
p_{i+1} = p_i + step*t_i + lateralAmp*noise_i*b_i
```

with direction update:

```text
t_{i+1} = normalize(t_i + turnAmp*noiseTurn_i*b_i)
```

Branching is a graph of such paths sharing a deterministic root ID.

## 2.9 Steel / reinforcement grid

The reference has Steel, Grid, scale, power, quality, count and offset.

For local wall plane coordinates `(u,v)` and grid spacings `(su,sv)`:

```text
i = round((u-ou)/su)
j = round((v-ov)/sv)
du = abs(u - (ou+i*su))
dv = abs(v - (ov+j*sv))
```

Cylinder/rod proximity in the wall plane:

```text
F_u = du - rodRadius
F_v = dv - rodRadius
F_grid = min(F_u,F_v)
```

Clip to the host/interior region. Damage does not generate steel where the material recipe says no reinforcement exists.

## 2.10 Brick / block lattice

Given cell dimensions `(L,H)`:

```text
row = floor((v-ov)/H)
rowOffset = (row & 1) ? 0.5*L : 0
col = floor((u-ou-rowOffset)/L)
```

Local cell coordinates:

```text
fu = frac((u-ou-rowOffset)/L)
fv = frac((v-ov)/H)
```

Mortar distance field approximation:

```text
dMortar = min(min(fu,1-fu)*L, min(fv,1-fv)*H) - mortarHalfWidth
```

Damage can expose brick identities `(row,col)` and preserve them as stable semantic region IDs.

## 2.11 Representation ladder

One `DamageRecipe` chooses the cheapest sufficient representation:

```text
Tier 0  AttributeMask
Tier 1  Material/Decal3D
Tier 2  Normal/Parallax/Displacement bake
Tier 3  Geometric indentation
Tier 4  Local SDF cavity / through-hole
Tier 5  Prefractured semantic variant
Tier 6  Physical chunk activation / Jolt
```

Escalation depends on silhouette, penetration, collision and gameplay—not artistic preference alone.

## 2.12 Locality requirement

Never rebuild an entire world mesh for one bullet hole.

```text
DamageAABB = ProjectorAABB expanded by influenceMargin
AffectedCells = SpatialIndex.query(DamageAABB)
```

Only those cells/chunks are remeshed/cooked. Neighbor rings are included only for watertight stitching and normal/tangent continuity.

---

# 3. Laces Generator 2.0 — generalized CurveWeave mathematics

## 3.1 Directly observed helper math

The useful helper graphs establish:

- cross products;
- vector projection;
- dot products;
- vector scaling/addition;
- path tangent rotation;
- scalar remapping with `MULTIPLY_ADD`;
- curve-controlled twist masks;
- topology boundary tests;
- blur/smoothing of vector attributes;
- under/over sign switching;
- ordered eyelet index classification.

This is enough to define a general ordered-anchor weave backend independently.

## 3.2 Ordered anchors

Each anchor has:

```text
Anchor {
    position
    normal
    side        // left/right/center/custom
    ordinal
    group
    semanticId
}
```

Sort primarily by logical row/ordinal, not world-axis alone. A stable key:

```text
key = (group, ordinal, sideOrder, stableId)
```

## 3.3 Side coordinate remaps observed in helper graphs

**[OBSERVED]** `DistanceFromSideChaos` contains:

```text
1 - 2u
1 - u
1 - selectedValue
```

from chained `MULTIPLY_ADD` operations.

So useful signed lateral forms are:

```text
sideSigned = 1 - 2u            // maps u=0..1 to +1..-1
sideOneSided = 1 - u           // maps u=0..1 to 1..0
sideInverted = 1 - sideValue
```

These should become named functions rather than opaque scalar wires.

## 3.4 Under/over parity

For crossing index `k` and user phase `phase in {0,1}`:

```text
under = ((k + phase) & 1) != 0
sigma = under ? -1 : +1
```

Crossing depth offset:

```text
dz = sigma * weaveDepth * crossingEnvelope(s)
```

`StartUnder` simply toggles `phase`.

**[OBSERVED]** `StartUnderLoop` explicitly selects a vector or its `-1` scaled version, confirming sign inversion rather than a separate geometric algorithm.

## 3.5 Curve through anchor pairs

For anchor pair `(p0,p1)` and endpoint directions `(m0,m1)`, use cubic Hermite:

```text
H00 =  2t^3 - 3t^2 + 1
H10 =    t^3 - 2t^2 + t
H01 = -2t^3 + 3t^2
H11 =    t^3 -   t^2

c(t) = H00*p0 + H10*m0 + H01*p1 + H11*m1
```

Tangents derive analytically from `dc/dt` then normalize.

For simple straight lacing, `m0` and `m1` can default to a fraction of `(p1-p0)` plus controlled surface-normal lift.

## 3.6 Rotation-minimizing frame

Use the shared RMF from section 1.4. The frame `(t,b,n)` is the authority for width, thickness, twist, seam and chaos.

## 3.7 Ribbon sweep

For a flat ribbon profile coordinate `(alpha,beta)`:

```text
p(s,alpha,beta) = c(s)
                + alpha * width(s)     * b(s)
                + beta  * thickness(s) * n(s)
```

with `alpha,beta` usually in `[-0.5,0.5]`.

A rounded ribbon replaces the rectangular profile with an authored 2D profile `P(phi)`.

## 3.8 Round cable / rope sweep

```text
P(phi) = [r cos(phi), r sin(phi)]
p(s,phi) = c(s) + P.x*b(s) + P.y*n(s)
```

An elliptical cable uses radii `rx,ry`.

## 3.9 Twist

**[OBSERVED]** The `Twist` helper rotates a width/extrusion vector around the path axis, with sign switches and a Float Curve multiplier.

Rodrigues rotation:

```text
RotateAxis(v,t,theta) =
    v cos(theta)
  + cross(t,v) sin(theta)
  + t dot(t,v) (1-cos(theta))
```

Twist field:

```text
theta(s) = signFlags * twistAmount * twistCurve(u(s))
```

Then rotate `b`/profile vector around tangent `t`.

## 3.10 Chaos / hand-made irregularity

**[OBSERVED]** The helper computes cross product, projection, dot product, multiple vector scales, vector blur and a Float Curve.

Useful decomposition:

```text
lateral = normalize(cross(sampledNormal, tangent))
normalProjected = project(sampledNormal, lateral)
signTerm = dot(vA,vB)
```

A bounded chaos displacement:

```text
Delta(s) = A_lat * C_lat(s) * lateral
         + A_n   * C_n(s)   * sampledNormal
```

where the random/noise fields are smoothed along the path:

```text
C_smooth[i] = WeightedBlur(C_raw, iterations)
```

and faded by endpoint/side curves.

Do not use unbounded white-noise offsets; preserve curvature and avoid self-intersection.

## 3.11 Boundary/topology coordinate

**[OBSERVED]** `YUVThickness0` tests mesh edge face count `< 1` and combines booleans with width-scaled terms. This establishes that border membership influences the ribbon coordinate stored as a named attribute.

Jerico should store explicit:

```text
PathU        = arcLength/L
ProfileV     = normalized cross-section coordinate
ProfileSide  = side enum / signed coordinate
BoundaryRole = interior/rim/seam
```

instead of reconstructing them from the final mesh.

## 3.12 Endpoint accessories

Accessory transform at endpoint `s_e`:

```text
T = c(s_e)
Z = tangent(s_e)
X = frame.b(s_e)
Y = frame.n(s_e)
M = [X Y Z T]
```

Apply an authored local offset and scale. This generalizes aglets into clamps, plugs, cable ends, hooks, connectors and knots.

## 3.13 General Jerico applications

The same `CurveWeave` operator supports:

- shoelaces;
- mine wiring through eyelets;
- cable lacing on racks;
- ropes through rings;
- stitched seams;
- straps and bindings;
- chained anchor runs;
- hoses constrained by clips;
- web/rope structures.

No shoe-specific runtime type is needed.

---

# 4. Fence / Gate / Railing / Wall Generator — Path Assembly mathematics

## 4.1 Path distribution

For path length `L` and target spacing `s`:

```text
n = max(1, round(L/s))
actualSpacing = L/n
s_i = i*actualSpacing, i=0..n
p_i = path(s_i)
```

Alternative fit policies:

```text
FitInside: n = max(1, floor(L/s))
Cover:     n = max(1, ceil(L/s))
Nearest:   n = max(1, round(L/s))
```

The policy must be explicit in a recipe.

## 4.2 Corner-aware path frames

Use RMF for ordinary spans. At a hard corner, compute incoming/outgoing tangents:

```text
t0 = normalize(p_i-p_{i-1})
t1 = normalize(p_{i+1}-p_i)
cornerAngle = acos(clamp(dot(t0,t1),-1,1))
```

Choose miter, bevel, round or split-post policy by threshold.

## 4.3 Gate span segmentation

A gate is an interval `[g0,g1]` in arc length or normalized `u`.

Ordinary span predicate:

```text
ordinary(s) = !(g0 <= s && s <= g1)
```

Gate center and width:

```text
gc = 0.5*(g0+g1)
gw = g1-g0
```

Posts bordering the gate are semantic `GatePost` elements, not anonymous repeated posts.

For multiple gates, sort intervals and reject overlaps unless the style explicitly supports them.

## 4.4 Post orientation

At path parameter `s`:

```text
up = groundNormalPolicy ? groundNormal : WorldUp
t = pathTangent(s)
side = normalize(cross(up,t))
forward = normalize(cross(side,up))
```

For architectural posts that must remain vertical, force `up=WorldUp` and use ground normal only for footing placement.

## 4.5 Terrain adaptation

**[OBSERVED]** The reference contains Raycast operations and resampled curves.

For each placement `p_i`, query terrain along gravity/up axis:

```text
hit_i = Raycast(p_i + up*hProbe, -up, maxDistance)
```

If hit:

```text
base_i = hit.position
groundN_i = hit.normal
```

Policy choices:

```text
VerticalPost: orientation up = WorldUp
ConformPost:  orientation up = groundN
```

Fence rails should usually remain governed by the path, not rotate independently per post.

## 4.6 Stepped fence

Quantize vertical base height:

```text
zStep = z0 + round((z-z0)/stepHeight)*stepHeight
```

or floor for monotonic stair-step behavior:

```text
zStep = z0 + floor((z-z0)/stepHeight)*stepHeight
```

Split spans where the quantized level changes.

## 4.7 Sloped top/bottom trim

For span endpoints with top heights `h0,h1`:

```text
lambda = s/Lspan
hTop(s) = lerp(h0,h1,lambda)
```

A picket at `s_i` is clipped or scaled to:

```text
height_i = hTop(s_i) - hBottom(s_i)
```

Use clipping planes when profile proportions must remain constant; use scale only when style allows deformation.

## 4.8 Rail sweep

For 2D rail profile `P(v)` and path frame:

```text
rail(s,v) = c(s) + P.x(v)*b(s) + P.y(v)*n(s)
```

A handrail offset:

```text
c_rail(s) = c(s) + sideOffset*b(s) + height*nWorld
```

## 4.9 Picket / baluster distribution

For span length `Ls` and target gap `g` plus member width `w`:

```text
n = max(1, floor((Ls+g)/(w+g)))
actualGap = (Ls - n*w)/(n+1)
s_i = actualGap + w/2 + i*(w+actualGap)
```

This gives equal outer and internal gaps.

## 4.10 Caps, collars and finials

These are endpoint transforms attached to post semantic sockets:

```text
T_accessory = T_post * T_localOffset * R_local * S_local
```

Store `PostTopSocket` rather than recomputing from mesh bounds after generation.

## 4.11 Laser-cut panel field

**[OBSERVED]** `Laser Cut Pattern` uses 2D Voronoi distance with scale `8`, randomness input, and two extremely narrow reversed remaps:

```text
A(D) = Remap(D, 0.050, 0.049, 0, 1)
B(D) = Remap(D, 0.400, 0.399, 0, 1)
M = lerp(A,B, Factor)
```

The reversed narrow intervals act as sharp threshold/band selectors around Voronoi distance contours.

Jerico should expose this as a generic cellular panel mask:

```text
D = VoronoiDistance(uv * frequency, seed, randomness)
M = smoothstep(d0+width, d0-width, D)
```

or a blend of two threshold families. It can drive:

- opacity/cutout;
- material mask;
- offline boolean/perforation;
- CNC/laser-style decorative panel generation.

Runtime mesh booleans are unnecessary for static panels; bake/cook perforated geometry.

## 4.12 Gate gameplay handoff

Generator output must include:

```text
GateElementId
HingeSocketA/B
ClosedTransform
OpenAxis
OpenLimit
PanelBounds
NavPortalId
```

Physics converts those semantics into Jolt constraints; the generator itself is not the hinge simulation.

---

# 5. AgedFX 1.22 — EPLS/material mathematics

## 5.1 Dirt: directly reconstructed field

**[OBSERVED]** The Dirt group combines Pointiness, a Color Ramp, gamma, Musgrave/Noise, bevel normal, original normal and a dot product.

### General dirt

Let:

```text
P = Ramp(Pointiness)
G = RemapClamped(GeneralDirt, 0,10, 5,0)
N = FractalNoise(position, DirtScale, detail=14, roughness≈1)
```

Then:

```text
GeneralMask = GammaOperator(P,G) * N
```

`GammaOperator` should be implemented according to Jerico material semantics rather than assuming Blender's shader gamma convention.

### Edge dirt

**[OBSERVED]** Noise drives bevel radius:

```text
r = RemapClamped(
      Noise(position, DirtScale, detail=15, roughness=.75, distortion=1.5),
      0.2, 0.5,
      EdgeDirtSpread, 0.01)
```

Let `n_b = BevelNormal(r)` and `n = geometric/shading normal`:

```text
e = dot(n_b,n)
EdgeMask = RemapClamped(e, 0.75,1.0, EdgeDirt,0)
```

Final reference structure:

```text
Dirt = EdgeMask + GeneralMask
```

Clamp/normalize as required by the material channel.

## 5.2 Jerico EPLS dirt formulation

Bevel shading normal is not ideal as persistent world-state truth. EPLS should compute or consume physical feature fields:

```text
Cavity(x)
Convexity(x)
EdgeDistance(x)
Exposure(x)
WetHistory(x)