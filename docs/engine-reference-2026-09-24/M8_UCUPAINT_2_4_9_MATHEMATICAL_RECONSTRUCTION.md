# M8 Ucupaint 2.4.9 — Clean-Room Mathematical Reconstruction for JericoEngine

**Milestone:** M8 research input  
**Scope:** independent mathematical specification inspired by observed behavior, not a source translation  
**Source snapshot:** owner-supplied Ucupaint 2.4.9  
**Source archive SHA-256:** `c4d87fd9d73b4855b5ed096a52e4374aadb099babeb924674a7674a9b50eb5f8`  
**License boundary:** no GPL implementation text, node graph or `.blend` payload is reproduced here  
**Implementation policy:** reuse Jerico's canonical mesh, tangent, texture, material, VFS, cook and job authorities; do not create parallel registries

---

## 0. Purpose

This document turns the useful behavior observed in Ucupaint into a Jerico-native,
dimensionally explicit and testable mathematical design.

The primary targets are:

```text
A. signed geometric curvature field
B. independent short-range contact occlusion field
C. UV-correspondence direction transport (flow field)
D. canonical tangent-space vector displacement
E. conservative height/displacement range propagation
F. typed layer/mask composition feeding MaterialBakeIR
G. semantic specialist bake fields
```

The goal is not feature parity with Ucupaint. The goal is to extract the smallest
reusable primitives that make Jerico Smart Materials, damage masks, geology materials,
remesh analysis and bake tooling better.

---

# Part I — Common conventions

## 1. Units

Jerico geometry fields must carry physical meaning where physical meaning exists.

```text
position                    metres
edge length                 metres
area                        metres^2
signed curvature            1 / metre
height/displacement         metres
thickness                   metres
ray/contact radius          metres
UV                          dimensionless
normalized mask             [0,1]
direction                   unit vector unless documented otherwise
```

Never hide geometry scale in arbitrary `*100`, `*400` or texture-resolution-specific
constants.

---

## 2. Coordinate authorities

The implementation must use existing Jerico authorities for:

```text
object/local axes
world axes
UV V orientation
normal convention
tangent generation
tangent handedness
texture color/data space
```

This research does not authorize another TBN convention.

If an external authoring source needs an axis change, store it as explicit import/cook
metadata and convert once at a defined boundary.

---

## 3. Mesh domains

A field must state its domain:

```text
Vertex
Corner
Edge
Face
Texel
WorldSample
```

Important examples:

- geometric signed curvature may begin as edge contributions and end as vertex/corner;
- UV flow is naturally **corner-domain** because seams may give one mesh vertex several
  unrelated UV directions;
- material masks usually become texel-domain when baked;
- world/EPLS fields may remain world-sample-domain until material evaluation.

Do not silently collapse corner data to vertex data.

---

## 4. Validity/confidence

Every derived geometric field should be able to report validity or confidence when
its input can be degenerate.

Conceptual result:

```text
FieldSample<T> {
    T value;
    float confidence;   // [0,1]
    uint flags;         // Degenerate, Boundary, NonManifold, Mirrored, etc.
}
```

This is especially useful for:

- UV Jacobians with nearly zero area;
- non-manifold curvature edges;
- missing tangent basis;
- thickness rays that miss;
- contact AO near open mesh boundaries.

A silent zero is not equivalent to a valid measured zero.

---

# Part II — Signed surface curvature

## 5. Required output

Define a reusable field:

```text
SignedCurvature(p) [1/m]
```

with Jerico convention:

```text
positive -> locally convex / exposed
negative -> locally concave / recessed
zero     -> locally planar under the selected estimator
```

Also expose derived fields without recomputation:

```text
ConvexCurvature  = max(SignedCurvature, 0)
ConcaveCurvature = max(-SignedCurvature, 0)
AbsCurvature     = abs(SignedCurvature)
```

The raw signed scalar is the authority. Material masks are later mappings of that
scalar.

---

## 6. Topology preflight

Before evaluating curvature, classify each undirected edge by incident face count:

```text
0 faces -> invalid topology
1 face  -> boundary edge
2 faces -> manifold interior edge
>2      -> non-manifold edge
```

Default P0 behavior:

- interior manifold edges contribute curvature;
- boundary edges contribute no signed dihedral by default and set `Boundary`;
- non-manifold edges are rejected or excluded with diagnostic according to job policy;
- coincident but disconnected geometry is **not** automatically treated as one surface.

### 6.1 Welding policy

Ucupaint's source analysis welds coincident positions to recover curvature across
some split meshes. Jerico should make this explicit:

```text
TopologyMode::Authored
TopologyMode::PositionWelded(toleranceMeters)
```

`PositionWelded` is an authoring analysis option, never a hidden mutation of source
mesh topology.

A weld candidate must respect optional preservation gates:

```text
NoWeld semantic
material boundary policy
region/part identity
hard discontinuity policy
maximum position tolerance
```

This avoids inventing curvature between intentionally overlapping parts.

---

## 7. Face geometry

For each oriented triangle face `f=(i,j,k)`:

```text
e1 = p_j - p_i
e2 = p_k - p_i
A2 = cross(e1, e2)
A_f = 0.5 * |A2|
n_f = A2 / |A2|
```

Reject or flag a face if:

```text
|A2| < epsilon_area
```

For polygons, use the canonical triangulated/evaluated mesh consumed by the field
builder so adjacency and area are unambiguous.

---

## 8. Signed dihedral angle

For an interior edge `e=(i,j)` shared by oriented faces `f` and `g`, orient the edge
according to its direction in face `f`:

```text
t_e = normalize(p_j - p_i)
```

Then compute a robust signed angle from `n_f` to `n_g` around the edge axis:

```text
x = clamp(dot(n_f, n_g), -1, 1)
y = dot(t_e, cross(n_f, n_g))
theta_e = atan2(y, x)
```

With the Jerico fixture winding convention, polarity is calibrated so an outward
convex cube edge is positive. If importer winding is reversed, canonical mesh
admission must correct/reject it rather than changing curvature semantics ad hoc.

Properties:

```text
theta_e in [-pi, pi]
planar continuation -> theta ~= 0
right-angle convex edge -> theta ~= +pi/2
right-angle concave edge -> theta ~= -pi/2
```

### 8.1 Independent polarity oracle

During development/tests, compare orientation against a centroid-plane oracle:

```text
s_oracle = sign(-dot(c_g - c_f, n_f))
```

for fixtures where this classification is valid.

If oriented-dihedral and oracle polarity disagree systematically, the issue is winding
or half-edge orientation, not a reason to add heuristic sign flips per asset.

---

## 9. Edge-integrated feature measure

Weight the signed angle by physical edge length:

```text
L_e = |p_j - p_i|
I_e = L_e * theta_e
```

Units:

```text
metres * radians ~= metres
```

`I_e` is an integrated sharpness/curvature measure along that edge.

Longer edges contribute proportionally more because they represent more surface
boundary.

---

## 10. Vertex dual area

Use a declared dual area `A_i` for normalization.

P0 robust option:

```text
A_i = sum_{f incident i} A_f / 3
```

for triangulated meshes.

Later, a mixed Voronoi area may be used for smoother differential estimates, but it
must remain a declared algorithm version because it changes numeric results.

Clamp:

```text
A_i = max(A_i, epsilon_area_vertex)
```

and report degenerate support rather than producing infinities.

---

## 11. Signed dihedral curvature estimator

Distribute half of each edge integral to each endpoint and normalize by dual area.
Define the Jerico material-feature estimator:

```text
K_i = (1 / (2 A_i)) * sum_{e incident i} (0.5 * I_e)
```

therefore:

```text
K_i = (1 / (4 A_i)) * sum_{e incident i} L_e * theta_e
```

Units:

```text
metres / metres^2 = 1/metre
```

This quantity is explicitly named `SignedDihedralCurvature` in implementation/tests
so convention changes cannot be confused with another textbook curvature definition.

### 11.1 Scale law

If the mesh is uniformly scaled by `s > 0`:

```text
L'_e = s L_e
A'_i = s^2 A_i
theta'_e = theta_e
```

then:

```text
K'_i = K_i / s
```

This inverse-length scale law is a mandatory test.

---

## 12. Smooth-surface alternative

For smooth surfaces, Jerico may additionally expose a cotangent-Laplacian estimator
without replacing the sharp dihedral field.

For vertex `i`:

```text
Hn_i = (1 / (2 A_i)) * sum_j (cot(alpha_ij) + cot(beta_ij)) * (p_i - p_j)
```

A signed scalar can be derived against the canonical vertex normal:

```text
K_smooth_i = 0.5 * dot(Hn_i, n_i)
```

Exact factor convention must be locked by sphere/cylinder fixtures.

Use cases differ:

```text
SignedDihedralCurvature -> hard-surface edge wear, feature masks
SmoothCurvature         -> smooth geology/organic analysis
```

Both should live behind the same surface-feature service, not separate subsystems.

---

## 13. Smoothing / relaxation

Raw vertex fields can be noisy on coarse/irregular meshes. Smoothing is optional and
must not erase semantic boundaries.

A simple graph relaxation:

```text
K'_i = (1-lambda) K_i + lambda * weighted_mean_j(K_j)
```

with `lambda in [0,1]`.

Recommended edge weight:

```text
w_ij = w_len * w_region * w_boundary
```

where, for example:

```text
w_len = 1 / max(|p_i-p_j|, epsilon)
w_region = 0 if a protected semantic boundary forbids smoothing, else 1
w_boundary = 0 across explicitly excluded analysis seams, else 1
```

A more aggressive bilateral normal weight may be added, but note that suppressing
weights at sharp edges can also suppress the very feature being measured. Therefore
its effect must be validated on cube/crease fixtures.

Default P0 should be small and explicit, e.g. zero or one declared relaxation pass.

---

## 14. Radius-based normalized mask

Do not destroy the raw `1/m` field. Derive a normalized mask from a physical radius
`r` only when a consumer requests it.

Dimensionless response:

```text
q = K * r
```

Two useful mappings:

Hard bounded:

```text
m_signed = clamp(q, -1, 1)
```

Smooth bounded:

```text
m_signed = tanh(gain * q)
```

Then:

```text
m01 = 0.5 + 0.5 * m_signed
convexMask  = saturate( m_signed)
concaveMask = saturate(-m_signed)
```

`tanh` is preferred for authoring because it avoids a hard derivative discontinuity
at saturation while remaining bounded.

---

## 15. Curvature confidence

Curvature confidence can be reduced for:

```text
boundary support
non-manifold adjacency
degenerate incident faces
analysis weld ambiguity
extreme local aspect ratio
```

One simple multiplicative model:

```text
C_i = C_topology * C_area * C_aspect
```

The exact confidence function is secondary to making invalidity explicit.

---

## 16. Texture bake of curvature

To bake vertex/corner curvature into UV space:

1. rasterize target UV triangles;
2. interpolate raw `K` or a declared normalized mapping barycentrically;
3. preserve sign in a signed/float target when possible;
4. dilate only after valid texels are identified;
5. build mips with a declared filter;
6. record mapping radius/gain only when the baked texture stores normalized values.

Preferred authoring representation:

```text
R16F or R32F raw signed curvature
```

Preferred packed runtime representation when only a mask is needed:

```text
R8/R16 UNORM derived mask with recipe provenance
```

Do not label an R8 mask as “curvature” without the mapping parameters that made it.

---

# Part III — Contact occlusion as a separate field

## 17. Motivation

Two surfaces can be extremely close while both are locally flat. Their contact region
has strong occlusion/proximity but zero local curvature.

Therefore:

```text
ContactOcclusion != SignedCurvature
```

Ucupaint's useful visual intent is preserved by allowing Smart Materials to combine
them later.

---

## 18. Short-range contact occlusion

At surface point `p`, normal `n`, sample a cosine-weighted hemisphere of directions
`omega_j` and trace to maximum physical distance `R`.

Let:

```text
d_j = first hit distance in (bias, R], or +inf if no hit
```

Define distance-weighted contact contribution:

```text
c_j = 0                                    if no hit
c_j = (1 - d_j/R)^p                        if hit
```

with `p >= 1` controlling how strongly near contact dominates.

Then:

```text
ContactOcclusion(p) = sum_j w_j c_j / sum_j w_j
```

Range:

```text
[0,1]
```

This differs from large-radius lighting AO: it is an **authoring feature field with a
specified radius in metres**.

---

## 19. Self-hit and open-boundary rules

Mandatory controls:

```text
normal bias / geometric bias
ignore source primitive
optional ignore same connected component
backface policy
maximum radius R
sample count / deterministic sequence
```

For deterministic CPU/GPU parity, use a known low-discrepancy sample sequence keyed by
job/sample index rather than unconstrained random numbers.

---

## 20. Example Smart Material combinations

These are recipes, not primitive fields.

Edge wear:

```text
wear = smoothstep(k0, k1, ConvexCurvature * radius)
     * exposureMask
     * noiseModulation
```

Cavity dirt:

```text
cavity = smoothstep(k0, k1, ConcaveCurvature * radius)
dirt = saturate(w1*cavity + w2*ContactOcclusion)
```

Paint chipping eligibility:

```text
chip = wear * DamageEligibility * authoredPaintMask
```

The shared fields stay reusable across materials.

---

# Part IV — UV correspondence / flow-direction transport

## 21. Problem statement

We have two UV parameterizations of the same triangle:

```text
S = reference/straight UV space
U = target/final UV space
```

A known direction `d_s` in reference space should be transported into the target UV
space.

This generalizes Ucupaint's straight-UV flow behavior and avoids an edge heuristic.

---

## 22. Triangle affine maps

For triangle corners `0,1,2`, define:

```text
Ds = [ s1-s0   s2-s0 ]    // 2x2
Du = [ u1-u0   u2-u0 ]    // 2x2
```

Within one triangle, the affine mapping from reference UV differential to target UV
differential is:

```text
J = Du * inverse(Ds)
```

provided:

```text
abs(det(Ds)) > epsilon_uv
```

`J` is the exact local Jacobian `dU/dS` of the triangle-to-triangle mapping.

---

## 23. Direction transport

Given a unit reference direction:

```text
d_s = normalize(d_s)
```

transport it:

```text
v_u = J * d_s
```

If:

```text
|v_u| <= epsilon
```

the direction is invalid for that triangle.

Otherwise:

```text
d_u = v_u / |v_u|
```

This correctly handles:

- rotation;
- non-uniform scale;
- shear;
- mirrored mapping;
- arbitrary reference direction.

---

## 24. Why a Jacobian is better than edge voting

Edge voting approximates a directional mapping from a few edge alignments. The
Jacobian is the exact affine derivative of the UV mapping over the triangle.

For an identity map:

```text
Du = Ds
J = I
d_u = d_s
```

For a pure UV rotation `R`:

```text
Du = R Ds
J = R
d_u = R d_s
```

For scale/shear `A`:

```text
Du = A Ds
J = A
```

No special cases are required.

---

## 25. Mirrored UV islands

A mirrored map has:

```text
det(J) < 0
```

Do not silently “fix” it. The mirror is real mapping information.

Output flags may include:

```text
Mirrored = det(J) < 0
```

The transported direction remains `normalize(J*d_s)`.

A consumer that needs orientation handedness can read the flag separately.

---

## 26. UV degeneracy confidence

Define normalized triangle quality in a UV space:

```text
q(a,b) = |det([a,b])| / (|a| |b| + epsilon)
```

This is approximately `|sin(angle)|` and lies near zero for collinear edges.

For the reference and target triangles:

```text
q_s = q(s1-s0, s2-s0)
q_u = q(u1-u0, u2-u0)
```

A simple confidence:

```text
C_tri = sqrt(q_s * q_u)
```

Optionally multiply by an area threshold ramp to reject extremely tiny UV triangles
that are numerically unstable despite acceptable angle.

---

## 27. Corner-domain accumulation

One mesh vertex may belong to several UV islands, so use a key such as:

```text
(meshVertexIndex, targetUvCornerIdentity)
```

For each incident valid triangle `t`, accumulate:

```text
v_acc += w_t * d_u_t
w_acc += w_t
```

Recommended weight:

```text
w_t = cornerAngle3D_t * C_tri
```

or a declared area/angle alternative.

Then:

```text
d_corner = normalize(v_acc / max(w_acc, epsilon))
```

If the vector cancels to near zero, mark low confidence instead of choosing an
arbitrary direction.

---

## 28. Encoding

Preferred authoring/cook intermediate:

```text
RG16_SNORM : (x,y) in [-1,1]
```

Alternative UNORM compatibility encoding:

```text
encoded = 0.5 * d + 0.5
decoded = normalize(2*encoded - 1)
```

Optional confidence can occupy another channel or a parallel mask.

Do not treat this as color; it is linear vector data.

---

## 29. Consumers

The same direction field can orient:

```text
wood grain
brushed metal anisotropy
cloth/fiber patterns
leak streaks
rust streaks
paint runs
scratch direction
procedural brush stamps
terrain/material flow marks
```

If a world-space force such as gravity or EPLS flow should dominate, that is a
separate world-field projected into the surface tangent plane. Do not misuse UV flow
as world gravity.

---

## 30. Flow acceptance fixtures

### Identity

```text
S == U
expected d_u == d_s
```

### 90-degree rotation

```text
U = R90 * S
expected d_u = R90 * d_s
```

### Non-uniform scale

Direction must equal normalized transformed vector, not merely preserve the original
angle.

### Shear

Compare against direct matrix multiplication.

### Mirror

```text
det(J) < 0
```

must be reported and direction must match mirrored transform.

### Degenerate reference triangle

```text
abs(det(Ds)) <= epsilon
```

must produce invalid/zero-confidence, never NaN.

### Seam

Two corners sharing the same 3D vertex but different UV locations must retain
independent flow results.

---

# Part V — Canonical tangent-space vector displacement

## 31. Semantic definition

A tangent-space vector displacement texel stores a 3D displacement vector expressed
in the local tangent frame of the base surface.

Unlike scalar height:

```text
scalar height -> displacement constrained mainly to N
VDM           -> arbitrary components along T, B and N
```

This can represent folds/overhang-like local offsets that scalar height cannot encode.

---

## 32. Canonical orthonormal basis

Start from canonical Jerico normal `N`, tangent `T_raw` and tangent handedness `h`.

Orthonormalize:

```text
N = normalize(N)
T = normalize(T_raw - N * dot(N, T_raw))
B = h * normalize(cross(N, T))
```

`h` must come from the existing Jerico tangent authority/cook convention.

Do not derive a second sign convention specifically for VDM.

---

## 33. Object/local vector to tangent vector

For object/local displacement `deltaP`:

```text
v_ts.x = dot(deltaP, T)
v_ts.y = dot(deltaP, B)
v_ts.z = dot(deltaP, N)
```

In matrix form, with basis matrix columns `[T B N]`:

```text
M = [T B N]
v_ts = transpose(M) * deltaP
```

because `M` is orthonormal.

---

## 34. Tangent vector to object/local vector

Reconstruction:

```text
deltaP = T*v_ts.x + B*v_ts.y + N*v_ts.z
```

or:

```text
deltaP = M * v_ts
```

Round-trip invariant:

```text
M * transpose(M) ~= I
M * (transpose(M) * deltaP) ~= deltaP
```

within floating-point tolerance.

---

## 35. Encoding options

### 35.1 Signed float

Preferred authoring/bake intermediate:

```text
RGB16F / RGB32F
stored value = v_ts in metres
```

Advantages:

- no midpoint quantization;
- direct physical units;
- no separate scale needed for decode, though bounds metadata is still useful.

### 35.2 Midpoint UNORM

For a symmetric per-axis range `R=(Rx,Ry,Rz)`:

```text
p = 0.5 + 0.5 * (v_ts / R)
```

component-wise.

Decode:

```text
v_ts = (2*p - 1) * R
```

Admission requires:

```text
abs(v_ts.xyz) <= R + tolerance
```

Clamping without a diagnostic loses geometry and is not an acceptable cook default.

---

## 36. Explicit axis transform

External tools may disagree on component order or axis polarity.

Represent conversion explicitly:

```text
v_jerico = A_source_to_jerico * v_source
```

where `A` is a signed permutation/orthogonal conversion matrix admitted by importer
policy.

Do not carry ambiguous flags such as `flipYZ` deep into runtime.

Cook provenance should state the source convention and the conversion applied.

---

## 37. Color management

VDM is data:

```text
colorSpace = LinearData
```

No sRGB decode/encode is permitted.

An automated test should deliberately label the same bytes as sRGB and prove asset
admission rejects or canonicalizes the mistake rather than silently deforming the
mesh.

---

## 38. Displacement bounds

For a texture with declared per-axis tangent range `R`:

```text
maxVectorLength <= sqrt(Rx^2 + Ry^2 + Rz^2)
```

A tighter bound can be measured during cook from actual decoded texels:

```text
Dmax = max_texel |v_ts|
```

Runtime/preview conservative bounds must expand enough to contain transformed
displacement.

If object transforms include non-uniform scale and displacement is authored in object
space, world bound should use the maximum singular value of the linear transform `A`:

```text
Dmax_world <= sigma_max(A) * Dmax_object
```

A cheaper conservative bound may use a matrix norm if already used elsewhere, but the
chosen bound must be explicit and tested.

---

## 39. Offline VDM bake path

For each target texel sample:

1. identify base target surface point and canonical TBN;
2. identify corresponding displaced/high-detail point under the declared projection
   policy;
3. compute object/local offset `deltaP`;
4. project to tangent basis;
5. encode in the target VDM format;
6. emit validity/miss diagnostics;
7. dilate only valid islands;
8. persist `rangeMeters`, UV set, tangent convention and axis conversion.

This can plug into `MaterialBakeIR` / geometry bake jobs without depending on Blender.

---

## 40. Runtime sampling

For runtime-supported displacement:

```text
sample -> decode -> tangent/object conversion -> apply under feature policy
```

The renderer must use the same tangent basis semantics as normal mapping.

If the current material path supports only scalar height/POM, VDM remains an offline
asset-preparation semantic until a measured runtime consumer is justified.

---

## 41. VDM mip/filter caution

Averaging tangent-space displacement vectors is not identical to filtering displaced
geometry, especially where tangent frames vary rapidly.

Minimum safe policy:

- keep authoring/bake source at full resolution;
- generate mips with linear-data filtering;
- preserve a conservative per-mip displacement bound;
- reject/avoid VDM across UV/tangent discontinuities that lack sufficient padding;
- validate silhouette/normal error on the intended consumer.

Do not assume normal-map mip rules are automatically sufficient for VDM.

---

## 42. VDM acceptance fixtures

### Basis vectors

Bake/decode:

```text
(1,0,0) tangent displacement
(0,1,0) bitangent displacement
(0,0,1) normal displacement
```

and verify object/local movement against canonical T/B/N.

### Mirrored UV

Tangent handedness must reconstruct bitangent correctly without a VDM-specific hack.

### Import V flip

Use Jerico's existing normal/tangent test asset path and ensure VDM remains aligned
after canonical UV conversion.

### Quantization

For UNORM encoding, measured error must stay below the declared half-step bound after
decode.

### Round trip

```text
object offset -> tangent -> encode -> decode -> object offset
```

must meet tolerance across randomized orthonormal frames.

---

# Part VI — Height/displacement interval propagation

## 43. Why intervals matter

A layered material can combine many height sources. POM, displacement, preview framing,
bake cage validation and bounds need a conservative answer before evaluating every
sample.

Represent each scalar height expression by:

```text
I(h) = [h_min, h_max] metres
```

---

## 44. Primitive intervals

Constant:

```text
I(c) = [c,c]
```

Texture with declared source range:

```text
I(texture) = [minDeclared, maxDeclared]
```

Normalized texture remapped to physical height:

```text
h = scale * (x - midpoint)
```

For `x in [0,1]`, evaluate the two endpoints and take min/max.

Unknown procedural source must provide either a proven bound or be classified as
unbounded/unsupported for a feature requiring conservative bounds.

---

## 45. Interval arithmetic

Addition:

```text
[a,b] + [c,d] = [a+c, b+d]
```

Subtraction:

```text
[a,b] - [c,d] = [a-d, b-c]
```

Positive scalar multiply `s >= 0`:

```text
s[a,b] = [sa,sb]
```

Negative scalar multiply `s < 0`:

```text
s[a,b] = [sb,sa]
```

Maximum:

```text
max([a,b],[c,d]) subset [max(a,c), max(b,d)]
```

Minimum:

```text
min([a,b],[c,d]) subset [min(a,c), min(b,d)]
```

For authoring bounds, the right-hand intervals are safe conservative envelopes.

---

## 46. Opacity/mask mix

For:

```text
h = (1-t) a + t b
```

with `t in [0,1]`, a safe bound is:

```text
I(h) = hull(I(a), I(b))
```

If tighter `t` bounds are known, standard interval multiplication can tighten the
result.

Do not assume mask and source are statistically correlated when proving safety.

---

## 47. Layer groups

A group publishes both:

```text
HeightInterval
HeightBlendSemantic
```

so a parent does not need to inspect every child during runtime.

The authoring compiler may cache the interval by recipe hash.

Any edit that changes:

```text
height source
mask bound
blend operation
scale/intensity
transition amount
```

invalidates the cached interval.

---

## 48. Derived bounds

From final interval `[hmin,hmax]` derive:

```text
maxNegativeDisplacement = max(0, -hmin)
maxPositiveDisplacement = max(0,  hmax)
maxAbsDisplacement      = max(abs(hmin), abs(hmax))
```

Consumers:

```text
POM depth/ray extent
runtime conservative bounds
preview framing
high-to-low cage policy
displacement tessellation/microdisplacement eligibility
```

This replaces Blender-specific hidden constants with physical contracts.

---

# Part VII — Typed material-layer composition

## 49. Core IR

Conceptual authoring IR:

```text
MaterialLayerIR {
    LayerGuid stableId;
    SourceField source;
    MappingIR mapping;
    MaskExpr mask;
    ModifierIR modifiers[];
    ChannelWrite writes[];
    LayerBlendPolicies blends;
    float opacity;
    uint64 deterministicSeed;
}
```

A `ChannelWrite` is typed:

```text
ChannelWrite {
    MaterialSemantic semantic;
    ValueExpr expr;
    optional ValueInterval interval;
}
```

This avoids treating normals, heights, roughness and base color as interchangeable
RGBA paint.

---

## 50. Color/value source-over

Choose one canonical working representation.

For premultiplied alpha source-over:

```text
Cout = Cs + Cd * (1 - As)
Aout = As + Ad * (1 - As)
```

where `Cs` and `Cd` are already premultiplied.

For straight source color `cs`, `cd`:

```text
Aout = As + Ad(1-As)
Cout_straight = (cs As + cd Ad(1-As)) / Aout
```

when `Aout > 0`.

Do not implement byte-rounding artifacts as semantic behavior.

---

## 51. Color space

Before physically meaningful blend math:

```text
sRGB texture -> linear working value
```

Data textures remain linear and bypass color transfer.

The texture asset semantic, not the shader author, should determine whether sRGB
conversion is legal.

---

## 52. Height composition policies

Minimum useful policies:

```text
Replace/Mix
Add
Max/Compare
Min
```

Each policy must define both sample evaluation and conservative interval propagation.

A visual “height blend” may later use a smooth max:

```text
smax_k(a,b) = log(exp(k a) + exp(k b)) / k
```

or polynomial smooth-max variant, but the exact function must be stable, bounded and
covered by CPU/GPU parity tests.

Do not call ordinary RGB Overlay a height blend.

---

## 53. Normal composition

Normal layers require a declared method such as:

```text
Replace/Lerp + renormalize
RNM-style reorientation
UDN/whiteout-style approximation if deliberately selected
```

The chosen method is Jerico policy. Ucupaint's presence of distinct normal blend modes
only confirms that normal composition must be typed.

Every normal layer must share the canonical tangent convention.

---

## 54. Mask expression

A mask expression can combine primitive fields and authored textures:

```text
MaskExpr = Clamp(
    Mul(
        TextureMask,
        SmoothStep(k0,k1,ConvexCurvature * radius)
    )
)
```

The IR should support a small composable set:

```text
constant
field reference
texture reference
add/sub/mul/min/max
abs
clamp/saturate
smoothstep
pow with validated domain
noise reference with explicit seed/domain
```

Avoid embedding an unrestricted second shader language merely to reproduce Blender
node freedom.

---

# Part VIII — Semantic specialist bake fields

## 55. Primitive versus recipe

Primitive geometric measurements belong in the bake backend. Visual concepts such as
“dust” belong in Smart Material recipes.

Recommended primitive set:

```text
SignedCurvature
ContactOcclusion
AmbientOcclusion
Thickness
ObjectNormal
TangentNormal
FlowDirection
BoundaryDistance
BevelFeature
Region/Selection/MaterialId
```

---

## 56. Thickness field

At point `p` with geometric normal `n`, cast inward:

```text
origin = p - bias*n
direction = -n
```

Let first valid hit distance be `d`.

Basic thickness:

```text
Thickness = d
```

A more robust two-sided estimate can sample both directions or several cone directions
for noisy/thin geometry, but its semantics must be named separately.

Miss:

```text
confidence = 0
value = policy-defined sentinel, never silently “very thick”
```

---

## 57. Boundary/wire distance

For each surface sample, distance to the nearest selected topology boundary/feature
can drive seams, wireframe-like masks and edge treatments.

Preferred metric depends on consumer:

```text
screen/UV texel distance -> visual wire mask
surface geodesic distance -> physical edge influence
world Euclidean distance -> cheap approximation
```

Name the metric in the field semantic so they are not interchangeable.

---

## 58. Bevel feature

A bevel mask need not physically bevel geometry. A generic feature measure can derive
from:

```text
AbsCurvature
edge classification
boundary distance
feature-angle threshold
```

If an actual bevel-normal baker exists, it is a separate normal-generation operation.

---

## 59. Ambient versus contact occlusion

Keep at least two radii/classes:

```text
AmbientOcclusion     -> lighting-scale visibility field
ContactOcclusion     -> short-range authoring/contact field
```

A single unnamed “AO” knob is insufficient for deterministic material recipes.

---

# Part IX — GPU/CPU implementation shape

## 60. CPU reference path first

The first native implementation should be a small deterministic CPU oracle for:

```text
adjacency
signed dihedral curvature
UV Jacobian flow
TBN/VDM transforms
interval propagation
```

Reasons:

- easier analytic fixtures;
- precise diagnostics;
- clean comparison target for compute kernels;
- lower risk of encoding a wrong convention into GPU code.

---

## 61. Packed adjacency for GPU curvature

A compute path can use arrays conceptually equivalent to:

```text
VertexPositions[Nv]
FaceNormals[Nf]
FaceAreas[Nf]
InteriorEdges[Ne] {
    uint v0, v1;
    uint f0, f1;
    uint orientationBits;
}
VertexDualArea[Nv]
```

Passes:

```text
1. per-edge theta and I_e
2. endpoint accumulation
3. area normalization
4. optional relaxation
5. optional normalized mask mapping
```

Atomic accumulation is easy but may vary in floating-point summation order. If exact
cross-run determinism is required, prefer sorted adjacency + fixed-order segmented
reduction or quantify/lock the tolerated result policy.

Do not claim determinism merely because the same GPU often produces the same bits.

---

## 62. GPU UV flow

Precompute/reference triangle-to-corner adjacency. Per triangle:

```text
compute Ds, Du
validate determinants
J = Du * inverse(Ds)
d = normalize(J*d_ref)
confidence
```

Then reduce to seam-aware corner identities in a deterministic order.

For a pure bake workflow, it may be cheaper to evaluate `J*d_ref` directly while
rasterizing each triangle, eliminating a separate corner field when interpolation is
not required elsewhere.

---

## 63. Bake transaction

All field bakes should follow the existing M8 transactional model:

```text
immutable input snapshot
-> job hash
-> temporary resources
-> execute
-> validate coverage/range/NaN
-> postprocess/dilate/pack
-> publish atomically
```

Cancellation/failure destroys temporary results and cannot mutate source assets.

---

# Part X — Determinism and provenance

## 64. Job hash inputs

At minimum include:

```text
mesh revision/hash
UV set identity
field algorithm/version
physical radius / thresholds
weld policy/tolerance
smoothing policy
ray sample sequence/version
sample count
texture resolution/tile set
TBN convention version
encoding/range
seed for any procedural modulation
```

If a quality-affecting parameter is absent from the hash, cache correctness is not
proven.

---

## 65. Provenance record

A baked field should be able to answer:

```text
what mesh revision produced me?
what UV set?
what algorithm/version?
what physical radius/scale?
what encoding?
what source recipe/job hash?
```

This is more valuable than preserving a Blender node name.

---

# Part XI — Acceptance suite

## 66. Curvature analytic fixtures

### F-CURV-01 Plane

A sufficiently regular planar grid:

```text
max |K| <= epsilon_plane
```

away from excluded boundaries.

### F-CURV-02 Convex cube

Shared manifold cube edges:

```text
K > 0 near convex edges
```

under Jerico polarity convention.

Face interiors after interpolation should trend toward zero as mesh representation
permits.

### F-CURV-03 Concave corner

An inward 90-degree manifold corner:

```text
K < 0
```

with opposite sign from convex fixture.

### F-CURV-04 Cylinder

Away from caps/seams:

```text
same sign around circumference
low angular variance
K scales approximately 1/R
```

### F-CURV-05 Sphere

Regular sphere away from triangulation pathologies:

```text
same sign globally
low coefficient of variation at sufficient tessellation
K scales approximately 1/R
```

Do not overfit an exact constant until the selected curvature convention is frozen.

### F-CURV-06 Uniform scale

For scale `s`:

```text
K_scaled * s ~= K_original
```

### F-CURV-07 UV seam independence

Duplicating UV corners must not change geometric curvature.

### F-CURV-08 Disconnected contact

Two parallel planes nearly touching:

```text
SignedCurvature ~= 0
ContactOcclusion > 0 near contact
```

This proves semantic separation.

### F-CURV-09 Non-manifold

Three faces sharing one edge must produce the declared diagnostic/policy outcome,
never an arbitrary face pair.

---

## 67. Flow analytic fixtures

### F-FLOW-01 Identity

`J=I` and output equals reference direction.

### F-FLOW-02 Rotation

Compare to known 2x2 rotation.

### F-FLOW-03 Shear/scale

Compare to direct matrix result within float tolerance.

### F-FLOW-04 Mirror

Correct transformed direction plus `Mirrored` flag.

### F-FLOW-05 Seam

Independent per-corner results at one geometric vertex.

### F-FLOW-06 Degenerate reference

No NaN/Inf; confidence zero and explicit invalid flag.

---

## 68. VDM fixtures

### F-VDM-01 Orthonormal basis round trip

Random unit normals/tangents with valid handedness:

```text
||deltaP_out - deltaP_in|| <= tolerance
```

### F-VDM-02 Tangent axes

Unit X/Y/Z tangent-space vectors map exactly to T/B/N directions.

### F-VDM-03 Mirrored UV

Handedness produces expected B direction.

### F-VDM-04 Midpoint encoding

Zero displacement encodes exactly/approximately at 0.5 per channel according to
format; extrema encode at boundaries without clipping under valid declared range.

### F-VDM-05 Color-space rejection

An sRGB-tagged VDM is rejected/canonicalized with diagnostic.

### F-VDM-06 Bounds

No decoded texel exceeds declared range beyond quantization tolerance.

---

## 69. Height interval fixtures

For randomly generated expression trees whose leaves have known intervals:

1. compute propagated interval;
2. sample many legal leaf values/masks;
3. verify every evaluated height lies inside propagated interval.

This is a property test, not only a hand-authored example.

---

## 70. Material transaction fixtures

- identical recipe twice -> identical canonical job hash/output under declared
  determinism policy;
- canceled bake -> no published texture/material mutation;
- one missing semantic field -> actionable diagnostic;
- changed physical curvature radius -> cache miss;
- changed UV set -> cache miss;
- reordered unrelated UI-only metadata -> no bake hash change if intentionally
  non-semantic;
- source-free runtime package uses only cooked Jerico assets.

---

# Part XII — Suggested C++ contracts

## 71. Surface feature request

Conceptual only:

```cpp
enum class SurfaceFeatureKind {
    SignedDihedralCurvature,
    SmoothCurvature,
    ContactOcclusion,
    Thickness,
    BoundaryDistance,
};

struct SurfaceFeatureRequest {
    MeshRevisionId mesh;
    SurfaceFeatureKind kind;
    float radiusMeters;
    TopologyAnalysisPolicy topology;
    SmoothingPolicy smoothing;
    DeterminismPolicy determinism;
};
```

Names may change to match repository conventions. The important requirement is one
shared service and typed units.

---

## 72. UV direction request

```cpp
struct UvDirectionFieldRequest {
    MeshRevisionId mesh;
    UvSetId referenceUv;
    UvSetId targetUv;
    Vec2 referenceDirection;
    UvDegeneracyPolicy degeneracy;
};
```

Output is corner-domain direction + confidence/flags.

---

## 73. VDM descriptor

```cpp
struct TangentVectorDisplacementDesc {
    UvSetId uvSet;
    TangentConventionId tangentConvention;
    VectorEncoding encoding;
    Vec3 rangeMeters;
    AxisTransform sourceToJerico;
    TextureAssetId texture;
};
```

The descriptor must reuse existing asset IDs and texture semantic infrastructure.

---

## 74. Material layer compiler boundary

```text
MaterialLayerIR[]
   + SurfaceFieldBindings
   + TextureBindings
   + EnvironmentFieldBindings
        -> validated authored material graph
        -> preview evaluator OR MaterialBakeIR
        -> existing canonical material/texture cook
```

No new runtime material database is required.

---

# Part XIII — What not to implement

## 75. Explicit non-goals

Do not implement because Ucupaint has them:

- a second POM renderer;
- Blender-node-compatible runtime graph;
- Ucupaint-specific node names;
- a duplicate sRGB implementation;
- a duplicate tangent generator;
- a duplicate texture atlas registry;
- a “Dust baker” as a core geometric primitive;
- arbitrary magic fine-bump scale constants;
- automatic welding that silently changes the authored mesh;
- GPL source transliteration.

---

# Part XIV — Practical Smart Material examples

## 76. Worn painted metal

Inputs:

```text
ConvexCurvature
DamageField
ContactOcclusion
manual paint mask
noise field with stable seed
```

Example:

```text
edge = smoothstep(0.15, 0.65, ConvexCurvature * 0.02m)
chipEligibility = edge * DamageField * manualMask
chip = chipEligibility * remap(noise)
```

Then material response can reduce paint coverage, alter roughness and expose the metal
layer. Geometry damage remains owned by the damage/destruction system.

---

## 77. Dirty concrete/rock cavity

```text
cavity = smoothstep(0.10, 0.60, ConcaveCurvature * 0.05m)
contact = ContactOcclusion(radius=0.03m)
dirtMask = saturate(0.65*cavity + 0.55*contact)
```

Coefficients are recipe parameters, not engine constants.

This works on imported props and generated geology using the same field semantics.

---

## 78. Wood grain direction after repack

If an authored straight UV set defines grain as:

```text
d_s = (0,1)
```

and final packed UV is `U`, compute per-triangle:

```text
d_u = normalize((dU/dS) * d_s)
```

Bake/store the direction field. A wood shader/material recipe can then orient grain,
scratches or anisotropy consistently despite atlas packing.

---

## 79. Rust/leak streak direction

Use the UV direction field only when it represents an authored surface flow basis.

For physically gravity-driven streaks, instead project world gravity onto the tangent
plane:

```text
g_t = g - N * dot(g,N)
```

if:

```text
|g_t| > epsilon
```

then:

```text
d_world_surface = normalize(g_t)
```

Convert to tangent/UV representation only at the consumer boundary.

This prevents a texture packing operation from changing gravity semantics.

---

# Part XV — Performance strategy

## 80. Reuse before acceleration

Before implementing any kernel, search for existing Jerico implementations of:

```text
mesh adjacency
half-edge/topology
curvature
ray/BVH query
AO bake
UV rasterization
tangent frame generation
texture dilation/mips
job cache/hash
```

Extend the owner rather than introducing a parallel path.

---

## 81. Cost model

Approximate complexity:

```text
adjacency build             O(F + E)
dihedral curvature          O(E + V)
UV Jacobian flow            O(F + corner reductions)
interval propagation        O(material graph nodes)
VDM basis conversion        O(samples)
contact AO                  O(samples * rays * queryCost)
```

Contact AO is the expensive primitive; curvature/flow should be cheap enough for
interactive rebuilds on ordinary assets if topology is cached.

---

## 82. Dirty-region updates

For authoring edits:

- material parameter-only edits should not rebuild mesh curvature;
- UV edits invalidate flow and texture-space bake but not geometry curvature;
- vertex position/topology edits invalidate curvature and related spatial structures;
- EPLS/world state changes should not rebake static curvature unless explicitly baked
  into a final texture;
- manual mask paint invalidates only dependent material outputs.

A dependency graph should drive this, not broad “rebuild material” calls.

---

# Part XVI — Final implementation priority

## 83. P0

```text
1. shared SignedDihedralCurvature field
2. analytic fixtures + scale law
3. expose positive/negative/absolute derived views
4. keep ContactOcclusion separate
5. wire field references into Smart Material / MaterialLayerIR design
```

This gives immediate value to edge wear, dirt, damage masks, remesh and geology.

---

## 84. P1

```text
1. UV Jacobian FlowDirection
2. specialist MaterialBakeIR outputs
3. typed height interval propagation
4. deterministic bake provenance/cache
```

---

## 85. P2

```text
1. tangent-space VDM semantic
2. offline bake/import path
3. only then evaluate runtime VDM/microdisplacement consumer by measured need
```

---

## 86. Final rule

The durable result of this research is not a `Ucupaint` module.

It is a set of Jerico-owned mathematical primitives:

```text
SurfaceFeatureFieldBuilder
UvDirectionTransport
canonical TBN vector transforms
MaterialLayerIR
MaterialBakeIR extensions
physical interval propagation
```

Those primitives can outlive Ucupaint, Blender and the exact add-on version that
inspired the investigation.