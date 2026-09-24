# JericoEngine — Adaptive Remesh, Feature Edges, Normals and Asset Retopo Research

**Status:** EXPERIMENTAL RESEARCH / PROTOTYPE DIRECTION — NOT IMPLEMENTED, NOT A PASS  
**Date:** 2026-09-16  
**Purpose:** define a Jerico-owned, feature-aware mesh-processing direction for the already-required `AdaptiveSurfaceRemesh` and `AssetRetopoRemesh` capabilities, informed by analysis of the owner's purchased Cracker v2.2 Blender addon plus public mesh-processing literature/documentation.  
**Repository effect:** this document does **not** change `ACTIVE_SLICE`, does not authorize opportunistic implementation, and does not supersede current Geology, Destruction, Asset Pipeline or RuntimeFormats authorities.

---

# 0. Executive conclusion

Jerico can and should implement its own practical triangle remeshing path for static/generated game geometry.

The recommended first architecture is **not** a Blender clone, QuadriFlow clone or universal voxel remesher.

Use:

```text
source mesh / generated surface
    -> build Jerico feature graph
    -> adaptive triangle remesh
       split
       collapse
       flip
       tangential relax
       reproject
    -> rebuild/transfer attributes
    -> rebuild smooth/hard normals
    -> split render vertices at attribute discontinuities
    -> generate deterministic tangents
    -> optional simplification / LOD
    -> collision / cook
```

Hard geometric features must be part of the remesher's constraints, not repaired after the mesh has already rounded them away.

The first implementation should be CPU/tool-side, deterministic and bounded. GPU remeshing is not justified for the initial product need.

---

# 1. Existing Jerico authority

`JERICO_1_0_DEFINITION_OF_DONE.md` already requires two distinct generic paths:

```text
AdaptiveSurfaceRemesh
AssetRetopoRemesh
```

Their intended responsibilities are different.

## 1.1 AdaptiveSurfaceRemesh

Primary consumers:

```text
SDF surfaces
cliffs
caves
rocks
terrain replacement regions
destruction chunks where appropriate
other generated static environmental geometry
```

Its core product requirement is:

```text
preserve important silhouette/features
while reducing unnecessary flat-region density
```

## 1.2 AssetRetopoRemesh

Secondary path for:

```text
imported meshes
AI-generated meshes
irregular external meshes
static game assets needing cleanup/restructure
```

It is intentionally **not** required to replace specialist character retopology.

## 1.3 Optimization is separate

Jerico's 1.0 target already states that:

```text
remesh != simplification != LOD != meshlet optimization
```

Keep that separation.

The remesher creates good topology and preserves features. A later optimizer/simplifier reduces triangles for runtime targets.

---

# 2. Current repository reality

At the time of this research:

- generated SDF/geology paths already reach game mesh/collision in bounded slices;
- Surface Nets/Hermite/QEF work already exists for geology extraction;
- the general adaptive-remesh / imported-asset-retopo stage is still a documented gap;
- `RuntimeFormats::MeshVertex` already stores:

```text
position float3
normal   float3
tangent  float4
uv0      float2
```

- the glTF importer currently requires source normals;
- missing tangents currently use an explicit neutral-tangent shortcut;
- the importer already contains a repository TODO for deterministic MikkTSpace generation in a future mesh-processing slice.

Therefore the remesh work should close an existing product gap rather than create a new unrelated subsystem.

---

# 3. What Cracker actually teaches us

Analysis of the supplied Cracker v2.2 package does **not** show a general-purpose remesh operator in its Python orchestration.

No meaningful Python route was found that invokes a Blender Remesh, QuadriFlow or Decimate operation as the core crack pipeline.

Instead, Cracker predominantly:

```text
generates required topology
extrudes/cuts/booleans
separates geometry
merges nearby vertices
tracks generated/original regions
controls shading/normals
```

This distinction matters.

## 3.1 Geometry Nodes evidence

The supplied `gn.blend` / `gn_bl5.blend` contain node/type identifiers including:

```text
Set Shade Smooth
Set Mesh Normal
Capture Attribute
Normal
Split Edges
Merge by Distance
Mesh Boolean
Separate Geometry
```

and named attributes/identifiers including:

```text
OG_faces
cracker_faces_a
cracker_faces_b
intersecting_edges
```

The serialized data strongly indicates explicit **surface provenance + normal/sharpness handling**.

This research does not claim an exact full node-link reconstruction merely from serialized identifiers.

## 3.2 Useful idea to port

The valuable architectural lesson is:

> Do not treat every triangle after topology editing as equivalent.

Jerico should retain provenance such as:

```text
OriginalExterior
GeneratedCut
IntersectionBoundary
InteriorFracture
MaterialRegion
GeologyRegion
```

and use that information when rebuilding topology and shading.

## 3.3 What not to port

Do not create:

```text
CrackerRemesh
BlenderNormalNodeRuntime
GeometryNodes ABI
Blender BMesh runtime dependency
```

Do not copy Blender GPL implementation code into Jerico merely because its behavior is useful.

Implement standard geometry algorithms independently under Jerico contracts.

---

# 4. Core design principle: features are data

A major failure mode of naive remesh is:

```text
sharp source
    -> resample/smooth
    -> rounded feature
    -> try to restore hard normal
```

A hard normal can make lighting look sharp, but it cannot recover geometry that was physically rounded.

Jerico must distinguish at least these concepts:

```text
GeometryCrease
ShadingHard
SemanticBoundary
UvSeam
MeshBoundary
Protected
```

They are not synonyms.

## 4.1 GeometryCrease

A real shape discontinuity or ridge.

Examples:

```text
cube edge
fracture lip
sharp rock ledge
BSP-derived plane intersection
strata step
machined hard-surface corner
```

The remesher must preserve its geometry.

## 4.2 ShadingHard

A normal discontinuity that may exist even when geometric topology is continuous.

It controls lighting, not necessarily vertex position constraints.

## 4.3 SemanticBoundary

A boundary that must not allow attributes/regions to bleed across it.

Examples:

```text
material boundary
OriginalExterior vs InteriorFracture
geology classification boundary
protected/manual region boundary
```

A semantic boundary may be geometrically flat.

## 4.4 UvSeam

A discontinuity in texture coordinates.

It requires render-vertex duplication but should not automatically create a geometric crease.

## 4.5 MeshBoundary

An actual open boundary of the mesh.

Default policy for ordinary remesh should preserve it as a constrained polyline unless the caller explicitly requests repair/fill behavior.

## 4.6 Protected

User/system-authored edges or vertices that the remesher must not destroy.

One edge may carry multiple flags.

---

# 5. Proposed Jerico FeatureGraph

The first reusable owner should be a Jerico feature representation independent of RuntimeFormats and third-party mesh types.

Conceptually:

```cpp
enum class MeshFeature : uint32_t
{
    None             = 0,
    GeometryCrease   = 1 << 0,
    ShadingHard      = 1 << 1,
    SemanticBoundary = 1 << 2,
    UvSeam           = 1 << 3,
    MeshBoundary     = 1 << 4,
    Protected        = 1 << 5
};
```

A feature chain should preserve stable source/provenance identity where available.

Conceptual data:

```text
FeatureGraph
    feature vertices
    ordered feature polylines
    junctions/corners
    per-edge flags
    source anchors
    region IDs on each side where applicable
```

This graph becomes an explicit constraint during remesh.

---

# 6. Feature detection

Use the strongest available source information first.

Priority:

```text
1 explicit authored/procedural feature lineage
2 explicit material/region/provenance boundaries
3 source hard/sharp information
4 mesh boundary
5 geometric dihedral-angle detection
6 optional user additions/removals
```

Do not throw away rich provenance and then try to rediscover it from triangle normals.

## 6.1 Dihedral fallback

For adjacent faces with normalized normals `n0`, `n1`:

```text
angle = acos(clamp(dot(n0, n1), -1, 1))
```

A fallback crease may be detected when:

```text
angle >= creaseThreshold
```

Typical editor presets could later expose values around 30–60 degrees, but no production default is frozen by this research.

## 6.2 Geometry vs shading thresholds

Allow separate policies:

```text
geometryCreaseAngle
shadingHardAngle
```

because an artist may want smooth geometric sampling around a moderately curved surface while still controlling shading differently.

## 6.3 Source normals are not sufficient

Duplicated source vertices/normals may imply a shading seam, not necessarily a real geometric crease.

Do not infer `GeometryCrease` solely because two co-located render vertices have different normals.

---

# 7. Intermediate editable topology

`RuntimeFormats::MeshVertex` is a good cooked render format, not an ideal topology-editing data structure.

The remesher needs a private tool-side editable topology.

Recommended family:

```text
half-edge triangle mesh
```

or an equivalent adjacency representation with the same constant/local operations.

Conceptually:

```cpp
struct TopoVertex
{
    double3 position;
    VertexClass featureClass;
    SourceAnchor source;
};

struct TopoHalfEdge
{
    VertexId origin;
    HalfEdgeId twin;
    HalfEdgeId next;
    FaceId face;
    MeshFeature flags;
};

struct TopoFace
{
    RegionId region;
    MaterialId material;
    SurfaceProvenance provenance;
};
```

Use double precision for tool-side geometric decisions where it materially improves robustness. Cooked output remains current RuntimeFormats float representation unless another authority changes it.

---

# 8. Vertex feature classes

Before relaxation/collapse decisions classify each topological vertex.

## Smooth

No geometric feature constraint.

May move tangentially over the source surface.

## Crease

Belongs to one coherent feature polyline, normally two incident feature edges.

May move **along the feature**, not away from it.

## Corner

Feature endpoint, junction, explicit protected corner or sufficiently strong corner event.

Default first implementation:

```text
corner position fixed to/reprojected onto its source corner
```

This simple classification prevents the classic "cube becomes a soap bar" failure.

---

# 9. Recommended remesh algorithm

Use the well-established incremental isotropic triangle remeshing family:

```text
split long edges
collapse short edges
flip eligible edges
relax vertices
project to source
repeat
```

This family is documented publicly by Botsch-style remeshing and implementations such as CGAL.

Jerico owns its implementation and its acceptance contract.

---

# 10. Target edge length

Do not make the production path globally uniform.

Start from a base target:

```text
Lbase
```

and derive a local sizing field:

```text
L(p) = clamp(min(Lbase, Lcurvature, Lfeature, Lsilhouette), Lmin, Lmax)
```

Not every consumer needs every term in v1.

## 10.1 Curvature sizing

A useful geometric intuition comes from circle sagitta error.

For local curvature magnitude `kappa` and allowed deviation `epsilon`, a chord length scale can be approximated by:

```text
Lcurvature ~= sqrt(8 * epsilon / max(kappa, kappaMin))
```

This does not need to be the exact final formula.

It expresses the desired policy:

```text
flat area    -> longer edges
high curvature -> shorter edges
```

## 10.2 Feature sizing

Near a sharp feature, reduce target length so the polyline and adjacent surface can represent the feature accurately.

Conceptually:

```text
Lfeature = blend(featureEdgeLength, Lbase, normalizedDistanceToFeature)
```

## 10.3 Source-driven sizing

Geology/SDF extraction may already know meaningful structural scales.

Where available, consume:

```text
voxel size
Hermite crossings
QEF/native constraints
region boundaries
procedural crack width
authored protected regions
```

rather than rediscovering all detail after extraction.

---

# 11. Split rule

Classic isotropic remesh commonly splits an edge when it is substantially larger than target.

A useful initial band is:

```text
split when length > 4/3 * Llocal
```

For a constrained feature edge:

- splitting is allowed/required when necessary to sample the feature;
- the new vertex must be placed/reprojected on the source feature polyline;
- both child edges retain feature identity and flags.

For a semantic/material boundary, new faces on each side retain their original region ownership.

---

# 12. Collapse rule

A useful initial band is:

```text
collapse when length < 4/5 * Llocal
```

but length alone is never enough.

Reject a collapse when it would:

```text
break manifold link condition
remove/problematically merge a protected corner
cross a semantic boundary
merge distinct feature polylines incorrectly
change feature graph topology
invert a triangle
create degenerate area
create duplicate/non-manifold topology
exceed source-surface error bound
exceed feature drift bound
violate region/material ownership
```

For the first implementation, be conservative. It is acceptable to retain extra triangles rather than silently destroy topology.

---

# 13. Edge flip rule

Flipping improves triangle quality/valence without changing vertex count.

Never flip:

```text
GeometryCrease
SemanticBoundary
Protected
MeshBoundary
```

unless a future explicit resampling contract allows and proves it.

For ordinary interior smooth edges, choose flips that improve local triangle quality / valence toward a regular triangulation.

A common target valence is approximately:

```text
6 interior
4 boundary
```

but Jerico acceptance should focus on quality and preserved surface error, not only valence score.

---

# 14. Tangential relaxation

Naive Laplacian smoothing shrinks and rounds geometry.

Use tangential relaxation plus reprojection.

Let:

```text
p = current vertex
c = weighted neighbor centroid
n = source/current surface normal
d = c - p
```

For a smooth vertex:

```text
dTangential = d - n * dot(d, n)
pCandidate = p + lambda * dTangential
```

Then project `pCandidate` back to the allowed source patch.

## 14.1 Crease vertex relaxation

For feature tangent `t`:

```text
dFeature = t * dot(d, t)
pCandidate = p + lambda * dFeature
```

Then project to the exact source feature polyline.

## 14.2 Corner relaxation

Default:

```text
no relaxation
```

Corner stays anchored to its source corner/constraint.

This is the critical geometry rule behind feature preservation.

---

# 15. Projection back to the source

Every remesh iteration should keep the output attached to a stable source representation.

Do not project all vertices against the entire mesh without region awareness.

## Smooth vertex

Project to:

```text
same allowed source patch / region
```

using a nearest-triangle acceleration structure such as a BVH.

## Crease vertex

Project to:

```text
same source feature polyline
```

## Corner

Project/fix to:

```text
same source corner
```

## 15.1 Region-aware projection prevents leaks

Without region constraints, a point near a thin wall, crack lip or adjacent surface may snap to the wrong side.

Source anchors should therefore retain enough information to constrain projection:

```text
source triangle / patch ID
feature ID
region/material/provenance ID
optional barycentric coordinates
```

---

# 16. Source surface acceleration

Build a read-only source BVH once per remesh operation.

Queries needed:

```text
closest triangle within allowed patch
closest point on feature polyline
local surface normal
optional signed side/orientation check
local error sampling
```

Do not rebuild the source BVH after every topology edit; the source is immutable during the operation.

---

# 17. Error envelope

Remesh quality cannot be judged only by triangle count.

Expose/measure:

```text
maximum vertex-to-source distance
mean vertex-to-source distance
sampled surface distance / approximate Hausdorff bound
feature polyline drift
silhouette error where admitted
normal deviation
```

The user-facing control should preferably be meaningful:

```text
Maximum Surface Error: millimetres/metres
```

instead of only an opaque "quality" slider.

---

# 18. Feature drift bound

For source feature polyline `F` and remeshed feature `F'`, record at least:

```text
max distance F' -> F
max source feature sample -> F'
```

A hard feature test should fail before Beauty review if this exceeds the admitted bound.

---

# 19. Topological safety

The first production-grade remesher needs explicit local guards.

Minimum:

```text
manifold link condition for collapse
non-zero triangle area
orientation / face-flip rejection
no duplicate face generation
no illegal duplicate edge
feature topology invariants
semantic region invariants
```

For complex/high-risk edits, perform local self-intersection checks using the source/output BVH.

A later optimized path may selectively skip expensive checks in trivially safe regions only after measurement and regression evidence.

---

# 20. Do not make AssetRetopo repair arbitrary garbage

Separate two modes.

## Valid-surface remesh

Input is a valid/manifold-enough surface.

Use the feature-aware triangle algorithm above.

## RepairVolume fallback

For:

```text
self-intersections
non-manifold soups
holes beyond admitted repair
AI meshes with severe internal garbage
```

use a separate repair/reconstruction path if/when admitted.

A volumetric/SDF/voxel reconstruction may be appropriate there because changing topology is intentional.

Do not force the ordinary remesher to silently repair every malformed asset.

---

# 21. Why voxel remesh is not the default

Voxel/volume remeshing is useful when topology repair is desired, but it:

```text
resamples through a grid/field
can erase thin details
can round sharp features without special reconstruction
can change topology
requires meaningful volume/thickness
requires attribute reprojection
```

This is a poor default for:

```text
hard-surface assets
zero-thickness sheets
already-valid fracture chunks
BSP-like planar geometry
```

Use it as an explicit repair/volume route, not as Jerico's universal remesh.

---

# 22. No need for quad topology in the first Jerico path

Jerico runtime rendering/collision ultimately consumes triangles.

For static game assets, an excellent triangle distribution is more useful than forcing artist-friendly quad loops.

Therefore:

```text
AdaptiveSurfaceRemesh -> triangle remesh
AssetRetopoRemesh 1.0 -> triangle remesh/cleanup
```

A future quad-retopo tool may be useful for round-tripping assets into a DCC, deformation workflows or character authoring, but it is not required to solve the present static-environment problem.

---

# 23. Surface provenance

Carry provenance through remeshing.

Conceptual face data:

```text
OriginalExterior
GeneratedCut
InteriorFracture
FreshCrack
GeologyRegion
BspPlaneRegion
ImportedMaterialRegion
```

When splitting/flipping/collapsing, propagation rules must be explicit.

## Split

Child triangles inherit parent region/provenance unless the split occurs exactly on a declared boundary.

## Collapse

Do not collapse across incompatible semantic/provenance boundaries.

## Flip

Do not flip across region boundaries.

This directly captures the useful idea observed in Cracker's `OG_faces` / generated-face tagging without adopting its implementation.

---

# 24. Normals are rebuilt after topology settles

Do not repeatedly bake final shading normals during every topology operation.

Recommended order:

```text
finish geometry topology
    -> identify final shading-hard graph
    -> compute per-corner normals
    -> flatten/disambiguate into render vertices
    -> compute tangents
```

During geometry operations use geometric/source normals only as needed for projection, curvature and safety.

---

# 25. Smooth and hard normal reconstruction

Jerico should compute normals per **face corner** internally.

That is the natural representation for mixed smooth/hard surfaces.

For each face:

```text
nf = normalize(cross(p1-p0, p2-p0))
```

For a corner on a smooth fan, compute an area/angle-weighted average over the connected faces reachable without crossing a `ShadingHard` edge.

Example angle-weighted form:

```text
nCorner = normalize(sum(cornerAngle(face, vertex) * nFace))
```

Area-angle weighting may also be evaluated.

The exact weighting should be frozen by fixtures, not by preference.

---

# 26. Shading-hard connectivity

At each topological vertex, build smoothing components by traversing incident faces without crossing:

```text
ShadingHard
flat-face boundary
explicit normal split
```

Each connected smoothing component receives its own corner normal.

Therefore one geometric position can legitimately produce multiple render vertices.

---

# 27. Geometry crease does not automatically define final normal policy

Often:

```text
GeometryCrease -> ShadingHard
```

is the desired default.

But keep the data separate.

Examples:

- a small geometric bevel can be smooth shaded;
- a coplanar material boundary should not create a geometry crease;
- stylized assets may request a shading hard edge without a large geometric angle.

The editor can provide a convenient default linking them while preserving independent semantics internally.

---

# 28. Preserve source normals mode

For imported hero/static assets, an optional mode may preserve source shading where meaningful.

For each new vertex/corner, transfer from the source triangle using barycentric interpolation, constrained to the same shading patch.

Conceptually:

```text
sourceAnchor = (triangleId, barycentrics)
normal = normalize(
    b0 * n0 +
    b1 * n1 +
    b2 * n2)
```

Never interpolate across a source hard-normal seam.

Generated cut/interior surfaces do not have a source normal and must receive Jerico-generated geometric/shading normals.

---

# 29. UV transfer

UVs are attributes, not geometry constraints by default.

For new vertices inside one source UV patch:

```text
uv = b0*uv0 + b1*uv1 + b2*uv2
```

At UV seams, preserve seam identity and generate independent render vertices on each side.

Do not let nearest-point transfer jump between adjacent UV islands.

---

# 30. Material boundaries

Material/submesh boundaries are semantic constraints.

A coplanar material transition should remain geometrically coplanar while preserving ownership on both sides.

Do not force it into a hard geometric crease simply because render vertices differ.

---

# 31. Interior fracture surfaces

Destruction already distinguishes original exterior and newly exposed surfaces conceptually.

The remesh/attribute layer should preserve that distinction.

Recommended default for a fresh cut:

```text
OriginalExterior
    preserve/transfer original UV + normal policy

InteriorFracture
    use interior material/UV policy
    derive normals from new cut geometry
    optionally harden the lip boundary
```

This is a direct Jerico-native counterpart to the provenance-aware behavior that made the Cracker inspection interesting.

---

# 32. Flatten per-corner attributes to RuntimeFormats

Current `RuntimeFormats::MeshVertex` is per vertex, not per face corner.

No format change is required merely to support hard edges.

Before cook:

```text
(topological position, corner normal, tangent, UV, material-relevant attributes)
    -> unique render vertex key
```

Duplicate a position whenever corner attributes differ.

Examples requiring a split:

```text
hard normal edge
UV seam
tangent discontinuity
other admitted per-corner attribute seam
```

This is normal indexed-render-mesh behavior.

---

# 33. Tangent generation

Tangents must be generated after final:

```text
positions
normals
UVs
corner/render-vertex splits
```

The current glTF importer explicitly has a TODO for deterministic MikkTSpace generation.

A mesh-processing slice should close this rather than leave remeshed assets with neutral tangents.

Two candidate directions:

```text
A. admit/reference MikkTSpace directly
B. admit a tested compatible Jerico/private backend
```

Exact dependency choice requires normal dependency admission.

Do not average/overwrite per-corner tangents onto an incompatible existing index because tangent discontinuities require render-vertex splits.

---

# 34. Meshoptimizer role

`meshoptimizer` remains a strong candidate for the **post-remesh** processing stage, not the core feature-aware remesher contract.

Useful current capabilities include:

```text
attribute-aware simplification
vertex locking/protection
error reporting
vertex-cache optimization
vertex-fetch optimization
meshlets / clustering
normal/tangent generation APIs in current upstream
```

Its current upstream also contains experimental remesh/normal APIs. Experimental upstream APIs should not silently become Jerico's production ABI.

Recommended first stance:

```text
own Jerico remesh contract + implementation
optional admitted meshoptimizer for simplification/optimization
```

---

# 35. Simplification after remesh

After a valid remesh exists, build LODs with an independent simplification stage.

A QEM-family simplifier is appropriate.

For face plane:

```text
pi = [a b c d]^T
Kp = pi * pi^T
```

Vertex quadric:

```text
Qv = sum(Kp over incident planes)
```

For candidate collapse `u,v`:

```text
Q = Qu + Qv
error(p) = p^T Q p
```

Feature/boundary preservation can be handled with constraints/line quadrics or conservative locked-vertex policies.

Jerico does not need to independently reinvent every mature runtime simplification/meshlet optimization algorithm if an admitted private dependency provides it safely.

---

# 36. AdaptiveSurfaceRemesh pipeline

Recommended generated-geometry path:

```text
Procedural/SDF source
    -> extraction
       Surface Nets / current admitted extractor
    -> carry native structural constraints when available
       Hermite positions/normals
       QEF/native planes
       region/classification lineage
       directed crack/structural boundaries
    -> feature graph
    -> adaptive remesh
    -> attribute/provenance reconstruction
    -> normals
    -> tangents
    -> optional simplification / LOD
    -> collision
    -> cook
```

Important:

> Do not throw away QEF/Hermite/native constraint knowledge and then detect all sharp features only from the extracted triangles.

For geology this is especially important because Jerico already proved that sampled gradients can dilute discontinuities while native constraints retain planar information.

---

# 37. AssetRetopoRemesh pipeline

Recommended imported-asset path:

```text
canonical imported triangle mesh
    -> validate manifold/topology class
    -> build source BVH
    -> discover/import feature graph
       geometry creases
       source shading splits
       UV seams
       material boundaries
       mesh boundaries
    -> adaptive triangle remesh
    -> region-aware source reprojection
    -> UV/attribute transfer
    -> normal rebuild / optional source-normal preservation
    -> tangent generation
    -> optional simplification / LOD
    -> collision/cook
```

For malformed assets requiring topology repair, route explicitly to `RepairVolume` or another future admitted repair mode.

---

# 38. User-facing modes

A useful eventual editor UX could be:

```text
Remesh Mode
    Adaptive Surface
    Asset Retopo
    Repair Volume          // later / separate contract

Target
    Maximum Surface Error
    Min Edge Length
    Max Edge Length

Features
    Preserve Boundaries
    Preserve Materials
    Preserve UV Seams
    Crease Angle
    Protect Authored Features

Shading
    Preserve Source Normals
    Rebuild Smooth/Hard
    Smooth Angle

Preview
    Wireframe
    Feature Edges
    Error Heatmap
    Source vs Result
```

This is illustrative, not an authorized UI spec.

---

# 39. Preview diagnostics

The 1.0 `Generate Game Mesh` preview should expose meaningful evidence.

Add candidates:

```text
input/output vertex count
input/output triangle count
feature edge count
hard shading edge count
UV seam count
material boundary count
max/mean source deviation
max feature drift
split/collapse/flip counts
rejected operation counts by reason
iteration count
time
memory
render-vertex duplicates caused by seams
```

This makes performance/quality decisions inspectable instead of subjective.

---

# 40. Determinism

For identical:

```text
source mesh/hash
feature policy
remesh settings
attribute-transfer policy
```

output should be deterministic under the admitted platform/numeric contract.

Avoid iteration order that depends on pointer/hash-table nondeterminism.

Use stable edge/vertex IDs and deterministic queues/sorts where operation priority matters.

---

# 41. Cancellation and bounded work

Tool operation must support cancellation and hard budgets.

Candidate bounds:

```text
maximum input triangles
maximum output triangles
maximum iterations
maximum topology operations
maximum BVH queries
maximum generated bytes
maximum wall-clock diagnostic threshold
```

Never let a contradictory feature constraint cause infinite splitting.

If protected feature edges are longer than the local target, either resample the feature under its own allowed contract or report that the constraints cannot satisfy the target.

---

# 42. Parallelism

Do not parallelize topology mutation first.

A robust sequence is more important than premature concurrency.

Parallel-friendly tasks later include:

```text
source face normals
feature candidates
BVH build/query batches
curvature estimates
attribute reprojection
error sampling
```

Topology split/collapse/flip mutation can remain controlled/deterministic initially.

---

# 43. GPU policy

No GPU implementation is required for the first remesh path.

Reasons:

```text
authoring/cook workload, not per-frame gameplay
branch-heavy topology edits
complex adjacency mutation
robust validation more important than throughput
CPU implementation easier to test deterministically
```

GPU preview/analysis may be evaluated later only with measured need.

---

# 44. Required prototype fixtures

No implementation should claim success on a rock alone.

## R1 — Flat plane grid

Input:

```text
overdense coplanar triangle grid
```

Expected:

```text
large density reduction
plane stays planar
no artificial crease
error approximately zero
```

## R2 — Cube

Expected:

```text
six planes preserved
12 geometric creases preserved
8 corners preserved
no soap-bar rounding
```

This is the minimum hard-edge gate.

## R3 — Beveled cube

Expected:

```text
major shape corners remain
bevel surface may smooth/remesh
intended smooth shading remains smooth
```

Proves GeometryCrease != every normal change.

## R4 — Cylinder

Expected:

```text
side smooth
cap/side hard boundary according to policy
circular silhouette within error
```

## R5 — Cylinder with UV seam

Expected:

```text
UV seam preserved as attribute seam
no geometric dent/crease caused solely by UV seam
correct duplicated render vertices
```

## R6 — Coplanar material boundary

Expected:

```text
material boundary remains exact
surface stays geometrically flat
no forced hard geometry edge
```

## R7 — Fracture chunk

Expected:

```text
OriginalExterior remains distinct from InteriorFracture
fresh cut material survives
no normal smoothing bleed across fracture lip when hard policy is selected
closed topology remains closed
```

## R8 — SDF/geology sharp ledge

Use a controlled rock/strata fixture with an admitted native feature.

Expected:

```text
flat/low-curvature density reduced
sharp ledge position preserved
silhouette/features within declared error
```

## R9 — Plane-cut native-constraint regression

Reuse a fixture related to the already-proven PlaneCut/Hermite issue.

Expected:

```text
feature graph consumes native structural constraint where available
remesh does not re-round the recovered plane
```

## R10 — Non-manifold input

Expected ordinary `AssetRetopoRemesh` result:

```text
stable rejection / explicit repair-required diagnostic
```

not silent destructive reconstruction.

## R11 — Determinism

Repeated same input/settings:

```text
same topology/attribute hashes
```

within the admitted deterministic contract.

## R12 — Normal-map tangent fixture

Include:

```text
smooth region
hard normal edge
UV seam
mirrored UV island if admitted
```

Expected:

```text
no invalid tangent
correct render-vertex splitting
normal-map orientation matches tangent convention
```

---

# 45. Red-line failures

The prototype fails if any of these occur:

```text
cube edges round despite preservation enabled
smooth surface develops unintended visible seams
UV seam changes geometry
material boundary is lost
fresh-cut/exterior classification mixes
collapse creates non-manifold topology
faces invert
output exceeds max surface error
feature graph topology changes without authorization
nondeterministic output hash on identical input
neutral tangents silently remain on final normal-mapped output
```

---

# 46. First implementation slice recommendation

Keep the first executable slice deliberately small:

```text
FeatureGraph + CPU isotropic remesh + normal rebuild
```

Scope:

```text
valid closed/open triangle meshes
boundary constraints
angle-detected geometry creases
explicit protected edges
uniform target edge length first
split/collapse/flip/tangential relax/reproject
cube + plane + cylinder tests
per-corner smooth/hard normals
flatten to current MeshVertex
```

Do **not** begin with:

```text
curvature adaptive sizing
broken AI mesh repair
GPU
quad retopo
full UV regeneration
LOD generation
all geology integration
all destruction integration
```

Once uniform feature preservation is proven, add adaptive sizing.

---

# 47. Second implementation slice recommendation

```text
Adaptive sizing + semantic/attribute transfer
```

Add:

```text
curvature sizing field
material/region boundaries
UV seam transfer
source barycentric anchors
error heatmap/counters
fracture provenance fixture
```

---

# 48. Third implementation slice recommendation

```text
Production consumers
```

Integrate independently into:

```text
AdaptiveSurfaceRemesh / Generate Game Mesh
AssetRetopoRemesh
```

Then evaluate simplification/LOD dependency admission separately.

---

# 49. Dependency policy

Jerico's dependency policy allows mature private libraries when they save substantial work, but third-party types/ABIs may not leak into Jerico project/runtime contracts.

Research candidates:

```text
CGAL / libigl / PMP-style implementations
    research oracle / possible private backend after license/admission review

meshoptimizer
    strong candidate for simplification/optimization/tangents after admission

MikkTSpace
    candidate for deterministic tangent standard after license/admission review
```

Do not select a dependency merely because this research cites it.

The first Jerico feature-aware remesher is simple enough to implement independently, while using public literature/reference implementations as correctness oracles.

---

# 50. Licensing guardrail

Cracker was purchased by the owner and may be inspected for behavior and mathematical ideas subject to its license.

Blender implementation source is GPL; public documentation/behavior may inform an independent implementation, but do not copy incompatible source code into Jerico.

CGAL licensing must be reviewed before any dependency decision; documentation is currently used as an algorithm/reference source only.

meshoptimizer is MIT upstream, but still requires the normal Jerico dependency-admission process before becoming a production dependency.

MikkTSpace also requires exact upstream/license/pin verification before admission.

---

# 51. Public research references used

Algorithm/behavior references consulted for this research include:

```text
CGAL Polygon Mesh Processing
https://doc.cgal.org/latest/Polygon_mesh_processing/
- isotropic incremental remeshing
- constrained feature polylines
- curvature-adaptive sizing fields
- surface reprojection

Blender Manual — Smooth By Angle
https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/normals/smooth_by_angle.html
- edge sharpness determined from neighboring face-normal angle

Blender Manual — Set Mesh Normal
https://docs.blender.org/manual/en/5.2/modeling/geometry_nodes/mesh/write/set_mesh_normal.html
- edge/face sharpness
- custom normals
- face-corner normal domain for mixed smooth/sharp data

meshoptimizer upstream
https://github.com/zeux/meshoptimizer
- attribute-aware simplification
- protected/locked vertices
- tangent generation
- current experimental normal/remesh APIs

MikkTSpace upstream
https://github.com/mmikk/MikkTSpace
- deterministic tangent-space generation
- unindexed/per-corner result semantics
```

These are research references, not automatically admitted Jerico dependencies.

---

# 52. Final proposed Jerico ownership

Do not make one giant `Remesher.cpp` that owns unrelated policy.

Conceptual boundaries:

```text
Jerico Mesh Processing
│
├── MeshTopology
│   ├── half-edge / adjacency
│   └── validation
│
├── MeshFeatureGraph
│   ├── feature detection
│   ├── authored/procedural constraints
│   └── source feature polylines
│
├── AdaptiveRemesh
│   ├── split
│   ├── collapse
│   ├── flip
│   ├── relax
│   ├── reproject
│   └── sizing field
│
├── MeshAttributeTransfer
│   ├── source anchors
│   ├── UV
│   ├── material/region
│   └── provenance
│
├── MeshShading
│   ├── smooth/hard corner normals
│   ├── render-vertex splitting
│   └── tangent generation
│
└── MeshOptimization
    ├── simplification/LOD adapter
    ├── cache/fetch optimization
    └── meshlet preparation where admitted
```

Exact directories/targets are slice-owned; this is a semantic decomposition, not permission to create parallel authorities.

---

# 53. Relation to Cracker research

The previous document:

```text
docs/JERICO_EXPERIMENTAL_CRACK_GLASS_FRACTURE_RESEARCH.md
```

remains the authority for the Cracker-derived fracture/deformation experiment.

This remesh research complements it:

```text
Cracker research
    -> crack/fracture fields
    -> glass 2D partition + explicit thickness
    -> provenance idea

Remesh research
    -> general triangle quality/density
    -> hard feature preservation
    -> shading/UV/material reconstruction
    -> asset/geology preparation
```

Do not require ordinary thin-glass fracture to run a heavyweight general remesher if the direct planar partition already gives valid closed shards.

Use the cheapest correct path.

---

# 54. Decision

The recommended research decision is:

```text
YES — Jerico should own a feature-aware triangle remesher.
```

But the intended product is **not** "our Blender Remesh".

It is:

```text
AdaptiveSurfaceRemesh
    for generated world geometry

AssetRetopoRemesh
    for valid imported/static asset cleanup

shared by:
    FeatureGraph
    topology kernel
    source reprojection
    attribute transfer
    normal/tangent rebuild
```

The highest-risk requirement is not splitting/collapsing triangles.

The highest-risk requirement is preserving the meaning of the surface:

```text
sharp geometry
shading boundaries
UV seams
material regions
fracture provenance
geology structural features
source error bounds
```

The first RED/PASS witness should therefore be a **cube**, not a beauty rock:

```text
overdense cube
-> remesh aggressively
-> six planes remain planes
-> 12 creases remain creases
-> 8 corners stay corners
-> triangle quality improves
-> hard/smooth normals are correct
-> output stays inside a measured error bound
```

Only after that passes should the implementation move to rocks, cliffs, fracture chunks and AI/imported assets.
