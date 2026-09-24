# M8 Zen UV 5.4.0.11 — Source-Inspected Technical Research Input

**Milestone:** M8  
**Category:** UV charting / seams / unwrap-relax / topology matching / texel density / trims / packing / UDIM  
**Status:** complete source package inspected; research input ready; native Jerico implementation not started  
**Owner timing:** read and act on this only when M8 is the legal `ACTIVE_SLICE`  
**Runtime dependency:** none intended  
**Inspected archive SHA-256:** `6347b67f91087b2f4a246afd3d457ec402751500af0a2682b2f1488fc7b5a3db`

This replaces the earlier incomplete-package analysis. The second archive supplied by
the owner contains the actual Zen UV 5.4.0 source tree and was inspected directly.
It is a durable M8 research specification for independent Jerico implementations;
it does not replace `ACTIVE_SLICE.md`, `HANDOFF.md`, the M8 closure plan, or the M8
Blender/geology authority.

## 1. Package inspection

The inspected package identifies itself as:

```text
id      = ZenUV
version = 5.4.0
Blender >= 4.2.0
license = GPL-3.0-or-later
```

The archive contains approximately:

```text
364 files
265 Python files
114,159 Python lines
```

Relevant source families include:

```text
ops/auto_seams/
ops/pack_sys/
ops/reshape/
ops/select_sys/
ops/texel_density/
ops/transform_sys/
ops/trimsheets/
stacks/
utils/clib/
utils/shortest_path.py
ops/relax.py
ops/quadrify.py
```

The useful lesson is not the Blender UI. Zen UV contains reusable classes of
algorithms for **partitioning a surface, comparing topology, parameterizing UVs,
measuring scale and mapping charts to authored regions**.

## 2. M8 interpretation

Zen UV is not a geology generator. It belongs to M8's asset-preparation and mesh
analysis side and can serve:

- generated rock/cliff/cave outputs;
- imported environment meshes;
- low-poly/remesh results before baking;
- texture-density normalization;
- trim-sheet/hotspot assignment;
- duplicated/modular topology detection;
- UV validation before texture cook;
- LOD/reimport consistency.

Jerico's target is a native, headless-capable toolchain. Blender/Zen UV may be an
offline reference/adapter but must not be required by Shipping.

---

# Part A — Automatic chart/seam generation

## 3. Scientific chart merger

`ops/auto_seams/scientific_chart_merger.py` contains the most generally reusable
piece of the add-on. It treats UV island creation as a **surface graph clustering
problem**.

High-level pipeline:

```text
mesh faces
  -> face geometry + adjacency
  -> mandatory hard boundaries
  -> one chart per face
  -> priority queue of adjacent-chart merge costs
  -> union-find greedy merges
  -> boundary smoothing
  -> tiny-chart absorption with topology guards
  -> seams from final chart boundaries
```

The inspected implementation stores enough statistics per chart to evaluate a
merge incrementally instead of recomputing the whole region:

- total surface area;
- area-weighted normal sum;
- area-weighted centroid sum;
- perimeter;
- second spatial moments (`x²`, `y²`, `z²`, `xy`, `xz`, `yz`);
- average curvature signal;
- shared-boundary length/dihedral information.

This data model is reusable beyond UVs: semantic mesh regions, fracture/material
regions, LOD clusters and procedural-selection tools can use the same pattern.

## 4. Face curvature approximation

The source computes a local normal-variation proxy approximately as:

```text
curvature(f) = mean_neighbors(1 - clamp(dot(n_f, n_j), -1, 1))
```

It is inexpensive, scale independent and useful as a feature signal. Jerico should
name it explicitly as **normal variation**, not claim it is a full differential
principal-curvature solution.

Potential consumers:

- seam preference;
- segmentation;
- edge wear/masks;
- remesh sizing;
- feature-preserving simplification;
- surface classification.

## 5. Hard boundaries

The merger prevents crossing:

- mesh boundaries;
- explicitly sharp/hard boundaries;
- edges whose dihedral exceeds the configured threshold.

The inspected defaults expose a hard-edge threshold in degrees and note practical
hard-surface ranges around 30–60 degrees. Jerico should never bake those values
into the algorithm: hard-boundary policy belongs in the recipe.

## 6. Merge-cost model

The source's default quality threshold is approximately `0.32`, but the important
part is the **multi-term normalized cost**. The inspected default weights are:

```text
normal deviation     0.28
planarity variance   0.28
boundary dihedral    0.17
compactness          0.15
curvature difference 0.12
```

### 6.1 Normal deviation

For merged chart area `A` and area-weighted normal sum `S_n`:

```text
normal_deviation = 1 - length(S_n) / A
```

A planar/coherent region has aligned normals and therefore a low cost.

### 6.2 Planarity from second moments

The chart keeps first and second spatial moments. Projecting the covariance-like
second moment onto the chart average normal provides a plane-thickness/variance
measure without iterating every face again.

Jerico should generalize this as a reusable incremental region statistic:

```text
RegionMoments {
    area
    sum_position
    sum_outer_product
    sum_normal
}
```

From this, a covariance matrix and best-fit plane error can be reconstructed.

### 6.3 Boundary dihedral

The source uses shared-boundary-length-weighted dihedral, normalized by pi.

Conceptually:

```text
boundary_angle =
    sum(shared_length_e * dihedral_e)
  / sum(shared_length_e)
  / pi
```

Merging across a strong fold is therefore more expensive.

### 6.4 Compactness

The inspected chart compactness term is based on the classical isoperimetric
ratio:

```text
q = P² / (4*pi*A)
compactness_penalty = max(0, q - 1) / scale
```

A circle gives the theoretical minimum. Thin, winding charts are penalized.

This metric is useful for more than UVs: cluster quality, nav regions, decal masks
and semantic segmentation can all use a compactness prior.

### 6.5 Curvature difference

The chart-average curvature difference is scaled and clamped before contributing
to the merge cost. This discourages joining regions with clearly different surface
behavior.

### 6.6 Combined cost

Conceptually:

```text
cost =
    0.28 * normal_deviation
  + 0.28 * planarity_error
  + 0.17 * boundary_dihedral
  + 0.15 * compactness_penalty
  + 0.12 * curvature_difference
```

Jerico should expose all weights in a versioned recipe. Do not freeze Zen UV's
numbers as engine constants.

## 7. Greedy hierarchy

The source uses a heap plus disjoint-set/union-find structure:

```text
push all adjacent chart-pair costs
while best_cost <= threshold:
    validate stale heap entry
    union charts
    incrementally update statistics
    enqueue new neighboring pair costs
```

This gives roughly the same architectural advantage observed in Smart Segmentation:
a hierarchy/region graph can support cheap quality/count adjustments after the
expensive geometry analysis.

## 8. Boundary smoothing

After greedy merging, the implementation may move a boundary face into one unique
neighbor chart when doing so drastically reduces boundary length. The default
`smooth_threshold` is around `0.20`.

Jerico equivalent should be an energy-reducing local refinement, never an
unconditional smoothing pass.

## 9. Tiny-chart absorption with topology safety

The source does not simply merge every tiny island into the nearest neighbor.
Important guards include:

- avoid ambiguous multi-neighbor/T-junction cases;
- test post-merge boundary connectivity;
- reject changes that create pathological open boundary ends.

This is valuable general policy: **region cleanup must preserve topology, not only
optimize scalar cost**.

## 10. Chart-quality metrics

The source tracks metrics such as:

- chart/island count;
- seam count;
- spread of chart area percentages;
- open boundary ends;
- min/max chart area.

Jerico should retain before/after metrics and make automated generation reviewable.

---

# Part B — Texel density

## 11. UV polygon area

The inspected texel-density utility uses the shoelace formula for UV polygon area:

```text
A_uv = 0.5 * abs(sum_i(x_i*y_{i+1} - y_i*x_{i+1}))
```

For triangles this reduces to the standard 2D cross-product area.

## 12. Source texel-density formula

The inspected implementation combines UV area, world/mesh area, texture size and
unit conversion. In simplified form:

```text
max_side    = max(texture_width, texture_height)
sqrt_aspect = sqrt(max(width/height, height/width))
A_geo       = sum(face_surface_area)
A_uv        = sum(uv_polygon_area)

TD = max_side * sqrt(A_uv)
   / (sqrt_aspect * sqrt(A_geo) * unit_scale)
```

It also corrects for object/world scale and chosen display units.

Jerico should define its own unit-safe canonical metric, preferably in pixels per
meter/centimeter, but this confirms the important invariant:

```text
linear texel density ~ sqrt(texture_texel_area / surface_area)
```

Required tests:

- texture resolution x2 -> linear TD approximately x2;
- geometric area x4 -> linear TD approximately x1/2;
- non-uniform object transform handled explicitly;
- stacked islands do not corrupt source-surface TD reporting;
- rectangular textures handled consistently.

---

# Part C — topology-aware stacking

## 13. Matching modes

The inspected stacks subsystem implements three useful semantic levels:

```text
Exact Match
Topology & Scale
Topology Only
```

This is useful well outside UV stacking. Jerico can reuse topology matching for:

- modular-mesh deduplication;
- instancing candidates;
- repeated destruction pieces;
- LOD correspondence;
- UV transfer;
- semantic part correspondence.

## 14. Topology fingerprint

One inspected fingerprint sorts per-vertex descriptors similar to:

```text
(vertex_degree, is_border)
```

This is a cheap rejection test before a real graph-isomorphism solve.

Jerico should use a staged matcher:

```text
cheap counts
-> degree/border signature
-> geometric/intrinsic fingerprints
-> canonical graph/isomorphism verification
-> correspondence map
```

Never use a hash collision as proof of equality.

## 15. Border-length fingerprint

Boundary edges are normalized by average boundary-edge length, quantized and
sorted. This provides a scale-invariant shape descriptor:

```text
r_i = boundary_edge_length_i / mean_boundary_edge_length
fingerprint = sort(quantize(r_i))
```

Useful for rejecting topologically similar but geometrically different candidates.

## 16. Radial fingerprint

The source also derives boundary-vertex distances from a boundary centroid,
normalized by the mean distance. This is another scale-normalized intrinsic-ish
shape descriptor.

Jerico can combine several weak fingerprints before invoking an expensive exact
matcher.

## 17. Exact geometric key

The exact mode incorporates values such as:

- vertex/edge/face counts;
- mesh area;
- perimeter;
- topology fingerprint.

Scale/topology modes progressively omit absolute measures.

## 18. Graph disambiguation

The source adds auxiliary graph structures for ambiguous polygons/borders before
solving correspondence. The transferable lesson is important:

> raw vertex adjacency alone may not encode enough orientation/face-loop context to
> recover a stable element mapping.

Jerico's canonical topology graph should encode vertices, edges **and face-loop
incidence**, or use a half-edge/bipartite representation that preserves cyclic face
structure.

## 19. Native implementation target

Suggested Jerico service:

```text
MeshTopologyMatcher
  FastSignature(mesh, mode)
  AreCandidatesCompatible(a, b, mode)
  SolveCorrespondence(a, b, mode)
```

The output correspondence should map stable source elements, not Blender indices.

---

# Part D — relax / unwrap

## 20. ZenRelax source inspection

The package includes C++ source for ZenRelax. It explicitly incorporates/references
GPL algorithms/code from SLIM/libigl-family work. The inspected path performs
validation such as:

- single connected component;
- edge-manifold requirement;
- disk topology via Euler characteristic;
- zero-area face rejection.

It normalizes total mesh area before solving and configures a global/local
parameterization in the family of:

```text
GLOBAL_ARAP_IRLS
SYMMETRIC_DIRICHLET energy
flip-aware line search
small proximal regularization
```

### Critical boundary

This C++ implementation is **research evidence only**. Do not copy it into Jerico
production code. Its value is that it identifies a solver class and validation
requirements to benchmark.

Jerico options later:

1. implement an independent solver from papers/math;
2. select an admitted permissive library with equivalent capability;
3. retain Blender/ZenRelax only as an optional offline external reference if ever
   legally useful.

The default Jerico/open-source path should remain license-flexible.

## 21. Relax proof gates

Any native solver should report:

- flips before/after;
- symmetric Dirichlet or selected distortion energy;
- max/mean angular distortion;
- area distortion;
- solver residual;
- iteration count/time;
- reason for invalid input.

Visual appearance alone is not a PASS.

---

# Part E — trims / hotspot mapping

## 22. Hotspot scoring

The inspected hotspot mapper compares island and trim descriptors including:

- bounding-box area;
- aspect ratio, including rotated orientation;
- world/surface size where requested;
- authored trim metadata/tags.

A simple observed selection pattern is effectively:

```text
score = area_difference + aspect_difference
```

with thresholding and optional world-size priority.

Jerico should generalize it into explicit weighted terms:

```text
score =
    w_area   * normalized_area_error
  + w_aspect * normalized_aspect_error
  + w_world  * world_size_error
  + w_tags   * semantic_penalty
```

Then fit through a declared 2D similarity/affine transform.

## 23. Trim representation

Recommended engine-independent authoring contract:

```text
TrimRegion {
    stable_id
    uv_bounds
    tags
    expected_aspect
    expected_world_size
    rotation_policy
    padding_policy
}
```

This can later serve decals, atlas regions and authored material modules, not just
classic trim sheets.

---

# Part F — structured quad UV propagation

## 24. Quadrify concept

The inspected quadrify subsystem propagates UV coordinates over connected quad
structure from a master edge, deriving an orthogonal 2D direction and scaling it
from neighboring geometry/UV edge ratios.

This is a useful specialized solver for regular grid-like surfaces:

```text
seed edge UV frame
-> traverse quad adjacency
-> infer next orthogonal edge
-> preserve geometric edge-length ratio
-> propagate coherent rectangular UV grid
```

Potential Jerico uses:

- walls/floors;
- stairs/platforms;
- modular panels;
- pipes/strips after parameterization;
- generated hard-surface surfaces.

Do not run a heavy global relax when topology already carries a strong grid
parameterization.

---

# Part G — graph utilities

## 25. Shortest path

The source includes heap-based Dijkstra-style graph traversal for mesh/UV tasks.
Jerico should not create a UV-only shortest-path implementation; use one generic
weighted graph service or mesh-topology traversal layer.

Potential weights:

- edge length;
- curvature/dihedral penalty;
- seam preference;
- semantic masks;
- protected/forbidden edges.

This can support seam suggestions, region boundaries and procedural selections.

---

# Part H — packing / UDIM / validation architecture

## 26. Packing contract

Even when implementation differs from Zen UV, M8 should define one deterministic
packing service with:

- deterministic seed;
- pixel-aware padding;
- allowed rotations;
- locked/pinned islands;
- stack groups as one logical footprint;
- UDIM tile constraints;
- occupancy/waste metrics;
- progress/cancellation;
- before/after diff.

Global search quality tiers can be added later; bounded deterministic behavior is
the first requirement.

## 27. Distortion diagnostics

Jerico should derive a per-triangle differential map between world tangent space
and UV space. Singular values of the 2x2 Jacobian provide:

```text
area scale  = abs(det(J))
anisotropy  = max(sigma1,sigma2) / max(min(sigma1,sigma2), eps)
orientation = sign(det(J))
```

This supports stretch, scale distortion and flipped-UV diagnostics independent of
any DCC.

---

# Part I — shared Jerico architecture

## 28. Proposed services

Do not reproduce Zen UV's Blender module structure. Suggested Jerico boundaries:

```text
UvIslandAnalyzer
SurfaceChartPartitioner
SurfaceRegionMoments
UvParameterizationSolver
TexelDensityService
MeshTopologyMatcher
UvStackService
TrimMappingService
UvPackService
UvValidationService
```

Shared with other M8 research:

```text
Mesh adjacency / half-edge topology
Signed dihedral / normal variation
Stable mesh-element identity
Graph shortest path / union-find / priority queue
Job/cancel/cache framework
Mesh spatial queries
Deterministic recipe serialization
```

## 29. CPU/GPU split

Good GPU candidates:

- face/edge metrics;
- large batches of UV/world area calculations;
- overlap broad phase;
- distortion/Jacobian metrics;
- debug/checker visualization;
- candidate descriptor evaluation.

Prefer CPU initially for:

- graph union/merge hierarchy;
- chart topology edits;
- graph isomorphism/canonicalization;
- deterministic global pack decisions;
- seam publication;
- final validation/serialization.

Only move graph-heavy stages to GPU after profiling demonstrates value.

---

# Part J — tests and acceptance

## 30. Surface-chart tests

Synthetic meshes:

- plane;
- cube;
- beveled cube;
- cylinder;
- sphere;
- L-shaped hard surface;
- concave folded strip;
- noisy triangulation of equivalent surfaces.

Assertions:

- rigid-transform invariance;
- hard boundary never crossed;
- stronger fold raises merge cost;
- planar coherent regions merge cheaply;
- tiny-chart cleanup preserves boundary topology;
- results deterministic for same recipe/mesh revision.

## 31. Topology-match tests

- exact duplicate;
- uniformly scaled duplicate;
- same topology with changed edge lengths;
- same degree histogram but non-isomorphic graph;
- n-gon ambiguity;
- mirrored candidate;
- disconnected components.

Require an explicit correspondence map, not just boolean equality.

## 32. Texel/UV tests

- 1K/2K/4K density scaling;
- rectangular textures;
- non-uniform object transforms;
- stacked islands;
- mirrored/flipped UVs;
- known distortion Jacobians;
- UDIM tile constraints;
- pixel-correct padding.

## 33. Parameterization tests

- disk topology;
- holes/non-disk rejection or explicit supported policy;
- degenerate triangles;
- non-manifold edges;
- pinned boundary;
- high-curvature organic patch;
- hard-surface quad strip;
- flips and energy regression.

---

# Part K — license and clean-room boundary

## 34. License facts

The inspected extension manifest declares GPL-3.0-or-later. Some source components,
including ZenRelax, explicitly incorporate/reference GPL algorithm implementations.
The stack subsystem also has a compiled core interface; its exact redistribution
and linkage status must be reviewed independently before any dependency decision.

Jerico direction remains:

```text
purchased addon/source
  -> private technical study
  -> equations / behavior / tests / acceptance
  -> independent Jerico implementation
```

Do not:

- copy Zen UV source into Jerico production modules;
- redistribute the purchased package from the repository;
- make Shipping depend on Blender/Zen UV;
- mechanically port ZenRelax C++ into a differently licensed engine;
- assume a compiled vendor library is admissible because its API is visible.

The math problems themselves can be implemented independently from published
mathematics and Jerico-owned tests.

---

# 35. M8 decision

**HIGH-VALUE M8 RESEARCH INPUT — NOW SOURCE VERIFIED.**

The strongest capabilities to implement independently are:

1. incremental surface chart clustering from normals/planarity/dihedral/compactness;
2. topology-safe cleanup of generated regions;
3. exact texel-density and UV distortion metrics;
4. topology-aware correspondence/stack matching;
5. trim/hotspot mapping;
6. deterministic packing and UDIM handling;
7. specialized structured-quad parameterization;
8. a measurable relax/unwrap solver selected independently of the GPL reference.

The previous statement that the archive lacked source is obsolete. This document is
the canonical M8 Zen UV research input for the newly supplied complete archive.