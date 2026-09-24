# Jerico Procedural Geology
## Research-Backed Architecture, Mathematics, Operator Library, and Implementation Blueprint

**Status:** Engineering direction for Codex  
**Purpose:** Replace trial-and-error “SDF + more noise” rock generation with a research-backed, inspectable, composable procedural geology system.  
**Current scope decision:** **BSP → VDB is cancelled/out of scope for this task.** It may become a separate external tool/application later. Do not reintroduce it into the current critical path.

---

# 0. Executive decision

The current visual failure is not primarily a meshing problem.

A pipeline can be perfectly valid:

```text
Primitive / SDF
    ↓
procedural evaluation
    ↓
GPU field
    ↓
Surface Nets
    ↓
mesh
```

and still produce bad geology if the field is mathematically just:

```text
simple primitive
+
isotropic noise
+
more isotropic noise
```

A correct mesher will faithfully reconstruct a bad field.

The target is therefore **not**:

> “Find a better noise combination until a rock looks good.”

The target is:

> **Represent geological structure explicitly, then use noise only where randomness, irregularity, weathering variation, or fine detail is needed.**

The research reviewed for this document repeatedly supports the same general lesson:

```text
STRUCTURE
    ↓
STRUCTURAL MODIFICATION / FRACTURE / CUTTING
    ↓
WEATHERING / MATERIAL REMOVAL
    ↓
MESO DETAIL
    ↓
MICRO DETAIL
    ↓
MESH
```

Different papers use different representations—surface meshes, voxel grids, narrow-band volumes, implicit blocks, Voronoi volumes, grammars—but the successful methods generally do **more than perturb a primitive with noise**.

Jerico should therefore use a **hybrid procedural architecture**:

```text
Source Geometry / Field
        ↓
Macro Structure
        ↓
Geological Structure
        ↓
Weathering / Carving
        ↓
Controlled Detail
        ↓
Field or Mesh Output
        ↓
Existing Meshing / Cleanup / LOD pipeline
```

SDFs remain useful.

Surface Nets remains useful.

Noise remains useful.

None of those should be asked to invent geological structure by themselves.

---

# 1. Source classification used in this document

Every important recommendation below is classified as one of:

### `[SOURCE]`

Directly supported by a paper, thesis, or inspected repository.

### `[JERICO SYNTHESIS]`

A proposed adaptation or combination for Jerico based on the reviewed sources.

### `[DEFERRED]`

Research-backed idea that is useful but should not be implemented before the first structural core works.

This distinction is important.

Codex must not treat a Jerico synthesis as though it were copied verbatim from a paper.

---

# 2. Research corpus

This document is based on the following sources.

## R1 — Anders Söderlund, *Procedural Modeling of Rocks* (2015)

Uppsala University thesis supplied with this task.

Main topics:

- recursive triangle subdivision / “sphere inflation”
- base-shape distortion
- local vs global vertex displacement
- feature edges
- attempted corner chipping
- fractal noise
- procedural granite shading

Important conclusion:

The sphere-inflation method can produce plausible rounded boulders, but the author explicitly notes a tendency toward round forms and difficulty producing sharp-edged rocks at higher densities.

Use this thesis mainly as:

- a historical comparison,
- a warning about repeated roundness,
- and a bibliography gateway.

Do **not** make sphere inflation Jerico’s primary geology architecture.

---

## R2 — Acibal, `acfaruk/proc-rock`

Repository:

https://github.com/acfaruk/proc-rock

License:

**GPL-3.0**

Key architecture visible in the repository:

- customizable ordered pipeline
- generators
- modifiers
- parameterization/texturing stages
- node-based noise graphs
- boolean mesh geometry
- skin surfaces
- deterministic/reproducible JSON configuration
- interactive GUI

Important licensing note:

> Do not copy GPL implementation code into Jerico unless Jerico is intentionally GPL-compatible.

The architecture and high-level mathematical ideas may be studied.

---

## R3 — Dorsey et al., *Modeling and Rendering of Weathered Stone* (SIGGRAPH 1999)

Paper:

https://www.cs.jhu.edu/~misha/ReadingSeminar/Papers/Dorsey99.pdf

Main ideas:

- a **surface-aligned narrow volumetric band** (“slab”)
- material/mineral composition near the surface
- moisture flow
- dissolution and recrystallization
- erosion
- volumetric phenomena localized near the visible surface

This is highly relevant to a possible future Jerico **narrow-band weathering** system.

It is **not** a recommendation to implement full porous-media simulation in the first milestone.

---

## R4 — Beardall et al., *Goblins by Spheroidal Weathering* (2007)

Paper:

https://faculty.cs.byu.edu/~jones/cgnp/papers/goblins.pdf

Main ideas:

- voxel representation
- weathering based on local exposure / neighborhood geometry
- corners and convex regions weather faster than broad flat regions
- material resistance
- interactive parameters
- extraction to mesh after weathering

This provides a much more useful first erosion model than “subtract random noise.”

---

## R5 — Jones et al., *Directable Weathering of Concave Rock Using Curvature Estimation* (2010)

DOI:

https://doi.org/10.1109/TVCG.2009.39

Main ideas:

- approximate mean curvature on a voxel grid
- spheroidal weathering concentrated at positive curvature
- cavernous weathering concentrated at negative curvature
- editable rock durability
- rockfall
- colluvium deposition
- authorable/directable behavior

This is a strong reference for a future `Weathering` operator family.

---

## R6 — Dart, De Rossi, Togelius, *SpeedRock: Procedural Rocks through Grammars and Evolution* (2011)

Paper:

https://pcgworkshop.com/archive/dart2011speedrock.pdf

Main ideas:

- 3D grammar / matrix division
- 8-way subdivision of cuboids
- solid/empty rule structure
- deterministic “implosion”
- optional neighborhood-based erosion
- user-controlled shape ratios
- final skinning by ray projection from an enclosing sphere

The evolutionary component is optional for Jerico.

The more useful idea is:

```text
structural mass
→ compaction
→ erosion
→ skin / mesh
```

---

## R7 — Peytavie et al., *Procedural Generation of Rock Piles Using Aperiodic Tiling* (2009)

DOI:

https://doi.org/10.1111/j.1467-8659.2009.01557.x

Main ideas:

- aperiodic tiling for piles
- anisotropic Voronoi rock volumes
- ellipsoidal anisotropic distance
- controllable orientation/aspect
- discrete voxel Voronoi computation
- erosion of the resulting coarse volumes
- contact preservation during erosion

This is highly relevant to Jerico even when generating a **single rock**, because anisotropic Voronoi cells are useful structural building blocks and fracture regions.

---

## R8 — Paris et al., *Modeling Rocky Scenery using Implicit Blocks* (2020)

Project:

https://aparis69.github.io/public_html/projects/paris2020_Blocks.html

Code:

https://github.com/aparis69/Rock-fracturing

DOI:

https://doi.org/10.1007/s00371-020-01905-6

Code license:

**MIT**

Main ideas:

- geologically motivated fracture distributions
- tiling blocks represented as implicit volumetric primitives
- implicit replication
- generalized union/blending
- multiple block families
- strata-aware placement
- gradient-based warping for detail
- cliffs/crags/promontories rather than only isolated rounded rocks

This is one of the strongest references for Jerico’s existing implicit/SDF direction.

The key lesson is:

> An implicit field works well when it represents **structured blocks/fractures**, not merely a noisy primitive.

---

## R9 — Desbenoit, Galin, Akkouche, *Modeling Cracks and Fractures* (2005)

DOI:

https://doi.org/10.1007/s00371-005-0317-z

Main ideas:

- editable 2D crack patterns
- crack profile curves
- map the crack network onto a surface
- construct a procedural swept carving volume
- subtract the volume from the object
- adaptive resampling to avoid bad self-intersections

This provides a useful reference for a future high-resolution directed crack operator.

---

## R10 — Venu, Bosak, Padrón-Griffe, *Procedural Multiscale Geometry Modeling using Implicit Surfaces* (2025)

DOI:

https://doi.org/10.1111/cgf.70233

Project:

https://graphics.unizar.es/projects/MultiscaleGeometry_2025/

Main ideas:

- implicit multiscale geometry
- mesoscopic to microscopic structures
- spatially varying procedural distributions
- particulate, fibrous, porous, and laminar structure classes
- implicit evaluation without mandatory precomputation

This source reinforces the value of **separating structural scales**. Jerico's
bounded implementation deliberately selects only two porous structural scales,
fixed local evaluation and the existing cumulative field/cook/runtime vertical;
it does not adopt the paper's general material framework or implementation.

---

# 3. What the supplied 2015 thesis tells us

## 3.1 Sphere inflation is useful but limited

`[SOURCE: R1]`

The thesis starts from triangular base shapes such as:

- tetrahedron
- octahedron
- icosahedron
- triangulated cube

and recursively subdivides triangles.

New vertices are displaced outward toward a spherical envelope, with variants based on:

- a fraction of the distance toward the sphere,
- or a fraction of the source edge length.

Randomness is added to avoid uniformity.

This creates useful rounded boulders.

However, the thesis also reports:

- excessive iterations create irregular valleys/pits,
- local displacement is more coherent than global displacement,
- base-shape topology remains visible,
- higher density tends toward roundness,
- sharp-edged rocks are difficult.

This is exactly the kind of failure Jerico must avoid turning into its universal representation.

---

## 3.2 The thesis already contains a structural idea that is more interesting than sphere inflation

`[SOURCE: R1]`

The unfinished “feature edge + corner chipping” experiment is conceptually important.

The intended process was:

```text
box-like source
→ preserve important feature edges
→ identify corners
→ construct a cutting plane
→ remove geometry on the outside
→ retriangulate the opening
```

The thesis did not successfully complete this method.

But the architectural idea is much closer to what we want:

> modify recognisable structure by explicit cuts.

That aligns strongly with `proc-rock` and later implicit-block work.

---

# 4. The central mistake to avoid

This pattern is insufficient:

```cpp
float d = sdSphere(p, radius);

d += fbm(p * f0) * a0;
d += ridged(p * f1) * a1;
d += worley(p * f2) * a2;

return d;
```

It can be:

- mathematically valid,
- deterministic,
- GPU-friendly,
- fast,
- and perfectly meshed,

while still producing a potato.

The issue is semantic.

The formula says:

> “Take a roughly spherical mass and randomly perturb its boundary.”

It does not say:

- “this material has bedding,”
- “this is separated by joint planes,”
- “this corner has weathered faster,”
- “this fracture family is aligned to stress,”
- “this fault offsets one region,”
- “this block is bounded by three fracture sets,”
- “this cavity grows in sheltered negative-curvature regions.”

---

# 5. The new mental model

Jerico should separate five concepts.

```text
1. REPRESENTATION
   What data represents the current shape?

2. STRUCTURE
   What large/mid-scale geological organization exists?

3. PROCESS
   What removes, moves, fractures, folds or deposits material?

4. IRREGULARITY
   Where does controlled randomness modify the structure?

5. EXTRACTION
   How does the final representation become a renderable mesh?
```

SDF is primarily part of **representation**.

Surface Nets is primarily part of **extraction**.

Perlin/Worley/FBM are primarily part of **irregularity** or building blocks for structure.

The missing core has been **structure + process**.

---

# 6. Recommended hybrid representation

Jerico should not force every operation into one representation if another representation is clearly better.

Use three execution domains.

## Domain A — Implicit / scalar field

Best suited for:

- primitives
- CSG
- smooth CSG
- large carving
- implicit blocks
- cavities
- strata masks
- faults as domain transforms
- directional warps
- analytic masks
- procedural fracture fields
- Surface Nets extraction

---

## Domain B — Voxel / discrete field

Best suited for:

- iterative weathering
- neighborhood erosion
- curvature approximation
- durability fields
- rockfall masks
- deposition
- discrete Voronoi ownership
- cellular processes

The existing GPU voxel/field path may already provide much of the infrastructure.

Do not bring VDB into this milestone merely to have voxels.

---

## Domain C — Mesh modifiers

Best suited for:

- exact boolean plane cuts when robust mesh CSG exists
- direct normal displacement
- post-extraction operations
- imported-mesh workflows
- surface crack mapping
- simplification/remeshing/retopology

---

# 7. Representation bridges

The system should support explicit bridges instead of pretending all stages are identical.

Conceptually:

```text
Implicit Field
    ↓ Surface Nets
Mesh

Mesh
    ↓ optional field sampling / voxelization
Discrete Field

Discrete Weathering Field
    ↓ isosurface extraction
Mesh
```

However:

> BSP → VDB is NOT part of this task.

Any future BSP/VDB converter belongs in a separate dedicated tool. Direct use of
Jerico's existing BSP/brush mathematics as a geology source is a different,
supported workflow and is defined below.

---

# BSP / Brush Driven Geology

This section defines a primary authored-space workflow that does not require,
preserve or reintroduce VDB.

The level designer may author a simple playable volume with Jerico BSP or convex
brushes. That authored geometry is the macro constraint: it establishes the
intended space, the surfaces that may receive geology, the regions that must
remain clear and the permitted exterior generation region. It is not the final
rock mesh.

The desired path is:

```text
BSP / Convex Brushes
    ↓
Existing Brush / BSP Field Mathematics
    ↓
Selected Geological Surfaces
    ↓
Rock Shell / Generation Region
    ↓
Geological Structural Operators
    ↓
Weathering
    ↓
Controlled Detail
    ↓
GPU Scalar Field
    ↓
Surface Nets
    ↓
Final Rock Mesh
```

The prohibited path remains:

```text
BSP
    ↓
OpenVDB / NanoVDB / PNanoVDB
    ↓
Conversion Pipeline
```

BSP → VDB remains cancelled and outside JericoEngine. This workflow must not
build a conversion layer, intermediate volume format or replacement field
engine around it.

## Authoring intent

For a rectangular tunnel authored with brushes, the BSP primarily defines:

- the playable space;
- the macro shape and spatial intent;
- where walls, ceiling or floor exist;
- which regions must remain free;
- which faces or surface groups may receive geology.

Jerico Geology transforms the permitted exterior region around those surfaces.
The original tunnel remains the designer's authoritative space constraint; the
generated rock is a derived product.

## Design volume and geology volume

The implementation must preserve two distinct conceptual volumes:

1. **Design volume** — space authored by the level designer that must remain
   usable and recognizable, such as the interior of a tunnel.
2. **Geology volume** — exterior space where the recipe has permission to create,
   remove or deform rock.

Do not collapse these into one ambiguous field. Their mathematical composition
must use Jerico's existing field/CSG sign convention and recipe representation;
it must not introduce a parallel volume type.

## Protected playable volume

The BSP interior must be able to produce a protected playable volume with an
optional safety clearance:

```text
ProtectedPlayableVolume = expand(BspPlayableRegion, safetyClearance)
```

The exact operation must follow the field and CSG conventions already admitted
by Jerico. The invariant is independent of sign convention:

> Final rock must not occupy the protected playable volume.

Directional warp, fractures, cavities, weathering, implicit blocks and detail
may alter the permitted rock region, but their composed result must be clipped
or constrained so that they cannot place large masses inside designer-reserved
walkable space.

The clearance value is authoring data, participates in deterministic identity,
is visible in diagnostics and must be tested at zero and non-zero values.

## Rock shell / generation depth

Selected BSP surfaces must be able to define a finite exterior rock shell rather
than merely displacing an infinitely thin face:

```text
Playable space
    ↓
Protected clearance
    ↓
Finite exterior rock-generation shell
```

The authoring model needs concepts equivalent to:

- playable clearance;
- exterior rock depth;
- enabled faces or surface groups;
- deterministic geological seed;
- material and recipe reference.

These names are conceptual. Reuse existing Jerico property, face metadata,
recipe, reference and unit conventions when they already express the same data.
Do not create duplicate parameters or a second recipe format.

Rock depth must be finite, expressed in meters, included in identity and bounded
by preview quality/workload guards. Zero depth is either rejected with a clear
diagnostic or produces the documented empty shell; it must never silently become
an unbounded solid.

## Face / surface selection

Geology must be applicable to any deliberate combination of BSP faces. For
example, a tunnel may enable its ceiling and side walls while leaving its floor
clean.

Prefer existing BSP face identities, surface metadata, material slots, tags and
editor selection. Selection must remain stable across deterministic BSP rebuilds
according to the repository's existing identity contract. Do not add a second
face-selection system if a current Jerico mechanism can represent this scope.

Disabled faces do not authorize nearby geology merely because another selected
face shares an edge. Edge and corner behavior must be defined and covered by
tests so that shell generation is continuous where intended without leaking
onto excluded surfaces.

## Direct field integration

Before implementation, audit the current BSP/brush path for reusable
mathematical evaluation through:

- brush planes;
- convex volumes;
- CSG or BSP plane classification;
- brush or solid fields;
- signed or implicit field evaluation.

The implementation must connect the strongest existing representation directly
to the procedural recipe and GPU field path:

```text
BSP / Brush
    ↓
Existing Jerico Mathematical Field Representation
    ↓
Generic Procedural Geology Recipe
```

If generic adapters created during earlier work remain useful, they may be kept
only when their API has no VDB dependency or terminology, represents this direct
mathematical bridge and does not duplicate existing Jerico functionality.

## Generic geological operators

A BSP-derived surface uses the same generic operator library as every other
source. It may receive, as admitted by the recipe and execution domain:

- `PlaneCut`;
- `DirectionalWarp`;
- `Strata`;
- `FaultDisplace`;
- `JointSets`;
- `AnisotropicVoronoi`;
- `FractureCells`;
- `ImplicitBlocks`;
- `CavityCarve`;
- `BubbleWeathering`;
- `CurvatureWeathering`;
- ridged/FBM detail;
- structural masks.

Do not add product-shaped entry points such as `GenerateTunnelRock`,
`GenerateCaveWall` or `GenerateBspRock`. Tunnels, caves, cliffs and other forms
must emerge from generic sources, constraints, operators and recipes.

## Mandatory primary validation scenario

The direct BSP/brush workflow is not accepted until this end-to-end scenario is
demonstrated:

1. Create a rectangular tunnel with BSP/convex brushes.
2. Mark the walls and ceiling to receive geology.
3. Leave the floor optionally excluded and visibly clean.
4. Define a non-zero playable safety clearance.
5. Generate a finite rock shell toward the exterior.
6. Apply strata, fractures, cavities, weathering and controlled detail through
   the generic stack.
7. Evaluate the scalar field through Jerico's existing GPU path.
8. Extract the final rock mesh with Surface Nets.
9. Show that the selected surfaces read as a coherent rock formation rather
   than displaced noise.
10. Prove mathematically and visually that no generated rock invades the
    protected playable volume.
11. Disable micro-detail and prove that the walls retain geological structure.
12. Change the source BSP dimensions or topology.
13. Rebuild.
14. Prove that the geology adapts deterministically to the changed authored
    space while respecting selection, clearance and rock depth.

The automated portion must include protected-volume sampling, excluded-face
coverage, deterministic identity, rebuild-after-source-change, CPU/GPU parity
for the admitted field operations and Surface Nets validity. The viewport proof
must show the source constraint, protected volume, generation shell and final
mesh independently as well as in the composed result.

## Architectural goal

> The level designer authors **space** with BSP/brushes.
>
> Jerico authors **geology** around that space.

BSP is the macro-design constraint. Procedural geology is the structural
transformation. Noise supplies controlled irregularity and detail. Surface Nets
extracts the mesh. VDB is not required for any part of this workflow.

---

# 8. Required procedural scale hierarchy

The Playground and runtime data model should explicitly recognize scale.

## 8.1 Macro

Purpose:

- silhouette
- overall mass
- major cuts
- large overhangs
- large directional deformation
- dominant fracture blocks

Examples:

```text
1 m → 100 m+
```

depending on asset type.

Operators:

- anisotropic scale
- stretch
- bend
- taper
- large plane cut
- large cavity
- major fault
- block distribution

---

## 8.2 Meso

Purpose:

- geology that is readable as geology

Operators:

- strata
- bedding
- fracture sets
- joints
- secondary faults
- block splitting
- terraces
- curvature weathering
- chunk erosion

Examples:

```text
5 cm → several meters
```

---

## 8.3 Micro

Purpose:

- final surface character

Operators:

- FBM
- ridged noise
- small cellular detail
- pitting
- micro cavities
- texture/bump/normal detail

Examples:

```text
sub-mm → centimeters
```

---

# 9. Critical design rule: noise does not own the silhouette

At macro scale, noise should normally be constrained by:

- direction,
- masks,
- structural domains,
- fracture planes,
- layer orientation,
- block orientation,
- curvature,
- height,
- slope,
- material/durability.

Bad:

```text
source
→ huge isotropic FBM
→ huge isotropic Worley
```

Better:

```text
source
→ anisotropic transform
→ plane cut
→ fracture set
→ cavity
→ small warped detail
```

---

# 10. Operator contract

Every operator must have an explicit contract.

```cpp
struct OperatorContract
{
    OperatorId id;
    OperatorCategory category;
    ExecutionDomain domain;

    InputSemantic input;
    OutputSemantic output;

    ParameterSchema parameters;

    bool deterministic;
    bool hasCpuReference;
    bool hasGpuImplementation;
};
```

Each operator specification must answer:

```text
What is the input?

What coordinates are used?

What mathematical transformation is performed?

What parameters control it?

What happens at zero strength?

What should be deterministic?

What invariants must hold?

How is it tested?

Can CPU and GPU versions be compared?
```

---

# 11. High-priority operator library

The following is the recommended first structural core.

```text
P0
    PlaneCut
    DirectionalWarp
    Strata
    FaultDisplace
    AnisotropicVoronoi
    FractureCells
    CavityCarve
    HeightMask
    DirectionMask
    SlopeMask
    CurvatureWeathering
    BubbleWeathering
    DurabilityMask
    SmoothBoolean
    FBMDetail
    RidgedDetail
```

The system should become visually useful before adding dozens of secondary operators.

---

# 12. Source operators

## 12.1 Box

`[JERICO SYNTHESIS / standard SDF]`

```text
parameters:
    halfExtents
    transform
```

Keep edges relatively clean at source.

Do not immediately destroy it with large isotropic displacement.

---

## 12.2 Sphere

Useful for:

- rounded source mass
- cavities
- local cutters
- comparison tests

Not the default source for every rock.

---

## 12.3 Cylinder

Useful for:

- columns
- pillar formations
- vertical masses
- columnar-rock experiments

---

## 12.4 Plane

Useful for:

- cliffs
- terrain surfaces
- clipping fields
- strata coordinates
- fault boundaries

---

## 12.5 Imported mesh

`[PASS: bounded direct-mesh offline/static planar displacement]`

Useful for applying:

- cuts
- cracks
- weathering
- surface displacement

Do not block P0 on imported-mesh field conversion.

The admitted P1 implementation does not perform field conversion. It deepens
the existing canonical glTF cook with one selected-submesh planar displacement,
preserves topology/material/UV/optional `GEOL` lineage, reconstructs directions
and bounds, and emits exact static triangle collision. Runtime mutation,
Boolean/remesh/retopology, classification synthesis and dedicated UI remain
separate `INTEGRATION REQUIRED` boundaries. See
`docs/evidence/M8_GEOLOGY_BLUEPRINT_P1_IMPORTED_MESH_STRUCTURAL_CLOSURE.md`.

---

# 13. PlaneCut

## Purpose

Create a dominant planar structural face.

This is one of the cheapest ways to stop a source from becoming a smooth blob.

## Research basis

`[SOURCE: R1, R2]`

- The 2015 thesis attempted corner cutting using a constructed plane.
- `proc-rock` has a cut-plane modifier implemented with boolean mesh difference.

## Field formulation

`[JERICO SYNTHESIS]`

Let:

```text
n = normalize(planeNormal)
P(p) = dot(p, n) - offset
```

Then combine `P` with the source according to Jerico’s signed-distance convention.

For a retained half-space, a common SDF-style intersection is:

```text
result = max(source, P)
```

or the sign-reversed equivalent.

**Do not paste this sign blindly.**

First verify Jerico’s exact convention.

## Optional irregular cut

Noise should perturb the plane, not replace it:

```text
P'(p) =
    dot(p, n)
    - offset
    + roughness * N(T(p))
```

where:

```text
T(p)
```

may be anisotropically scaled.

## Parameters

```text
normal
offset
roughness
roughnessFrequency
roughnessSeed
roughnessScaleXYZ
```

## Tests

```text
roughness = 0 → exact plane
normal length independent after normalization
fixed seed → deterministic
far retained side remains retained
far removed side remains removed
CPU/GPU parity
```

---

# 14. DirectionalWarp

## Purpose

Create non-isotropic macro deformation.

## Why it matters

A free 3D warp often turns into another blob generator.

Directional warp preserves a structural axis.

## Formula

`[JERICO SYNTHESIS]`

```text
d = normalize(direction)

w =
    amplitude *
    noise(p * frequency, seed)

q =
    p + d * w

result =
    source(q)
```

An anisotropic domain can be used:

```text
u =
    R * p

u' =
    u * frequencyXYZ

w =
    noise(u', seed)

q =
    p + d * amplitude * w
```

## Parameters

```text
direction
strength
frequencyXYZ
seed
octaves
mask
```

## Identity

```text
strength = 0
```

must exactly reproduce the source.

---

# 15. Strata / Bedding

## Purpose

Represent oriented layering.

## Research basis

`[SOURCE: R3, R8]`

Dorsey et al. model sandstone-like bedding as layered mineral structure, and Implicit Blocks adapts placement to geological strata.

## Core coordinate

`[JERICO SYNTHESIS]`

```text
a = normalize(layerAxis)

h =
    dot(p, a)
```

Optional low-frequency layer deformation:

```text
hw =
    h
    + warpStrength *
      N(p * warpFrequency, warpSeed)
```

Periodic phase:

```text
phi =
    hw * frequency
    + phase
```

Example profile:

```text
b =
    sin(phi)
```

or:

```text
b =
    triangleWave(phi)
```

or:

```text
b =
    sawtooth(phi)
```

depending on desired sediment profile.

Convert to a mask:

```text
m =
    smoothstep(threshold0,
               threshold1,
               b)
```

Use `m` to control:

- displacement
- hardness
- erosion resistance
- material ID
- fracture probability
- cavity strength

## Critical rule

Strata should preferably become a **shared structural field**, not merely geometry displacement.

Example:

```text
StrataField
    ↓
geometry
durability
material
weathering
fracture bias
```

This gives the system internal geological coherence.

---

# 16. FaultDisplace

## Purpose

Offset one region of material relative to another along a fault plane.

## Formula

`[JERICO SYNTHESIS]`

Fault coordinate:

```text
n = normalize(faultNormal)

h =
    dot(p, n)
    - planeOffset
```

Smooth side selector:

```text
s =
    smoothstep(-width,
                width,
                h)
```

Displacement:

```text
v =
    normalize(slipDirection)
    * displacement
```

Deformed sampling coordinate:

```text
q =
    p
    + s * v
```

Result:

```text
result =
    source(q)
```

Optional irregular fault boundary:

```text
h' =
    h
    + boundaryRoughness *
      N(p * roughnessFrequency,
        roughnessSeed)
```

## Parameters

```text
faultNormal
planeOffset
slipDirection
displacement
width
boundaryRoughness
roughnessFrequency
roughnessSeed
```

## Tests

```text
displacement = 0 → identity
width > 0 → finite smooth transition
same seed → deterministic
opposite sides evaluate at expected shifted positions
CPU/GPU parity
```

---

# 17. AnisotropicVoronoi

This operator is a major recommendation.

## Research basis

`[SOURCE: R7]`

Peytavie et al. explicitly reject ordinary Euclidean Voronoi for rock shaping because it tends toward round cells.

They use an ellipsoidal anisotropic metric.

For seed `p_k`, local frame:

```text
x_k
y_k
z_k
```

and coefficients:

```text
alpha_k
beta_k
gamma_k
```

their scalar form is:

```text
f_k(p) =
    alpha_k * ((p - p_k) · x_k)^2
  + beta_k  * ((p - p_k) · y_k)^2
  + gamma_k * ((p - p_k) · z_k)^2

d_k(p) =
    f_k(p) / g_k
```

where `g_k > 0` controls relative scale.

A voxel belongs to the seed with minimum `d_k`.

The paper demonstrates that changing the anisotropy coefficients changes rock shape; e.g. stronger coefficients on two axes can create flatter stone volumes.

## Jerico value

This should not only be called “Worley noise.”

It should exist as an actual structural primitive:

```text
AnisotropicVoronoiCells
```

with explicit:

- seed positions
- local orientation
- axis weights
- relative sizes

## Parameters

```text
seed
cellCount / density
anisotropyXYZ
orientationMode
orientationJitter
sizeMin
sizeMax
jitter
```

## Outputs

Prefer exposing more than a scalar:

```text
nearestCellId
nearestMetric
secondMetric
boundaryMetric
localCellFrame
```

This enables later operators to use the same structure.

---

# 18. FractureCells

## Purpose

Turn cell boundaries into actual structural fractures.

## Research basis

`[JERICO SYNTHESIS based on R7 + cellular/Worley principles]`

If the evaluator computes:

```text
d1 = nearest anisotropic-cell distance
d2 = second-nearest distance
```

then:

```text
boundary =
    d2 - d1
```

is small near a cell boundary.

A fracture mask can be derived:

```text
crack =
    1
    - smoothstep(width,
                 width + softness,
                 boundary)
```

Then use the fracture mask for:

- carving
- weakening durability
- separating block IDs
- material variation
- cavity growth

## Better design

Do not immediately destroy all boundaries.

Expose:

```text
fractureMask
fractureId
fractureNormalApprox
```

so weathering can act on them later.

---

# 19. JointSet / fracture-plane families

`[JERICO SYNTHESIS strongly motivated by R8]`

For cliffs, many geological forms are better described as sets of roughly parallel planes than cellular noise.

A joint family:

```text
J_i(p) =
    dot(p, n_i)
    - offset_i
```

Use several families:

```text
family A
family B
family C
```

to construct:

- tabular blocks
- columnar blocks
- equidimensional blocks
- rhombohedral/polyhedral blocks

### Example conceptual family

```text
Family A:
    normal ≈ up

Family B:
    normal ≈ horizontal X

Family C:
    normal ≈ horizontal Z
```

Irregularity:

```text
offset_i += smallNoise
normal_i += smallAngularJitter
```

The structural plane remains dominant.

---

# 20. ImplicitBlocks

## Research basis

`[SOURCE: R8]`

Paris et al. create fracture distributions in bedrock, form tiling blocks as implicit volumetric primitives, replicate them over steep terrain, and combine them using generalized union/blending.

The paper/repository is especially relevant because it demonstrates:

> implicit fields are absolutely capable of producing rocky cliffs when the field is structurally meaningful.

## Jerico design

Create a reusable concept:

```cpp
struct ImplicitBlockDefinition
{
    BlockArchetype archetype;

    Frame localFrame;
    Vec3 extent;

    FractureFamily families[MAX_FAMILIES];

    WarpParameters mesoWarp;
    Seed seed;
};
```

Possible archetypes:

```text
Tabular
Columnar
Equidimensional
Rhombohedral
IrregularPolyhedral
```

Do not hardcode final cliff presets.

The archetype describes a mathematical block organization.

---

# 21. CavityCarve

## Purpose

Remove real volume.

Do not fake large cavities by adding negative surface noise.

## Basic formulation

`[JERICO SYNTHESIS]`

Cavity field:

```text
C(p)
```

may be:

- sphere
- ellipsoid
- capsule
- warped capsule
- swept tube
- cellular region

Subtract using existing field CSG:

```text
result =
    subtract(source, C)
```

or smooth subtraction.

## Important distinction

Large cavities belong to macro/meso geometry.

Tiny pores belong to microstructure/material shading.

---

# 22. Directed crack volumes

## Research basis

`[SOURCE: R9]`

Desbenoit et al. do not generate thin cracks by indiscriminately voxel-noising a surface.

They:

```text
2D crack graph / pattern
    ↓
map onto surface
    ↓
attach profile curves
    ↓
construct swept 3D carving volume
    ↓
boolean subtract from object
```

## Jerico adaptation

`[IMPLEMENTED P1 OPERATOR; P1 AND BLUEPRINT REMAIN OPEN]`

The implemented `DirectedCrack` operator contains:

```text
graph
surface projection mode
orientation
branch probability
profile width
profile depth
profile curve
propagation parameter
```

This is useful for:

- hero assets
- explicit artist-directed cracks
- very thin fractures

It complements—not replaces—bulk fracture fields.

## Jerico implementation status — 2026-08-28

Jerico schema 17 kind 21 implements `DirectedCrack` using the canonical recipe
and `ProceduralSpline` storage, deterministic graph preparation, implicit-surface
planar projection, explicit width/depth profiles and a bounded swept subtraction
inside the resident Diligent/Vulkan field evaluator. The final product continues
through the existing classified Surface Nets, JRMS 1.1, exact collision and
source-free world/triplanar PBR path.

Research provenance, independent design, limits, RED chronology, CPU/GPU parity,
topology, cook/package and paired Beauty evidence are recorded in:

- `docs/evidence/M8_GEOLOGY_BLUEPRINT_P1_DIRECTED_CRACK_RESEARCH.md`;
- `docs/superpowers/specs/2026-08-28-m8-directed-crack-swept-volume-design.md`;
- `docs/evidence/M8_GEOLOGY_BLUEPRINT_P1_DIRECTED_CRACK_SWEPT_VOLUME_CLOSURE.md`.

No implementation from the research references was copied, translated or
ported. Durability graph and every subsequent P1/P2 item remain open.

---

# 23. HeightMask

```text
h =
    dot(p, normalize(axis))

m =
    smoothstep(minValue,
               maxValue,
               h)
```

Use to localize:

- erosion
- strata
- cavities
- fractures
- detail

---

# 24. DirectionMask

Given a point-derived direction or surface normal:

```text
m =
    saturate(
        dot(normalize(v),
            normalize(targetDirection))
    )
```

Can be remapped:

```text
m =
    pow(m, exponent)
```

Use to make operators directional.

---

# 25. Gradient and surface normal from field

For field `F(p)`, central finite differences:

```text
gx =
    F(p + ex*h)
    - F(p - ex*h)

gy =
    F(p + ey*h)
    - F(p - ey*h)

gz =
    F(p + ez*h)
    - F(p - ez*h)

g =
    vec3(gx, gy, gz) / (2*h)

n =
    normalize(g)
```

## Rules

`[JERICO SYNTHESIS / standard numerical method]`

- `h` must have an explicit relation to voxel size.
- Never normalize a near-zero vector without guarding it.
- CPU and GPU must use equivalent sampling convention.
- Document whether the normal points inward or outward under Jerico’s sign convention.

---

# 26. SlopeMask

Assuming `up`:

```text
c =
    abs(dot(n, normalize(up)))
```

One possible steepness definition:

```text
slope =
    1 - c
```

Then remap:

```text
mask =
    smoothstep(slopeMin,
               slopeMax,
               slope)
```

Use for:

- cliff erosion
- sediment
- detail
- fracture exposure

---

# 27. CurvatureWeathering

## Research basis

`[SOURCE: R5]`

The 2010 work explicitly connects:

- spheroidal weathering to high positive mean curvature,
- cavernous weathering to high negative mean curvature,
- and both to rock durability.

## Field approximation

`[JERICO SYNTHESIS / standard level-set relation]`

For a sufficiently regular implicit field, mean-curvature information can be estimated from:

```text
n =
    ∇F / |∇F|

kappa ≈ div(n)
```

Compute divergence by finite differences.

This is not necessarily the exact discrete formula used by R5.

It is a Jerico implementation direction consistent with the geometric principle.

## Weathering response

Example conceptual response:

```text
convex =
    max(kappa, 0)

concave =
    max(-kappa, 0)

rate =
    convexStrength * convex
    + concaveStrength * concave
```

Modulate by durability:

```text
rate *=
    1 - durability
```

and masks.

## Critical requirement

Do not run this on the whole world at max resolution every slider movement.

Use:

- preview quality,
- narrow active region,
- iteration limit,
- GPU ping-pong,
- cancellation.

---

# 28. BubbleWeathering / spheroidal local exposure

## Research basis

`[SOURCE: R4]`

The Goblins work models weathering on a voxel grid using local exposure.

Core geological idea:

- a broad flat surface has less exposed surrounding air,
- a sharp corner has more surrounding air,
- therefore corners weather faster.

## Jerico approximation

`[JERICO SYNTHESIS]`

For a surface voxel `v`, define a neighborhood kernel `B`.

Count:

```text
rockCount
airCount
```

or weighted equivalents.

Exposure:

```text
E =
    airWeight /
    max(airWeight + rockWeight, epsilon)
```

Weathering:

```text
rate =
    E
    * materialSusceptibility
    * strength
```

Then decrease occupancy/density/durability.

## Why this is valuable

It produces **shape-aware erosion** without solving a full PDE.

It is therefore a strong P0/P1 weathering candidate.

---

# 29. DurabilityField

## Research basis

`[SOURCE: R5]`

The directable-weathering work gives users control over material durability.

## Jerico design

Durability should be a first-class scalar field:

```text
D(p) ∈ [0,1]
```

where:

```text
0 = weak
1 = resistant
```

Possible inputs:

```text
constant
strata
height
noise
fracture proximity
material ID
painted author field
curve/graph
```

Weathering uses:

```text
weatheringRate *= 1 - D(p)
```

or another documented convention.

---

# 30. Rockfall and colluvium

## Research basis

`[SOURCE: R5]`

The directable-weathering paper includes rockfall and colluvium deposition to improve plausibility.

## Jerico status

`[ROCKFALL PASS / COLLUVIUM PASS; P1 AND BLUEPRINT REMAIN OPEN]`

The bounded authoring/cook Rockfall operator is implemented by schema 19 / kind
23 after basic weathering and Durability Graph. Its selected boundary is
explicitly offline/static:

```text
weathered resident scalar field + packed fracture + RBrush mask
-> six-face support graph against an authored gravity/support plane
-> canonical detached components and cubic-meter diagnostics
-> remove detached voxels + mark retained adjacent surfaces fresh
-> existing classified Surface Nets / JRMS / exact static collision
```

The productive seam performs one bounded GPU readback, global CPU topology
evaluation and GPU upload before the existing dirty/surface/cook path. The
field ceiling is 16,777,216 voxels. Durability influences the decision only
through the already ordered weathering result; no normalized scalar is treated
as force, density or mass.

The bounded authoring/cook Colluvium operator is implemented by schema 20 / kind
24 immediately after productive Rockfall. It retains the unchanged Rockfall
detached-component/voxel/volume lineage, performs deterministic supported
settling through the already allocated topology, honors existing strength and
RBrush semantics, and publishes deposited plus explicit unsettled identities
with exact cubic-meter conservation. It reuses the Rockfall CPU snapshot, so it
adds zero GPU readback bytes before the existing classified Surface Nets, JRMS
and exact static-collision path.

The admitted distance ceiling is 64 cells and the checked work ceiling is
67,108,864 visited nodes. The selected boundary is cardinal-gravity
offline/static settling, not fluid sediment, mass, granular dynamics, runtime
destruction or mutable save-state. Debris, Jolt break dynamics, runtime remesh,
mutable save-state and imported-mesh structural operators remain separate work
marked `INTEGRATION REQUIRED`.

Closure and exact evidence:

- `docs/evidence/M8_GEOLOGY_BLUEPRINT_P1_ROCKFALL_CLOSURE.md`;
- `docs/evidence/M8_GEOLOGY_BLUEPRINT_P1_COLLUVIUM_CLOSURE.md`.

---

# 31. Dorsey narrow-band weathering

## Research basis

`[SOURCE: R3]`

Dorsey et al. introduce a slab representation:

> a thin volume aligned with the surface boundary.

This is valuable because most visual weathering is localized near the surface.

## Jerico adaptation

`[PASS: BOUNDED_INTERIOR_MINERAL_RESPONSE_BAND]`

Instead of allocating a giant dense volume for an entire asset, the bounded
Jerico adaptation evaluates:

```text
surface
± narrow band
```

A future complete physical-weathering vertical could carry fields for:

```text
density
moisture
durability
mineral/material
dissolved material
```

This could become a high-quality final cook mode.

Do not make it P0.

The admitted schema-24 Jerico adaptation is classification-only: it consumes
the existing Strata mineral/material class inside one finite interior SDF band,
preserves the scalar field bit-exactly and writes a monotonic weathered-mineral
amount through the existing deposition-response material slot. It terminates
after one bounded Vulkan pass and retains canonical recipe identity, immutable
preview/diff/cancel and explicit designer choice. It does not implement
moisture, transport, dissolution, recrystallization or a second slab/field/
material system.

Research, independent design, RED chronology, exact contracts and Release
Vulkan/cook/package/Beauty evidence are retained in:

- `docs/evidence/M8_GEOLOGY_BLUEPRINT_P2_DORSEY_NARROW_BAND_MINERAL_WEATHERING_RESEARCH.md`;
- `docs/superpowers/specs/2026-08-29-m8-dorsey-narrow-band-mineral-weathering-design.md`;
- `docs/evidence/M8_GEOLOGY_BLUEPRINT_P2_DORSEY_NARROW_BAND_MINERAL_WEATHERING_CLOSURE.md`.

---

# 32. Full physical moisture/dissolution simulation is not P0

Dorsey’s work contains physically motivated flow/transport ideas.

Those are valuable references.

They are not required to stop Jerico producing potatoes.

The first visible gain will come from:

```text
cuts
fractures
anisotropy
bedding
curvature/exposure weathering
cavities
```

not from solving porous-medium equations.

The admitted schema-25 Jerico boundary is therefore deliberately smaller than a
physical simulator. `BOUNDED_NORMALIZED_MOISTURE_PRESSURE_PROFILE` evaluates one
dimensionless pressure-potential field in a finite inside-surface band. A
deterministic authored surface source decays to a fixed zero inner boundary over
the characteristic distance `sqrt(D * t)`, with no lateral flow, moving front,
history or numerical iterations. One checked Vulkan pass owns one float per
admitted voxel and a separate field hash while preserving scalar geometry,
classification, Strata, durability and the preceding Dorsey response exactly.
The field is authoring-only; cook records schema-25 recipe provenance but no
runtime moisture state.

Research, independent design, RED chronology, exact contracts and Release
CPU/Vulkan/cook evidence are retained in:

- `docs/evidence/M8_GEOLOGY_BLUEPRINT_P2_MOISTURE_TRANSPORT_RESEARCH.md`;
- `docs/superpowers/specs/2026-08-29-m8-bounded-moisture-transport-design.md`;
- `docs/evidence/M8_GEOLOGY_BLUEPRINT_P2_MOISTURE_TRANSPORT_CLOSURE.md`.

The admitted schema-26 Jerico boundary stops before transport and
recrystallization. `BOUNDED_LOCAL_MINERAL_DISSOLUTION_FRACTION` consumes the
existing schema-24 weathered-mineral susceptibility and the schema-25 normalized
moisture exposure, combines them with one explicit authored normalized mineral
inventory, and evaluates a closed-form local dissolved fraction. It performs one
finite pointwise Vulkan dispatch and zero numerical iterations. The moisture
field is inspected and hashed before the same auxiliary float buffer is
overwritten with the dissolved fraction, so the boundary adds no buffer,
descriptor or device-limit increase. Scalar geometry, classification, Strata,
durability, Surface Nets and collision remain exact.

The result is dimensionless authoring data and a bounded local partition only.
It is not mineral concentration, transported mass, global conservation,
recrystallization, erosion, topology change or runtime mutable state. The
canonical recipe, immutable preview, pure diff, cancellation without a published
candidate and explicit designer choice remain the acceptance boundary.

Primary-source research, independent design, RED chronology, exact contracts and
Release CPU/Vulkan/cook evidence are retained in:

- `docs/evidence/M8_GEOLOGY_BLUEPRINT_P2_MINERAL_DISSOLUTION_RESEARCH.md`;
- `docs/superpowers/specs/2026-08-29-m8-bounded-mineral-dissolution-design.md`;
- `docs/evidence/M8_GEOLOGY_BLUEPRINT_P2_MINERAL_DISSOLUTION_CLOSURE.md`.

---

# 33. SpeedRock as an optional alternate source

## Research basis

`[SOURCE: R6]`

SpeedRock:

1. starts with a cubic 3D grammar,
2. recursively subdivides a cube into 8 cuboids,
3. marks sub-cuboids solid or empty,
4. repeats the rewrite,
5. implodes solid blocks toward central planes,
6. optionally erodes based on neighbor count,
7. wraps/skins the final structure.

## Jerico interpretation

`[JERICO SYNTHESIS]`

The evolutionary search is not the most interesting component for us.

The useful concept is a structural source:

```text
BlockMassSource
```

that can generate:

- chunky rock masses
- blocky fracture masses
- porous coarse skeletons

Then feed them to Jerico’s normal operator stack.

## Priority

P2.

Do not block the main geology system on genetic algorithms.

---

# 34. Neighborhood erosion from SpeedRock

## Research basis

`[SOURCE: R6]`

SpeedRock uses a one-step cellular-automaton-like erosion.

Deletion probability depends on the number of neighbors and an overall erosion amount.

The paper gives examples such as:

```text
4 neighbors → 45%
3 neighbors → 65%
```

before multiplying by the user erosion amount.

## Jerico use

Do **not** copy those probabilities as universal geology constants.

Use the concept:

```text
lower support / fewer neighbors
→ more exposed
→ higher erosion probability
```

This is useful as a cheap preview erosion mode.

---

# 35. Noise graph architecture

## Research basis

`[SOURCE: R2]`

`proc-rock` has a dedicated noise graph rather than embedding all noise in each geometry generator.

Its graph includes families such as:

- Perlin
- ridged
- Voronoi/cellular
- turbulence
- coordinate transforms
- terrace
- curve
- scale/bias
- combinations

## Jerico recommendation

Create or consolidate:

```text
ScalarGraph / NoiseGraph
```

separate from:

```text
GeometryOperatorStack
```

A structural operator can reference a scalar graph.

Example:

```text
PlaneCut
    roughnessGraph = LowFrequencyRidgedGraph
```

or:

```text
Strata.warpGraph
```

This gives both:

- semantic authoring,
- low-level mathematical freedom.

---

# 36. Do not expose only “Perlin / FBM / Worley” to the artist

Low-level nodes are useful.

But the main Playground should also expose semantic structural operators:

```text
Plane Cut
Strata
Fault
Fracture Set
Anisotropic Cells
Cavity
Weathering
Durability
```

A user should not have to invent geology by manually wiring five noise nodes just to get a fault.

---

# 37. Distinguish value displacement from domain deformation

## Value displacement

```text
result =
    F(p)
    + amount * N(p)
```

This changes the scalar value.

## Domain deformation

```text
q =
    p + W(p)

result =
    F(q)
```

This changes where the field is sampled.

They are not equivalent.

The operator API and UI should name the behavior explicitly.

---

# 38. Gradient-based detail warping

## Research basis

`[SOURCE: R8]`

The Implicit Blocks work proposes gradient-based warping to add small-scale detail to implicit blocks while avoiding undesirable holes.

## Jerico direction

`[P1]`

Investigate the paper/repository implementation before writing a custom version.

Because the public `Rock-fracturing` repository is MIT, Codex may inspect its implementation carefully.

Still:

- adapt to Jerico’s coordinate/sign conventions,
- do not paste blindly,
- write Jerico tests,
- preserve CPU/GPU parity.

## Jerico implementation status — 2026-08-28

`[IMPLEMENTED P1 OPERATOR; P1 AND BLUEPRINT REMAIN OPEN]`

Jerico schema 16 now contains the typed `GradientDetailWarp` layer. Its
independent implementation uses a one-voxel central gradient over the existing
negative-inside input field, the existing seeded fBm with local triplanar
weights, an inward candidate sample and `min(source, candidate)` composition.
That composition preserves input occupancy and prevents detail-created holes in
the admitted parameter domain; it is not represented as exact redistanced SDF.

The productive proof composes the operator after the existing
`IrregularPolyhedral` `ImplicitBlock`, preserves its structural classification,
produces a closed classified Surface Nets mesh, cooks exact collision and reuses
the existing source-free geology PBR package/runtime path. Research provenance,
the no-copy boundary, validation limits, CPU/GPU measurements and paired Beauty
evidence are recorded in:

- `docs/evidence/M8_GEOLOGY_BLUEPRINT_P1_GRADIENT_DETAIL_WARP_RESEARCH.md`;
- `docs/superpowers/specs/2026-08-28-m8-gradient-detail-warp-design.md`;
- `docs/evidence/M8_GEOLOGY_BLUEPRINT_P1_GRADIENT_DETAIL_WARP_CLOSURE.md`.

No code, shader, API or implementation from `Rock-fracturing` was copied,
translated or ported. The repository remains an MIT research/provenance source,
not a Jerico runtime dependency.

---

# 39. Smooth booleans remain useful

Standard operators:

```text
union
intersection
subtraction
smooth union
smooth intersection
smooth subtraction
```

should remain first-class.

However, smoothness should be controlled.

Excessive smoothing destroys:

- planar fracture faces
- crisp bedding
- rock block identity

Use smoothing at deliberate scales.

---

# 40. Structural masks as first-class data

An operator should be able to emit reusable masks.

Examples:

```text
StrataMask
FractureMask
FaultSide
CellId
CellBoundary
HeightMask
SlopeMask
CurvatureMask
ExposureMask
DurabilityMask
CavityProximity
```

This is important because a rock becomes coherent when multiple processes react to the **same structure**.

Example:

```text
FractureMask
    ↓
lower durability
    ↓
higher weathering
    ↓
deeper crack
```

Instead of three unrelated noise functions.

---

# 41. Recommended data model

```cpp
enum class GeoDomain
{
    ScalarField,
    DiscreteVoxel,
    Mesh
};

enum class GeoScale
{
    Macro,
    Meso,
    Micro
};

enum class GeoStageKind
{
    Source,
    Transform,
    Structure,
    Carve,
    Weathering,
    Detail,
    Extract,
    MeshPost
};

struct GeoParameter
{
    ParameterId id;
    ParameterType type;

    Value defaultValue;
    Value minimum;
    Value maximum;
    Value step;

    Unit unit;
};

struct GeoOperatorDefinition
{
    OperatorId id;
    std::string_view displayName;

    GeoDomain domain;
    GeoScale scale;
    GeoStageKind kind;

    Span<const GeoParameter> parameters;

    OperatorCapabilities capabilities;
};
```

---

# 42. Recipe stage

```cpp
struct GeoStage
{
    OperatorId type;

    bool enabled = true;

    uint64_t seed = 0;

    ParameterBlock parameters;

    Optional<MaskReference> mask;
};
```

Recipe:

```cpp
struct GeoRecipe
{
    GeoSource source;

    std::vector<GeoStage> stages;

    GeoQualityProfile previewQuality;
    GeoQualityProfile finalQuality;
};
```

Order must be deterministic.

---

# 43. Stage execution

Conceptually:

```cpp
GeoResult EvaluateRecipe(
    const GeoRecipe& recipe,
    const GeoEvaluationContext& ctx)
{
    auto value = EvaluateSource(recipe.source, ctx);

    for (const GeoStage& stage : recipe.stages)
    {
        if (!stage.enabled)
            continue;

        value = EvaluateStage(value, stage, ctx);
    }

    return value;
}
```

Do not introduce:

```cpp
if (preset == "cool_rock")
```

inside the evaluator.

---

# 44. GPU operator representation

A practical GPU implementation may compile an operator stack into:

```text
header
stage descriptors
parameter buffer
seed buffer
mask references
```

Example:

```cpp
struct GpuGeoStage
{
    uint32_t opcode;
    uint32_t parameterOffset;
    uint32_t maskIndex;
    uint32_t flags;
};
```

Then the compute evaluator interprets or specializes the sequence.

Which approach is best depends on existing Jerico Vulkan architecture.

**Audit first.**

Do not build a new shader virtual machine if the existing recipe GPU path already has a suitable representation.

---

# 45. CPU reference architecture

Every P0 mathematical operator should have a small reference function.

Example:

```cpp
float EvalPlaneCutCpu(
    const GeoField& source,
    const Vec3& p,
    const PlaneCutParams& params);
```

The CPU version is the semantic reference.

It does not need production performance.

---

# 46. CPU/GPU parity

For deterministic scalar operators:

```text
same point
same source
same parameters
same seed
same coordinate convention
```

must produce values within the established Jerico tolerance.

Do not loosen existing tolerances to make tests green.

Find the mismatch.

---

# 47. Field convention audit

Before new math, Codex must report:

```text
inside sign
outside sign
surface convention
world unit
local/world transform order
matrix convention
frequency unit
rotation convention
seed hashing
voxel-center convention
voxel-corner convention
finite-difference step
```

This should become a short reusable `GeoMathConventions` documentation block in source.

---

# 48. Seed convention

Every stochastic operator must accept an explicit deterministic seed.

Avoid:

```cpp
std::random_device
```

inside recipe evaluation.

Prefer deterministic hash-based sampling.

Requirements:

```text
same seed → same samples
different seed → meaningfully different samples
stage reorder behavior documented
seed hashing independent from pointer/address
```

---

# 49. Coordinate-space rule

Every operator must declare whether it evaluates in:

```text
source local space
recipe local space
world space
```

Default recommendation:

```text
procedural structural math
→ recipe/object local space

placement/orientation
→ transform outside evaluator
```

unless an operator explicitly needs world direction such as gravity.

---

# 50. Gravity / geological up

Do not silently assume Y-up inside every formula.

Expose a shared context:

```cpp
Vec3 upDirection;
Vec3 gravityDirection;
```

This will matter for:

- deposition
- slope
- vertical bedding
- rockfall
- erosion

---

# 51. Weathering iteration model

For iterative discrete weathering:

```text
state A
    ↓ compute
state B
    ↓ swap
state A
```

Use ping-pong buffers.

Never update neighbor-dependent weathering in-place unless the algorithm is explicitly designed for order dependence.

---

# 52. Weathering state

Possible P0/P1 voxel state:

```cpp
struct WeatherVoxel
{
    float density;
    float durability;
    float exposure;
    float fractureWeakness;
};
```

Do not add mineral chemistry yet.

---

# 53. Live preview behavior

The user should be able to drag parameters.

Therefore:

```text
parameter changes
    ↓
debounce
    ↓
cancel obsolete request
    ↓
evaluate newest recipe
    ↓
extract preview mesh
```

Recommended behavior:

- coarse preview voxel size
- limited weathering iterations
- optional partial stage cache
- cancellation token
- do not queue hundreds of cooks

---

# 54. Stage caching

Potential optimization:

```text
Stage 0 result hash
Stage 1 result hash
Stage 2 result hash
...
```

If only stage 6 changes, stages 0–5 may be reusable if existing infrastructure makes this safe.

Do **not** build complex caching before profiling.

---

# 55. Debugging visualizations are mandatory

This is extremely important.

A bad final mesh is too late to understand what failed.

The Playground should eventually support:

```text
final mesh
field slice X
field slice Y
field slice Z
iso-contour
gradient
normal
strata mask
fracture mask
cell IDs
cell boundary
fault selector
height mask
slope mask
curvature
durability
weathering rate
```

At minimum implement a way to inspect intermediate stage outputs.

---

# 56. Stage solo / bypass

Each stage:

```text
Enabled
Solo
Bypass
```

would be valuable.

Minimum requirement:

```text
enable/disable
```

The user must be able to answer:

> “Which operator turned this into a potato?”

---

# 57. Intermediate stage preview

Provide:

```text
Preview after stage N
```

This is more important than a polished editor.

Example:

```text
Source: Box
[1] Directional Warp
[2] Plane Cut
[3] Strata
[4] Fracture
[5] Weathering
[6] Detail
```

Selecting stage 3 can show the result before weathering/detail.

---

# 58. Quality profiles

Example conceptual profiles:

```text
Draft
    coarse voxel
    low weather iterations
    lower meshing resolution

Interactive
    medium voxel
    medium iterations

Final
    user-selected high resolution
```

Use the existing Jerico quality abstraction if present.

Do not hardcode these names if an equivalent already exists.

---

# 59. The “potato detector”

We need more than compilation tests.

Add qualitative/structural acceptance criteria.

A recipe fails the structural goal if:

- dominant silhouette remains nearly spherical for unrelated recipes,
- large noise amplitude is required for recognizable shape,
- fracture operator only looks like bumps,
- strata are unreadable,
- changing fault displacement does not produce coherent offset,
- cell anisotropy does not change block proportions,
- weathering does not preferentially affect exposed/curved areas.

---

# 60. Optional quantitative structural metrics

`[JERICO SYNTHESIS]`

These are optional diagnostics, not strict geology truth.

## Bounding-box anisotropy

```text
A =
    max(extentXYZ)
    / max(min(extentXYZ), epsilon)
```

## Surface normal distribution

Histogram normals.

A planar fractured rock should show stronger directional peaks than a smooth noise sphere.

## Planarity sample

Fit local planes to sampled surface neighborhoods and report residual.

## Curvature distribution

Report percentiles.

These diagnostics can help identify when everything is over-smoothed.

---

# 61. Proof recipe A — Sedimentary cliff

Purpose:

Demonstrate directional structure.

```text
Source: Box / slab

1. Anisotropic Scale
2. Plane Cut
3. Strata
4. Low-frequency Strata Warp
5. Secondary Plane Cut
6. FaultDisplace
7. FractureSet
8. CavityCarve
9. Curvature/Bubble Weathering
10. RidgedDetail
11. Fine FBM
```

Success:

- dominant cliff face exists,
- bedding is obvious,
- fault offsets bedding coherently,
- fractures are structural,
- cavities remove volume,
- fine noise does not dominate silhouette.

---

# 62. Proof recipe B — Fractured boulder

```text
Source: Box

1. Anisotropic Scale
2. DirectionalWarp
3. PlaneCut
4. PlaneCut
5. AnisotropicVoronoi
6. FractureCells
7. CavityCarve
8. BubbleWeathering
9. RidgedDetail
10. FBMDetail
```

Success:

- clear broken planes,
- non-spherical silhouette,
- block logic visible,
- corners can weather,
- micro detail stays subordinate.

---

# 63. Proof recipe C — Folded sediment block

```text
Source: Slab / Box

1. StrataField
2. FoldDomain
3. FaultDisplace
4. PlaneCut
5. FractureSet
6. DurabilityFromStrata
7. Weathering
8. Detail
```

This recipe requires a future `FoldDomain` if not in P0.

It may be P1.

---

# 64. Proof recipe D — Columnar cliff

Strongly inspired by `Implicit Blocks`.

```text
Source: cliff volume

1. JointFamily A
2. JointFamily B
3. JointFamily C / polygonal lateral structure
4. ImplicitBlock construction
5. block-scale warp
6. local cavity/weathering
7. meso detail
```

Success:

- repeated structure is geological, not obvious texture tiling,
- columns retain dominant directional organization,
- detail does not punch random holes through every block.

---

# 65. Proof recipe E — Weathered rounded boulder

This proves Jerico can still make rounded rock intentionally.

```text
Source: Icosahedral-ish / sphere / box

1. AnisotropicScale
2. large low-frequency directional deformation
3. BubbleWeathering
4. CurvatureWeathering
5. Cavity/Pit sparse mask
6. fine surface detail
```

The difference is that roundness is now the **chosen process outcome**, not the unavoidable default.

---

# 66. FoldDomain

`[P1 JERICO SYNTHESIS]`

Let:

```text
axis = normalize(foldAxis)

u =
    dot(p, axis)

offset =
    amplitude
    * sin(frequency * u + phase)
```

Displace along direction `d`:

```text
q =
    p
    + normalize(d) * offset

result =
    source(q)
```

Use a spatial envelope to localize the fold.

Do not use random 3D displacement as a substitute for folding.

---

# 67. Terrace

Useful for:

- stepped weathering
- sedimentary profiles
- cliff shelves

Apply to a scalar coordinate or mask, not automatically to final SDF.

Example:

```text
x =
    input / stepSize

terraced =
    floor(x)
```

with smooth transition option.

---

# 68. Remap / Bias / Gain / Curve

These are support math.

They should be available in scalar graphs.

They are not geology by themselves.

Example usage:

```text
curvature
→ remap
→ gain
→ weathering strength
```

---

# 69. Material-aware geometry

A valuable future design is to let structural fields influence both geometry and material.

Example:

```text
StrataField
    ├── geometry displacement
    ├── color/material layer
    ├── durability
    └── roughness
```

This produces coherence between shape and appearance.

Do not make P0 dependent on a new material system.

---

# 70. Contact-preserving erosion

## Research basis

`[SOURCE: R7]`

The rock-pile method generates contact points and reduces erosion near those contacts so the rocks remain in contact.

## Jerico use

`[DEFERRED]`

This becomes relevant for:

- rock piles
- stacked blocks
- rubble
- dry-stone walls

Not necessary for isolated rock P0.

---

# 71. Aperiodic tiling

## Research basis

`[SOURCE: R7]`

The purpose is to avoid visible periodic repetition while maintaining compatible tile boundaries.

## Jerico status

`[DEFERRED]`

Useful later for:

- vast cliffs
- rubble fields
- rock piles
- repeated procedural walls

Do not build it before one convincing local rock/cliff works.

---

# 72. Why Surface Nets should not be rewritten yet

Surface Nets receives an implicit/scalar field.

If input:

```text
F(p)
```

describes a blob, the mesh should describe a blob.

Before blaming Surface Nets:

1. inspect scalar slices,
2. inspect zero contour,
3. compare CPU field samples,
4. compare GPU field samples,
5. verify topology around zero crossings,
6. only then inspect extraction.

Do not rewrite meshing merely because the geology is aesthetically bad.

---

# 73. Surface Nets validation

Still test:

```text
simple sphere
simple box
plane cut box
union
subtraction
thin cavity
fracture gap
```

If these field shapes look correct in slices but mesh incorrectly, then the mesher is legitimately suspect.

---

# 74. Avoid SDF purity dogma

Not every temporary intermediate has to be a mathematically exact signed distance.

After arbitrary operations:

```text
F(p)
```

may be better described as a scalar implicit field.

This is acceptable if:

- zero level set is correct,
- gradients are handled appropriately,
- operations document assumptions,
- mesher expects a scalar field rather than exact metric distance.

Do not falsely call every arbitrary function a true SDF.

---

# 75. Re-distance only when needed

If some operation needs accurate distance magnitude, consider:

- analytic exact source,
- local re-distance,
- distance transform,
- approximation.

Do not automatically add a costly full re-distance pass after every operator.

Profile and document need.

---

# 76. Mathematical units

Prefer parameters with physical meaning.

Examples:

```text
fracture width → meters
cavity radius → meters
strata spacing → meters
fault slip → meters
warp wavelength → meters
weathering radius → meters
```

Rather than arbitrary “frequency 723.4” wherever possible.

---

# 77. Frequency convention

Recommended:

```text
wavelength in meters
```

or clearly define:

```text
frequency = cycles / meter
```

Then:

```text
phase =
    2π * frequency * coordinate
```

Do not mix “noise scale” and “cycles per meter” without naming them.

---

# 78. Parameter UI metadata

Example:

```cpp
ParameterSchema strataSpacing {
    .name = "Spacing",
    .unit = Unit::Meters,
    .defaultValue = 0.35f,
    .minimum = 0.01f,
    .maximum = 20.0f,
    .step = 0.01f
};
```

The exact ranges depend on Jerico scale.

Do not blindly use the example values.

---

# 79. Operator categories for Playground

Recommended UI:

```text
Sources
    Box
    Sphere
    Cylinder
    Plane

Macro
    Scale
    Directional Warp
    Bend
    Plane Cut
    Cavity

Structure
    Strata
    Fault
    Joint Set
    Anisotropic Cells
    Fracture Cells
    Implicit Blocks

Weathering
    Bubble Weathering
    Curvature Weathering
    Durability
    Slope Wear

Detail
    FBM
    Ridged
    Cellular
    Turbulence

Utility
    Mask
    Remap
    Curve
    Clamp
    Boolean
```

---

# 80. Noise graph UI

Noise graph can remain advanced.

Example:

```text
Perlin
    ↓
Ridged
    ↓
Curve
    ↓
ScaleBias
```

Then assign it to:

```text
PlaneCut.roughnessGraph
```

or:

```text
Strata.warpGraph
```

This gives both:

- semantic authoring,
- low-level mathematical freedom.

---

# 81. Recipe persistence

Use existing recipe/asset serialization if present.

Do not invent a second unrelated format.

A stage should serialize:

```json
{
  "type": "FaultDisplace",
  "enabled": true,
  "seed": 4102,
  "parameters": {
    "planeOffset": 0.0,
    "width": 0.15,
    "displacement": 0.7
  }
}
```

Direction values omitted here for brevity.

---

# 82. Determinism

Recipe identity includes:

```text
source
source transform
operator types
operator order
parameters
seeds
quality
coordinate convention version
```

Hash these into a reproducibility/debug ID if useful.

---

# 83. Versioning

Procedural math changes can break saved assets.

Consider:

```text
recipeVersion
operatorVersion
```

for operators whose semantics may evolve.

Do not overbuild migrations now, but avoid silent unversioned semantic changes.

---

# 84. Cancellation

GPU cooking:

```text
request N
request N+1
```

If N becomes obsolete, do not force the user to wait for it before seeing N+1 if the existing system can cancel safely.

Reuse current geology-session cancellation facilities.

---

# 85. Profiling

Report separately:

```text
CPU recipe planning
field GPU evaluation
weathering iterations
readback
Surface Nets
mesh upload
total preview latency
```

Avoid attributing all latency to “GPU geology.”

---

# 86. Intermediate readback policy

Prefer:

```text
GPU field
→ GPU field
→ GPU extraction
```

where existing architecture supports it.

Read back only when needed for:

- diagnostics,
- tests,
- final CPU mesh path,
- explicit debug inspection.

Do not introduce repeated hidden field readbacks between stages.

---

# 87. P0 implementation order

## Step 1 — Repository audit

Before coding anything:

Search for:

```text
SDF primitives
field representation
noise
Perlin
Simplex
Worley/cellular
FBM
ridged
domain warp
CSG
smooth CSG
recipe
layer
operator
Vulkan geology
GPU session
CPU reference
Surface Nets
editor inspector
property metadata
serialization
cancellation
quality profile
```

Produce an inventory.

Classification:

```text
Implemented and usable
Implemented but hidden
Partial
Missing
Duplicate
```

---

## Step 2 — Freeze conventions

Document:

```text
sign
units
coordinates
transforms
seed
finite differences
quality
```

Add small tests.

---

## Step 3 — Debug field visualization

Before complicated geology:

Implement enough inspection to see:

```text
field slice
zero contour
stage output
```

Without this, Codex will continue guessing from final meshes.

---

## Step 4 — PlaneCut

CPU.

Tests.

GPU.

Parity.

Expose to Playground.

---

## Step 5 — DirectionalWarp

CPU.

Tests.

GPU.

Parity.

Expose.

---

## Step 6 — StrataField

Do not start with visual displacement only.

Create a reusable scalar/mask result.

Expose spacing/orientation/warp.

---

## Step 7 — FaultDisplace

Verify layer offset visually using strata.

This is an important proof because the structure should move coherently.

---

## Step 8 — AnisotropicVoronoi

Implement:

```text
cell IDs
nearest metric
second metric
boundary metric
```

Prefer reusable structural output.

---

## Step 9 — FractureCells

Carve or weaken boundaries.

Do not over-smooth.

---

## Step 10 — CavityCarve

Use explicit primitive/swept cavity geometry.

---

## Step 11 — BubbleWeathering

First iterative erosion model.

GPU if architecture supports it cleanly.

Start with:

```text
exposure
durability
iterations
```

---

## Step 12 — CurvatureWeathering

Only after gradients/finite differences are validated.

---

## Step 13 — Detail

Now add:

```text
Ridged
FBM
small domain warp
```

and verify that disabling detail leaves a recognizable geological structure.

---

## Step 14 — Proof recipes

Build all required proof compositions.

---

# 88. P1 implementation order

After P0 succeeds:

```text
ImplicitBlock archetypes
JointSet
FoldDomain
gradient-based implicit detail warp
DirectedCrack swept volume
durability graph
rockfall [PASS: bounded offline/static field operator]
colluvium [PASS: bounded offline/static field operator]
imported mesh structural operators [PASS: bounded direct-mesh offline/static operator]
```

The ordered P1 operator list is closed. This does not close the P2/research
features, the complete Geology blueprint, the later PWAS/EPLS production
expansion or JericoEngine 1.0.

---

# 89. P2 / research features

```text
SpeedRock-like block grammar source
evolution/search
aperiodic tiling
contact-preserving erosion
Dorsey narrow-band mineral weathering [PASS: bounded interior mineral response band]
moisture transport [PASS: bounded normalized moisture pressure profile]
mineral dissolution [PASS: bounded local dissolved-mineral fraction]
multi-scale microstructure synthesis [PASS: bounded exactly-two-scale pore synthesis]
```

All eight ordered P2/research entries are closed under their individual bounded
evidence. The repeated complete-blueprint exit audit remains required before the
later production expansion may begin. This does not complete JericoEngine 1.0.

---

# 90. Tests — scalar basics

For every scalar operator:

```text
finite output
determinism
supported parameter bounds
identity condition
seed variation
```

---

# 91. Tests — PlaneCut

Representative points:

```text
retained half-space
removed half-space
on plane
near plane
```

Roughness zero must match analytic plane.

---

# 92. Tests — DirectionalWarp

```text
strength = 0 → exact source
direction normalization invariant
same seed deterministic
warping affects expected coordinate direction
```

---

# 93. Tests — Strata

```text
known points separated by one layer spacing
axis rotation
phase shift
warp strength = 0
mask remains bounded
```

---

# 94. Tests — Fault

Use a trivial source where translation is analytically visible.

```text
displacement = 0 → identity
positive side displaced
negative side unchanged
transition finite
```

---

# 95. Tests — AnisotropicVoronoi

Use fixed seed locations.

Test:

```text
cell ownership
anisotropy axis effect
scale parameter
orientation
determinism
nearest <= second-nearest
boundary metric >= expected bound
```

---

# 96. Tests — FractureCells

```text
cell centers → low crack mask
known boundary → high crack mask
width increase → wider fracture
softness finite
```

---

# 97. Tests — Weathering

Small synthetic grids.

Examples:

## Flat plane

Central flat region should have lower exposure than an isolated corner under the chosen kernel.

## Convex corner

Should weather faster under spheroidal mode.

## Concave pocket

Should react more strongly under cavernous/negative-curvature mode when enabled.

## Durability

Higher durability must reduce removal.

---

# 98. GPU tests

Preserve/extend:

```text
deterministic field hash
workload guard
cancellation
device memory bounds
CPU/GPU sample parity
no unexpected readback
```

---

# 99. Mesh tests

After extraction:

```text
finite vertices
finite normals
no invalid indices
expected topology for simple tests
triangle count nonzero
bounds plausible
```

If watertightness is an existing invariant, preserve it.

---

# 100. Visual regression

For a small set of deterministic proof recipes:

Save:

```text
recipe
field hash
mesh statistics
optional reference screenshot
```

Do not use screenshot equality as the only test.

---

# 101. Required P0 proof: structure survives detail removal

For each proof recipe:

1. render/evaluate full recipe,
2. disable all micro detail,
3. rebuild.

If the object stops looking geologically structured after micro detail is disabled, the macro/meso architecture is still wrong.

This is one of the strongest “not a potato” tests.

---

# 102. Required P0 proof: same source, radically different structures

Use the exact same source box.

Recipe A:

```text
Strata
Fault
PlaneCut
```

Recipe B:

```text
AnisotropicVoronoi
FractureCells
Cavity
```

Recipe C:

```text
DirectionalWarp
PlaneCuts
Weathering
```

The three results should be structurally distinct.

No custom generators.

---

# 103. Research conclusion: strongest references by problem

## Authoring architecture

Best reference:

```text
proc-rock
```

Take:

- ordered pipeline
- interactive modifiers
- separate noise graph
- reproducible recipes

Do not copy GPL source.

---

## Implicit cliffs / block geology

Best reference:

```text
Modeling Rocky Scenery using Implicit Blocks
```

Take:

- fracture-driven blocks
- implicit representation
- block archetypes
- strata-aware structure
- gradient-aware detail

Public companion code is MIT.

---

## Rock-cell shape

Best reference:

```text
Procedural Generation of Rock Piles using Aperiodic Tiling
```

Take:

- anisotropic Voronoi metric
- oriented local frames
- cell size control
- voxel ownership
- erosion after structural cell generation

---

## Shape-aware erosion

Best references:

```text
Goblins by Spheroidal Weathering
Directable Weathering of Concave Rock Using Curvature Estimation
```

Take:

- exposure / curvature
- durability
- iterative weathering
- concave vs convex behavior

---

## Directed cracks

Best reference:

```text
Modeling Cracks and Fractures
```

Take:

- crack graph
- profile curves
- swept carving volume
- artist control

---

## Physically richer weathering

Best reference:

```text
Modeling and Rendering of Weathered Stone
```

Take later:

- narrow-band representation
- material fields
- moisture/mineral transport concept

---

## Alternate coarse mass source

Reference:

```text
SpeedRock
```

Take later:

- grammar-generated block mass
- implosion
- neighbor erosion

Do not prioritize its genetic algorithm.

---

# 104. Licensing rules

## proc-rock

License:

```text
GPL-3.0
```

Therefore:

- architecture study: yes
- mathematical idea study: yes
- independent Jerico implementation: yes, subject to normal legal review
- copying source into a non-GPL Jerico codebase: **no**

---

## Rock-fracturing / Implicit Blocks companion repository

License:

```text
MIT
```

This repository can be inspected more directly.

Still:

- retain license notices where required,
- prefer adaptation to Jerico conventions,
- test independently.

---

## Papers

Academic publications describe algorithms and mathematics.

The paper text/figures remain copyrighted.

Do not copy long text or figures into Jerico documentation.

Implement concepts/equations independently and cite the source.

---

# 105. What Codex must not do

Do not:

- reintroduce BSP → VDB
- create a second toy geology system
- replace Surface Nets without evidence
- add five more noise octaves as the solution
- search random seeds until one looks good
- use huge isotropic FBM for silhouette
- call generic Worley displacement a fracture
- call Y-axis sine a full strata system
- hide arbitrary magic constants in shaders
- create `GenerateRock()`
- create `GenerateCliff()`
- create `GenerateCoolRock2()`
- hardcode the proof recipes as unique C++ algorithms
- make every operator a new bespoke UI panel
- invent unrelated CPU/GPU formulas
- weaken reference tolerances
- declare success because code compiles
- declare success because Surface Nets outputs triangles
- declare success because one screenshot looks acceptable
- spend P0 implementing full water/mineral physics
- spend P0 implementing retopology from scratch
- spend P0 polishing UI

---

# 106. What Codex must do before implementation

Report:

```text
A. Existing field/SDF architecture
B. Existing noise math
C. Existing procedural recipe representation
D. Existing GPU geology evaluator
E. Existing CPU reference math
F. Existing Surface Nets path
G. Existing editor property system
H. Existing diagnostics
I. Existing cancellation/debounce
J. Existing quality profiles
```

Then map this document onto existing types.

Do not create parallel abstractions when existing Jerico types are suitable.

---

# 107. Suggested code organization

This is conceptual.

Adapt to repository reality.

```text
src/geology/math/
    GeoMathConventions.*
    Noise.*
    ScalarRemap.*
    FieldGradient.*

src/geology/operators/
    PlaneCut.*
    DirectionalWarp.*
    Strata.*
    FaultDisplace.*
    AnisotropicVoronoi.*
    FractureCells.*
    CavityCarve.*
    Masks.*
    Weathering.*

src/geology/gpu/
    existing GPU evaluator extensions

src/geology/reference/
    CPU operator references

src/editor/geology/
    existing Playground integration

tests/geology/
    operator tests
    gpu parity
    recipes
```

Do not create these exact directories if repository conventions differ.

---

# 108. Suggested operator metadata

```cpp
struct GeoOperatorUiMetadata
{
    std::string_view name;
    std::string_view category;
    std::string_view description;

    GeoScale scale;
    GeoDomain domain;

    Span<const GeoParameterUi> params;
};
```

Parameter UI can be generated from metadata.

---

# 109. Suggested diagnostics per stage

```text
operator
domain
time
temporary bytes
active voxels
changed field hash
```

Final:

```text
GPU ms
CPU planning ms
Surface Nets ms
vertices
triangles
device bytes
field hash
```

---

# 110. Field hash

Use existing deterministic field hashing.

Potential use:

```text
Stage 0 hash
Stage 1 hash
...
Final hash
```

This helps prove:

- order changes matter,
- disable/enable matters,
- seeds matter,
- CPU/GPU mismatch location.

Do this only if existing hashing can support it cheaply.

---

# 111. Failure diagnosis tree

## Final mesh looks like a sphere/blob

Check:

```text
1. source
2. macro stages
3. structure stages
4. field slice
5. zero contour
```

If macro field is already blob:

> not a Surface Nets problem.

---

## Fracture looks like noise

Check:

```text
cell IDs
boundary metric
fracture width
anisotropy
orientation
```

Do not add more noise.

---

## Strata looks like horizontal texture

Check:

```text
does it affect volume?
is layer axis explicit?
does fault move it coherently?
is it used as structural field?
```

---

## Weathering looks random

Check:

```text
exposure
curvature
durability
iterations
kernel scale
```

Do not substitute FBM.

---

## Mesh has holes but field slices look correct

Now investigate:

```text
Surface Nets
cell sign changes
boundary handling
degeneracy
precision
```

---

# 112. First manual experiment after P0

```text
1. Open Geology Playground.
2. Spawn Box.
3. Stretch it.
4. Add PlaneCut.
5. Add Strata.
6. Rotate strata axis.
7. Add FaultDisplace.
8. Move the fault plane.
9. Verify strata offset coherently.
10. Add AnisotropicVoronoi.
11. Visualize cell IDs.
12. Add FractureCells.
13. Visualize fracture mask.
14. Add CavityCarve.
15. Add BubbleWeathering.
16. Increase iterations.
17. Add Ridged detail.
18. Disable Ridged.
19. Confirm the geology is still readable.
20. Reorder Fault and Strata.
21. Confirm evaluation order changes result.
22. Change seed.
23. Confirm deterministic variation.
24. Increase final quality.
25. Extract mesh.
26. Record diagnostics.
```

---

# 113. Definition of done for the new math core

This task is complete only when:

- at least four simple sources work,
- structural operator stack is data-driven,
- PlaneCut works,
- DirectionalWarp works,
- Strata works,
- FaultDisplace works,
- AnisotropicVoronoi works,
- FractureCells works,
- CavityCarve works,
- at least one shape-aware weathering method works,
- masks are inspectable,
- intermediate stage debugging works,
- deterministic seed behavior works,
- CPU/GPU tests pass where applicable,
- Surface Nets receives the actual structural field,
- proof recipes are built without custom generators,
- disabling micro noise still leaves meaningful geology,
- manual authoring works without editing C++.

---

# 114. Definition of failure

The task is **not** complete if the report says:

```text
Implemented Perlin.
Implemented FBM.
Implemented Worley.
Implemented Ridged.
Implemented Surface Nets.
Generated mesh successfully.
```

That only proves infrastructure.

It does not prove geology.

---

# 115. Mandatory final report from Codex

## Existing systems reused

Exact:

```text
files
classes
functions
GPU kernels
editor systems
```

---

## Conventions

Document:

```text
sign
coordinates
units
seed
frequency
finite differences
```

---

## Operators

For every implemented operator:

```text
semantic purpose
math
parameters
CPU path
GPU path
tests
```

---

## Research mapping

State which source influenced each operator.

Example:

```text
AnisotropicVoronoi
    inspired by Peytavie et al. 2009

BubbleWeathering
    inspired by Beardall et al. 2007

ImplicitBlock architecture
    inspired by Paris et al. 2020
```

Do not claim identical reproduction unless it truly is.

---

## Tests

Exact command lines and results.

---

## Proof recipes

Exact stage list.

Exact relevant parameters.

Field hash.

---

## Performance

For each representative proof:

```text
preview voxel size
active voxels
CPU planning ms
GPU field ms
weathering ms
meshing ms
required device bytes
vertices
triangles
field hash
```

---

## Visual/structural validation

Show:

```text
full result
without micro detail
selected intermediate masks
```

---

## Limitations

Truthfully list:

- missing operators
- approximate math
- unsupported scale
- performance issues
- known GPU differences
- missing persistence
- missing editor controls

---

# 116. The research-backed architectural conclusion

The reviewed work does **not** point to one universal rock algorithm.

It points to a toolbox.

Different problems benefit from different mathematics.

```text
rounded eroded boulder
    → exposure/curvature weathering

fractured cliff
    → fracture distributions + implicit blocks

rock cells
    → anisotropic Voronoi

directed hero crack
    → crack graph + swept carving volume

coarse chunky mass
    → block grammar / implosion

physically aged stone
    → narrow-band material/weathering model
```

Jerico should therefore become an **authoring framework for these operators**, not a single rock generator.

---

# 117. Recommended Jerico architecture in one diagram

```text
                        ┌─────────────────────┐
                        │   SOURCE GEOMETRY   │
                        │ Box/Sphere/Plane... │
                        └──────────┬──────────┘
                                   │
                                   ▼
                        ┌─────────────────────┐
                        │    MACRO SHAPING    │
                        │ cut/warp/scale/etc. │
                        └──────────┬──────────┘
                                   │
                                   ▼
                     ┌───────────────────────────┐
                     │   GEOLOGICAL STRUCTURE    │
                     │ strata/fault/joints/cells │
                     │ implicit blocks/fractures │
                     └─────────────┬─────────────┘
                                   │
                                   ▼
                     ┌───────────────────────────┐
                     │   CARVING / WEATHERING    │
                     │ cavities/exposure/curv.   │
                     │ durability                │
                     └─────────────┬─────────────┘
                                   │
                                   ▼
                     ┌───────────────────────────┐
                     │     CONTROLLED DETAIL     │
                     │ FBM/ridged/cellular       │
                     └─────────────┬─────────────┘
                                   │
                                   ▼
                     ┌───────────────────────────┐
                     │    GPU SCALAR / VOXEL     │
                     │          FIELD            │
                     └─────────────┬─────────────┘
                                   │
                                   ▼
                     ┌───────────────────────────┐
                     │       SURFACE NETS        │
                     └─────────────┬─────────────┘
                                   │
                                   ▼
                     ┌───────────────────────────┐
                     │           MESH            │
                     │ cleanup / LOD / asset     │
                     └───────────────────────────┘
```

Some operators may execute in mesh space or discrete voxel space.

The recipe system should make the domain explicit.

---

# 118. The most important engineering instruction

> **Stop asking Codex to decide what a rock should look like.**

Codex should implement:

- explicit structural operators,
- research-backed algorithms,
- deterministic math,
- tests,
- CPU/GPU parity,
- authoring controls.

The artist/user should choose:

- layer orientation,
- fault displacement,
- fracture style,
- weathering amount,
- cell anisotropy,
- cavity placement,
- detail.

---

# 119. Immediate Codex task statement

Read this entire document first.

Then:

1. Audit the existing Jerico repository.
2. Do not touch BSP → VDB.
3. Do not create new presets.
4. Do not rewrite Surface Nets.
5. Identify reusable existing field/noise/recipe/UI/GPU code.
6. Freeze coordinate/sign/seed conventions.
7. Add intermediate field/mask inspection.
8. Implement the P0 structural operators in the specified order.
9. Use CPU reference implementations.
10. Add numerical tests before/with GPU ports.
11. Preserve CPU/GPU parity.
12. Build the proof recipes as normal data-driven stacks.
13. Demonstrate the structures with micro detail disabled.
14. Only after this is working, consider P1/P2 research features.

---

# 120. Final principle

The correct separation is:

```text
SDF / scalar field
    = representation

fracture / bedding / faults / blocks
    = geological structure

erosion / weathering / carving
    = geological process

noise
    = controlled irregularity

GPU
    = execution

Surface Nets
    = extraction

recipe/playground
    = authoring instrument
```

The missing piece was not “more noise.”

The missing piece was:

> **structured mathematical meaning between the source and the mesh.**

Build that first.

---

# Appendix A — Research source details

## A.1 Procedural Modeling of Rocks

Anders Söderlund.  
Uppsala University, 2015.  
Supplied directly with this task as `FULLTEXT01.pdf`.

Useful sections:

```text
Related work
Sphere inflation
Feature edges
Corner chipping
Results/discussion
Bibliography
```

---

## A.2 proc-rock

Faruk Acibal / contributors.

Repository:

https://github.com/acfaruk/proc-rock

License:

GPL-3.0

Key source areas to inspect conceptually:

```text
pipeline
cut-plane modifier
normal displacement modifier
skin-surface generator
noise graph
recipe/json serialization
```

---

## A.3 Modeling and Rendering of Weathered Stone

Julie Dorsey, Alan Edelman, Henrik Wann Jensen, Justin Legakis, Hans Køhling Pedersen.

SIGGRAPH 1999.

PDF:

https://www.cs.jhu.edu/~misha/ReadingSeminar/Papers/Dorsey99.pdf

Use for:

```text
surface-local volumetric modeling
weathering
material transport
future narrow-band system
```

---

## A.4 Goblins by Spheroidal Weathering

Matthew Beardall et al.

Eurographics Workshop on Natural Phenomena, 2007.

PDF:

https://faculty.cs.byu.edu/~jones/cgnp/papers/goblins.pdf

Use for:

```text
voxel weathering
exposure
material resistance
shape-aware erosion
```

---

## A.5 Directable Weathering of Concave Rock Using Curvature Estimation

Michael D. Jones, McKay Farley, Joseph Butler, Matthew Beardall.

IEEE Transactions on Visualization and Computer Graphics, 2010.

DOI:

https://doi.org/10.1109/TVCG.2009.39

Use for:

```text
curvature weathering
durability
concave/cavernous erosion
rockfall
colluvium
```

---

## A.6 SpeedRock

Isaac M. Dart, Gabriele De Rossi, Julian Togelius.

2011.

PDF:

https://pcgworkshop.com/archive/dart2011speedrock.pdf

Use for:

```text
coarse structural mass
3D block grammar
implosion
simple neighbor erosion
```

---

## A.7 Procedural Generation of Rock Piles Using Aperiodic Tiling

Adrien Peytavie, Eric Galin, Jérôme Grosjean, Stéphane Mérillou.

Computer Graphics Forum 28(7), 2009.

DOI:

https://doi.org/10.1111/j.1467-8659.2009.01557.x

Use for:

```text
anisotropic Voronoi
rock-cell orientation
discrete voxel cells
contact-aware erosion
aperiodic tiling
```

---

## A.8 Modeling Rocky Scenery using Implicit Blocks

Axel Paris, Adrien Peytavie, Eric Guérin, Jean-Michel Dischler, Eric Galin.

The Visual Computer 36, 2020.

Project:

https://aparis69.github.io/public_html/projects/paris2020_Blocks.html

Code:

https://github.com/aparis69/Rock-fracturing

DOI:

https://doi.org/10.1007/s00371-020-01905-6

Use for:

```text
implicit block geology
fracture distributions
cliffs
block archetypes
strata adaptation
implicit detail warping
```

---

## A.9 Modeling Cracks and Fractures

Brett Desbenoit, Eric Galin, Samir Akkouche.

The Visual Computer 21, 2005.

DOI:

https://doi.org/10.1007/s00371-005-0317-z

Use for:

```text
directed crack networks
profile curves
swept carving volumes
```

---

## A.10 Procedural Multiscale Geometry Modeling using Implicit Surfaces

Bojja Venu, Adam Bosak, Juan Raúl Padrón-Griffe.

Computer Graphics Forum 44(7), 2025.

DOI:

https://doi.org/10.1111/cgf.70233

Project:

https://graphics.unizar.es/projects/MultiscaleGeometry_2025/

Jerico's admitted bounded use:

```text
exactly two implicit pore scales
fine/mesoscopic separation
fixed local distribution and existing scalar/cavity consumers
```

---

# Appendix B — Research evidence summary

| Problem | Strongest source | Jerico takeaway |
|---|---|---|
| Interactive operator pipeline | proc-rock | Ordered data-driven modifiers + separate noise graph |
| Rounded boulder generation | Söderlund 2015 | Useful limited method; proves roundness trap |
| Rock cell geometry | Peytavie et al. 2009 | Anisotropic Voronoi, not ordinary isotropic cells |
| Cliff/block geology | Paris et al. 2020 | Fracture-driven implicit blocks |
| Convex weathering | Beardall et al. 2007 | Local exposure / spheroidal erosion |
| Concave weathering | Jones et al. 2010 | Curvature + durability |
| Directed cracks | Desbenoit et al. 2005 | Pattern + profile + swept carve |
| Coarse random mass | SpeedRock 2011 | Grammar → implosion → erosion |
| Physical stone aging | Dorsey et al. 1999 | Surface-aligned narrow volume + material processes |
| Multi-scale pore synthesis | Venu et al. 2025 | Explicit two-scale separation behind a finite Jerico contract |

---

# Appendix C — Minimum operator acceptance checklist

## PlaneCut

- [ ] exact plane with roughness zero
- [ ] deterministic roughness
- [ ] orientation works
- [ ] GPU parity
- [ ] visible planar face

## DirectionalWarp

- [ ] zero-strength identity
- [ ] actual directional deformation
- [ ] anisotropic frequency
- [ ] deterministic
- [ ] GPU parity

## Strata

- [ ] explicit axis
- [ ] spacing in documented units
- [ ] optional low-frequency warp
- [ ] reusable mask
- [ ] visible in field debug
- [ ] can influence durability later

## FaultDisplace

- [ ] explicit plane
- [ ] explicit slip direction
- [ ] zero displacement identity
- [ ] coherent offset of existing strata
- [ ] GPU parity

## AnisotropicVoronoi

- [ ] local frame
- [ ] alpha/beta/gamma or equivalent anisotropy
- [ ] scale term
- [ ] cell ID
- [ ] nearest metric
- [ ] second-nearest metric
- [ ] deterministic

## FractureCells

- [ ] boundary mask
- [ ] width
- [ ] softness
- [ ] carving/weakening mode
- [ ] no random bump behavior masquerading as fracture

## Cavity

- [ ] actual volume subtraction
- [ ] transform
- [ ] mask support
- [ ] no NaN/Inf

## BubbleWeathering

- [ ] exposure field visible
- [ ] corner > flat exposure under test setup
- [ ] durability modulation
- [ ] deterministic iteration
- [ ] ping-pong or mathematically safe update

## CurvatureWeathering

- [ ] gradient stable
- [ ] curvature visualization
- [ ] convex mode
- [ ] concave mode
- [ ] durability
- [ ] finite differences tied to scale

## Detail

- [ ] can be disabled
- [ ] geology still readable when disabled
- [ ] scale explicitly micro/meso
- [ ] does not own macro silhouette

---

# Appendix D — Final “potato” gate

Before Codex claims success, answer all of these with **yes**:

```text
Can the same Box become a layered cliff?

Can the same Box become a fractured boulder?

Can the same Box become a weathered mass?

Can the same Box show a coherent fault through existing strata?

Can fracture-cell anisotropy visibly change the block family?

Can micro noise be disabled without destroying geological identity?

Can we inspect the fracture/strata/weathering masks?

Can we prove CPU/GPU math is consistent?

Can we distinguish a bad field from a bad Surface Nets extraction?

Can the user manipulate all of this without new C++ per rock?
```

If any critical answer is no:

> the procedural geology instrument is not finished.
