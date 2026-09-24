# M8 DirectedCrack Swept-Volume Design

**Slice:** `M8-GEOLOGY-BLUEPRINT-P1-DIRECTED-CRACK-SWEPT-VOLUME`

**Status:** Approved on 2026-08-28 after the repository's recommended-option
and written-spec review windows.

## Observable outcome

An artist-authored branching graph made from the existing canonical Jerico
splines is mapped through a bounded local chart onto an admitted geological
surface. Explicit width, depth and profile controls form a true swept implicit
carving volume. The existing negative-inside field subtracts that volume and
classified Surface Nets produces thin open cracks with real topology.

The enabled result must have measured opening width/depth, no unintended
disconnected holes or undeclared self-intersection artifacts, deterministic
field/mesh identity and CPU/Vulkan parity within a declared physical bound.
Disabled, zero-strength, zero-mask and zero-propagation paths are field-identical
to the admitted source. Classification and material lineage remain unchanged.

## Selected architecture

Advance the cumulative geology recipe from schema 16 to 17 and add one
`DirectedCrack` kind after `GradientDetailWarp`. The existing layer header keeps
ownership of ID, name, enabled state, strength, seed, mask and transform.

The new payload contains only crack-specific state:

- projection mode `ImplicitSurfacePlanarChart`;
- a bounded ordered list of branch records;
- one bounded ordered piecewise-linear width-profile curve;
- maximum resampling spacing in authored metres;
- exterior opening outset in metres;
- admitted surface/projection band in metres.

Each branch record contains stable `branchGuid`, `splineGuid`, optional
`parentBranchGuid`, the parent's attachment `pointGuid`, depth in metres,
activation probability and propagation fraction. The branch references one
existing `GeologyStackRecipe::splines` entry. It does not embed or clone spline
geometry.

The width at a graph point is the existing
`SplineControlPoint::widthMeters`, interpreted here as full crack-opening width
at the surface before the layer's uniform scale. The profile curve owns keys
`(normalizedDepth, widthScale)` and linearly interpolates width scale from the
surface to the tip. The curve must begin at normalized depth zero and end at
one; keys are strictly increasing and scales are finite and non-negative.

No second spline, graph, field, CSG, mesher, material, renderer or cook path is
introduced.

## Graph, branch and identity contract

Every referenced spline must be canonical, open, have at least two points and be
coplanar in local chart coordinates (`abs(point.z) <= 1.0e-9 m`, including
Bezier handles when applicable). The selected design admits at most 32 branches,
128 prepared segments and 8 profile keys.

Exactly one root branch has zero parent and activation probability `1`. Every
other branch names an earlier parent branch and one control-point GUID on that
parent. Its first spline point must share that attachment position exactly in
canonical local coordinates. Branch GUIDs, spline references and parent edges
must be unique and acyclic.

Child activation is deterministic:

```text
activation = canonicalUnorm(hash(seed, branchGuid)) < probability
```

The root is always active. Seed selects among authored children and never
creates, perturbs or reorders geometry. Probability zero suppresses a child;
probability one always admits it.

Propagation is the retained fraction of cumulative spline arc length in `[0,1]`.
Segments wholly beyond that distance are omitted and the crossing segment is
clipped exactly. Propagation zero contributes no volume. Parent suppression also
suppresses its descendants, even if their own probability passes.

Canonical serialization and hashing include every branch/profile control and
the referenced spline inventory. Prepared segment identity derives from branch
GUID, `segmentStartPointGuid` and stable `segmentSlot`; it is not array-position
identity.

## Units and transform contract

Spline positions, handles, widths, depth, resampling spacing, opening outset and
projection band are authored metres. `GeologyTransform` places the local chart:

```text
world = translation + rotation(uniformScale * local)
```

The transform requires a finite unit quaternion and positive uniform scale.
Non-uniform or mirrored scale is rejected. The uniform scale multiplies graph
coordinates, widths, depth, spacing, outset and band exactly once.

The local XY axes define chart orientation. The local Z axis identifies the
front side for authoring/debugging but does not replace the actual implicit
surface normal used by the carve. No hidden world-up or mesh-UV convention is
used.

## Surface mapping and field contract

Let `s(p)` be the current resident negative-inside source field, `v` the admitted
voxel size and `n` the normalized centered gradient using one-voxel offsets:

```text
g = (s(p+vx)-s(p-vx), s(p+vy)-s(p-vy), s(p+vz)-s(p-vz)) / (2*v)
n = normalize(g)
q = p - s(p)*n
u = inverseTransform(q).xy
z = -s(p)
```

If `dot(g,g) <= 1.0e-12`, the result is exactly `s(p)`. `q` is the one-step
closest-surface estimate and `u` is the point in the local 2D chart. `z` is
positive inward under Jerico's sign convention.

If `abs(s(p)) > surfaceBandMeters`, the result is also exactly `s(p)`. Every
branch depth and the exterior opening outset must fit inside that band. The band
therefore bounds both the closest-surface approximation and all segment work; it
is not an unused authoring hint.

For each admitted prepared segment, the evaluator finds the closest chart point
and interpolation parameter. It interpolates the endpoint full width and branch
depth, samples the explicit profile at `z/depth`, and constructs:

```text
lateral = distance2D(u, segment) - 0.5*width*profile(z/depth)
axial   = max(-openingOutset - z, z - depth)
segmentVolume = max(lateral, axial)
crackVolume   = min(all active segmentVolume)
fullCarve     = max(s(p), -crackVolume)
result        = lerp(s(p), fullCarve, saturate(strength)*mask(p))
```

Each `segmentVolume` is negative inside its bounded swept section. `min` is the
implicit union; `max(source, -volume)` is the existing negative-inside
subtraction. The small exterior outset makes the cut open at the original zero
surface and is bounded to at most one voxel. The field is evaluated only while
`z` lies inside both the admitted surface band and the segment outset/depth
interval; outside either one it is exact identity.

This is a coherent scalar carving field, not a claim of exact redistance.
Surface extraction and collision consume its sign/topology. Any future consumer
requiring exact Euclidean distance needs a separately authorized redistance
stage.

## Preparation, resampling and self-intersection

Preparation reuses `validateProceduralSpline` and `sampleProceduralSpline` with
`SegmentStable` identity. Its requested maximum spacing is reduced to the most
restrictive of:

```text
authored maximum spacing
0.5 * voxel size
0.5 * minimum surface half-width
0.25 * minimum branch depth
```

This is Jerico's independent physical-scale admission rule. It does not copy the
paper's mesh cross-section merge/split algorithm. The prepared result is rejected
if the limits require more than 128 segments or if any segment is non-finite or
shorter than `1.0e-6 m` after transform/clipping.

In the 2D chart, adjacent segments of one spline and a declared child attachment
may share an endpoint. Any other segment crossing, T-junction, collinear overlap
or repeated edge is rejected with a stable graph-self-intersection error. The
implicit union makes allowed branch junctions watertight without constructing a
self-intersecting swept mesh.

The local planar chart is intentionally bounded. Highly noisy/global surfaces,
chart foldovers and arbitrary carrier surfaces are not admitted. The product
test must keep the chart inside a smooth source region and prove no disconnected
holes. A later carrier/UV system would require its own authorized slice.

## Validation and failure handling

Canonical recipe validation rejects:

- schema/kind/payload mismatch or missing spline references;
- duplicate, cyclic or forward parent identities;
- invalid attachment point or non-coincident attachment position;
- closed/non-coplanar/degenerate splines;
- invalid unit quaternion or non-positive/non-uniform scale;
- non-finite/out-of-range width, depth, spacing, band, outset, probability or
  propagation;
- fewer than two or more than eight profile keys, unordered endpoints or
  negative/non-finite scales;
- enabled non-zero geometry below the admitted voxel resolution;
- prepared graph, storage bytes or voxel-segment work above the hard limits;
- values that cannot be represented by the GPU float contract.

Validation never silently clamps authored state. Stable errors distinguish
schema, graph identity, graph geometry, profile, transform, resolution,
self-intersection, segment-budget, work-budget and GPU-representation failures.

## CPU reference and Vulkan integration

A small `GeologyDirectedCrack` module owns preparation and the double-precision
CPU reference. Tests pass literal prepared segments and hand-derived planar
fields; they do not calculate expectations through the implementation under
test.

The productive `GeologyGpuSession` prepares the graph before allocation,
checking cancellation before and after canonical spline sampling. It uploads one
bounded structured segment buffer and one bounded profile buffer, then executes
one new branch in the existing layer compute shader. The resident input/output
field, masks, classification, hash, dirty comparison, inspections, Surface Nets
and ping-pong behavior are unchanged.

The GPU admission checks:

```text
segmentCount <= 128
profileKeyCount <= 8
segment/profile bytes fit checked 64-bit arithmetic and allocation limits
activeVoxelCount * segmentCount <= declared crack distance-test budget
```

The shader loop bound is the uploaded admitted count; there is no unbounded
graph traversal on GPU. Cancellation remains between preparation, upload,
dispatch and readback boundaries. GPU duration, segment count, distance-test
count and dirty voxels are retained as evidence.

Because subtraction cannot create occupied samples outside the source, topology
planning adds zero occupancy expansion for `DirectedCrack`. The existing source
narrow band must still include the maximum admitted depth/outset; validation
therefore bounds them against the current finite background/band representation.

## Determinism and dirty tracking

CPU and GPU share exact constants for gradient spacing/threshold, seed folding,
branch activation, closest-segment interpolation, profile interpolation, axial
interval and negative-inside subtraction. GPU `float` parity is measured against
the double reference after the existing field quantization/clamp.

Repeated evaluation of the same document/recipe must preserve exact recipe,
field and mesh hashes and produce zero dirty voxels. Changing one branch,
profile key, propagation fraction or seed must change the recipe hash and only
the attributable bounded region of the field. Disabled, zero-strength, zero-mask
and all-zero-propagation variants must be field-identical to the source.

## Material lineage and productive proof

DirectedCrack changes only signed distance. The existing packed geological
classification field passes through byte-identically. Classified Surface Nets,
JRMS 1.1 `GEOL`, exact triangle collision and world/triplanar PBR consume the
result through their existing paths.

The productive proof extends an existing accepted geological product. It does
not add a named rock generator, final preset or dedicated proof mesh. Required
evidence includes:

1. schema round-trip/hash/validation and CPU reference RED/green;
2. preparation/resampling/graph-intersection and topology-plan RED/green;
3. real Diligent/Vulkan parity, repeat identity, branch/seed/propagation,
   disabled identity, dirty work, cancellation and workload rejection;
4. measured width/depth, real open topology, closed remaining manifold and no
   disconnected holes on classified Surface Nets;
5. classified cook/reread, exact collision and material-lineage compatibility;
6. source-free isolated package validation and representative Vulkan Beauty;
7. `git diff --check`, selective publication and exact foreign-inventory
   preservation.

## End-to-end completeness and data lineage

```text
CANONICAL SPLINES + BRANCH IDS + PROFILE + MASK/SEED/TRANSFORM
-> RECIPE-17 VALIDATION / HASH
-> BOUNDED SEGMENT PREPARATION / RESAMPLING
-> RESIDENT SOURCE FIELD SURFACE CHART
-> IMPLICIT SWEPT UNION / NEGATIVE-INSIDE SUBTRACTION
-> PASS-THROUGH GEOLOGICAL CLASSIFICATION
-> CLASSIFIED SURFACE NETS
-> JRMS 1.1 GEOL + EXACT COLLISION
-> ASSET REGISTRY / STATIC-MESH UPLOAD
-> CANONICAL DILIGENT WORLD/TRIPLANAR PBR
-> SOURCE-FREE VULKAN BEAUTY
```

Applicable and required here: typed authoring input, canonical serialization,
CPU preparation/reference, GPU processing, field shader, masks, deterministic
identity, cancellation, bounded resources/work, dirty tracking, Surface Nets,
cook, collision, materials/rendering, debugging/inspection, profiling, tests,
documentation and visible failure handling.

Animation and Sequencer are not applicable to this static authoring operator.
The seed/progression controls do not create a runtime animation contract.

The following remain `INTEGRATION REQUIRED` under existing owners:

- fresh crack-surface semantic classification/editor views;
- Jolt physical geology properties/contact queries;
- classified LOD and geology-aware world-cell streaming;
- runtime remesh, destruction, durability, save/load and fracture ownership;
- navigation, gameplay/Luau, events, particles and audio consumers.

No upstream material, strata, fracture-cell, erosion, cavity or other geological
classification may be discarded. This slice preserves the existing packed
lineage exactly and records the missing fresh-surface semantic rather than
inventing it inside a static carve operator.

## Explicit boundary

This design does not implement durability, rockfall, colluvium, imported-mesh
structural operators, later P1/P2/M9/Task23 work, OpenVDB, restored Geology UI,
named C++ generators/presets, bespoke proof geometry or a general carrier/UV
surface system. It does not start
`docs/JERICO_1_0_PRODUCTION_EXPANSION_GEOLOGY_PWAS_EPLS_ANIMATION_DESTRUCTION.md`.

Completing DirectedCrack will not complete P1, the Geology blueprint or the
JericoEngine 1.0 Persistent Goal.
