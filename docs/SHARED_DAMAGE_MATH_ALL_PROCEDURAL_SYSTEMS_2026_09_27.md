# JericoGeologyPlayground — Shared Damage Mathematics for ALL Procedural Systems

**Owner request:** 2026-09-27  
**Status:** GLOBAL PLAYGROUND RULE / ACTIVE EXPERIMENTAL AUTHORITY  
**Repository:** `JericoFX/JericoGeologyPlayground`  
**Scope:** geology, splines, procedural assemblies, doors, windows, hatches, rails, bridges, piers, catwalks, pipes, ducts, cable trays, webs, props and future procedural families.

---

# 0. Direct instruction

Do **not** invent a separate damage algorithm for every generator.

All procedural systems in this Playground must consume one shared set of Jerico-native mathematical damage primitives derived from the clean-room/research work already documented in JericoEngine from the owner-supplied Blender references.

Relevant read-only references in JericoEngine include:

- `docs/research/M8_MIX_DAMAGE_LACES_FENCE_AGEDFX_PBG2_MATHEMATICAL_RECONSTRUCTION.md`
- `docs/research/M8_MIX_DAMAGE_LACES_FENCE_AGEDFX_PBG2_TECHNICAL_INPUT.md`
- `docs/research/M8_INFINITE_FRACTURE_TECHNICAL_INPUT.md`
- `docs/research/M8_DAMAGE_CONTROL_TECHNICAL_INPUT.md`
- `docs/research/M8_UCUPAINT_2_4_9_MATHEMATICAL_RECONSTRUCTION.md`
- `docs/research/M8_UCUPAINT_2_4_9_TECHNICAL_INPUT.md`
- `docs/JERICO_ADAPTIVE_REMESH_FEATURE_EDGES_RESEARCH.md`
- `docs/JERICO_EXPERIMENTAL_CRACK_GLASS_FRACTURE_RESEARCH.md`
- `Tools/BlenderWorker/REFERENCE_AUDIT.md`
- `Tools/BlenderWorker/UI_CONTROLS.md`

These are mathematical/behavioral references only. Do not copy Blender node graphs, GPL implementation text or addon internals into the Playground.

JericoEngine remains READ_ONLY.

---

# 1. One shared SurfaceFeatureField

Build one reusable feature-analysis layer.

Conceptual fields:

```text
Curvature(x)
Convexity(x)
Concavity(x)
Cavity(x)
Contact(x)
EdgeDistance(x)
BoundaryDistance(x)
Exposure(x)
Slope(x)
Orientation(x)
Thickness(x)
Height(x)
WorldPosition(x)
SurfaceNormal(x)
SemanticRole(x)
MaterialRole(x)
DamageDistance(x)
```

The same computed fields must be reusable by:

- geology;
- edge chipping;
- cracks;
- weathering;
- dirt;
- rust;
- moss/frost eligibility;
- remesh density;
- damage masks;
- scatter;
- semantic segmentation;
- procedural assemblies.

Do not recompute curvature, cavity and edge distance differently in every tool.

---

# 2. Signed curvature / edge eligibility

Use signed curvature or a stable local normal-variation equivalent.

Convention:

```text
kappa > 0 -> convex tendency
kappa < 0 -> concave tendency
```

A generic convex-edge eligibility may be:

```text
ConvexMask =
    smoothstep(k0, k1, max(kappa, 0))
```

A boundary/feature proximity mask:

```text
EdgeMask =
    1 - smoothstep(d0, d1, EdgeDistance)
```

Combined edge-damage eligibility:

```text
EdgeDamageEligibility =
    saturate(ConvexMask * EdgeMask * SemanticMask)
```

Optional deterministic breakup:

```text
N = remap(noise(position * frequency, seed), -1,1, 0,1)

EdgeDamageField =
    EdgeDamageEligibility
    * lerp(1-breakup, 1, N)
```

Do not use unbounded white-noise displacement.

All amplitudes must be scale-aware and bounded by local feature size/thickness.

---

# 3. Concavity / cavity damage and aging

Concave areas often collect dirt, moisture, corrosion or crack preference.

Example:

```text
ConcavityMask =
    smoothstep(c0, c1, max(-kappa, 0))
```

Combined cavity/contact mask:

```text
CavityAccumulation =
    saturate(
        wConcavity * ConcavityMask
      + wCavity    * Cavity
      + wContact   * Contact)
```

Consumers decide meaning:

- geology -> sediment/dirt accumulation;
- metal -> rust;
- wood -> damp/rot;
- wall -> grime;
- glass/frame -> dirt;
- bridge/pier -> wet decay;
- material -> visual only.

Feature fields remain shared; semantics remain consumer-owned.

---

# 4. Shared DamageField

Every generator may produce one or more masks but combines them through the same DamageField concept.

Conceptually:

```text
DamageField(x) =
    saturate(
        wManual   * ManualMask(x)
      + wEdge     * EdgeDamage(x)
      + wCavity   * CavityDamage(x)
      + wImpact   * ImpactField(x)
      + wSpline   * SplineDamage(x)
      + wAge      * AgingField(x)
      + wNoise    * StructuredNoise(x))
```

Do not require every consumer to use all terms.

Manual/semantic masks may override or protect procedural damage.

Protected regions:

```text
FinalDamage =
    DamageField * (1 - ProtectedMask)
```

Locked/manual states must survive regeneration.

---

# 5. Representation tiers — cheapest valid damage wins

Do not boolean or fracture everything.

Select damage representation by required visual/gameplay effect.

Use the existing tiered philosophy:

```text
Tier 0  no damage / state only
Tier 1  material mask / tint / roughness
Tier 2  normal / parallax / displacement bake
Tier 3  geometric indentation / edge chip
Tier 4  local SDF cavity / through-hole
Tier 5  prepared/prefractured semantic variant
Tier 6  physical chunk activation / Jolt
```

Escalate only when needed by:

- silhouette;
- penetration;
- collision;
- interaction;
- gameplay;
- destruction state.

Examples:

```text
small paint chip -> Tier 1
rust pit -> Tier 2/3
broken plank silhouette -> Tier 3/5
hole in metal door -> Tier 4
shattered concrete chunk -> Tier 5/6
loose railing piece -> Tier 6
```

This rule applies to ALL procedural families.

---

# 6. Edge Damage primitive

Create one reusable Edge Damage operation.

Inputs:

- source selection / semantic role;
- edge/feature selection;
- metric or relative width;
- amount;
- maximum affected length;
- taper length;
- convexity/concavity policy;
- AO/cavity policy;
- overlap policy;
- seed;
- UV/material region policy.

The reference Edge Decal research established that relative width and metric width are different semantics. Preserve that distinction.

Example width policy:

```text
if widthMode == Metric:
    width = widthMeters

if widthMode == Relative:
    width = relativeWidth * sourceCharacteristicSize
```

Endpoint taper for normalized path coordinate `u in [0,1]`:

```text
taperStart = smoothstep(0, taperFraction, u)
taperEnd   = smoothstep(0, taperFraction, 1-u)

Taper = taperStart * taperEnd
```

Final local amplitude:

```text
A(u,x) = baseAmplitude * Taper * EdgeDamageField(x)
```

Use it for:

- rock edges;
- cliff edges;
- door corners;
- window frames;
- plank edges;
- railings;
- bridge beams;
- pipes/collars;
- rails/sleepers;
- metal panels;
- stairs;
- lockers/props.

---

# 7. Damage Projector / Decal3D

Use one shared local projector coordinate system.

For projector center `c`, forward axis `n` and tangent basis `t,b`:

```text
q = x - c

u = dot(q,t)
v = dot(q,b)
w = dot(q,n)
```

Optional 2D rotation/nonuniform scale:

```text
[u';v'] = R(theta) * [u/sx; v/sy]
w' = w/sz
```

Elliptical footprint:

```text
rho = sqrt(u'^2 + v'^2)
```

Noisy boundary:

```text
rhoN =
    rho
    + edgeNoiseAmp
      * signedNoise(edgeFrequency * [u',v'], seed)
```

Soft mask:

```text
M = 1 - smoothstep(rInner, rOuter, rhoN)
```

This same projector must work on:

- rock;
- wall;
- floor;
- ceiling;
- door;
- window frame;
- metal panel;
- pipe;
- bridge;
- pier;
- arbitrary oriented surface.

Do not write a separate impact coordinate system per generator.

---

# 8. Geometric cavity / through-hole

For a bounded cavity use a local SDF cutter.

Radial field:

```text
F_r = rhoN - 1
```

Depth/slab field concept:

```text
F_w = boundedDepthFunction(w, depth)
```

Cutter:

```text
F_cavity = max(F_r, F_w)
```

Host subtraction for SDF where negative is inside:

```text
F_out = max(F_host, -F_cavity)
```

Through-hole condition:

```text
depth >= measuredHostThickness + exitMargin
```

Do not make every damage mark a through-hole.

Use measured host thickness where available.

---

# 9. Fresh interior provenance

When geometric damage creates new interior surfaces, mark them during generation.

Conceptual predicate:

```text
FreshInterior =
    abs(F_out) < eps
    && F_host < -eps
    && abs(F_cavity) < epsCut
```

Persist a semantic region/face stream.

Consumers:

- wood -> fresh exposed wood;
- metal -> bare fractured metal;
- brick -> brick/mortar interior;
- rock -> fresh rock;
- concrete -> aggregate/interior material;
- glass -> fractured edge material.

Do not rediscover fresh interior later from triangle angle heuristics.

---

# 10. Spline-driven damage / Infinite Fracture principle

Infinite Fracture research reinforces one rule:

**damage follows the canonical path; it does not create another spline type.**

For crack/damage path `c(s)` with local radius `r(s)`:

```text
F_crack(x) =
    min_s ( length(x-c(s)) - r(s) )
```

For shallow surface crack, constrain depth:

```text
F_depth =
    max(w, -w-crackDepth)

F_cut =
    max(F_crack, F_depth)
```

Possible deterministic path perturbation:

```text
p_(i+1) =
    p_i
    + step * tangent_i
    + lateralAmplitude * noise_i * binormal_i
```

Keep offsets bounded relative to local thickness/feature scale.

Applications:

- geology cracks;
- damaged rail;
- broken pipe seam;
- door split;
- plank crack;
- bridge beam crack;
- wall fracture;
- broken window-frame path;
- destructive spline cut.

Path arc-length, UV, semantic tile/part identity and damage lineage must survive together.

---

# 11. Infinite Fracture semantic variant principle

Do not regenerate arbitrary fracture topology everywhere.

For repeated/path assemblies:

```text
canonical path
-> stable semantic strip/tile/part IDs
-> DamageField
-> damaged variant resolver
```

Examples:

- railway sleeper 42 -> broken variant;
- deck plank 18 -> split variant;
- bridge hanger 12 -> missing;
- railing bay 7 -> broken top rail;
- facade tile 22 -> chipped;
- cliff strip -> fractured section.

Stable semantic identity is more important than final triangle identity.

---

# 12. Internal damage

For deterministic internal cavity centers:

```text
c_i =
    SampleInside(
        damageRegion,
        Hash(seed, i))
    + offset

r_i =
    baseRadius
    * RadiusVariation(Hash(seed, i, "radius"))

F_i(x) =
    length(x-c_i) - r_i
```

Use only when material/object semantics justify internal damage.

Do not apply porous internal voids blindly to every object.

---

# 13. AgedFX / EPLS-style aging fields

Aging is not a separate geometry system.

Use shared fields such as:

```text
Cavity
Convexity
EdgeDistance
Exposure
WetHistory
Height
Orientation
Contact
```

Examples:

### Dirt

```text
Dirt =
    saturate(
        edgeDirtWeight * EdgeDirt
      + cavityDirtWeight * Cavity
      + contactWeight * Contact
      + environmentalNoise)
```

### Edge wear

```text
Wear =
    ConvexMask
    * EdgeMask
    * Exposure
    * MaterialWearability
```

### Rot / damp wood

```text
Rot =
    Cavity
    * WetHistory
    * (1 - ExposureDrying)
    * WoodMask
```

Use materials/shaders when geometry is unnecessary.

---

# 14. Ucupaint-derived shared surface fields

The Ucupaint clean-room research explicitly exists so Smart Materials, geology, remesh and damage can share analysis.

Use:

- signed curvature;
- cavity/contact;
- boundary distance;
- edge strength;
- slope/orientation;
- height/exposure;
- UV/directional flow.

Do not implement separate versions for:

- geology;
- damage;
- materials;
- remesh;
- scatter.

One backend, many consumers.

---

# 15. Cracker / feature-edge / remesh integration

Damage must preserve meaningful hard/feature edges.

Do not smooth/remesh away:

- fresh fracture boundaries;
- authored hard edges;
- hinge/socket boundaries;
- semantic part borders;
- material boundaries;
- opening edges;
- critical silhouette.

Adaptive remesh may increase density near:

```text
abs(curvature)
edge damage
fracture boundary
fresh interior
material boundary
semantic boundary
```

but must obey budgets.

A contradictory feature set must fail/diagnose rather than split forever.

---

# 16. Consumer rules by system

## Geology

Use shared damage for:

- edge chipping;
- fracture paths;
- cavity/pits;
- fresh rock;
- erosion eligibility;
- cliff edge damage;
- crack networks.

Geology-specific macroform remains geology-owned.

---

## Splines / pipes / rails

Use shared damage for:

- chipped/broken profile edges;
- dents;
- cracks;
- missing spans;
- cut/gap;
- rust/wear masks;
- damaged collars/joints.

Do not damage every spline sample independently.

Damage references stable path intervals/part IDs.

---

## Doors / windows / hatches

Use shared damage for:

- chipped corners;
- wood splits;
- metal dents;
- through-holes;
- broken frame edges;
- glass fracture masks/variants;
- rust/dirt;
- fresh broken interior.

Interaction state remains assembly-owned.

---

## Wooden piers / bridges / stairs

Use shared damage for:

- board edge wear;
- broken plank split;
- rot/damp masks;
- missing members;
- cracked beams;
- fresh interior;
- loose/detached pieces.

Physical promotion to Jolt is separate from geometric damage generation.

---

## Railings / catwalks / structures

Use:

- dents/bends;
- missing semantic members;
- edge wear;
- rust;
- cut/broken members;
- detached parts.

---

## Webs

Spider webs use their own Web Graph for strand identity.

They may consume shared aging/environment fields for:

- dust;
- wetness;
- frost;
- dirt;
- broken/missing state masks.

Do not force solid SDF damage logic onto a thin web.

---

# 17. Manual selection and procedural selection

Every damage consumer must support a reusable selection stack when appropriate:

- semantic Part ID;
- path-distance interval;
- index range;
- surface feature;
- projector volume;
- spline mask;
- painted/manual mask;
- deterministic probability;
- damage cluster;
- locked/protected region.

Manual/locked selection wins over procedural regeneration.

---

# 18. Determinism

Every procedural damage proposal derives from stable inputs:

```text
recipeGuid
part/edge/region stable ID
damage operation ID
seed
parameter values
```

Do not use:

- frame time;
- pointer address;
- unordered iteration order;
- global random state.

Same recipe + same inputs -> same damage proposal.

---

# 19. Debug UI

Add a shared damage debug surface usable by all Playground generators.

Display:

- DamageField
- Curvature
- Convexity
- Concavity
- Cavity
- EdgeDistance
- Exposure
- Thickness
- FreshInterior
- ProtectedMask
- selected damage tier
- semantic Part/Edge IDs

Controls:

- Enable Damage
- Damage operation
- representation tier / Auto
- seed
- amount
- feature weights
- edge width mode
- edge width
- taper
- projector transform
- cavity depth
- crack width/depth
- protected/locked selection
- regenerate

Do not make every generator duplicate these controls.

---

# 20. Tests

Shared backend tests:

- signed curvature known fixture;
- convex/concave classification;
- edge distance;
- cavity/contact fixture;
- deterministic feature fields;
- projector transform;
- elliptical footprint;
- crack distance field;
- SDF subtraction;
- through-hole threshold;
- fresh interior classification;
- deterministic damage seed;
- protected region exclusion;
- tier selection.

Cross-consumer tests:

- same Edge Damage operation on rock and door-frame edge;
- same DamageDecal3D on rock and metal panel;
- same spline crack field on geology and wooden beam;
- same aging fields drive dirt on metal/wood without geometry changes;
- semantic IDs survive quality/tessellation changes.

Robustness:

- NaN/Inf reject;
- zero scale reject;
- impossible depth reject;
- excessive voxel/mesh budget reject;
- remesh feature contradiction terminates with diagnostic;
- no unbounded recursion/splitting.

---

# 21. Performance rule

Cache shared SurfaceFeatureFields where source topology/revision is unchanged.

Do not recompute expensive curvature/cavity/raycast analysis independently for five consumers.

Conceptually:

```text
source mesh revision
-> SurfaceFeatureField cache
-> geology
-> materials
-> damage
-> remesh
-> scatter
```

Invalidate only when relevant geometry/topology changes.

Measure cache hit/miss and generation cost in the Playground.

---

# 22. Global acceptance rule

A new procedural generator in this Playground is not allowed to introduce a private ad-hoc damage system if the shared DamageField/SurfaceFeatureField can express the need.

Before adding new damage math, document:

1. which existing shared primitive is insufficient;
2. the concrete failing fixture;
3. why extending the shared backend is better/worse;
4. tests proving the new primitive.

Default action: extend/reuse shared math.

---

# 23. Owner-facing intent

The owner should be able to create:

- a rock;
- a pipe;
- a rail;
- a door;
- a window;
- a wooden pier;
- a bridge;
- a railing;

and apply conceptually the same damage tools:

```text
Edge Damage
Crack
Dent / Projector
Cavity
Through Hole
Missing Part
Broken Variant
Aging / Dirt / Rust / Rot
Fresh Interior
```

Each consumer decides what representations are valid, but the underlying fields and damage semantics stay shared.

This is the point of translating/reconstructing all those Blender addon ideas into C++/Jerico mathematics: **use them everywhere, not only in geology.**

**CHOTO = JericoEngine 1.2 codename. Not a damage algorithm.**
