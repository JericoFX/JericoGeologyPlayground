# M8 F4: composed source and the next extraction blocker

Date: 2026-09-06. Base: `a0522f27`; exclusive Astra checkout on `main`.
State: **PARTIAL. Source-query repair verified; structural Beauty remains RED.**
The protected LOD witness is unchanged. PlaneCut investigation remains closed.

## Fresh visual baseline

The actual weathered member was regenerated with its original material descriptor
and photographed through the existing native Vulkan host at1280x720, zero
fallbacks, yaw25/pitch-15/distance5/worldY-1.4/FOV40.
[Original capture](images/m8-f4-structural-source-01/baseline.bmp).
The broad regular mass still fails the natural-rock reference. LOD repair did
not supply the missing composition. No material/preset/lighting tuning followed.

## One fixed composition A/B

Both variants keep the same envelope, six cuts, resolution16/6cm, material and
weathering settings. In BOTH, Strata shape/groove displacement is zero: this
isolates source composition and avoids claiming displaced source-region transport.
A fills all eight grammar cells. B removes the positiveXYZ octant at the same
seed, producing seven occupied cells represented by three source boxes.
The original protected fixture is not edited. This is a structural witness,
not a replacement family or a claim to model arbitrary fracture networks.

1. Original B rejected during source sampling:1024-test BVH work budget exceeded.
2. Minimal query correction: reject positive-X ray traversal for nodes with zero
   YZ projected area; discard equal distance bounds that cannot improve the
   scalar minimum. The1024 budget,32-node stack, triangle query and parity rule
   remain unchanged. No buffer, schema, dependency or runtime change.
3. B now passes source sampling and rejects later in analytic Hermite extraction:
   flags16, cell121903, voxel(16,13,-1), four constraints. Neither weathering nor
   LOD of B is reached. Stop the Beauty A/B here; no seed search or capacity change.

The unchanged A control has byte-identical mesh, collision and geometric dump
before/after the query fix. A separate source-only B control removes all shaping
operators and passes native evaluation/cook/capture. Independent mesh checks:

| Source-only result | Measured |
|---|---|
| Triangles / regions |24300 /15|
| Boundary or nonmanifold welded edges |0|
| Signed volume |17.4777760244m3|
| Analytic seven-octant volume |17.477775m3|
| Relative volume error |5.86113e-8|
| Winding at removed-octant probe / three retained probes |0 /1,1,1|
| Native capture |1280x720, zero fallback, exact mesh/collision inputs|

This verifies geometry separately from shading. The source-only photograph keeps
the fixed rock camera and clips the larger raw source at the frame boundary; it
is retained only as diagnostic context, never as Beauty acceptance. Source-only
structural-field export and LOD are not claimed by this control.

## Concrete limit and next repair

The recorded QR system minimizes at local(0.368141,1.391477,1.0), outside its
cell. The mixed native/implicit path uses the strict fit whenever native
constraints exist. A local feasibility calculation retains the source's z=1
plane at(0.385638,1,1), with0.7442mm QR RMS; this is NOT a production fix or proof
for all constraints/cells. It shows why simply declaring the shape impossible
would be unjustified. The source-only result also refutes a general SDF ceiling.

An intermediate control removing ImplicitBlock but retaining all six cuts also
rejected Hermite(flags48) in a different cell and extraction route. Therefore
this evidence does not attribute every source-composition rejection solely to
curvature. Another intermediate source-only draft failed recipe validation due
to retained disabled dependencies; the final control removes those layers.

Next action: use the retained actual B cell as a known-answer witness; inspect
its native versus implicit Hermite constraints and repair their feasible bounded
fit without moving exact native faces or accepting multiple crossings. Then run
the SAME A/B through weathering/LOD/cook. Do not change source seeds, limits,
region ownership, the protected LOD recipe, or post-weathering plane constraints.
No generalized provenance, normal bake, erosion research or new UI is introduced.

## Verification and retained evidence

- Final Debug build of`JericoGeologyGpuSessionTests`: exit0.
- Manual`--structural-source-query`: exit0; independent geometry checks pass.
- Manual`--structural-source-beauty`: remains exit1 at the recorded Hermite RED.
- CTest working LOD, GPU Hermite and actual working LOD:3/3,111.87s total wall.
- Fresh actual LOD:17.014496deg,3000 triangles,2.953137mm. Dense/LOD geometry,
  `.rmesh` and`.rcoll` are byte-identical to the protected prior run.
- Native preview captures both load exact matching collision and report no fallback.
- These durations are test wall time, not generation GPU time or frame time.

The two manual modes take the committed control recipe, a cooked material-reference
mesh and a NEW output directory, in that order. The reference mesh is included in
the archive so replay does not depend on a preexisting material workspace. Build
the configured GPU test target; reuse the existing preview for photography.
The source query runs source+Rebuild only. The Beauty mode deliberately retains
the failing composition. No new CTest claim is made for that RED diagnostic.

Data: [results](data/m8-f4-structural-source-01/results.txt),
[source geometry](data/m8-f4-structural-source-01/source-geometry.json),
[local QR calculation](data/m8-f4-structural-source-01/local-fit.json),
[protected byte identity](data/m8-f4-structural-source-01/baseline-identity.json),
[archive and per-file hashes](data/m8-f4-structural-source-01/manifest.json).

## Completeness boundary

Lineage checked: grammar source -> native dense source -> GPU distance field ->
extraction -> region/classification -> native mesh/collision cook -> renderer.
The source-only control completes that path; the actual structural composition
stops at extraction. The query repair adds no semantic data or consumer interface.
Source/CPU/GPU/cancellation budgets/tests are applicable; existing asset formats,
materials, physics and rendering remain consumers. Original-member LOD/cook are
recertified. Four-member authoring/transactions, RBrush, source-free package,
same-run performance and remaining M8 Beauty gates remain INTEGRATION REQUIRED
under the active slice; no gameplay, streaming, ECS, animation, Luau, event,
Sequencer, navigation, particles or audio capability is newly claimed. Final UI
remains M10.5. Cache WIP remains foreign and unaccepted.

Persistent Goal: **ACTIVE until full JericoEngine1.0 closure**.
