# Smart Remesh 3.0.0 — Technical Analysis for JericoEngine

## Executive summary

The uploaded package is not a Geometry Nodes product. It is a Blender 4.2+ Python/NumPy add-on with two distinct remeshing paths:

- **Organic:** primarily drives the bundled **Instant Meshes** Windows executable, then adds its own analysis, calibration, candidate search, preprocessing, UV handling, thin-shell safeguards and topology scoring.
- **Hard Surface:** a custom add-on-side pipeline that detects feature lines, identifies planar regions, rebuilds them as large n-gons, creates chamfer bands, repairs gaps and optionally performs topology-cleaning passes.

There are no geometry-node groups to extract. The only Blender node construction found is material/shader-node setup used by baking. The valuable transfer target is therefore **mesh-processing mathematics and topology logic**, not a node graph.

The package manifest identifies Smart Remesh 3.0.0 as `GPL-3.0-or-later`; the bundled Instant Meshes executable is BSD-3-Clause. Any source-derived C++ port must stay isolated from JericoEngine production unless Jerico is prepared to satisfy GPL obligations or the implementation is independently re-created from permissive/public references.

## Package inventory

The archive contains 28 files. The important sources are:

| File | Role |
|---|---|
| `core/mesh.py` | triangulated working mesh, adjacency, normals, tangent frames, curvature, feature-edge detection |
| `core/detect.py` | automatic crease threshold, orientation sign, multi-scale cavity/concavity, model classification |
| `core/field.py` | 4-RoSy orientation field, field combing, local position field |
| `core/hierarchy.py` | multiresolution coarse-to-fine hierarchy for the orientation solve |
| `core/sizing.py` | adaptive target-edge field from creases, curvature, inner/outer importance, thinness and source density |
| `core/extract.py` | lattice clustering, graph construction and polygon face extraction |
| `core/remesh.py` | in-house field-guided remesh orchestration |
| `core/instantmeshes.py` | PLY/OBJ bridge and Instant Meshes CLI driver |
| `core/auto.py` | model analysis, face-count calibration and candidate search |
| `core/score.py` | compact topology quality metric |
| `core/hardsurface.py` | hard-surface detector and panel/chamfer reconstruction |
| `core/panelize.py` | BMesh panelization and structured loop thinning/cleaning |
| `core/prune.py` | topology pruning while preserving panel planarity |
| `core/poles.py` | valence-pole relocation through edge rotations |
| `core/subdivide.py` / `core/unsubdiv.py` | cage/topology refinement and reverse-subdivision helpers |
| `core/uvheal.py` | UV discontinuity/smear healing by local affine/similarity reconstruction |
| `lines_preview.py` | Blender integration, live previews, UV/seam tools, bake orchestration and many cleanup operations |
| `__init__.py` | UI/operator orchestration, organic preprocessing and remesh workflows |

The Python sources compile successfully with `py_compile`. The bundled executable is a 64-bit Windows PE console application and identifies itself as Instant Meshes in its embedded strings.

---

# 1. Mesh representation and geometric primitives

## 1.1 Polygon triangulation without losing source-edge semantics

The solver fans every source polygon into triangles to obtain a uniform triangle working mesh. It also records which edges existed in the original polygon. Fan diagonals are considered **virtual edges** and must not later become crease constraints.

This matters for Jerico because a non-planar n-gon can produce a non-zero dihedral across an internal fan diagonal. Treating that diagonal as authored geometry invents a feature the artist never created.

### C++ mapping

Use a compact triangle mesh plus a sorted `EdgeKey(min(v0,v1), max(v0,v1))` set of authored/source edges. Feature detection must reject triangle-only diagonals.

## 1.2 Edge topology

Undirected edges are built by sorting triangle edge pairs, grouping equal keys and recording up to two incident faces. The same data supplies:

- boundary detection;
- manifold checks;
- CSR vertex adjacency;
- per-vertex degree/valence;
- dihedral evaluation.

This is an excellent fit for Jerico's geometry utilities and can be built on CPU or with radix-sort/scan on GPU for large batches.

## 1.3 Normals

Triangle normal before normalization:

`N_f = (p1 - p0) × (p2 - p0)`

Its magnitude is twice the triangle area. Vertex normals are area-weighted because the unnormalized face vectors are summed into each incident vertex before normalization.

## 1.4 Stable tangent frame

For each vertex normal `n`, choose the Cartesian axis least aligned with `n`, call it `h`, then:

`u = normalize(n × h)`

`v = n × u`

This avoids the numerical instability of always using one global helper axis.

---

# 2. Principal-curvature estimator

The add-on estimates principal curvature from the way normals change across the one-ring. In each vertex tangent frame `(u,v)`, an edge to a neighbor is represented by normalized tangent components `(ex, ey)`, while the normal change is represented as `(dx, dy)`.

It fits the symmetric second fundamental form

`II = [[a,b],[b,c]]`

through the linear relations

`dx ≈ a*ex + b*ey`

`dy ≈ b*ex + c*ey`

Accumulating normal equations over all neighbors yields a 3x3 least-squares solve for `(a,b,c)`. A tiny trace-scaled diagonal regularizer is added before solving.

The principal curvatures are the eigenvalues of the symmetric 2x2 matrix:

`k1,k2 = tr/2 ± sqrt(((a-c)/2)^2 + b^2)`

and the principal direction is reconstructed in world space from the eigenvector in `(u,v)` coordinates.

Anisotropy is measured as:

`|k1-k2| / (|k1|+|k2|+eps)`

### Jerico relevance

High. This is cheap, local and avoids a sparse solver. It can drive:

- remesh density;
- directional strata/detail placement;
- anisotropic erosion/deformation masks;
- edge-aware procedural texturing.

---

# 3. Automatic crease detection

## 3.1 Raw dihedral

For each manifold edge with normalized adjacent face normals `n0,n1`:

`angle = acos(clamp(dot(n0,n1), -1, 1))`

## 3.2 Histogram-valley threshold

The organic detector does not assume one fixed angle. It searches a candidate interval for an under-populated histogram bin with meaningful mass above and below it. Conceptually this separates a low-angle smooth population from a high-angle crease population.

If the histogram has no convincing valley, it falls back to a high percentile so roughly the strongest few percent of edges become crease candidates.

### Why this is useful

A user-facing fixed angle is fragile across scans, AI meshes and hard-surface assets. The threshold can be a default suggestion while still allowing explicit override.

---

# 4. Surface orientation and cavity/concavity signal

## 4.1 Closed-mesh orientation

For a closed triangle mesh, signed volume is computed from the scalar triple product:

`V = Σ dot(p0, p1 × p2) / 6`

The sign tells whether winding points outward or inward.

For open meshes, the fallback compares vertex normals with vectors pointing away from the vertex centroid.

## 4.2 Multi-scale cavity without ray casting

At several graph-smoothing radii, vertex positions are umbrella-smoothed. For each scale:

`d = dot(P_smooth - P_original, n) / bboxDiagonal`

Positive displacement along the outward normal is interpreted as a valley/concavity; negative is a ridge/convexity. Each scale is normalized by a robust high percentile of absolute displacement and the normalized scales are averaged.

This gives a cheap signed cavity proxy without AO rays or a BVH.

### Jerico relevance

Very high for procedural geology because it gives a reusable concave/convex field that can drive:

- sediment/dirt accumulation;
- edge wear masks;
- crack preference;
- material layering;
- remesh density around inner corners.

---

# 5. Adaptive sizing field

The add-on contains one of its most reusable ideas: a **per-vertex target edge length** rather than one global remesh size.

Let `base` be the uniform target edge length and `floor = base * minRatio`.

## 5.1 Feature-band refinement

Feature-edge endpoints become zero-distance seeds. A graph distance transform is solved by repeated edge relaxation:

`value_i <= value_j + slope * length(i,j)`

With slope 1, this is a Bellman-Ford-like geodesic distance approximation over mesh edges.

Inside a band of width `band * base`, size ramps linearly from `floor` at the crease back to `base`.

## 5.2 Curvature refinement

Use `max(|k1|,|k2|)` and normalize it by a high percentile. High-curvature vertices interpolate toward `floor`.

## 5.3 Independent inner/outer density

The cavity sign separates concave and convex edge importance. Inner and outer edges can therefore receive different refinement weights.

## 5.4 Thin-sheet safeguard

For closed solids, the add-on estimates local thickness by ray casting inward from just under the surface. Desired edge length can be clamped below a fraction of that thickness so opposite sides of thin cloth or shells do not collapse into the same remesh cell.

For Jerico, this would be better implemented with the engine BVH/mesh acceleration structure rather than Blender's BVH.

## 5.5 Source-density matching

Another mode measures the mean incident source-edge length per vertex, rescales that field so its mean equals `base`, blends by a strength parameter, clamps to min/max ratios and applies the same downhill gradient limiter.

This preserves intentional high-density regions even when curvature alone would not explain them.

## 5.6 Lipschitz/gradient limiter

A raw adaptive field can jump from very small to very large values between neighbors. The add-on enforces a slope bound with repeated relaxation:

`size_i <= size_j + gradientLimit * edgeLength_ij`

This is crucial. Adaptive remeshing without a smooth size field creates stretched elements and extraction failures.

## 5.7 Preserve requested face count

For triangle `t` with area `A_t` and local size `s_t` (average of its three vertex sizes), estimated quad count is approximately:

`F_est = Σ A_t / s_t^2`

The entire size field is multiplied by one scalar found by bracketing + binary search until this estimate matches the requested face count. This cleanly separates:

- **where** density goes — shape of the field;
- **how many** faces — global scale multiplier.

This is directly useful in Jerico.

---

# 6. 4-RoSy orientation field

A quad orientation is unchanged by 90-degree rotation. Therefore a direction is not a single vector; it is an equivalence class `{o, R90(o), -o, -R90(o)}`.

## 6.1 Best quarter-turn representative

Given a target direction `t`, neighbor direction `o` and normal `n`:

1. project `o` into the tangent plane and normalize to `a`;
2. compute `b = n × a`;
3. compare `dot(t,a)` and `dot(t,b)` by absolute magnitude;
4. choose whichever of `±a` or `±b` aligns best.

This is the core operation in the field solver and field combing.

## 6.2 Feature constraints in exp(4iθ)

Crease tangent directions incident to a vertex are mapped to their angle `θ` in the local tangent frame and accumulated as:

`z = Σ (cos(4θ), sin(4θ))`

Then:

`θ_locked = atan2(z_y,z_x)/4`

The factor 4 makes perpendicular crease directions reinforce instead of cancel.

A coherence term

`|z| / count`

identifies corners. Low coherence or three-or-more crease incidences pins the vertex in the later position solve.

## 6.3 Jacobi smoothing

Each iteration gathers every neighbor after converting it to the best equivalent 4-RoSy representative. Crease edges are given stronger weights. The old field value is also retained as an anchor, then the sum is projected back into the tangent plane and normalized.

Locked feature vertices are overwritten after each sweep.

### GPU potential

Excellent. Each Jacobi iteration is edge-parallel scatter/gather plus per-vertex normalization. A GPU version can use edge contributions into a temporary buffer followed by segmented reduction or atomics.

---

# 7. Multiresolution orientation solve

Pure Jacobi propagation moves information about one graph edge per iteration and can settle into poor local minima on large meshes. The add-on builds a hierarchy by greedily matching pairs of vertices, preferring edges that are:

- short;
- between aligned normals.

A simple weight is proportional to:

`abs(dot(n_i,n_j)) / edgeLength`

Matched children are averaged into coarse vertices/normals, edges are remapped and duplicates removed.

The orientation field is restricted to the coarsest level, solved there, then prolonged level by level and refined.

### Jerico relevance

High. This is a cheap multigrid-like accelerator that does not require a sparse matrix package.

---

# 8. Field combing

Before integrating the orientation field into positions, the 90-degree ambiguity must be resolved consistently. A graph traversal starts at each unvisited component and rotates/sign-flips each newly visited neighbor to the representative that best agrees with the parent.

Where no globally consistent representative exists, the disagreement corresponds to a field singularity and eventually a topology pole.

---

# 9. Local position field

Each source vertex keeps a local lattice point rather than participating in one global UV-like parameterization. This avoids the topological impossibility of globally parameterizing arbitrary closed surfaces without singularities.

Let orientation `o`, tangent `t = n × o`, local cell size `s`, and current lattice points `p_i,p_j`.

For an edge `i→j`, estimate whole-cell offsets:

`k_o = round(dot(p_j-p_i, o_i) / s_ij)`

`k_t = round(dot(p_j-p_i, t_i) / s_ij)`

where `s_ij` is the mean endpoint size.

The neighbor proposes to vertex `i`:

`q = p_j - s_ij * (k_o*o_i + k_t*t_i)`

All proposals plus an anchor to the original vertex are averaged. Drift is projected onto the tangent plane, and displacement is hard-clamped to at most half a local cell.

Feature vertices may slide only along the crease tangent; detected corners are fixed exactly.

### Jerico relevance

This is the core of a native field-guided remesher. It is useful but much larger to productionize than the sizing field or detector.

---

# 10. Lattice collapse and topology extraction

## 10.1 Centroid-bounded clustering

Vertices are considered in ascending distance between their solved lattice points. Two components merge only if their current weighted centroids are within a radius proportional to the mean local cell size.

After a merge, centroid and local size are recomputed.

This avoids a classic union-find chain-collapse failure where A merges B, B merges C, and transitively A and C end up together even though they are a full cell apart.

## 10.2 Cluster geometry

Each cluster gets:

- mean solved position;
- normalized mean source normal;
- representative orientation rather than naïve average, because cross-field representatives can cancel;
- mean target size.

## 10.3 Lattice graph classification

Cluster-to-cluster displacement is interpreted in the local orientation basis and rounded to integer lattice offsets.

The conceptual classification is:

- `(0,0)` same site;
- one step on one axis = real quad edge;
- one step on both axes = diagonal, discard;
- larger offsets = discard.

This is stronger than judging geometry only by angle because stretched one-step edges and diagonals can have similar geometric angles.

## 10.4 Face extraction

After adjacency around each vertex is sorted in tangent-plane angular order, faces are recovered by standard half-edge rotation-system traversal: `next = rotate(twin)`. Remaining one-sided half-edges are used to identify bounded holes; only reasonably sized rings are filled.

---

# 11. Hard-surface detector

The hard-surface path is custom and does not use Instant Meshes.

## 11.1 Newell polygon normals

For arbitrary n-gons, polygon normal/area is accumulated with a Newell-style cross sum around the loop. This avoids first triangulating the face just to classify it.

## 11.2 Bilateral filtering of face normals

Adjacent face centroids provide a spatial Gaussian weight:

`w_s = exp(-||c_i-c_j||^2 / (2σ_s^2))`

Normal difference provides a range Gaussian:

`w_r = exp(-||n_i-n_j||^2 / (2σ_r^2))`

Combined neighbor influence is weighted by face area. Repeating the bilateral update suppresses tessellation noise while preserving actual discontinuities.

## 11.3 Persistent bend signal

Filtered dihedral alone marks every facet of a curved cylinder. The detector therefore compares an edge's bend against the average bend around its adjacent faces:

`persist = edgeAngle - 0.5*(neighborMeanA + neighborMeanB)`

A true crease remains an outlier relative to its neighborhood; ordinary curvature does not.

## 11.4 Otsu thresholding

A histogram is split by maximizing between-class variance. This is used to derive a high/low separator for a bimodal bend distribution.

## 11.5 Surface-type boundary

Some visually important boundaries are transitions from planar regions to curved regions with almost zero local dihedral. The detector therefore also grows coplanar face regions from raw normals and marks edges where panel membership or panel region changes.

This is a strong design point: **edge semantics are not reducible to dihedral angle**.

## 11.6 Connected-chain length filter and gap closing

Candidate feature edges are grouped into connected components. Very short chains relative to model diagonal are rejected. Dangling lines can be extended preferentially in their current direction instead of bridged by arbitrary shortest paths.

---

# 12. Hard-surface panel reconstruction

The intended geometry is not uniform quads. It is:

- large n-gons on planar panels;
- dense quads only in chamfer/border bands.

The pipeline is roughly:

1. classify feature edges;
2. grow regions bounded by those edges;
3. extract closed region loops;
4. simplify straight boundary runs globally;
5. project each planar region into a local 2D basis;
6. inset outer/inner rings;
7. connect original and inset rings with chamfer quads/triangles;
8. emit remaining interior as an n-gon or split annulus around holes;
9. retain original geometry only when a region cannot be reconstructed safely;
10. compact unused vertices.

## 12.1 Douglas-Peucker boundary simplification

Straight runs are simplified by recursive maximum point-to-segment deviation. Importantly, the simplification is applied to the boundary network coherently so adjacent regions do not independently choose incompatible borders.

## 12.2 Local ring inset

Inset width is locally constrained and its rate of change around the loop is limited. Independent per-vertex clamping was found to cause neighboring inset vertices to disagree and produce unstable chamfer strips.

## 12.3 Holes

A panel with holes cannot be represented as one simple Blender n-gon. The implementation grows concentric rings around small holes before connecting them to the panel ring, reducing extreme scale ratios, then cuts an annulus into simple polygons.

### Jerico relevance

Very high for procedural hard-surface authoring, cliffs cut into architectural forms, destruction preprocessing, and decimated imported assets. It should be a **tool-time** path, not runtime gameplay geometry.

---

# 13. Panelize and topology cleanup

`core/panelize.py` contains a large collection of useful topology operations:

- region-wide planarity grouping rather than pairwise-only flatness;
- preserve marked/feature borders while dissolving interior edges;
- identify edges strictly inside marked closed regions;
- recover full edge loops from quad topology;
- rank removable loops by resulting shape error, not loop-local bend;
- remove loops in non-adjacent passes to spread simplification spatially;
- alternate-loop thinning for readable regular patterns;
- preserve curved rims while dissolving spokes;
- order families of parallel loops into chains;
- rebuild selected regions from border rings/fans;
- heal dead-end stubs;
- force/complete loops;
- remove loops to a polygon budget;
- measure surface drift after cleanup.

The important design lesson is that hard-surface simplification is treated as a **structured loop problem**, not generic edge collapse.

---

# 14. Pole relaxation

A quad mesh ideally has interior valence 4. The add-on treats any other interior valence as a pole.

It tries local edge rotations around pole neighborhoods. Candidate topology is compared lexicographically:

1. number of poles;
2. total valence deviation from 4;
3. distance to poles of the opposite type during merge-oriented rounds;
4. bend severity at remaining pole locations.

A lower-valence and higher-valence pole can cancel when brought together. Alternate rounds therefore first pull opposite pole types together, then move remaining poles toward flatter surface regions.

### Jerico relevance

Useful as a post-process after any quad remesher. The lexicographic scoring is more robust than one weighted scalar because it prevents a visually nicer placement from increasing topological defects.

---

# 15. UV preservation/healing

The add-on contains several practical UV repair ideas:

- detect UV seams by comparing the UV coordinates assigned by the two incident faces;
- transfer UVs by barycentric/closest-triangle style projection for remeshed vertices;
- repair isolated distorted faces by fitting a local affine map from a healthy neighbor;
- if affine mapping is unreliable, match a shared edge exactly and extend a 2D similarity transform across the face;
- rejoin one-face UV islands to neighbors rather than simply laying them out separately;
- preserve UDIM tile usage;
- build a bake cage whose offset follows local source/target drift instead of using one global cage distance.

These are tool-pipeline features, not remeshing mathematics, but they are directly relevant to Jerico AssetPipeline if topology-changing tools are added.

---

# 16. Instant Meshes integration

The Organic production path invokes the bundled executable through PLY input and OBJ output. Exposed controls map to Instant Meshes batch arguments for:

- target edge size or face count;
- crease angle;
- smoothing steps;
- dominant/pure quad behavior;
- deterministic mode;
- boundary alignment;
- thread count.

The add-on also compensates for observed output-count behavior by calibrating the requested face count and, when needed, running one corrective pass.

Progress is inferred by parsing stage lines from the executable's stdout.

### Important distinction

The add-on contains an in-house field-guided remesher, but its own UI comments state that Instant Meshes measured significantly better on their benchmark and is the Organic engine exposed to users. Therefore Jerico should not assume the Python solver is automatically the best production baseline.

---

# 17. Automatic model analysis and candidate search

The auto mode derives:

- vertex/triangle counts;
- mean edge length;
- bounding diagonal and area;
- closed/open status;
- crease threshold and fraction;
- flat-surface fraction;
- curvature percentile;
- coarse hard-surface vs organic classification.

It then calibrates face count and evaluates a small candidate set around the detected crease threshold, including a quad-dominant variant.

The topology score combines:

- quad area purity;
- interior valence-4 regularity;
- boundary-edge penalty;
- non-manifold penalty;
- bad face-aspect penalty.

The exact weights are add-on policy, not universal truth. For Jerico, expose the raw metrics and let different authoring presets choose weights.

---

# 18. What maps well to GPU

## Excellent GPU candidates

- normal/area computation;
- dihedral computation;
- curvature least-squares accumulation;
- cavity smoothing passes;
- Jacobi orientation solve;
- adaptive sizing relaxation;
- field face-count estimate;
- graph-scatter accumulation.

## CPU-friendlier candidates

- disjoint-set clustering;
- graph component filtering;
- half-edge face walking;
- Douglas-Peucker chain simplification;
- hole/annulus topology surgery;
- structured loop cleanup;
- pole edge-rotation search.

A sensible Jerico design is hybrid: GPU produces fields and per-element measurements; CPU performs sparse topology edits.

---

# 19. Suggested JericoEngine decomposition

A future permissively implemented system should be split into independent modules rather than one monolithic remesher:

1. `MeshAnalysis`
   - topology/adjacency
   - normals/areas
   - curvature
   - dihedral/crease statistics
   - cavity/concavity

2. `MeshSizingField`
   - feature distance
   - curvature density
   - thickness density
   - source-density matching
   - gradient limiting
   - face-budget fit

3. `QuadOrientationField`
   - 4-RoSy representation
   - feature constraints
   - multires hierarchy
   - combing

4. `QuadPositionField`
   - local lattice integration
   - feature sliding/corner pinning

5. `QuadExtraction`
   - centroid-bounded collapse
   - lattice adjacency classification
   - half-edge polygon extraction

6. `HardSurfaceFeatureGraph`
   - bilateral normal denoise
   - persistent crease signal
   - coplanar regions
   - type boundaries
   - chain pruning/gap continuation

7. `PanelRebuild`
   - boundary network simplification
   - plane projection
   - inset/chamfer bands
   - holes/annuli

8. `TopologyOptimize`
   - loop thinning
   - pole relocation
   - drift checks

9. `AttributeTransfer`
   - UV/material/normal reprojection
   - seam handling
   - bake cage generation

This decomposition also matches Jerico's procedural-first requirement: non-destructive settings, deterministic seeds where applicable, preview/diff, cancellation, undo/redo and stable bake/cook.

---

# 20. Recommended implementation order for Jerico

## Tier A — strong value / modest risk

1. **Mesh analysis + curvature**
2. **Adaptive sizing field + face-budget fit**
3. **Automatic crease/panel detector**
4. **Topology metrics**

These can improve existing geology/remesh tools without committing to a full new remesher.

## Tier B — high value / larger project

5. **4-RoSy orientation field**
6. **Multires orientation hierarchy**
7. **Local position field**
8. **Lattice collapse/extraction**

This becomes a native quad-remesh subsystem.

## Tier C — authoring polish

9. **Hard-surface panel rebuild**
10. **Structured loop cleaner**
11. **Pole relaxation**
12. **UV/attribute transfer and baking**

---

# 21. Performance notes

- Repeated Jacobi and relaxation sweeps are predictable and data-parallel.
- Full Bellman-Ford-style relaxation can require many passes; on large meshes a priority-queue CPU geodesic or GPU multi-source shortest path may beat the literal algorithm.
- The add-on intentionally avoids sparse numerical libraries. Jerico is not constrained by Blender's Python environment and can use better native data structures.
- Thickness ray casts are potentially expensive; only run where thin-sheet risk exists or against a coarse acceleration structure.
- Hard-surface rebuild is topology-heavy and is best kept off the frame loop.
- UV transfer/bake is offline/tool work.
- Do not port the Python implementation structure mechanically. Preserve the math, redesign the memory layout for SoA/parallel execution.

---

# 22. Research branch contents

This analysis accompanies an isolated GPL research target in `Research/SmartRemesh3GPL/` implementing a subset of the kernels in modern C++ with standalone tests. It is deliberately not linked by the root build.

That target is a verification artifact, not production Jerico code.
