# M8 F4 sampled contour repair and bounded region correspondence

Status: local topology repair demonstrated; structural composition B remains
PARTIAL at a source-region boundary. Natural-rock Beauty is RED. No M8 closure.
Date: 2026-09-06. Preceding checkpoint:29a6cad1, native mixed-Hermite fit.

## Observable defect and repair

The SAME A/B from M8_F4_NATIVE_CONSTRAINED_FIT.md retains source composition,
resolution, scalar field, four weathering steps, QEF rank, LOD and cook gates.
The old extraction joined four triangles onto one edge around a checkerboard
sampled voxel face. A second diagonal and quarter-size weathering steps did not
repair it. This change addresses that recorded defect, not PlaneCut precision.

Private sampled post-weathering mode2 now associates each crossing with a local
contour cycle, fits each cycle with the EXISTING bounded QEF, and represents the
two arcs on an ambiguous shared face separately. Incident grid-edge polygons
share these arc vertices. Ordinary quads retain their triangulation. No new GPU
buffer, runtime schema, dependency, provenance volume, octree or meshlet system.
Native/analytic extraction and sampled Strata modes retain their own contracts.
The main vertex phase has one fitting call site shared by ordinary/split cells.
Capacity failures still reject; additional vertices consume existing capacity.

Local CPU topology oracle, independent of QEF positions:
- all4096 binary two-cell fields, embedded in closed positive exterior;
- cycle-only connectivity fails1151 cases; separate shared-face arcs fail0;
-128 seeded3x3x3 fields with signed magnitudes10^-5..10^1:0 failures;
- checks opposite two-face edge incidence and single-cycle vertex links.
These are topological checks, not proof against geometric self-intersection.
GPU known-answer RED/GREEN covers both actual neighboring cells and sign reversal.
The actual native A below supplies the position, frame, closure and cook proof.

## Geometry and shading remain separate

| Product | Dense triangles | LOD triangles | LOD angular error | Spatial deviation |
|---|---:|---:|---:|---:|
| Protected original member |5460|3000|17.014496deg|2.953137mm|
| A, zero structural Strata relief |5372|3000|19.7384deg|2.95314mm|
| B, removed source octant |Not published|Not produced|Not measured|Not measured|

A dense/LOD have no bad welded edges, no disconnected vertex links, no zero-area
faces and no faces opposing their final output-normal sum. Both preserve7regions;
mesh and exact collision cook succeed. Volumes:1.5088696411m3 dense and
1.5080244981m3 LOD. The geometric-face/output-normal angle is a DIFFERENT metric:
43.4692deg dense /45.9486deg LOD maximum, not the35deg LOD-fidelity gate.

Protected run-a5c7d210df8a retains all SIX baseline files byte-for-byte against
run-ac84c31ca48e: dense/LOD dumps, .rmesh and .rcoll. Native structural A/B and
weathered A outputs are also identical before/after sharing the shader fit call.
Structural prefixes remain1926tri/7regions and2886tri/10regions, closed, positive
volume and cooked. Their four previously recorded opposed output-normal faces
remain separate; this repair does not relabel them fixed.

![Native A, still Beauty RED](images/m8-f4-contour-repair-01/solid-weathered.bmp)

The unchanged1280x720 native A capture still reads as a rounded, broadly faceted
brown mass. Geometry/cook success does not establish Hunt-reference realism.
Four authored rocks/cliff, family composition and representative Shipping Beauty
remain unaccepted. No new camera, lighting, material or preset tuning was used.

## B: correspondence cause, bounded proof and exact remaining RED

The topology repair removes the earlier raw-face rejection. The next failure is
Euclidean nearest-region projection at a shared source edge. For triangle992,
centroid(0.8352721135,-0.04203972065,-0.01097780932), Euclidean nearest distance is
43.4494mm and two source regions tie at the SAME edge point. Outward geometric-
normal projection instead reaches region E2178D96A95A2C403CBCFF8B9B720ACC.
Seven rays agree; an independent polygon-area calculation proves100% coverage
by source triangles2848/2859, with no closer region intersecting the patch.

The private CPU correspondence helper admits only this strictly bounded case:
projected source patches must cover the COMPLETE output triangle, without gaps,
overlaps, an intervening region, backwards travel or leaving one voxel diagonal.
They must belong to one forward-facing planar region within1um float32 source
precision. The helper changes region assignment only; no vertex/normal is moved
and no old cut plane is reimposed on weathered geometry. Existing unambiguous
nearest mappings remain unchanged. Genuine ties and unsupported coverage reject.

Known-answer RED/GREEN includes missing coverage, region crossings, a small
occluder missed by centroid probes, reversed direction, distance bounds,
nonplanarity, ordering and duplicate patches hiding a hole. The overlap test
first failed and then passed after explicit pairwise overlap rejection.

The SAME B now reaches triangle1444 and correctly REJECTS:
centroid(0.8559331695,-0.01886625464,-0.05642903099), nearest distance59.4993mm.
Its vertices are:
(0.8444655538,0.0090853460,-0.0505911820),
(0.8535562158,-0.0354344398,-0.0287060700),
(0.8697777390,-0.0302496701,-0.0899898410).
Geometric normal:(0.9097554495,0.3173410930,0.2676558478).
Normal-directed rays reach TWO actual owners: E2178D96A95A2C403CBCFF8B9B720ACC
and01B8E3D2EA4D1654C10BAADB378D4448, at55.27..89.96mm. This is a real projected
region border, not another four-face edge or a tunable normal-angle threshold.
A single-owner complete-patch proof cannot certify this triangle. An exact
regional border crossing a triangle requires a topology split or richer regional
representation; tuning scalar parameters cannot add that missing information. Do not select
a GUID winner, discard a region, loosen the proof or claim B cook/LOD success.

Next exact work: preserve this projected regional border in the existing
weathered remesh correspondence, with shared-edge consistency and no movement
of the weathered surface. First prove the retained local crossing, then rerun
THIS B and the protected member. Do not start a general semantic-provenance
framework or repeat seed/step/roughness searches.

## Compilation cost and verification limits

Cold native factory time to Surface Nets pipeline ready:
- first contour integration:185935ms;
- forced loops:180946ms;
- packed local labels:183520ms; no meaningful improvement, packing removed;
- one shared fitting call:144937ms; A geometry/cook bytes unchanged.
Original pre-contour cold witness was47393.7ms. The current integration therefore
still adds substantial preparation cost. A later fresh process with the retained
cache WIP reported1884ms shader-ready and57480ms pipeline-ready. These are
cumulative CPU/wall preparation observations, NOT GPU generation or frame time.
No shader-cache WIP is included in this repair or declared production-ready.
Preparation performance remains INTEGRATION REQUIRED before product closure.

Four focused final gates passed in186.23s total: topology1.98s,
working LOD, GPU Hermite69.74s, protected actual member114.36s. The additional
contour fixture reuses the actual working-LOD verifier to protect the formerly
folded A through dense validation, regional LOD, determinism and both cooks.
This added gate also PASSES in103.90s: run-2e6f0d6fbc34,
3000tri,19.73841157116054deg,2.953137mm. Thus five focused gates passed.
Its scheduling inventory is audited explicitly with the existing GPU lock.
Initial configure correctly rejected the missing scheduling entry; registration
was repaired without changing the scheduling contract or a timeout.

Evidence archive and per-file hashes: data/m8-f4-contour-repair-01/manifest.json.
Archive:82 files,4946029bytes, SHA256
1685c9c37e01f69fc7225d258f499adcd733c5f5972748e08afc016d4b06afb3.
It includes retained failures, topology oracle, CPU projection proofs, native
A/B outputs, independent metrics and regression logs. Main native A/B still
returns exit1 because B remains rejected. That is expected PARTIAL evidence,
not an overall passing integration test.

## End-to-end completeness boundary

Lineage: source/actual cut reference -> unchanged weathered scalar field ->
GPU contour cycles and arc connectivity -> dense positions/normal/classification
-> CPU source-region proof -> existing regional LOD/cook -> native rendering.
Applicable: CPU/GPU, source semantics, materials, topology/LOD, asset/collision
cook, cancellation/capacity failures, debugging, tests, profiling, documentation.
A demonstrates existing rendering/material/physics data consumption; B stops
before publication, preserving the accepted candidate. No material-region stream
or new runtime consumer is implied by this local projection helper.
INTEGRATION REQUIRED in F4/M8: B regional border transport, realistic four-member
Beauty, affected authoring/paint transactions, package-only Shipping, measured
preparation/generation performance and remaining product gates. Existing owners
and interfaces remain those in ACTIVE_SLICE and the geology consumer audit.
No new ECS, animation, Luau, events, Sequencer, streaming, save/load, destruction,
navigation, particles or audio API is introduced. Their downstream applicability
remains in that audit. High-to-low bake and macroform research stay deferred;
final UI is M10.5. PlaneCut diagnostic c029e87b remains closed.

## Follow-up: bounded region partition, not yet integrated

The private partitionWorkingRegionPatch helper clips source triangles onto the
output face, preserves barycentric positions on that face, and reuses the
complete unoccluded patch proof on its children. It introduces no production
call site, new runtime representation or geometry displacement. Work and patch
counts are bounded. Coincident consecutive vertices emitted by clipping through
an existing corner are removed before constructing nonzero fan faces.

The new topology known-answer test first failed at split.has_value(); it now
passes alongside the existing topology tests (2.05s total). It checks two owners,
complete area0.5, unchanged output depth, and rejection of holes, duplicate
coverage and an occluder. Logs: build/windows-vulkan-editor/
f4-region-border-red.log and f4-region-border-green.log. This follow-up is not
inside the immutable82-file archive above.

The local actual-face oracle f4-region-border-local.json gives five patches and
two owners covering triangle1444 exactly. This is local evidence only: inserting
their edge cuts into neighboring faces without cracks is still INTEGRATION
REQUIRED before rerunning B. B remains rejected; Beauty remains RED.

Persistent Goal: ACTIVE until full JericoEngine1.0 closure.
