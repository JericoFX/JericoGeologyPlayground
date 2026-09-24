# M8 Smart Segmentation 1.0 — Technical Research Input

**Milestone:** M8  
**Category:** semantic part segmentation / mesh partitioning / selection preparation  
**Status:** source inspected; research input ready; native Jerico implementation not started  
**Owner timing:** read and act on this only when M8 is the legal `ACTIVE_SLICE`  
**Runtime dependency:** none intended

## 1. Source provenance and duplicate archives

Two Smart Segmentation ZIPs were supplied:

- `[Blender] Smart Segmentation v1.0.zip`
  - SHA-256 `a16fc68eb78f92163e1c4d1bdb944fe8071420fcac2c3a05f4ded7aaf0a18155`
- `Smart Segmentation v1.0.zip`
  - SHA-256 `8edd1bd3df1e65766dd2ad4ef38dd5eaf01aae2d45162d77ec4bfa88d2c74aa3`

Both contain the same `Smart_Segmentation.py` byte-for-byte:

- source SHA-256 `f508e481f0677274b346cd16a37b32cf8ca90c6a95de6c83a634eeaaccc8050b`
- 3,540 lines
- 129,199 bytes

One archive only adds a third-party readme. There is no second implementation to
merge or compare, so Jerico should treat these as **one source snapshot**.

The inspected add-on identifies itself as Smart Segmentation 1.0.0 by
Milad Kambari / 3DRedbox Studio.

## 2. Why this matters to M8

This is not primarily a geometry generator. Its useful idea is to infer meaningful
mesh regions from existing geometry, then let the designer refine those regions.

That maps well to M8 for:

- breaking generated/imported geometry into semantic regions;
- preparing material regions;
- authoring destruction/fracture groups;
- selecting logical rock/cliff parts;
- bake groups;
- collision/nav proxy grouping;
- LOD semantic preservation;
- reusable selection masks.

The product's key insight is the combination of **local thickness** and
**surface concavity**, not a voxel grid or arbitrary spatial split.

## 3. Inspected algorithm pipeline

```text
evaluated mesh
  -> proxy mesh (~3k triangles)
  -> geometry + adjacency
  -> shape-diameter/thickness signal
  -> bilateral smoothing
  -> concavity/dihedral signal
  -> agglomerative region hierarchy
  -> automatic K suggestion
  -> map proxy labels to full resolution
  -> full-resolution boundary refinement
  -> optional manual split/merge/paint correction
  -> publish regions/materials/parts
```

## 4. Proxy construction

The add-on evaluates the mesh, duplicates it into a working proxy, reduces the
proxy to roughly 3,000 faces, triangulates it and removes duplicate vertices.

Jerico should generalize this into a deterministic analysis proxy:

```text
AnalysisProxy {
    source_revision
    source_to_proxy_mapping
    proxy_to_source_mapping
    triangle_geometry
    adjacency
}
```

Do not use Blender object state as the identity contract.

For very large meshes, Jerico can reuse existing LOD/simplification infrastructure
provided semantic mapping and error bounds are retained.

## 5. Shape Diameter Function / thickness signal

For every proxy face, the source implementation samples rays from the face toward
the interior inside a cone.

Observed defaults include:

```text
rays = 20
cone angle = 120 degrees
smoothing passes = 2
bilateral sigma = 0.08
log alpha = 4
```

The important behavior is:

1. build a cone of inward directions;
2. raycast against the evaluated/full mesh;
3. reject misses and geometrically invalid hits;
4. weight accepted distances by ray angle;
5. robustly reject distance outliers;
6. aggregate to a face thickness estimate;
7. normalize and compress the dynamic range.

The inspected angular weight is conceptually:

```text
w_i = 1 / (theta_i + c)
```

with a small positive stabilizer.

For samples with enough hits, the implementation uses median/standard-deviation
filtering before the weighted mean.

After min/max normalization `x in [0,1]`, the implementation applies logarithmic
compression equivalent to:

```text
x' = log(alpha*x + 1) / log(alpha + 1)
```

This emphasizes useful differences in the low/mid range.

### Jerico implementation note

Use an explicit ray-query service with:
- max ray length;
- backface/normal validity rule;
- deterministic sampling seed;
- per-triangle confidence;
- no dependence on viewport state.

This signal is a good GPU candidate.

## 6. Cone sampling

The source distributes ray directions across the cone using a golden-angle-like
sequence rather than a regular polar grid.

Jerico can independently use a deterministic Fibonacci/golden-angle disk/hemisphere
sampling scheme, transform it into the local tangent frame and clip by cone angle.

Required tests:
- rotation invariance within tolerance;
- thin plate;
- sphere;
- hollow shell;
- open mesh / missing-hit confidence;
- nested surfaces.

## 7. Bilateral smoothing of thickness

The source smooths SDF values across adjacent faces but suppresses smoothing across
large signal discontinuities.

The observed shape is:

```text
w_ij = edge_length_ij * exp(-abs(sdf_i - sdf_j) / sigma)
```

plus a self weight.

This is useful because it removes triangle noise without erasing a meaningful
thickness boundary.

Jerico should store both:
- raw thickness;
- smoothed thickness;
- confidence/sample count.

## 8. Concavity / dihedral feature

For adjacent faces, the implementation computes an orientation test from the face
normal and the vector toward the neighbor center to classify a fold as concave or
convex.

The angular cost is based on:

```text
1 - dot(n_i, n_j)
```

with concave folds receiving the stronger weight and convex folds heavily reduced.

This encodes the product intent: **inward creases are more likely to be semantic
part boundaries than smooth convex bending**.

For Jerico this should become a reusable signed-dihedral feature:

```text
SignedDihedral {
    angle
    concave
    shared_edge_length
    confidence
}
```

## 9. Agglomerative region graph

Each triangle starts as a region. Adjacent regions are repeatedly merged from the
lowest merge cost, producing a complete hierarchy/dendrogram rather than one fixed
segmentation.

The inspected merge cost combines:

- region thickness difference;
- accumulated angular/concavity cost;
- region area;
- shared boundary length;
- global mean edge length.

Conceptually:

```text
feature =
    w_thickness * abs(sdf_A - sdf_B)
  + w_angle     * angular_boundary_cost
  + epsilon

area_factor =
    sqrt(min(area_A, area_B) / total_area)

boundary_factor =
    sqrt(max(shared_length / mean_edge_length, epsilon))

cost =
    feature * area_factor / boundary_factor
```

Consequences:
- large shared boundaries make merging easier;
- strong concave boundaries make merging harder;
- tiny regions are absorbed earlier;
- changing part count is cheap after initial analysis because the hierarchy already
  exists.

This is highly reusable in Jerico.

## 10. Disconnected shells

A pure adjacency graph has no path between disconnected components. The source
adds a higher-level bridge based on component/region centroid proximity, effectively
forming a single-link/MST-style continuation so one hierarchy can still reach the
requested part count.

Jerico should make disconnected-shell policy explicit:

```text
KeepSeparate
NearestComponentHierarchy
AuthorGrouped
MaterialGrouped
```

Do not force unrelated components to merge silently.

## 11. Automatic number of regions

The source suggests a part count from the **largest relative jump in hierarchy
merge cost**.

That is a sensible default because a large cost jump often means subsequent merges
would cross a strong semantic boundary.

Jerico should expose:
- suggested K;
- confidence / jump ratio;
- full cost curve;
- designer override.

The system suggests; the designer decides.

## 12. Mapping proxy segmentation to full resolution

The source defaults to accurate nearest-surface mapping using a spatial acceleration
structure and also contains a faster approximate mapping mode.

Jerico should retain explicit provenance:

```text
full triangle
  -> closest proxy triangle / barycentric coordinate
  -> region id
```

Better still, if the analysis proxy is produced by Jerico's own simplifier, preserve
the collapse/remap lineage and avoid an unnecessary spatial nearest query.

## 13. Full-resolution boundary refinement

This is the most valuable part after the SDF hierarchy.

The source builds the full-resolution triangle adjacency graph and assigns a cheap
cut cost to edges that lie on strong creases/concavity.

Observed form:

```text
cut_weight =
    (edge_length / mean_edge_length)
    / (1 + angular_signal / mean_angular_signal)
```

Strong creases therefore become attractive boundary locations.

For every pair of neighboring semantic regions:

1. build a band of a configurable number of face rings around the current boundary;
2. determine core faces for each side;
3. compute graph distances from each core;
4. convert distances into unary preferences;
5. sweep candidate level-set thresholds to move the whole contour coherently;
6. minimize boundary cut cost plus a stiffness/unary penalty;
7. perform local ICM-style cleanup.

This is effectively a small binary labeling / MRF refinement problem constrained
to a narrow band.

### Jerico version

Provide a generic `RegionBoundaryRefiner`:

```text
Inputs:
- triangle graph
- initial labels
- boundary edge feature weights
- locked labels
- band width
- stiffness

Outputs:
- refined labels
- changed triangles
- before/after cut energy
```

It can serve material masks, destruction zones and author selections in addition
to segmentation.

## 14. Manual correction model

The add-on supports:
- merge;
- split one region without recutting the whole model;
- brush/lasso/box-style corrections;
- explode preview;
- publish as separate objects or material IDs.

A useful design detail is that manual operations are represented by stable reference
faces/relationships instead of relying only on transient numeric label IDs.

Jerico should go further and serialize corrections as non-destructive constraints:

```text
RegionConstraint {
    kind: Merge | Separate | PinLabel | Include | Exclude
    source_element_guids / authored selection
    revision
}
```

Re-analysis can then replay authored intent.

## 15. GPU/CPU split

Excellent GPU candidates:
- ray-based thickness/SDF;
- face geometry;
- adjacency feature evaluation;
- bilateral signal smoothing;
- bulk nearest-triangle mapping.

Prefer CPU initially:
- hierarchical region graph / heap merges;
- dendrogram representation;
- manual constraint replay;
- narrow-band graph refinement;
- deterministic publication.

The graph refinement can move to GPU only if profiling proves value.

## 16. Jerico integration targets

Potential M8 services:

```text
MeshSemanticAnalyzer
ThicknessFieldBuilder
SignedDihedralField
HierarchicalRegionSegmenter
RegionBoundaryRefiner
RegionConstraintSet
RegionPublicationService
```

Potential downstream consumers:
- Physical Material / destruction;
- geology material region selection;
- decal or wear masks;
- bake groups;
- collision proxy grouping;
- navigation exclusions;
- LOD semantic streams;
- editor selection sets.

Do not make this geology-specific.

## 17. Tests Jerico should require

Synthetic analytical meshes:
- capsule with limbs;
- dumbbell shape;
- Y junction;
- thin plate into thick block;
- concave box union;
- smooth sphere;
- disconnected shells;
- noisy triangulation of same geometry.

Assertions:
- segmentation invariant under rigid transforms;
- close results under remeshing density changes;
- thin/thick boundary recovered;
- concave crease attracts boundary;
- convex smooth bend does not over-segment;
- automatic K curve stable;
- locks/merge/split constraints survive re-analysis;
- LOD mapping preserves region IDs within defined tolerance.

## 18. License boundary

The inspected marketplace product is distributed as GPL.

For Jerico:
- do not copy the add-on source into production code;
- formulas and behavioral observations in this document are a research
  specification, not production source;
- implement the algorithms independently under Jerico's chosen license;
- Shipping must not require Blender or this add-on.

Official behavior reference:
- https://superhivemarket.com/products/smart-segmention-/docs

## 19. M8 decision

**HIGH-VALUE M8 INPUT.**

This addon is useful even though it does not create geometry. Its best contribution
is a reusable semantic segmentation stack:

```text
thickness + signed concavity
  -> hierarchy
  -> suggested semantic regions
  -> full-resolution crease-aware refinement
  -> designer constraints
```

That belongs in Jerico as a general mesh-analysis/authoring capability, not as a
dependency on Smart Segmentation.
