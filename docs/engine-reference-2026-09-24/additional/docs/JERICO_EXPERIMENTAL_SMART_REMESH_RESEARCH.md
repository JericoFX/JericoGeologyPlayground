# JericoEngine — Experimental Smart Remesh / Feature-Preserving Retopology Research

**Status:** EXPERIMENTAL RESEARCH / PROTOTYPE DESIGN — NOT IMPLEMENTED, NOT A PASS  
**Date:** 2026-09-17  
**Purpose:** extract reusable mathematical/algorithmic ideas from the owner's purchased Smart Remesh 3 Blender addon, cross-check them against public remeshing literature, and define a Jerico-native path for smooth-surface and hard-edge-preserving remeshing.  
**Repository effect:** this document does not change `ACTIVE_SLICE`, does not authorize opportunistic implementation, and does not supersede existing Procedural Authoring / asset-pipeline authorities.

---

## 0. Executive conclusion

Yes: Jerico can implement its own remeshing/retopology pipeline without Blender.

The supplied addon is especially useful because it contains **two distinct remeshing strategies**:

1. a custom NumPy-only, field-guided quad-dominant remesher;
2. an Instant Meshes backend plus automatic analysis, target calibration, hard-surface classification, panelization and adaptive refinement.

The important lesson is not to port the addon as one monolithic tool. Jerico should implement reusable primitives and expose two product-level paths already required by `JERICO_1_0_DEFINITION_OF_DONE.md`:

```text
AdaptiveSurfaceRemesh
AssetRetopoRemesh
```

Recommended architecture:

```text
Source Mesh
   ↓
Topology + geometric analysis
   ↓
Feature graph
   ├─ boundaries
   ├─ artist sharp edges
   ├─ dihedral creases
   ├─ material/UV seams when requested
   └─ corners / junctions
   ↓
Sizing field
   ├─ uniform target
   ├─ curvature
   ├─ feature proximity
   ├─ thin-region protection
   └─ optional source-density matching
   ↓
4-RoSy orientation field
   ↓
Position / lattice field
   ↓
Cluster + graph extraction
   ↓
Quad-dominant topology
   ↓
REPROJECT TO SOURCE
   ↓
Feature-constrained tangential relaxation
   ↓
REPROJECT TO SOURCE
   ↓
Normals / tangents / seams / material provenance rebuild
   ↓
Cooked Jerico mesh
```

Hard-surface assets should additionally support a panelization mode:

```text
flat regions -> one panel n-gon each
feature boundaries -> retained geometry
chamfer/bevel bands -> dense topology only where shape requires it
```

The biggest improvement Jerico should make over the addon's custom remesher is **surface reprojection during smoothing/relaxation**. The addon itself documents that its smooth pass is disabled by default because it has no reprojection and therefore drifts away from the source surface.

---

## 1. IP / licensing guardrail

The purchased addon is a private behavioral and algorithmic reference.

Jerico must not:

- redistribute Smart Remesh 3;
- copy proprietary addon source into the engine;
- require Blender or `bpy` at runtime;
- ship addon-specific data or UI;
- claim binary compatibility with Smart Remesh.

The addon bundles the public Instant Meshes implementation as an external backend and identifies it as BSD-licensed. Any use of that public implementation would still require a normal dependency/license review.

Preferred Jerico path:

```text
observe behavior
-> isolate generic mathematics
-> cross-check against public literature
-> specify Jerico-owned semantics
-> independently implement in C++ / compute where justified
-> validate with Jerico fixtures
```

---

## 2. Supplied addon inventory

Relevant files in the supplied package include:

```text
core/remesh.py
core/field.py
core/mesh.py
core/sizing.py
core/detect.py
core/extract.py
core/hierarchy.py
core/hardsurface.py
core/panelize.py
core/subdivide.py
core/prune.py
core/poles.py
core/score.py
core/auto.py
core/instantmeshes.py
bin/Instant Meshes.exe
```

This is not merely a Blender wrapper. A substantial amount of the remeshing math is independent of Blender and implemented with NumPy arrays.

---

## 3. Two remesh engines are present

### 3.1 Custom field-guided remesher

`core/remesh.py` explicitly describes an end-to-end polygon-mesh to quad-dominant pipeline implemented without Blender operators.

Pipeline:

```text
TriMesh conversion
-> target scale / sizing field
-> orientation field
-> comb cross field
-> position field
-> lattice collapse / clustering
-> output graph
-> face extraction
-> cleanup
-> optional relaxation
```

### 3.2 Instant Meshes backend

`core/instantmeshes.py` invokes the public Instant Meshes executable.

The addon layers additional behavior around it:

```text
input analysis
-> automatic crease angle
-> target-face calibration
-> candidate search
-> topology scoring
-> optional adaptive post-refinement
-> hard-surface cleanup / panelization
```

This separation is useful for Jerico: remeshing mathematics and authoring heuristics should remain separate modules.

---

## 4. Public algorithm family

The custom remesher clearly belongs to the same algorithmic family as **Instant Field-Aligned Meshes** (Jakob, Tarini, Panozzo, Sorkine-Hornung, 2015):

```text
N-RoSy / cross orientation field
+
position field
+
field-aligned extraction
```

For quads, the field has 4-fold rotational symmetry: a direction and that direction rotated by 90 degrees represent the same cross.

The public Instant Meshes method is specifically designed to align output edges to surface features and uses local smoothing rather than one enormous global optimization.

Jerico should independently implement the concepts, not copy proprietary addon source.

---

## 5. Source topology normalization

The addon internally triangulates arbitrary polygons for geometric analysis but tracks which edges were real polygon edges.

This matters because fan diagonals introduced only for analysis must not become fake features.

Jerico requirement:

```cpp
struct AnalysisTriangleMesh
{
    positions;
    triangles;
    originalEdgeIdentity;
    triangleToSourceFace;
};
```

A triangulation edge that did not exist in the authored mesh cannot become a hard edge solely because the triangulated surface is non-planar.

---

## 6. Feature graph — the core of hard-edge preservation

Hard-edge preservation must be geometric, not merely a shading flag applied after remesh.

The addon treats the following as features:

```text
open boundaries
sharp/crease edges
optional authored sharp edges
optional selected edges
optional UV seams
optional crease-weighted edges
```

Jerico should generalize this as:

```cpp
struct RemeshFeatureEdge
{
    uint32_t v0;
    uint32_t v1;
    FeatureKind kind;
    float importance;
    bool geometricConstraint;
    bool shadingDiscontinuity;
};
```

Suggested kinds:

```text
Boundary
AuthoredSharp
DetectedCrease
MaterialBoundary
UVSeam
FractureBoundary
ProtectedEdge
```

One edge may carry multiple semantics.

---

## 7. Automatic crease detection

The addon does not rely solely on a fixed 30-degree threshold.

It measures manifold-edge dihedral angles and estimates a crease threshold from the model's own distribution.

The practical strategy is:

```text
compute all manifold dihedral angles
-> histogram candidate range
-> search upper-tail valley
-> if no convincing valley exists, use a high percentile fallback
```

This is valuable because hard-surface assets and scanned/organic assets have very different angle distributions.

Jerico should expose:

```text
Feature Detection
    Auto
    Angle Override
    Authored Only
```

Auto should report the chosen angle rather than hide it.

---

## 8. Hard-surface semantic detection beyond dihedral angle

The addon contains an important second signal: **panel type transitions**.

A visually meaningful edge may separate a flat panel from a curved shell while having only a small local dihedral angle. A simple angle threshold can miss that edge.

The addon therefore:

1. groups faces into approximately coplanar regions;
2. determines whether each region behaves like a real panel;
3. detects transitions between panel and non-panel regions;
4. treats boundaries between distinct flat panels as features too.

Jerico should preserve this idea.

Feature classification becomes:

```text
crease feature
OR
panel-type boundary
OR
authored feature
```

This is substantially more reliable for machinery, doors, sci-fi assets, BSP-derived props and architectural meshes.

---

## 9. Bilateral normal analysis for hard surfaces

The hard-surface analyser smooths face normals with bilateral weighting:

```text
spatial weight
*
normal-similarity weight
```

Conceptually:

```text
w_spatial = exp(-|c_i-c_j|² / (2 sigma_s²))
w_normal  = exp(-|n_i-n_j|² / (2 sigma_r²))
w = w_spatial * w_normal
```

This reduces noisy normal variation while retaining meaningful discontinuities.

Jerico does not need this exact implementation, but the design should preserve the distinction:

```text
geometric feature detection != raw imported vertex-normal discontinuity
```

Geometry should be classified from surface shape and explicit authoring semantics.

---

## 10. Curvature estimation

The addon's custom mesh analyser estimates principal curvature by fitting a local 2x2 second fundamental form over each vertex 1-ring.

For tangent basis `(u,v)`, it solves for coefficients corresponding to:

```text
II = [ a b ]
     [ b c ]
```

from changes in vertex normals across incident edges.

The eigenvalues provide principal curvatures:

```text
k1, k2
```

and the eigenvector provides a principal direction.

Jerico can use the same mathematical concept for:

- orientation-field initialization;
- adaptive density;
- curvature-aware error metrics;
- smooth/hard region analysis.

This should live in a shared mesh-analysis module, not inside a UI tool.

---

## 11. 4-RoSy orientation field

For quad remeshing, every surface point owns a cross rather than one directed vector.

If `o` is one direction and `n` is the surface normal, the equivalent directions are:

```text
 o
 cross(n,o)
-o
-cross(n,o)
```

When comparing neighboring directions, choose whichever of those four best aligns with the target.

The addon's implementation effectively computes:

```text
bestRosy(target, neighbour, normal)
```

before averaging.

This avoids the classic bug where two equivalent cross directions cancel merely because one is rotated by 90 degrees.

---

## 12. Feature-constrained orientation field

This is the most important hard-edge technique in the addon.

Feature edges are not repaired after remesh. They constrain the field **before topology extraction**.

For feature vertices:

1. project feature-edge tangents into the local tangent plane;
2. represent the cross angle with `exp(i * 4 theta)` semantics;
3. average compatible crease directions;
4. lock the orientation field to the resulting feature direction.

The conceptual representation is:

```text
z = exp(i * 4 theta)
```

so creases separated by 90 degrees reinforce instead of cancel.

Jerico should implement feature constraints as first-class solver inputs.

---

## 13. Corner detection and pinning

A vertex where a crease changes direction cannot safely slide along a feature line.

The addon detects low coherence in the feature cross representation and also marks vertices touched by 3+ crease segments as corners.

Conceptually:

```text
feature vertex with one coherent line
    -> may slide only along feature tangent

feature corner / junction
    -> fully pinned
```

This is a critical rule for preserving box corners and mechanical intersections.

Jerico constraint classes:

```text
FreeSurfaceVertex
FeatureSlidingVertex
PinnedFeatureCorner
BoundarySlidingVertex
PinnedBoundaryCorner
```

---

## 14. Weighted field smoothing

Feature edges receive greater solver weight than ordinary neighborhood edges.

The supplied custom solver uses a stronger feature weight so a smooth surrounding field cannot pull orientation away from the crease.

Jerico should expose feature weights internally but avoid arbitrary artist-facing complexity unless tests prove it useful.

---

## 15. Multiresolution orientation solve

The addon observed an important convergence problem: local Jacobi sweeps on a fine mesh can settle into poor local minima before information propagates across the model.

Its orientation solver therefore builds a coarse hierarchy:

```text
fine
-> restrict
-> coarse solve
-> prolong
-> fine solve
```

This is a strong candidate for Jerico because:

- Jacobi is simple;
- it parallelizes well;
- coarse-to-fine improves global coherence;
- GPU implementation is plausible later.

Do not prematurely GPU this. First freeze a deterministic CPU reference.

---

## 16. Position field

After orientation, the addon solves a local position/lattice field.

Each vertex carries a local lattice point. Neighbor agreement is only required modulo integer grid cells.

For an edge `(a,b)` with local orientation `o` and perpendicular tangent `t`:

```text
delta = p_b - p_a

k_o = round(dot(delta, o_a) / scale_edge)
k_t = round(dot(delta, t_a) / scale_edge)

q = p_b - scale_edge * (k_o * o_a + k_t * t_a)
```

Neighbor estimates are averaged with an anchor to the original source position.

After each iteration, drift is projected into the tangent plane:

```text
p = V + tangentProject(p - V, normal)
```

and a hard half-cell leash prevents the local lattice point from wandering too far from its source vertex.

---

## 17. Feature-constrained position solve

For a normal feature vertex:

```text
movement = project(movement, featureTangent)
```

For a feature corner:

```text
position = originalPosition
```

This is exactly the behavior Jerico needs:

```text
smooth surface -> may slide tangentially
hard line      -> may slide only along line
hard corner    -> may not move
```

---

## 18. Adaptive sizing field

The addon uses a per-vertex target edge length rather than only one global size.

Desired target size is driven by several requests, combined by taking the most restrictive:

```text
feature proximity
curvature
concave/convex importance
thin-region protection
optional source mesh density
```

Final size is clamped to:

```text
base * minRatio <= size <= base
```

or a wider range for explicit source-density matching.

---

## 19. Feature-distance refinement

Feature edges seed a geodesic-like graph distance over the mesh.

The addon uses repeated edge relaxation:

```text
value_i <= value_j + slope * edgeLength(i,j)
```

With feature vertices initialized to zero, this becomes a simple graph distance transform.

Target size ramps from the minimum at the feature to the base size outside a configurable band.

Jerico can implement this efficiently with a priority-queue shortest path or parallel relaxation depending on mesh size.

---

## 20. Gradient-limited sizing

A target-size field that jumps abruptly from large to tiny elements is geometrically impossible for a clean quad layout.

The addon enforces a Lipschitz-like constraint:

```text
size_i <= size_j + gradientLimit * edgeLength(i,j)
```

This is one of the most important quality rules in the package.

Jerico should make it mandatory internally for adaptive remesh.

---

## 21. Thin-region protection

The addon detects local thickness by ray casting inward along the normal and measuring the opposing surface.

Where a solid is thin, requested quad size is reduced.

The reason is practical: if cells become wider than a thin sheet, opposite sides can merge or produce broken topology.

Jerico should support this as an optional expensive analysis pass for:

- cloth-like static meshes;
- thin metal;
- window frames;
- shells;
- scanned assets.

It is unnecessary for every mesh.

---

## 22. Density matching

An alternative mode copies the *authoring density* of the source mesh rather than deriving density only from curvature.

Local source edge length is averaged at each vertex, normalized so the global average still corresponds to the requested base size, and used as a sizing field.

This can preserve intentionally dense decorated areas even when they are locally flat.

Jerico should call this something explicit such as:

```text
DensityMode::GeometryDriven
DensityMode::SourceDensity
DensityMode::Uniform
```

Do not infer artistic intent silently from source density in the default mode.

---

## 23. Lattice collapse and topology extraction

Once orientation and position fields are solved, source vertices are clustered into lattice sites.

The collapsed clusters provide:

```text
output positions
output normals
output orientation
output scale
```

Source adjacency is then transformed into an output graph and faces are extracted from oriented adjacency cycles.

This is where singularities in the cross field naturally become irregular valence/poles.

Jerico's first prototype does not need to promise pure quads. A **quad-dominant** result with explicit quality metrics is the safer target.

---

## 24. Why pure quads are not always the correct constraint

The addon's adaptive subdivision experiments explicitly document a key problem: enforcing pure quads during local refinement can cause split propagation across large parts of the model.

A locally refined quad with an odd number of split edges creates an odd boundary; forcing all-quad topology requires propagating extra splits to neighbors.

The addon therefore supports allowing a small number of triangles in transition rings to preserve locality.

Jerico recommendation:

```text
AssetRetopoRemesh default -> quad-dominant, local transitions allowed
PureQuad mode             -> explicit expensive/offline authoring option
```

Never sacrifice bounded/local behavior merely to display "100% quads".

---

## 25. Hard-surface panelization

The hard-surface module contains a second topology strategy that is extremely relevant to Jerico.

Reference hard-surface assets often do not use a uniform quad grid over flat panels. Instead:

```text
flat panel interior -> one n-gon
chamfer / bevel band -> dense geometry
feature edge         -> retained boundary
```

The addon measured an example where only a small fraction of edges lie in flat panel interiors because those interiors have been dissolved away.

Pipeline:

```text
1. group faces into flat/coplanar regions
2. extract closed boundaries
3. simplify the boundary network globally
4. inset boundaries
5. construct chamfer bands
6. represent interior panel as one n-gon
```

This is not the same product as a deformable character retopo mesh.

Jerico should expose hard-surface panelization as a distinct policy/preset under AssetRetopoRemesh.

---

## 26. Boundary simplification

The addon uses Douglas-Peucker-style polyline simplification on panel boundaries.

For segment endpoints `A,B`, interior points are removed when their maximum distance to segment `AB` falls below a tolerance.

This can drastically reduce unnecessary boundary vertices while preserving silhouette.

Jerico should additionally protect:

```text
feature corners
material boundaries
UV seams
user locks
high-curvature silhouette samples
```

from simplification.

---

## 27. Loop thinning

The addon can reduce topology by removing entire loops rather than arbitrary isolated edges.

It scores how much shape is lost when a loop disappears and avoids loops that carry hard edges or rings supporting those hard edges.

This is a much better principle for quad topology than ordinary triangle decimation.

Potential Jerico use:

```text
quad retopo simplification
LOD preparation
hard-surface cleanup
```

This is separate from runtime triangle LOD simplification.

---

## 28. Smooth shading is not geometric smoothness

The Cracker analysis earlier exposed `Set Shade Smooth` and explicit normal handling. Smart Remesh makes the deeper distinction clearer:

```text
Topology / vertex positions define geometric shape.
Normals define shading continuity.
Hard edges may require both geometric feature preservation and normal splits.
```

Jerico must never use "smooth normals" as a substitute for preserving a crease geometrically.

---

## 29. Normal reconstruction policy

Jerico's canonical `MeshVertex` currently contains:

```text
position
normal
tangent
texCoord0
```

A remesh changes topology, so vertex normals and tangents must be rebuilt.

Recommended rules:

### Smooth region

Area/angle-weight adjacent face normals within the same smoothing island.

### Geometric hard feature

Split vertex shading across the feature or otherwise emit separate corner vertices in the cooked indexed mesh.

### Authored normal provenance

If future requirements need transfer of high-poly custom normals, transfer them as a surface attribute from the source and then re-orthogonalize tangent frames. Do not average through a hard feature.

### Tangents

Recompute from final UVs + final normals; never preserve stale tangents after topology changes.

---

## 30. UV handling

A remesher that dissolves or reconnects edges can destroy UV seams.

Smart Remesh explicitly contains seam-preservation logic in panel/loop processing.

Jerico needs an explicit policy:

```text
UVPolicy::PreserveSeams
UVPolicy::TransferAndReunwrap
UVPolicy::DiscardAndGenerate
```

A UV seam must optionally become a topological protection line during cleanup.

For geometry generation where UVs are regenerated procedurally, seam preservation may be unnecessary.

---

## 31. Material / region provenance

Jerico already relies heavily on stable regions and material lineage.

The remesh must preserve or transfer:

```text
material region
surface classification
geology classification where applicable
fracture/exposed semantics where applicable
source provenance
```

Recommended transfer methods:

- nearest source triangle + barycentric coordinates;
- closest-point projection against source BVH;
- source region chosen from projected carrier triangle;
- ties resolved deterministically.

Do not infer a material from interpolated vertex labels.

---

## 32. The addon's smoothing weakness

The custom remesher contains an optional Laplacian-like vertex relaxation after face extraction.

The addon itself documents why it is disabled by default:

```text
there is no reprojection step
-> smoothing pulls vertices off the source surface
-> measured surface deviation increases
```

This is a direct opportunity for Jerico to be better.

---

## 33. Jerico feature-constrained smooth/reprojection loop

Recommended relaxation:

```text
for each iteration:
    propose tangential fairing displacement

    if pinned corner:
        displacement = 0

    else if feature vertex:
        displacement = project(displacement, feature tangent)

    else:
        displacement = tangentProject(displacement, source normal)

    candidate = current + displacement

    candidate = closestPointOnSource(candidate)

    if feature vertex:
        candidate = closestPointOnSourceFeaturePolyline(candidate)

    if pinned corner:
        candidate = original feature corner
```

This preserves the source surface while still evening cell sizes.

A BVH / spatial acceleration structure makes the projection practical offline/editor-side.

---

## 34. Projection targets must carry provenance

Do not project merely to "the closest triangle" without safeguards around thin shells and nearby disconnected pieces.

Each output vertex should retain a source carrier or allowed source-region set where possible.

Candidate projection should prefer:

```text
same source component
same material/region
same side of a thin sheet
same feature branch for constrained vertices
```

Fallback to unrestricted closest-point projection only when the bounded carrier cannot be recovered.

This avoids vertices snapping across thin gaps.

---

## 35. Geometry fidelity metric

Topology quality alone is not enough.

The addon scores:

```text
quad purity
valence regularity
watertightness
manifoldness
face aspect quality
```

Jerico should add geometric fidelity metrics:

```text
max source distance
RMS source distance
normal deviation
feature Hausdorff distance
boundary deviation
volume drift for closed solids
```

Hard-surface acceptance must explicitly measure feature-line preservation.

---

## 36. Recommended Jerico quality report

```cpp
struct RemeshQualityReport
{
    uint32_t vertices;
    uint32_t faces;
    uint32_t quads;
    uint32_t triangles;
    uint32_t ngons;

    uint32_t boundaryEdges;
    uint32_t nonManifoldEdges;
    uint32_t extraordinaryVertices;

    double quadAreaRatio;
    double rmsSurfaceErrorMeters;
    double maxSurfaceErrorMeters;
    double maxFeatureErrorMeters;
    double maxNormalErrorDegrees;

    double minEdgeMeters;
    double maxEdgeMeters;
    double worstAspectRatio;
};
```

Preview should show these metrics before Apply.

---

## 37. Recommended data model

Conceptual only; exact repository ownership must be selected during a future admitted slice.

```cpp
enum class RemeshMode
{
    AdaptiveSurface,
    AssetRetopo,
    HardSurfacePanels
};

enum class FeatureDetectionMode
{
    Auto,
    Angle,
    AuthoredOnly
};

struct RemeshRecipe
{
    RemeshMode mode;
    uint32_t targetFaces;
    float targetEdgeMeters;

    FeatureDetectionMode featureMode;
    float creaseAngleDegrees;

    bool preserveBoundaries;
    bool preserveUvSeams;
    bool preserveMaterialBoundaries;

    float featureDensity;
    float curvatureDensity;
    float minimumSizeRatio;
    float sizingGradientLimit;

    uint32_t orientationIterations;
    uint32_t positionIterations;
    uint32_t relaxIterations;

    uint64_t seed;
};
```

The recipe must be deterministic and hashable.

---

## 38. Proposed module ownership

Avoid one giant `Remesher.cpp`.

Suggested conceptual split:

```text
Tools/GeometryProcessing/
    MeshAnalysis
        Topology
        Normals
        Curvature
        FeatureDetection
        Thickness

    Remesh
        CrossField
        PositionField
        SizingField
        LatticeExtraction
        SurfaceProjection
        Relaxation
        AttributeTransfer
        QualityMetrics

    HardSurface
        PanelClassifier
        BoundaryGraph
        PanelSimplify
        ChamferBand
        LoopSimplify
```

This math can be consumed by editor tools, cook tools and tests without depending on Qt or Blender.

---

## 39. CPU first, GPU later

The orientation and position solvers are heavily local and parallelizable. Jacobi-style updates map naturally to compute.

However:

```text
CPU double-precision reference first
-> deterministic fixtures
-> topology extraction correctness
-> feature-preservation evidence
-> only then evaluate GPU acceleration
```

GPU acceleration should target analysis/field iterations on very large meshes, not complicate the first proof.

---

## 40. Reuse versus Geology Surface Nets/QEF

Do **not** merge this remesher with the current geology Surface Nets/QEF extractor.

They solve different problems:

```text
Surface Nets/QEF
    scalar field -> surface extraction

Asset remesher
    existing surface -> new surface topology
```

They should share low-level utilities where sensible:

```text
vector math
BVH / closest point
normals/tangents
feature constraints
quality metrics
attribute provenance
```

but remain separate semantic products.

---

## 41. Relation to the existing Jerico 1.0 target

The repository already names both:

```text
AdaptiveSurfaceRemesh
AssetRetopoRemesh
```

This research provides a plausible implementation direction rather than inventing a new goal.

Recommended mapping:

### AdaptiveSurfaceRemesh

Use for generated/SDF/dense working surfaces where topology quality and bounded adaptive density matter.

Likely priorities:

```text
surface fidelity
adaptive density
watertightness
manifoldness
classification transfer
```

Pure quads are not mandatory.

### AssetRetopoRemesh

Use for imported/static assets requiring clean authoring topology.

Priorities:

```text
feature lines
hard edges
quad flow
UV/material seam policy
panelization presets
artist preview/control
```

---

## 42. Proposed prototype R1 — feature detection only

No remesh yet.

Fixture set:

```text
cube with bevel
cylinder with hard caps
mechanical bracket
smooth sphere
organic scan
thin plate
mixed flat + curved shell
```

Output overlays:

```text
red   = detected hard feature
blue  = boundary
orange= panel transition
white = ordinary edge
points= pinned feature corners
```

Acceptance:

- deterministic;
- no fake fan-diagonal features;
- authored boundaries always retained;
- cube/mechanical sharp lines recovered;
- smooth sphere does not become over-segmented.

---

## 43. Prototype R2 — 4-RoSy orientation field

Implement CPU reference.

Acceptance:

- field remains tangent to source;
- 90-degree equivalent directions do not cancel;
- locked feature vertices align to feature tangent;
- box corners remain pinned;
- repeated run produces identical field hash;
- visual debug glyphs show coherent cross flow.

---

## 44. Prototype R3 — position field + lattice extraction

Acceptance:

- simple plane -> regular grid;
- cylinder -> coherent wrapped quads;
- box -> field respects all crease lines;
- no non-finite output;
- closed fixture remains closed where algorithm admits closure;
- log holes/non-manifold results explicitly rather than hiding them.

Quad-dominant is sufficient.

---

## 45. Prototype R4 — reprojection + constrained smooth

This is the Jerico improvement over the supplied custom remesher.

A/B:

```text
A = extracted topology, no smooth
B = 1..N constrained relaxation iterations + source reprojection
```

Measure:

```text
RMS source deviation
max source deviation
feature deviation
aspect-ratio distribution
normal deviation
```

Pass only if smoothing improves topology/spacing without increasing error beyond the declared bound.

---

## 46. Prototype R5 — adaptive sizing

Test separately:

```text
crease proximity
curvature
source density
thin region
```

Then combine.

Acceptance requires size-field gradient bounds and an output face count that remains near the requested target after field rescaling.

---

## 47. Prototype R6 — hard-surface panel mode

Use a bevelled mechanical fixture.

Expected structure:

```text
large flat regions -> n-gon panels
bevel/chamfer regions -> retained dense strips
feature boundaries -> exact topological borders
```

Measure:

```text
panel normal error
boundary deviation
feature line deviation
face-count reduction
manifoldness
```

Do not run this mode on character/organic deformation meshes.

---

## 48. Prototype R7 — attribute transfer

Prove transfer of:

```text
material/submesh
UV or generated UV policy
normal discontinuity policy
surface/region classification
```

A remesh is not production-ready if topology looks good but the material lineage is lost.

---

## 49. Performance strategy

Offline/editor remesh is allowed to be heavier than runtime rendering, but still needs bounded work.

Recommended accelerators:

```text
CSR adjacency
SoA arrays
BVH closest-point queries
multiresolution hierarchy
parallel/Jacobi field sweeps
spatial hash for local transfers
bounded iterations
cancellation checks
```

No per-frame remeshing is implied by this research.

---

## 50. Failure modes that must remain explicit

A future implementation must report rather than conceal:

```text
non-manifold source
self intersections
zero-area faces
invalid normals
field non-convergence
lattice collapse
unfilled extraction cycles
feature graph contradictions
projection to wrong component
size field below source resolution
budget overflow
```

Do not publish a mesh merely because it can be rendered.

---

## 51. What not to do

Do not:

1. voxel-remesh every imported asset just to obtain uniform topology;
2. use smooth normals to fake missing geometric creases;
3. run unconstrained Laplacian smooth over hard edges;
4. rebuild hard-surface flat panels as dense uniform quad grids by default;
5. make pure quads mandatory when a local triangle transition is cheaper and cleaner;
6. reuse the geology QEF extractor as an asset-retopo system;
7. discard UV/material/region provenance;
8. apply global target density near small details without a gradient-limited field;
9. copy proprietary addon implementation into Jerico.

---

## 52. Practical first target for Jerico

The first useful product is not "beat Blender at every retopology case".

It is:

```text
Select static mesh
-> Remesh Preview
-> choose target face count
-> Auto Detect Features
-> preserve sharp/material/UV boundaries
-> Adaptive density ON/OFF
-> Preview original vs result
-> inspect quality report
-> Apply / Undo / Redo
```

with these presets:

```text
Organic / Smooth
Hard Surface
Preserve Source Density
Generated Surface Cleanup
```

Hard Surface additionally exposes panelization when appropriate.

---

## 53. C++ mathematical skeleton

Conceptual reference only:

```cpp
FeatureGraph features = detectFeatures(source, recipe.featurePolicy);
SizingField size = buildSizingField(source, features, recipe);

CrossField orientation = solveCrossField(
    source,
    features,
    recipe.orientationIterations);

combCrossField(source, orientation);

PositionField positions = solvePositionField(
    source,
    orientation,
    size,
    features,
    recipe.positionIterations);

RemeshGraph graph = collapseToLattice(
    source,
    orientation,
    positions,
    size);

Mesh result = extractQuadDominantMesh(graph);

SurfaceCarrier carrier(source);
result = reproject(result, carrier, features);
result = constrainedRelax(result, carrier, features, recipe.relaxIterations);

transferAttributes(source, result, carrier, recipe.attributePolicy);
rebuildNormalsAndTangents(result, features, recipe.shadingPolicy);

RemeshQualityReport report = evaluateRemesh(source, result, features);
```

---

## 54. Hard-edge constrained relaxation skeleton

```cpp
for (VertexId v : result.vertices())
{
    if (constraints.isPinnedCorner(v))
        continue;

    Vec3 delta = fairingProposal(result, v);

    if (constraints.isFeatureVertex(v))
    {
        const Vec3 tangent = constraints.featureTangent(v);
        delta = tangent * dot(delta, tangent);

        Vec3 q = result.position(v) + strength * delta;
        result.position(v) = carrier.closestPointOnFeature(
            constraints.featureId(v), q);
    }
    else
    {
        const Vec3 normal = carrier.normalAtCarrier(v);
        delta -= normal * dot(delta, normal);

        Vec3 q = result.position(v) + strength * delta;
        result.position(v) = carrier.closestAllowedPoint(v, q);
    }
}
```

This is the crucial difference between generic smoothing and **feature-preserving smoothing**.

---

## 55. Decision

The research supports proceeding later with a Jerico-native remesher.

Recommended direction:

```text
DO:
    independent C++ implementation
    feature-constrained field-guided remesh
    adaptive sizing
    reprojection-based smoothing
    hard-surface panel mode
    explicit quality metrics

DO NOT:
    depend on Blender
    depend on Smart Remesh runtime
    blindly voxelize imported meshes
    erase sharp features and reconstruct them only from normals afterward
```

The strongest concepts worth carrying into Jerico are:

1. **4-RoSy feature-constrained orientation field**;
2. **feature-sliding vertices + pinned corners**;
3. **gradient-limited adaptive sizing**;
4. **automatic crease + panel-transition detection**;
5. **quad-dominant extraction rather than dogmatic pure-quads**;
6. **hard-surface panelization with topology concentrated in chamfers/edges**;
7. **surface reprojection after relaxation** — Jerico improvement over the custom addon path;
8. **quality/evidence-driven candidate selection**.

This is sufficient to justify a future implementation slice, but this document by itself is not implementation evidence.
