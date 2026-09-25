# F4 dense working mesh prerequisite

Status: PARTIAL. This is a tested subdivision operation, not completed geology
authoring, final remeshing or rock-family Beauty. Persistent Goal remains ACTIVE.

The owner requires sufficiently subdivided working geometry before final remesh.
The existing private Manifold dependency provides linear Refine; Jerico now exposes
it as subdivideClosedMesh through SolidEvaluator. No new dependency, runtime API,
alternate generator or renderer path is introduced. Input TriangleMesh positions,
indices and region GUIDs remain unchanged. Output is a new closed working mesh.
This operation does not supply smoothing tangents, so it subdivides the existing
surface without inflating it. It does not yet carry an explicit feature-edge graph.

The API takes edge subdivision count, maximum triangle count and cancellation.
It checks input triangle count times the squared subdivision count before native
allocation, using checked division to avoid overflow. Native status and output
budget are checked; cancellation fences surround the bounded synchronous call and
candidate extraction. No partial candidate is returned. Mid-call cancellation is
not demonstrated as immediately interruptible.

## Evidence

The first build succeeded with an unimplemented operation; the new behavior test
then exited1 at refined.hasValue (f4-subdivision-red.log). Implementation and initial
integration tests exited0. The final dense test uses32 segments per source edge,
with a12288-triangle budget on a12-triangle box. It verifies increased density,
unchanged bounds and volume, every output vertex on the original surface, region
GUID preservation and each region's vertices on its original face plane, closure,
deterministic repeated hash and source immutability. It then invokes the existing
plane splitter on the dense mesh and verifies two closed halves of volume4 each.
Invalid winding/open input, zero subdivision, oversized subdivision, insufficient
budget and pre-cancellation are rejected. The existing BSP boolean tests still run.

Configured target: JericoBspSolidEvaluatorTests, Debug in build/windows-vulkan-editor.
CTest: procedural.bsp_solid_evaluator. Final dense run passed1/1 in1.99 seconds
(test time, not an isolated subdivision benchmark). Logs: f4-subdivision-red-build.log,
f4-subdivision-red.log, f4-subdivision-green-build.log, f4-subdivision-green.log,
f4-subdivision-integration-build.log, f4-subdivision-integration.log,
f4-subdivision-dense-build.log and f4-subdivision-dense-tests.log.
After final error-stage naming, f4-subdivision-final-build.log and
f4-subdivision-final-tests.log also passed (1/1,1.97 seconds test time).
Read-only review raised a possible coplanar-region merge. The explicit fixture
assigns a seventh GUID to one triangle of a coplanar box face, verifies each GUID
retains exactly four descendants at subdivision2, and checks the special region's
vertices remain inside its original triangle. It passes. Native quad pairing uses
SameFace including meshID; different input region runs prevent the suspected merge.
The reviewer withdrew the finding; no concrete scoped issues remained. Final logs
f4-subdivision-region-final-build.log and f4-subdivision-region-final-tests.log passed
(1/1,1.98 seconds). The failed-behavior RED remains the original missing-operation
test, not this disproved review hypothesis.

## Reference and end-to-end boundary

Read-only FULLTEXT01.pdf, PDF pages19-24 (printed15-20), describes tessellation
before corner operations and retaining feature edges across operations. Pages37/39
explicitly report the corner-chipping prototype unfinished; it is not a validated
algorithm adopted here. The native Refine implementation/docs were inspected in
the existing build dependency. No external reference source was edited or copied.

Lineage: source TriangleMesh geometry/region IDs -> linear subdivision -> dense
working geometry/region IDs -> existing plane split. The next consumers still
require integration; this prerequisite is not the requested complete workflow.

| Integration | Status and ownership |
|---|---|
| Input validation, CPU geometry, regions, errors, bounded work, cancellation, tests | Implemented in ProceduralAuthoring; explicit limitation on native-call cancellation |
| Structural feature-edge identity, general geological classifications | INTEGRATION REQUIRED, F4, ProceduralAuthoring; distinguish region boundaries from geometric feature edges and preserve semantics |
| Editor layers, settings, Preview/Cancel, last-applied restoration | INTEGRATION REQUIRED, F4, Editor; expose dense working controls in the existing recipe/workspace, retain source immutability |
| GPU field and final remesh | INTEGRATION REQUIRED, F4, ProceduralAuthoring; connect the dense working stage to existing geological evaluation and final topology without merely subdividing the finished output |
| Cook, serialization, source-free runtime mesh/materials, exact collision/Jolt | INTEGRATION REQUIRED, F4; use existing cooked contracts after complete generation, preserve semantic lineage and prove final visual/physical parity |
| LOD, structural storage, memory/performance, debug/visualization | INTEGRATION REQUIRED, F4; separate working requirements from runtime storage limits and measure representative work |
| ECS, gameplay/Luau/events, save/load, streaming, destruction, navigation | No new runtime behavior in this operation; downstream geology representation and prior applicable contracts remain required before F4 closure |
| Animation, Sequencer, particles, audio | Not applicable to this pure offline geometry operation; no new data for these consumers |

Next: integrate dense working authoring and feature preservation into the existing
geology path, then prove the final remesh and native Beauty rather than accepting
the subdivision test as a visual result.

## Dense source bridge investigation, 2026-09-05

The first experimental fixture was invalid: SdfRegionRecipe requires non-empty
operations and selectedFaceRegion derived from sourceBrush face1. That failure
did not demonstrate a production defect. The corrected fixture keeps a required
additive stamp wholly inside the retained solid, uses a distinct cut region and
does not alter the expected surface through the stamp.

Verified chain: closed box with distinct face GUIDs -> subdivision32 (12288
triangles before cutting) -> x=0 plane cut -> positive closed half -> existing
SparseSdfBuilder BVH sampling. The field preserves the cut GUID and signed samples
at x=+0.25/-0.25 are -0.25/+0.25 within1mm. Source hash remains unchanged.
The subsequent generic meshSparseSdf call fails with:

`procedural.sdf.ownership_ambiguous: An extracted edge has no single stable region owner.`

Logs in build/windows-vulkan-editor:
f4-dense-source-bridge-valid-build.log (build0),
f4-dense-source-bridge-valid-tests.log (CTest8),
f4-dense-source-bridge-diagnostic-build.log (build0),
f4-dense-source-bridge-diagnostic-tests.log (CTest8, exact cause).

This is an intentional generic mesher boundary, not evidence that subdivision
lost regions: SparseSdfMesher.cpp requires equal unambiguous owners at an extracted
edge; SparseSdfBuilder.cpp marks equal-distance competing regions ambiguous.
No production rejection was weakened and no source regions were flattened.
The retained test is explicitly named
`rejectsAmbiguousRemeshOfCutDenseWorkingMesh`; it records this unsafe bridge as
a rejected route, not successful geological remeshing. Final Debug target
JericoSparseSdfBuilderTests build0 and CTest procedural.sdf_builder1/1 pass0,
1.53s test time. Logs: f4-dense-source-boundary-build.log and
f4-dense-source-boundary-tests.log. Requested end-to-end remesh acceptance stays RED.

### Actual GPU integration boundary

Read-only source audit confirms GeologyGpuEvaluationRequest has no TriangleMesh
input. BspBrush/Group/Selection are direct sources; BlockGrammar/AperiodicTiling
materialize temporary BSP documents. The existing source shader scans each
triangle for every sample and determines sign using convex half-spaces
(GeologyGpuShaders.hpp, brushSignedDistance). Passing a concave mesh as a brush
would give incorrect signs; inserting dense triangles also multiplies work.
The GPU result named denseMaster is already the final extracted surface, so
subdividing it alone does not meet the owner's requested order.

The productive continuation must enter the existing GeologyGpuSession source
stage before its structural layers and final Surface Nets extraction. It needs:

1. A closed working-surface input with canonical provenance, retained region IDs,
   checked hash, bounds, triangle/memory limits and cancellation. Canonical recipe
   settings must reproduce its derivation; a transient pointer alone cannot close
   save/load or deterministic regeneration.
2. Correct inside/outside sampling for concave surfaces with acceleration. The
   existing generic CPU BVH sampler is a reference/reuse candidate; its restricted
   SdfRegionRecipe is not an excuse to inject fake stamps into production. Evaluate
   shared sampling/field upload versus GPU BVH against current workload budgets.
   Do not use the rejected generic mesher or revive the cancelled BSP-to-VDB route.
3. Working-source bounds and workload admission in GeologyGpuTopologyPlan and
   GeologyGpuWorkloadGuard, separately reported working/final hashes and counts,
   and region-to-geological-classification lineage through layers and GeologyCook.
4. A concave, cut, multi-region source witness with deterministic source-only
   field samples, then one structural layer and existing final Surface Nets.
   Prove that changing working geometry changes output without mutating source.
   Only then expand to editor controls, native multiview, exact collision and cook.

Next exact action at that checkpoint: define the bounded working-source sampling contract at the
existing GPU source seam and its concave sign/region tests, then implement that
bridge with measured admission. No new source kind, persisted format, alternate
evaluator, renderer or dependency has been added by this investigation.

## GPU working-source bridge, 2026-09-05

Implementation now accepts a transient immutable `GeologyGpuWorkingSurfaceSource`
before the existing geological layer loop. Canonical BSP/procedural provenance and
the working mesh hash are checked independently. The actual chain is:

`closed source -> native linear subdivision -> shared triangle BVH -> GPU signed
source field -> existing PlaneCut/layers -> existing Surface Nets -> existing cook`.

The CPU BVH was extracted from SparseSdfBuilder into the private TriangleBvh.hpp;
its established sampling code remains shared. Concave signs use positive-X parity
with half-open edge ownership rather than convex BSP half-spaces. The GPU traversal
visits nearer children first, has a 32-entry stack and enforces 1024 node/triangle
tests per voxel. An overflow rejects evaluation before layers publish a result.
The existing total-work, dispatch and adapter-memory limits remain unchanged.
Working input is limited to262144 triangles/786432 positions; bounds use referenced
vertices. Macro-heightfield plus working-surface input is explicitly unsupported.

No new canonical source kind, recipe encoding, mesher, renderer or dependency was
introduced. This is an execution bridge, not complete persisted authoring.

### Verification and review trail

- Initial real RED: the request was rejected with
  `procedural.geology.gpu_working_surface_unimplemented`; build0/test1,
  f4-gpu-working-source-red-build.log and f4-gpu-working-source-red-test.log.
- Intermediate failures: private-header include, MSVC literal-size limit, reserved
  HLSL identifier, then traversal exceeding the unchanged1024 work cap. These were
  corrected without increasing the cap. Raw atomic vertex order then produced a
  repeated surface-identity failure despite an identical scalar-field hash.
- Canonical geometry ordering corrected that observed identity failure. Focused
  export build/test0, canonical-source test0, topology guard CTest1/1 pass0.
  Logs: f4-gpu-working-source-final-build.log,
  f4-gpu-working-source-focused-export.log,
  f4-working-source-canonical-test.log, f4-working-source-topology-test.log.
- Shared CPU BVH regression: procedural.sdf_builder1/1 pass0,1.52s;
  f4-shared-bvh-regression.log. Default GPU-session suite invokes only the working
  rejection contract using its existing shared session; the full suite was not
  recertified. Positive acceptance remains a separate explicit RED mode.
- A mistaken export invocation included an extra `.` argument and started the
  default suite. Only that identified process was stopped; its log
  f4-gpu-working-source-final-test.log is not focused acceptance evidence.
- Review required rejection of ambiguous nearest-region correspondence and exact
  duplicate canonical vertex keys on the new path, plus matching CPU/GPU parity
  epsilon1e-12. Those restrictions are part of the implementation. Float packing
  remains a precision boundary; matching epsilon does not prove all-scale parity.

**Final acceptance state: RED.** Once ambiguous provenance was rejected, the
original notched fixture failed at `evaluated.hasValue()` with
`procedural.geology.gpu_working_surface_region_ambiguous`. Moving only the notch
to1.47 and then translating both brushes also failed. These experiments did not
fix the correspondence problem and were reverted. Logs (build0/test1):
f4-working-source-review-*.log, f4-working-source-region-final-*.log,
f4-working-source-offset-*.log. No witness-02 package was produced.

The test modes deliberately distinguish the safety contract from the product:

- `--dense-working-source-boundary`: requires ambiguity rejection, stale/zero
  region input rejection, cancellation and unchanged source. This cannot accept
  a geological product or export a package.
- `--dense-working-source`: retains the complete positive geometry/sign/identity
  assertions and is currently RED at ambiguous correspondence.
- `--dense-working-source-export <material.epak> <output.epak>`: requires those
  same positive assertions before cook/export and currently fails; the final
  implementation cannot regenerate the pre-review diagnostic package.

The default shared-session suite exercises the safety contract only. This is not
a conversion of the positive acceptance criterion to PASS: positive acceptance,
authoring integration and Beauty stay explicitly unfulfilled.

Final unchanged-backend regressions: `--canonical-source` exit0 with cold6/6 and
warm7/7 physical copies/maps, log f4-working-source-boundary-canonical-test.log;
procedural.geology_gpu_topology1/1 pass0 in0.09s, total0.14s, log
f4-working-source-boundary-topology-test.log. These do not establish positive
working-mesh acceptance or a whole-repository PASS.

Checkpoint verification on the final test binary: Debug build0,
`--dense-working-source-boundary` exit0, and independent
`--dense-working-source` exit1 at `GeologyGpuSessionTests.cpp:232`,
`procedural.geology.gpu_working_surface_region_ambiguous`. Logs:
f4-working-source-checkpoint-build.log, f4-working-source-checkpoint-boundary.log,
f4-working-source-checkpoint-positive-red.log. Both processes are terminal.
The positive mode does not require the same input to be rejected as well;
the temporary safety boundary and the future successful remesh have separate
oracles. Resolving the region boundary will require reviewing both deliberately.

Focused fixture: a box with a subtracted corner notch,5120 subdivided input
triangles,4484 final cut triangles in the pre-review export. Source samples check
interior, notch exterior and exterior; a Y-plane cut removes an interior sample.
Repeated evaluation preserves field/surface hashes; a changed working mesh changes
source/field identity; stale hashes and cancellation fail without source mutation.
The pre-review run measured source GPU8.988416ms and CPU validation/index217.1568ms
on the local RTX3090. These are individual stage timings, not frame time, whole
generation time, a performance improvement claim or lower-tier hardware acceptance.

### Native diagnostic images (pre-review, not accepted outputs)

Original1280x720 captures from the pre-review package, inspected without resizing:

- `images/m8-f4-working-source-diagnostic-01/source-vulkan.bmp`: GPU extracted
  source before PlaneCut, not a wireframe of the5120-triangle working input.
- `images/m8-f4-working-source-diagnostic-01/cut-vulkan.bmp`: after Y-plane cut and
  final extraction. The notch and removed upper volume are visibly retained.

Both use native CookedAssetProof, world-scale1, world-y-1, world-yaw295,
camera-z9, camera-pitch-15, the same material/light and source_free=true. Logs:
build/windows-vulkan-editor/f4-working-source-witness-01/source-yaw295-capture.log
and cut-yaw295-capture.log. Package SHA256:
16DDA30B5AB8A3DD2EDD479EAB40C8AD90111496710EE3B8FECB5E9535DC0A7C.
This package predates the safe region rejection and must never be promoted as
accepted cook/material lineage evidence. The images show bounded geometry work
only; neither is a natural rock or a new rock-family Beauty acceptance.

Image SHA256: source
AA415A769E1513ED32D52D7DD24E784CC8B6301B133FE3ECEA4206CF1F9A5E2E;
cut53B48CDEB6079E7F59AC015AA13E21A6A8E099C2B4922B39905B11EDDCC791FB.

### Semantic lineage and completeness

Source regions are projected from the nearest source triangle at each output
triangle centroid. This is provenance projection, not exact region-boundary
preservation. Ambiguous correspondence must reject the entire candidate. The
working hash participates in source identity; retained geometry/region content
participates in surface identity. The scalar field hash remains the actual GPU
field hash. The same rules apply to requested LOD output.

Review limitation: hostSourceBytes counts BVH nodes in use rather than reserved
capacity. Current hard input caps bound the working-side allocation below512MiB;
this estimate is not an exact peak-allocation measurement. Capacity accounting
remains a performance follow-up, not permission to increase those input caps.

| Consumer | Current status / required continuation |
|---|---|
| Input, CPU index, GPU field, layers, final extraction, validation, cancellation | Implemented bounded route; native closed-mesh validation/index are synchronous, cancellation checked at boundaries |
| Editor, settings, save/load, Preview/Cancel/restoration | INTEGRATION REQUIRED, F4 Editor: persist deterministic dense-source derivation in existing authoring settings and pass immutable candidates to this request |
| Feature edges, source regions, geological materials | INTEGRATION REQUIRED, F4 ProceduralAuthoring: preserve useful source/cut semantics, resolve region correspondence and retain material boundaries; projection alone does not close this |
| Cook and runtime serialization/materials | Existing cook creates mesh/collision and retains identities/classifications; INTEGRATION REQUIRED, F4 ProceduralAuthoring/RuntimeFormats: per-triangle source region/material streams are currently not serialized by GeologyCook |
| Physics/Jolt and navigation | Cooked collision is available; INTEGRATION REQUIRED, F4 product proof: exact rendered/collision correspondence, Jolt interaction and applicable navigation checks |
| LOD, memory, profiling, debug visualization | Existing LOD route accepts the working field; INTEGRATION REQUIRED, F4: representative LOD/error/feature tests and hardware-tier budgets; timings above are one local witness |
| Streaming, World Cells, destruction, gameplay/Luau/events | No new runtime mechanism here; INTEGRATION REQUIRED in the existing geology consumer contracts, with cooked semantic data and ownership assessed before F4 recertification |
| Animation, Sequencer, particles, audio | No new direct operation; downstream semantic uses remain subject to the geology audit rather than implied implemented support |

F4 remains IN_PROGRESS and rock-family Beauty remains RED. Next exact action:
diagnose the exact competing face regions/centroids and integrate region/feature
boundary preservation into the existing final remesh. A nearest-centroid winner
cannot solve this. Do not flatten source regions, weaken ambiguity tolerance or
keep moving fixtures. The preserved positive mode must pass before promoting a
package. Then persist/connect dense derivation in the existing editor generation
path and produce the structural rock witness.

## Exact new-face diagnosis and region reference, 2026-09-05

The detailed RED changed the diagnosis. Output triangle1262 has centroid
(-1.2199999888737996,0.11999999731779099,-0.21999998887379962); all three vertices
are on the new PlaneCut surface y=.12. Its two nearest old faces are x=-2 and
z=-1, both0.78000001112620043m away. Their regions are respectively
8E9CC8EF42D94A0DC1D1EA09DDE8ADA1 and12B1ACCB171C21893DBD062E09A5D625.
Log f4-region-projection-diagnostic-test.log (build0/test1).

This is not an original source-face seam and moving the fixture could not create
the missing new-face provenance. The old projection asks a newly exposed face to
inherit one of two unrelated old faces. Existing MeshPlaneCut already has the
required contract: preserve retained regions and assign a fresh interiorRegionGuid
only to new cut faces.

The working implementation therefore builds a post-operation region reference
with existing native splitClosedMeshByPlane, retaining the negative half and
using deriveFaceRegionGuid(layer.header.id,0) for each new cut interior. The GPU
still samples the original dense working source and evaluates the existing layers;
its scalar field and extraction are not replaced by the native reference. Only
the region correspondence queries use the updated reference surface. Materials
remain inherited and are not replaced by region GUIDs. No new recipe field or
runtime encoding is introduced.

Initial coverage is exact full-strength, unmasked, identity-transform PlaneCut and
Rebuild. Disabled/zero-strength layers do nothing. A plane retaining all vertices
is an identity; a plane removing all volume rejects. Native split enforces262144
combined input/output triangle budget; cancellation is checked around synchronous
native calls. Input and reference BVH reserved-node storage are included in host
admission. The existing CPU preparation timer now includes reference cuts/indexing.

Other working-surface operators, roughness, masks and blended cuts explicitly
return gpu_working_surface_region_operator_unsupported until their correspondence
is implemented. This does not change the existing canonical-source operators or
waive their required F4 integration. Source/cut reference projection can still
reject a real seam ambiguity; this is not an exact feature-preserving remesher.

The positive test now also requires the stable cut region on every triangle wholly
on y=.12, and accepts only original regions plus that new region. Positive
generation, editor/persistence, LOD/collision/material serialization and Beauty
must still be verified; this section records the diagnosed fix, not their PASS.

### CPU regression and remaining concave-edge RED

The private `GeologyWorkingRegionReference` module isolates the existing native
region preparation so its geometric contract can be tested without creating a
Vulkan session. Debug `JericoGeologyGpuTopologyPlanTests` builds successfully;
CTest `procedural.geology_gpu_topology` passes 1/1 in 1.43s (2.18s total).
Logs: `build/windows-vulkan-editor/f4-region-reference-cpu-final-build.log` and
`f4-region-reference-cpu-final-test.log`. The initial build exposed an ambiguous
`Detail` namespace in the new test; explicit qualification repaired it.

The test uses the exact failed centroid above: its old-source query is ambiguous
and 0.780000011m away, whereas the post-cut query finds only the fresh cut region
within 1e-8m. It also checks closed geometry, retained regions, deterministic
repeat, two ordered cuts with distinct interior IDs, no-op/empty cuts, unsupported
roughness, zero strength, cancellation and immutable source geometry.

The GPU build before extraction into this module succeeded, but its positive
generation still exited1 (`f4-cut-region-reference-{build,test}.log`). It advanced
past the false new-face correspondence to a different failure: triangle3348,
centroid (0.7364999850591023,-1.4756666819254556,-0.015999998897314072).
Its three vertices are:

- (0.75749999284744263,-1.4600000381469727,0.059999998658895493)
- (0.65999996662139893,-1.4950000047683716,-0.059999998658895493)
- (0.79199999570846558,-1.4720000028610229,-0.047999996691942215)

Both competing faces have the closest point (0.75,-1.4756666819254556,0),
0.020934430207639104m away. Regions D08E5D8DD048CACD5FCAB86820CBF8A1 and
54A34169A48BA249DBB5D48C4C892125 belong to perpendicular +X/+Z faces meeting at
the notch's concave edge. The current mean-crossing Surface Nets placement rounds
that edge. This is a geometric feature defect, not missing cut-face identity.
The safe rejection remains required; witness02 has no accepted package or images.

Next bounded repair: preserve geometric features in the existing extraction.
[Dual Contouring of Hermite Data, SIGGRAPH2002](https://www.cs.rice.edu/~jwarren/papers/dualcontour.pdf)
describes positioning each cell vertex by a quadratic error fit to exact edge
intersections and normals while retaining dual connectivity.
[The Secret Sauce](https://www.cs.rice.edu/~jwarren/papers/techreport02408.pdf)
explains stable factorization and anchoring underdetermined fits near the mass
point. These are research inputs, not an implemented or accepted Jerico solver.

The next RED must cover this exact concave feature as well as a rotated edge,
planar cell and corner, with bounded cell positions and nondegenerate closed
output. Determine the Hermite-data path from the actual final surface before
changing vertex placement; original-source normals alone are invalid after
geological deformation. The exact post-cut reference is valid only for its
explicitly admitted operations. Do not introduce a second remesher, snap to an
axis, discard ambiguous regions or relax the geometric acceptance.

Review found a cancellation-contract mismatch in the private reference helper:
it returned `procedural.geology.cancelled` while the GPU session exposes
`procedural.geology.gpu_cancelled`. Two exact-code assertions, before and after
the bounded native split, reproduced the mismatch (CTest exit8, 0/1, two failed
assertions; `f4-region-reference-cancel-red-test.log`). The helper now returns the
GPU code at both boundaries. The same CTest passes 1/1 in 1.72s, total1.78s, exit0
(`f4-region-reference-cancel-green-test.log`). Review confirms the blocker resolved.

Before that two-string correction, the final GPU module build and both bounded
regressions passed: `--dense-working-source-boundary` exit0 in67887ms and
`--canonical-source` exit0 in68112ms. Logs are
`f4-region-reference-gpu-final-{build,boundary,canonical}.log`. Those durations
include Vulkan/session startup and are not generation-only or frame-time metrics.
The boundary test now verifies explicit rejection of unsupported roughness,
stale/invalid source and cancellation; it does not replace positive acceptance.

Publication binary: combined Debug topology/session build exit0
(`f4-region-reference-publish-build.log`). Positive `--dense-working-source`
exits1 in67272ms and reproduces exactly triangle3348 and the competing regions
above (`f4-region-reference-publish-positive.log:183-184`). No export/capture was
attempted. All execution processes are terminal. Focused review has no remaining
blockers for this provenance increment; geometric acceptance remains RED.
The185 foreign status entries retain their recorded SHA256. This checkpoint
does not close F4, Geometry or M8. Persistent Goal: ACTIVE until full JericoEngine
1.0 closure.

## Feature-preserving placement work, 2026-09-05

Starting publication: e201c0bd. The preceding turn made concrete progress by
publishing new-cut region provenance and reproducing the remaining concave edge.
This increment extends vertex placement inside the existing GPU extraction;
it does not substitute a CPU mesh for the final field or introduce another mesher.

The GPU reference already mirrors the admitted exact PlaneCut stack. Its packed
triangle/BVH data now supplies segment intersections and incident normals for
each sign-changing cell edge. Coincident triangle hits retain their normals;
missing or distinct multiple crossings reject rather than invent correspondence.
The reference retains the original source input and geological layer stack;
working-source distance arithmetic corrections are documented below.

`GeologyGpuHermiteFit.hpp` contains the shared productive HLSL math: streaming
Givens QR of the Hermite equations, one-sided Jacobi SVD of the3x3 factor, and a
mass-point anchor in unconstrained directions. Positions are local to a unit
cell. Singular values below1e-5 of the maximum are truncated; non-finite or
out-of-cell fits reject (2e-5 normalized boundary allowance handles roundoff).
This is not a general constrained minimizer or adaptive topology implementation.
Its numerical acceptance does not establish closed geometric or Beauty acceptance.

The existing `JericoGeologyGpuSessionTests` executable has a focused `--hermite`
mode, also called by its default CTest path. It uploads analytic constraints to
Vulkan and executes the exact shared shader; no new public runtime test API or
CTest inventory entry is added. Initial mean-crossing math reproduces11 coordinate
failures on edge/corner cases while the plane passes (`f4-hermite-contract-red-math.log`,
exit1,442ms). QR/SVD passes the original6 cases, then103 cases covering rotations,
reversed normals, near-singular directions and both out-of-cell sides. The mode
also compiles the productive Surface Nets shader to catch integration syntax
without starting every geology pipeline. `f4-hermite-integrated3-math.log` exits0
in12356ms including that compilation. This is not a frame-time measurement.

The first integrated builds exposed MSVC's raw-string size limit, explicit const
span deduction and the HLSL reserved identifier `triangle`; these were corrected.
They were compile failures, not geometric test outcomes. The original test-only
identifier `point` was likewise renamed before the numerical RED was obtained.

Memory admission includes both original and final-reference CPU/GPU geometry.
Source sampling retains1024 tests/voxel; Hermite gathering enforces1024 tests
across all12 edges of one active cell and a32-node stack. The unchanged536870912
work cap conservatively counts source plus dense extraction and requested LOD
extraction. The CPU budget RED rejected the previously omitted extra work;
focused topology tests now cover one- and two-extraction boundaries. Overflow,
missing/multiple intersections and invalid fits carry distinct flags and a failing
cell into a visible candidate rejection before Apply/Cook.

End-to-end scope: source parameters -> dense source -> original GPU field/layers
-> final-reference Hermite -> existing cell connectivity -> classified mesh and
region projection. The same path applies to requested LOD. Material, collision,
editor, persistence, painting and Shipping consumers retain the earlier explicit
F4 INTEGRATION REQUIRED entries. They require final geometric acceptance and
recertification; a103-case math pass does not close those consumers.

The integrated3 positive run was assertion-aborted, not a geometric result.
After pipeline readiness it waited in a modal Diligent assertion. Read-only
inspection of its own main-thread stack showed `WindowsDebug::AssertionFailed`
-> `RenderDeviceBase::CreateDeviceObject` -> `createStructuredBuffer<GpuTriangle>`
-> `GeologyGpuSession::evaluate`. The dialog reported
`RenderDeviceBase.hpp:437: Overwriting reference to existing object may cause memory leaks`.
Evidence: `build/windows-vulkan-editor/f4-hermite-buffer-assertion.log`.
Only the owned stalled test process was terminated; no canonical run from that
binary followed. New Hermite buffer refs now start null, and canonical aliases
are assigned only in the nonworking branch. The integrated4 run verifies the fix:
no assertion, canonical-source exit0 in72055ms, and the original5120-triangle
working source produces4484 final triangles with closed-mesh, fresh-cut-region
and deterministic-repeat checks passing. This repairs the original triangle3348
concave-edge failure. The complete positive still exits1 because its independent
x-scaled source variant does not evaluate successfully.

`f4-hermite-edge-diagnostic-positive.log` isolates that remaining rejection:
flags8 (no Hermite intersection), cell3647, edge5, voxel(-16,-9,-9).
The edge lies on the scaled source's planar face x=-1.8, from y=-1.08 to-.96 at
z=-.96. The GPU scalar field reports a sign change on that coplanar edge.
Missing intersections and distinct multiple intersections now have separate
flags8/32; the diagnostic identifies the voxel and edge (edge12 means fit failure).
This is a numerical boundary investigation, not authorization to relax region
ambiguity, move the fixture or accept a missing crossing. Safe export remains
gated on the entire positive test, including its scaled-source variant.

The isolated productive distance probe reproduces the suspected source-field
defect without a full session: six coplanar face cases return squared distances
7.10543e-15 or1.42109e-14 instead of zero (`f4-coplanar-distance-red-math.log`,
exit1). Computing perpendicular plane distance only after closest-feature
classification repairs those cases; the full positive then exposes an internal
triangle-edge case at voxel(-16,-11,-1), edge5 (`f4-coplanar-distance-green-positive.log`).
The corresponding edge probe reproduces eight nonzero residues across AB/AC/BC
permutations (`f4-edge-distance-red-math.log`, exit1). Working-source distance now
uses perpendicular line distance for edge interiors and plane distance for face
interiors, preserving the existing closest-feature tests and vertex branches.
Canonical-source arithmetic keeps the historical branch. No epsilon snap,
fixture displacement or Hermite rejection relaxation is introduced. The enlarged
probe also checks points0.125m away from faces/edges against analytic d²=.015625;
validation of this final correction is recorded below when executed.

Final isolated run `f4-edge-distance-green-math.log`:167 cases pass, exit0,
12469ms including productive shader compilation. The full positive remains
RED, exit1 in76487ms (`f4-edge-distance-green-positive.log`), at the same
cell22063/edge5/voxel(-16,-11,-1). The original unscaled source still passes its
closed geometry, regions and repeat checks. This proves the bounded math
corrections, not resolution of the complete scaled-source defect. The next
diagnostic must use the actual incident source triangles and compare field
samples; do not continue adjusting formulas based only on simplified examples.
No new export/package/image was produced. Existing diagnostic images remain
historical; natural rock-family Beauty is still RED.

Checkpoint validation on the final sources: Debug topology build exit0 in9377ms
(`f4-hermite-checkpoint-build.log`), CTest `procedural.geology_gpu_topology`
1/1 pass in1.77s (`f4-hermite-checkpoint-cpu.log`), and canonical-source exit0
in71871ms (`f4-hermite-checkpoint-canonical.log`). The latest full positive RED
above is retained, not reclassified. Focused read-only review found no remaining
blocker in the shared math, canonical branch separation, buffer lifetime,
counter encoding or budget accounting; gather stress/LOD acceptance remains
unproven. All build/runtime processes are terminal. The185 foreign status
entries retain SHA2569a0431d3cdcb45e8e343f6c39179c90b8668f9161b2f8baa681bb737ab94ddd1.
Only this increment is selected for publication; earlier editor WIP stays local.
F4/M8 remain PARTIAL. Persistent Goal: ACTIVE until full JericoEngine1.0 closure.

## Actual source boundary diagnosis and repair, 2026-09-05

Starting publication: fe9fb0b8. That turn made progress by publishing guarded
Hermite placement and retaining the scaled-source RED. This investigation uses
the exact same native document, subdivision16 and x*.9 source in a small Vulkan
probe. It evaluates the productive distance/parity functions against all5120
triangles for each endpoint of the failed edge, without BVH pruning or full
session startup. The default session test also calls this regression; the
focused mode is `--working-source-distance`.

The RED (`f4-actual-source-distance-probe.log`, exit1,12795ms) shows:

- p0=(-1.7999999523162842,-1.3199999332427979,0): d²=0, intersections1.
- p1=(-1.7999999523162842,-1.1999999284744263,0):
  d²=3.0815116619747712e-35, intersections1, closest triangle85.
- Triangle85 has vertices (-1.7999999523162842,-1.3125,-.125),
  (-1.7999999523162842,-1.3125,2.7755575615628914e-17), and
  (-1.7999999523162842,-1.125,-2.7755575615628914e-17).

The source retains tiny native subdivision coordinates around z=0. Cancellation
in the old products used for closest-feature classification erases a valid
interior weight. It chooses an edge and produces a nonzero distance even though
the sample lies in the triangle's x plane. Odd parity then makes that residual
negative, creating a spurious sign-changing coplanar grid edge.

Working-source classification now tests oriented triangle edges relative to the
sample, using precise arithmetic for those predicates. Interior distance is
perpendicular to the face; exterior distance is the minimum over the three
segments, including their endpoints. Canonical-source arithmetic remains on its
previous branch. Source coordinates are not snapped or modified; no tolerance,
region-ambiguity rule or missing-crossing rejection is relaxed.

Primary reference checked for the conditioning of local edge tests:
[Woop, Benthin and Wald, JCGT2013](https://jcgt.org/published/0002/01/05/).
This change is an independently written point-distance classification repair,
not an implementation or claim of that paper's complete watertight ray test.
No new dependency or GPU double-precision requirement is introduced.

`f4-oriented-distance-green-probe.log`: exit0,12752ms,167 analytic cases and
both actual source endpoints at d²=0. Full `--dense-working-source` now exits0
in76312ms: closed original and scaled meshes, fresh cut/source regions,
deterministic repeat, source-change identity, cancellation and immutable inputs.
The original source has5120 working triangles and4484 final triangles;
source-field GPU time is15867744ns and host index/reference preparation is
526546800ns. These measure different stages; the full test wall time includes
pipeline startup and several evaluations. Canonical-source exits0 in71086ms.
Logs: `f4-oriented-distance-green-{build,probe,positive,canonical}.log`.
The default suite now invokes the full working-source positive through its shared
session as well; this does not claim a new full monolithic-suite run.

This closes the demonstrated scaled-source boundary defect, not F4 Beauty or
general LOD/authoring/semantic-cook acceptance. Required downstream integrations
listed above remain active. A fresh export and native visual check follow below.

The first export exposed a second source-only RED, before writing a package:
cell63894/edge5/voxel(6,2,-2), no Hermite hit
(`f4-source-only-diagnostic-export.log`, exit1,73690ms). The expanded actual-source
probe proves that (.84,.24,-.12) and (.84,.36,-.12) both have correct d²=.0144,
but the second returns zero positive-X intersections despite being inside the
known solid (`f4-source-parity-red-probe.log`, exit1). The cut at y=.12 had
removed this defective source-only area from the earlier positive witness.

`workingEdge` now evaluates both endpoints relative to the ray sample with
precise arithmetic. Reversing a shared edge reverses the same determinant,
preserving its sign and the existing top-left policy/tolerance. This is the
same conditioning principle used in the face-distance repair; no source snap or
arbitrary sign is substituted. The final probe includes cavity/exterior samples
with expected even parity as well as the two interior and two coplanar samples.

`f4-relative-edge-green-probe.log` passes the four fault-reproducing samples;
`f4-relative-edge-green-export.log` exits0 in77259ms. The fresh technical package
contains both source-only and cut products plus material/texture dependencies:
`build/windows-vulkan-editor/f4-working-source-witness-02/Packages/candidates.epak`,
2831433 bytes, SHA256
F1AC1835C9D5CAA2CD682F32B24C25A1BB3E784BA3E161A389B96EB2F7BAA167.
Both native captures exit0 with Vulkan1280x720, source_free=true and fallbacks0:

- `images/m8-f4-working-source-diagnostic-02/source-vulkan.bmp`, SHA256
  FC70236B7AED45127CD996582293649ED24EDFF77012190103398592A24C8A19.
  This is the GPU reconstruction of the dense source (3444 vertices,6884 triangles),
  not a wireframe or direct rendering of the5120-triangle working input.
- `images/m8-f4-working-source-diagnostic-02/cut-vulkan.bmp`, SHA256
  439C6320B970ACABA731B972C52812DD15F672889D9A5B6D6C32911316588BE8.
  Final cut remesh:2244 vertices,4484 triangles.

Matched camera/light/material: worldScale1, worldY-1, yaw295, cameraZ9,
cameraPitch-15, FOV40; native images inspected at original resolution. The
concavity remains visible and the cut has a closed top. Smooth field normals
still soften the apparent edges; these block-shaped technical witnesses do not
establish natural rock form, ground contact or rock-family Beauty. Capture logs
are `f4-working-source-witness-02/{source,cut}-capture.log`. Their GPU timing is
unavailable and zero CPU/GPU counters are not performance evidence.

End-to-end follow-up confirmed READ_ONLY against current editor WIP:
`GeologyGenerationJob.cpp` constructs the productive GPU request without
workingSurface. `GeologyWorldPreparation.cpp` rebuilds triangleRegions as the
single mesh GUID. These remain INTEGRATION REQUIRED in F4, owned by the existing
geology job/cook/world-preparation adapters: carry a cancellable, bounded dense
working mesh and its immutable hashes into evaluate; retain region identity
through cooked meshes, publication, RBrush and restoration. Operator correspondence
still admits exact PlaneCut/Rebuild only. No silently unsupported operator or
flattened region stream can be accepted as completion of that editor vertical.

Final-source validation (`f4-boundary-final-{build,probe,cpu,canonical}.log`):
combined Debug build exit0 in16674ms;167 analytic GPU cases plus six actual-mesh
distance/parity points pass in12549ms; topology CTest1/1 pass in1821ms; canonical
regression exit0 in71315ms. The two exterior controls return parity0/2 and
d²=.0080999955534935/.1600000709295273, so the repair does not merely mark all
samples inside. Productive source code is identical to the successful export;
only the extra exterior test cases were added afterward. No full monolithic
suite, general LOD, new rock Beauty or lower-capability hardware PASS is claimed.
Focused read-only review found no remaining blocker in the changed arithmetic,
sign/winding, canonical separation or test wiring. All execution processes are
terminal. Preserve the185 foreign status entries and earlier editor WIP.
Persistent Goal: ACTIVE until full JericoEngine1.0 closure.

Publication also requires explicit `validateClosedMesh` for the source-only
surface before package write (the original/scaled cut surfaces already had it).
The first rerun stopped at the existing no-overwrite precondition, not geometry
(`f4-source-closed-final-export.log`, exit1). Repeating to a new build-local name
passes in74209ms (`f4-source-closed-final-export2.log`):
`Packages/candidates-closed-check.epak` is byte-identical to the captured package,
SHA256F1AC1835C9D5CAA2CD682F32B24C25A1BB3E784BA3E161A389B96EB2F7BAA167.
Thus both source and cut closure checks apply to the retained images' exact
package. Final Debug test build exit0 in13326ms; no productive shader changes
occurred after the final probe/canonical checks. All execution processes are terminal.

## Productive editor / painting lineage audit and real-package RED

2026-09-05, after published boundary fix6c0a5f33. This audit changes the next
implementation decision; it does not certify editor integration or Beauty.

The earlier instruction to preserve regions through GeologyWorldPreparation
*before painting* conflated two different obligations. Current productive
SurfacePaint does not consume triangle region GUIDs:

- `ViewportSurfacePicking.hpp` carries entity/mesh/material IDs and a transient
  triangle index. `SurfacePaintViewportInteraction.cpp` converts a hit into
  position/normal/tangent samples. `Tools/SurfacePaint/.../SurfacePaintRecipe.hpp`
  persists those samples and their geometric projection, without triangle IDs.
- `GeologyCook.cpp::appendRuntimeSurface` canonicalizes triangle order and drops
  source region GUIDs. GEOL stores descriptor and per-vertex classification;
  `RuntimeFormats/.../MeshAsset.hpp` has no persistent region stream or sidecar.
- `GeologyWorldPreparation.cpp` assigns mesh GUID to reconstructed triangles for
  the closed-body collision bridge. This is not the SurfacePaint attachment path.

The actual immediate paint blocker is the projection contract. GeologyCook emits
zero `texCoord0` and geology renders through its own material projection.
`ViewportSurfacePicking.cpp::makeSurfaceProjection` instead requires a nonzero
UV determinant and rejects these triangles before a brush stroke can start.
Retaining region GUIDs alone cannot repair that rejection.

### Executed acceptance RED

Existing target `JericoViewportSurfacePickingTests` now has an opt-in
`--geology-package <epak>` acceptance mode. It reads original mesh, material and
texture payloads using the runtime readers, places an instance with the real
mesh GUID, and aims the productive picker at each geological mesh's bounds
center from +Z. No source geometry, UV repair, synthetic material replacement,
renderer changes or package rewrite participates. The success criterion is a
paintable hit on each actual geological product; default UV-plane tests cannot
satisfy this criterion.

Input: `build/windows-vulkan-editor/f4-working-source-witness-02/Packages/candidates.epak`,
SHA256 `F1AC1835C9D5CAA2CD682F32B24C25A1BB3E784BA3E161A389B96EB2F7BAA167`.
Both the 6884-triangle source and 4484-triangle cut fail with
`editor.viewport.surface_projection_unsupported`. This is an expected open
acceptance failure, not a passing regression or a fixed painting feature.

Initial Debug build exit0 in6218ms, default CTest
`^editor.viewport_surface_picking$` 1/1 PASS in134ms, opt-in exit1 in303ms.
Logs: `build/windows-vulkan-editor/f4-geology-paint-pick-{build,baseline,red}.log`.
The mode retains its positive expectation so an implemented bridge must actually
produce hits; it does not assert that rejection is the desired final behavior.

Final repeat after adding the exact error message: Debug build exit0 in5406ms,
default CTest1/1 PASS in134ms, opt-in exit1 in304ms. Both messages are
`The selected triangle does not have a finite paint projection.` Thus this RED
reaches the UV-projection check after intersection and material/texture lookup;
it is neither a ray miss nor a missing texture. Final logs use
`f4-geology-paint-pick-final-{build,baseline,red}.log`. Package SHA256 before and
after is unchanged. All processes are terminal and focused read-only review
found no blocker. Scope limitation: the +Z/bounds-center camera is valid for
these two witnesses; another hollow product may need its own landmark camera.
This probe only tests entry into painting, not brush application or paint replay.

### Integration decisions and next implementation

| Subsystem | Required input and existing interface | Owner / dependency / milestone |
| --- | --- | --- |
| Dense authoring | Canonical source plus reproducible working-density policy -> worker-local closed subdivided mesh -> immutable `GeologyGpuWorkingSurfaceSource`; preserve lifetime, hashes, cancellation and caps. Job currently passes no working surface. | ProceduralAuthoring + Editor; required operator correspondence; F4 INTEGRATION REQUIRED |
| Painting / materials / renderer | Persisted SurfacePaint samples and projection -> body-local painted result consumed by the geological material, including cross-face continuity. The existing UV projection fails on the actual product. | SurfacePaint + Editor + Render; admitted mapping/bake contract, not a fake picker UV; F4 INTEGRATION REQUIRED |
| Regeneration / save-load / restoration | Preserve accepted recipe, spatial strokes and product hashes until candidate validation and explicit Apply; detect invalid replay/reprojection and restore the applied version atomically. | Editor transactions + SurfacePaint; dense generation and localized paint consumption; F4 INTEGRATION REQUIRED |
| Cooked semantic regions / downstream consumers | Source and newly exposed region IDs survive in the working/remeshed mesh, but not in GEOL. Decide necessary material, physical, destruction and gameplay consumers before extending a format; classification already survives separately. | ProceduralAuthoring + RuntimeFormats + each consumer; versioned contract if persistent IDs are required; F4 lineage decision INTEGRATION REQUIRED |
| Physics / navigation / Shipping | Revalidate actual finalized geometry, classified data and paint assets through existing collision, cooking and source-free consumers. Current technical package proves only the bounded unpainted source/cut path. | Existing consumer owners; final F4 product identities; F4 INTEGRATION REQUIRED |

This is a delta to the slice's end-to-end audit, not a replacement or deferral of
its other consumers. No new runtime format, shader, dependency or authoring UI is
introduced by the probe. Do not conflate source-region loss with the demonstrated
UV/projection rejection, fabricate planar UVs on closed rocks, or modify a shared
repeating material texture and call it localized painting on one body.

Resume the dense-source job bridge first, freezing its persisted derivation and
unsupported-operator behavior. The painting RED is then the concrete downstream
contract to address with the existing SurfacePaint/material pipeline. Full F4,
natural rock-family Beauty and M8 remain PARTIAL; Persistent Goal remains ACTIVE.

## Persisted working density and shared evaluator integration

Contract implemented, following a847ec96. Cumulative authoring schema29
adds required `quality.workingEdgeSubdivisions`:0 uses the canonical source path;
2..32 requests bounded linear subdivision before geology/remesh. One is rejected
because it would promise a subdivision without adding working detail. Schemas1-28
omit the new key and retain their exact canonical bytes; nonzero density in an
older in-memory schema or an unknown serialized key rejects. Schema29 retains
the schema28 explicit Noise semantics; it does not silently migrate old Noise.
Density participates in the existing canonical recipe hash and publication.
No new runtime asset format, dependency, operator graph or evaluator is introduced.

The shared GPU evaluator owns the working mesh through the synchronous evaluation.
After ordinary source materialization it checks the canonical document/hash and
topology bounds, selects brushes with the existing source-selection rule, evaluates
the solid, and refines it within the existing262144-triangle ceiling. BspGroup
means the whole document; BspBrush/BspSelection must not include unselected
brushes. BlockGrammar/Aperiodic sources retain their existing materialization.
The effective immutable working request then feeds the existing BVH, region
reference, GPU source field, Hermite remesh, identity and diagnostic paths.
Explicit external working geometry plus persisted density is rejected as two
competing source authorities. Heightfield plus this density is also rejected.

Cancellation fences surround synchronous native work; there is no claim of
interrupting the native Boolean/refinement call midway. Existing final GPU
budget/cancellation checks remain. Preparation CPU nanoseconds are separate from
BVH indexing and GPU timings. Counts and authored density are deterministic,
not hardware-dependent; the unchanged caps can reject an oversized request.
This does not certify lower-capability hardware performance.

REDs already executed: schema29 recipe round-trip initially failed four positive
parse assertions (`f4-working-recipe-red-{build,test}.log`). After parser/writer
integration the recipe CTest passes1/1. The actual GPU recipe-only request then
failed three assertions, reporting working_triangles=0 despite persisted16
(`f4-working-recipe-evaluation-red.log`, exit1 in71092ms). This proves why merely
serializing the setting or calling the old editor job is insufficient. The new
shared-evaluator path is being checked against that RED and editor controls.

This section does not close the earlier paint-projection RED or region persistence
audit. Geometry-changing operators without working-reference correspondence must
continue to reject explicitly.

### Focused validation of persisted density

The new GPU request takes only the loaded recipe and canonical document, matching
the productive generation job. It now reports5120 working triangles and4484 final
triangles for density16. It verifies the independent native working hash, closed
remesh, repeated identity, selected-brush isolation, stale-source rejection,
conflicting input rejection, incompatible-operator rejection and cancellation.
The cooked product also retains GEOL/GSTR with the same recipe identity.

Two additional REDs preceded their implementation: nondisplacing classification
layers were rejected by the working reference (`f4-working-classification-red.log`),
and the editor had no density control (`f4-working-controls-red.log`). Exact
PlaneCut/Rebuild plus nondisplacing Strata/Durability now retain correspondence;
Strata shape/groove displacement still rejects. The editor candidate control
preserves density across layer selection, rejects1 and restores0 on Cancel
without writing project files.

The first integrated GPU export failed `structural_durability_missing`: the new
test requested structural export with a PlaneCut-only recipe. Adding required
nondisplacing Strata and a valid DurabilityField corrected the test input; no
export validation was relaxed (`f4-working-integrated-recipe-gpu.log`).

Final focused logs under `build/windows-vulkan-editor/`:

| Check | Result | Wall time / log |
| --- | --- | --- |
| Five Debug targets | exit0 |67.405s; `f4-working-integrated2-build.log` |
| Recipe + topology CTests |2/2 PASS |2.546s; `f4-working-integrated2-cpu.log` |
| Persisted recipe GPU + structural cook |exit0 |73.650s; `f4-working-integrated2-recipe-gpu.log` |
| Canonical source GPU regression |exit0 |71.606s; `f4-working-integrated2-canonical.log` |
| Editor layer/density controls |exit0 |16.158s; `f4-working-integrated2-controls.log` |
| Full World UI, first integrated run |exit1 |123.974s; `f4-working-integrated2-ui.log` |

The full UI first demonstrated Generate/Cancel and density4 publication, parsed
the actual saved recipe, matched its hash to the cooked mesh and restored/reapplied
the exact project snapshots. It then failed opening Fracture after the last
restore. The shared publication bridge skipped selection for every Undo, including
restoring an earlier version of a body that still exists. Preserve selection when
that entity remains; retain empty selection when undo removes the original body.
The new test asserts the restored selection before entering ordinary Fracture.
The fix passes the complete World UI flow (exit0,235.795s), including native
generation, dense publication/restoration, ordinary Fracture and later reopen
checks. Editor Debug rebuild exits0 in62.404s. Logs:
`f4-working-selection-{build,ui}.log`. The120s per-generation wait was unchanged.
No new image was generated by this test, and its success is not a Beauty claim.
The focused authoring-publication negatives/restoration suite also exits0 in
7.953s (`f4-working-selection-publication.log`), covering canonical recipe
publication, wrong hashes, noncanonical/oversized settings and the unchanged
4MiB product ceiling. No full Debug suite or separate BSP showcase rerun is claimed.

The coherent publication includes the previously local layer controls, accepted
recipe snapshot/cook checks, cached generation-session lifetime and exact
transaction restoration needed by this UI flow. It includes only the new selection
hunk in the otherwise foreign-dirty main-window file. Read-only dependency review
confirmed no additional source/CMake dependency; structural recipeHash validation
is already enforced by the mesh decoder and catalog replacement precedes UI reload.

Review limits: topology admission currently plans once before native preparation
and again using the actual working mesh bounds. This duplicates bounded CPU work;
consolidation requires preserving both source validation and final bounds, and is
an explicit performance follow-up. Invalid-recipe/input diagnostics retain their
existing precedence over cancellation; native Boolean/refinement remains fenced,
not interruptible midway. These tests do not measure other hardware or establish
natural rock Beauty, painted replay, generalized operator correspondence or full F4.

## Actual member1: smoothed ImplicitBlock boundary

The next product RED uses the existing member1 structural witness unchanged
(irregular polyhedral seed7101, irregularity0.42, smoothing0.035m, disabled warp,
exact cuts and0.06m voxel size), adding only schema29 and density16.
JericoGeologyRockFamilyTests --dense-structural-member builds successfully
in10.714s and exits1 in76.175s at evaluated.hasValue():
procedural.geology.gpu_working_surface_region_operator_unsupported.
Logs: f4-member1-dense-red-{build,test}.log in the configured editor build.
This is the real member recipe, not the concave-prism technical witness.

The first active unsupported operator is ImplicitBlock. Its productive scalar
intersects the prior field with a seeded, transformed convex implicit shape.
Its smoothed boundary is not a collection of sharp planes. Furthermore, the
post-cut working-region mesh is uploaded as Hermite intersection geometry before
it is used for final region projection. Merely assigning new region labels, or
building sharp native plane cuts, would therefore preserve neither geometry nor
the0.035m smoothing. Both substitutions remain invalid.

### Shared analytic boundary prerequisite

GeologyGpuImplicitBlock.hpp now holds the existing productive shader scalar and
hash function with explicit parameters. The original layer wrapper preserves
packing, seed folding, arithmetic order and transforms. No new evaluator,
authoring parameter, runtime format or dependency is introduced.
The private geologyImplicitBlockEdgeRoot performs24 bounded bisections for a
finite sign-changing edge against that same scalar. It finds the implicit
operand's root only; it does not assert that this boundary is active in the
composed final solid, supply a normal, or assign region ownership.

The Hermite probe adds182 cases. Ten shape/transform tuples cover all five
archetypes with recipe admission asserted, full quaternion components,
nonuniform scale and the irregular block's folded64-bit seed. Double-precision
CPU reference roots check120 forward/reverse edges;60 degenerate edges reject.
A separate closed-form rounded seam requires x=y=0.99 for
smoothMax(x-1,y-1,0.04)=0, rejecting the sharp-plane answer1. One nonfinite
endpoint rejects. The closed-form case intentionally isolates the scalar's
zero-skew algebra; it is not presented as an admitted Rhombohedral recipe.

The first standalone GPU probe passed, but the productive operator compilation
found layerMixHash declared after a ValueNoise consumer. Restoring its original
declaration order corrected that integration failure. The productive
--implicit-block test then exits0 in70.535s; --hermite exits0 in13.163s,
with Debug build0 in15.951s (f4-implicit-root-final-*.log). Subsequent review
strengthened admitted shape tuples and all quaternion components; its final
results are recorded below. An intermediate test build required an explicit
Guid byte conversion; no runtime test ran from that failed build.

Final reviewed validation: Debug build0 in18.807s; --hermite exits0 in13.121s
with the182 new root cases and existing Hermite/distance probes. The default
RockFamily test exits0 in146.780s for all four current diagnostic recipes,
including fresh-session deterministic regeneration, classification, support,
closure and cooked mesh/collision identity. Logs:
f4-implicit-root-reviewed2-{build,hermite,family}.log. The default family route
uses its prior canonical source; this does not turn the dense member RED green.
Review found no remaining blocker for this shared-math prerequisite. All build
and test processes are terminal. No whole repository or Beauty PASS is claimed.

### Exact remaining integration

1. Feed the admitted analytic block parameters to the existing Surface Nets
   Hermite path; use this shared scalar for root and normal evaluation. Preserve
   the dense source/post-cut intersections for surviving original boundaries.
2. Admit a root only when it is active inside the other intersected operands.
   Keep bounded traversal and reject missing/multiple crossings or invalid fits.
   Same-sign endpoint rejection follows Surface Nets' negative-inside convention;
   root-only tests are not evidence of final Boolean-boundary correctness.
3. Preserve old regions on surviving source/cut boundaries and derive the new
   block boundary GUID from its layer identity. Validate both master and LOD
   attribution; this does not replace GEOL classification or settle runtime
   semantic persistence.
4. Rerun the real member1 RED, closure/support/collision/cook and replay checks,
   then capture matched dense source, structure and remesh views at native
   resolution. Do not change the recipe to avoid the unsupported operator.

Member1 dense generation is still RED. This prerequisite does not add a new
rock image or claim Beauty, painted replay, operator correspondence or F4 PASS.

## Analytic working-boundary integration and actual-cell diagnostics

The next implementation admits one full-strength, unmasked ImplicitBlock along
with exact PlaneCut and nondisplacing metadata layers. It retains the native
post-cut mesh for polygonal intersections and solves the smoothed implicit
boundary analytically. Shared triangle distance/parity predicates test whether
an implicit root is inside or on that reference; coincident boundaries retain
incident normals. Source hits outside the block reject. Existing traversal,
missing-hit, multiple-crossing and fit guards remain active. Surface constants
are192 bytes; first-error diagnostics retain the QR factor, mass and fitted
position in24 counter words, covered by the existing fixed-buffer allowance.
Master/LOD region attribution compares the native signed distance with the
existing CPU block scalar; the new boundary derives its GUID from the block
layer. Equal or ambiguous attribution rejects. This does not establish cooked
per-triangle GUID persistence or fix the separate RBrush projection RED.

The unchanged member1 shape with density16 initially fails after integration:
flags24, cell109564, edge0, voxel(15,-9,-4), voxel size0.06m. Build0/30.065s,
Hermite0/16.409s and member1 exit1/80.534s are retained in
f4-dense-implicit-integration2-{build,hermite,member}.log. The normal and endpoint
known answers pass; this is not a successful product evaluation.

Rebuild's initial seven-sample majority filter moved the sign boundary while
the working Hermite reference retained the authored surface. The working route
now preserves source signs during that pass, then re-distances magnitudes;
canonical behavior is unchanged and its undeclared majority filtering remains
a separate follow-up. The mathematical reference is SparseSdfRedistance's
source-sign retention and SDF_GEOLOGY_EROSION_FRACTURE_MATH's re-distancing
contract. A seven-point inspection around the failed corner asserts sign
retention between the last authored layer and Rebuild.

This removes flags8: f4-dense-implicit-signs-member.log exits1/75.019s with only
flags16 at cell113694, edge12, voxel(16,-5,-2). The diagnostic rerun
f4-dense-implicit-fit-diagnostic-test.log exits1/79.321s and records a rank-two
fit with three samples. Its closest-to-mass point is
(0.08206194,0.88403362,-0.03307204), outside the cell. However the same minimizer
line intersects z=0 at approximately(0.07101,0.99875,0). This is a feasible
constraint-preserving location, not a reason to loosen the singular cutoff.

The captured QR/mass is now a standalone GPU known-answer RED. Build0/10.996s
and probe exit1/17.426s reproduce the exact invalid point in case354:
f4-hermite-nullspace-red2-{build,hermite}.log. The proposed correction clips only
the sole null direction of a rank-two fit to the unit box and selects the
nearest feasible parameter. Empty intervals, full-rank outside fits and rank-one
outside fits still reject; the existing cutoff and final tolerance are unchanged.
Additional probes cover a near-parallel null direction and a unique outside
solution. Product validation and native imagery must follow before acceptance.

The nullspace correction passes the complete Hermite probe in17.417s after a
25.427s build, retaining the prior impossible-line rejections. The real member
still fails in80.006s, now at cell113613, voxel(15,-7,-3), fit_count5. Logs:
f4-hermite-nullspace-green-{build,hermite,member}.log. Its rank-two row0 is
(1.67052007,0.0626131967,0.340685129 | 2.31760216): its maximum over the unit
box is2.0738183967, below the RHS. The feasible interval is genuinely empty;
neither a different nullspace anchor nor a looser singular cutoff is a repair.

A double-precision diagnostic using the retained member1 recipe identifies the
underlying topology defect. In this cell, edge10 runs from
(0.96,-0.36,-0.18) to(0.96,-0.36,-0.12). The composed scalar at its endpoints
is+0.0025320051 and+0.0118496234m, but at its midpoint it is-0.0084047201m.
Secondary fracture face crosses at t=0.0724980870; Authored fracture2 crosses
at t=0.7074794505. The two five-edge Hermite groups match the normals/positions
encoded by the GPU QR diagnostic. These are distinct exact PlaneCut boundaries,
not noisy normals from the smoothed ImplicitBlock. The diagnostic script is
retained under build/windows-vulkan-editor/f4_trace_actual_cell.py; these
numbers are CPU analytical evidence, not a new GPU sampling acceptance.

The Surface Nets caller skips same-sign endpoints before the existing
multiple-crossing guard can see them. One vertex per cell cannot represent the
two separate local boundary pieces here. The next required work is topology
refinement/multiple-component extraction driven by the exact boundary crossings
and QEF feasibility, consistent with SDF_GEOLOGY_EROSION_FRACTURE_MATH:149.
Keep the real recipe, material/region lineage, hard memory/work limits and
closed-mesh/collision checks. Do not make this specimen pass by clamping QEF,
silently dropping a cut, replacing it with the simple box regression, or merely
changing global voxel size. Cells with all same-sign corners also require an
interval/feature audit rather than assuming empty space.

No new native rock image was generated during this integration attempt. F4,
rock-family Beauty and the full M8 plan remain open; the persistent Goal stays
ACTIVE. Native cancellation remains bounded by the existing before/after
fences, not mid-call interruption, as already documented above.

Final regression validation: GPU test build0/13.749s;
--recipe-working-source exits0/80.206s, including the existing5120->4484
concave/cut product and a new partly overlapping tabular block. That bounded
block retains both original source GUIDs and its new block GUID, is closed,
and regenerates deterministically. It is explicitly not the rock-family
acceptance specimen. --canonical-source exits0/75.604s (cold6 copies/maps,
warm7 copies/maps), and RockFamily default exits0/155.705s for all four previous
canonical diagnostic members with fresh-session repetition. Logs:
f4-working-analytic-regression-{build,recipe,canonical,family}.log. The final
Hermite probe remains0/17.417s. Review found no blocker in the nullspace math or
24-word dense/LOD diagnostics. All processes are terminal. No complete F4/M8
or cross-hardware performance claim follows from these bounded checks.

## Remesh literature and representation audit, 2026-09-05

Read all six pages of Schaefer/Warren's
[Dual Contouring: The Secret Sauce](https://www.cs.rice.edu/~jwarren/papers/techreport02408.pdf).
Pages3-4 discuss mass-point placement and disconnected surface pieces sharing
a cell; replacing a bad minimizer with the mass point prevents certain spikes.
That is not proof of recovering the two roots omitted by Jerico's endpoint-sign
test. The paper still constructs edge topology from sign changes. Pages4-6 also
describe sparse storage and local CSG updates; these are research directions,
not measured Jerico performance gains. Keep the exact member1 RED and its guard.

Authority check: blueprint sections6-7 explicitly allow exact native plane cuts,
post-extraction mesh operations and explicit representation bridges. Section86
allows a final CPU mesh path when needed, without repeated hidden field readbacks.
The statements at earlier checkpoints313-318 and419-420 in this evidence file
describe those increments; they are not a higher-level architectural ban on a
future declared hybrid route. A hybrid final route still needs an explicit
contract for layer order, geometry/field consistency, identity, region lineage,
collision, LOD, budgets and cancellation before implementation. No route changed
in this audit; current production remains native preparation plus GPU extraction.

The user offered to discuss useful new dependencies. A bounded libfive review
found that the [C++ oracle interface](https://github.com/libfive/libfive/blob/master/libfive/include/libfive/oracle/oracle.hpp)
does support custom scalar/interval/derivative evaluation, and
[BRepSettings](https://github.com/libfive/libfive/blob/master/libfive/include/libfive/render/brep/settings.hpp)
supports cancellation and worker configuration. Do not mistake the older C-API
callback limitation for absence of C++ oracles. It is not a drop-in mesh/region
adapter: Jerico would still supply dense-BVH intervals/features, semantic
provenance and deterministic resource contracts. No dependency was proposed for
admission, downloaded as source, installed or integrated. Assess the existing
Manifold bridge against topology refinement before requesting another library.

This checkpoint changes documentation only; no build/test rerun or new native
rock image. The actual member1 extraction remains RED. The user's later lighting
references are recorded in the complete M8 plan B3 without starting that block.

## Native cut bridge and curved-cell RED, 2026-09-05

The user requests a prompt repair of the actual stone and a bounded check of
GeoGen/open-source terrain generators. Reviewed
[JangaFX GeoGen](https://jangafx.com/software/geogen) and its
[published dependency notices](https://jangafx.com/legal/licenses-we-use): public
material describes GPU terrain/planet authoring, but supplies no source for the
required remesh implementation. The notices cover the suite and do not establish
a GeoGen-specific geometry library. [Hesiod](https://github.com/ottolink-dev/Hesiod)
provides open terrain authoring; its heightmap representation cannot represent
this arbitrary closed solid. [Godot Voxel Tools Transvoxel source](https://github.com/Zylann/godot_voxel/blob/master/meshers/transvoxel/transvoxel.cpp)
uses corner signs and one interpolated crossing per selected edge, so importing
it would not recover the hidden pair of roots. No dependency was added.

Added --dense-cut-bridge to the existing RockFamily test executable as a
geometry-only feasibility probe. It takes the actual member1, density16 and
voxel.06; extracts its source/ImplicitBlock before applying the same six authored
exact PlaneCuts with the existing native kernel. It does not replace the
--dense-structural-member acceptance, field lineage or cook with this probe.

Initial build0/11.402s and probe1/75.768s, logs
f4-native-cut-bridge-{build,test}.log. Extraction fails before any native cut at
cell105528, voxel(16,-17,-8), four tangent constraints from the rounded block:
QR row0(1.45526195,-.00435676565,-.330739945,.353566319),
row1(0,.0266502909,.930501163,.500030518),
row2(0,0,.951978147,.516079724),
massSum(.736246467,1.99999976,2.89668131), count4.
The unconstrained point(.365669,-.165333,.542113) lies outside the cell.

Hermite case357 independently derives the constrained optimum on y=0:
z=(.930501163*.500030518+.951978147*.516079724)/
(.930501163^2+.951978147^2), x=(.353566319+.330739945*z)/1.45526195.
The existing solver reproduces the RED, build0/11.086s and test1/17.912s,
f4-bounded-curve-red-{build,test}.log. New bounded solve enumerates the box's
26 faces/edges/corners, re-solving free coordinates for each candidate and
minimizing the original QEF. It does not clamp the unconstrained solution.
Production eligibility requires a smoothed implicit block and zero native
constraints; exact native planes and multi-crossing rejection stay strict.
The first green Hermite run exits0/22.146s, including the previous exact cases.
The complete bridge and integration remain to be verified.

The expanded Hermite RED exposed rank-one tie handling and rejection of an
all-zero system. The corrected solver passes all 26 boundary active sets,
rank-one mass anchoring, zero-system rejection, mixed native constraints and
zero-smoothing guards: f4-cut-bridge-clean-green-hermite.log, exit0/25.226s.
The prefix then exposed two triangles with identical geometric corners.
Working GPU emission now omits only exactly collapsed triangles (no epsilon),
with an explicit emitted-quad counter instead of deriving quad count from
triangle count. This yielded9426 triangles and zero collapsed triangles, but
closed-solid validation still rejected the mesh.

Retained f4-cut-bridge-invalid.obj from f4-cut-bridge-edge-red.log
(exit1/91.410s) proves all14139 geometric edges have exactly two incident
triangles, while241 pairs have the same direction. The shader independently
overrode each triangle's sign-derived winding from averaged surface normals.
The working path now retains the shared sign-derived orientation; canonical
orientation behavior remains unchanged. f4-cut-bridge-winding-green.log passes
(exit0/104.153s):9426 extracted triangles, all six original native cuts,
1926 final triangles, closed volume and all authored halfspaces.

This is geometry-only feasibility, not full actual-member acceptance. The
integration contract is an explicit hybrid permitted by blueprint sections6
and86: retain one admitted pre-cut GPU scalar buffer for working stacks whose
ImplicitBlock precedes the exact cuts; continue every authored GPU layer and
retain the full final field/hash/semantic fields. Extract the prefix against
the uncut source BVH, apply the identical native cuts, retain source/block/cut
regions and resample classifications at final vertices from the resident final
GPU classification field. Rebuild region-separated normals/tangents, bounds,
counts and identities after cutting; apply the same path to requested LODs.
GPU retention/readback and bounded CPU work must be accounted for, with
cancellation and unchanged publication/source lifetime. No intermediate field
CPU readback, hidden operator reordering, source mutation or dependency addition.
Editor/cook/collision/Shipping, LOD, painting and original-resolution Beauty
remain INTEGRATION REQUIRED until their actual consumers are verified.

Hybrid implementation now retains the pre-cut field, keeps full final field
inspection/export/hash, applies native cuts to the extracted prefix, consumes
the float32 canonical closed mesh, separates frames by semantic region and
resamples final GPU classifications. The same finalizer is wired before LOD
counts/deviation/normal checks. Additional classification staging and the full
786432-vertex native host geometry reserve are included in preflight. Native
CPU time, classification GPU time, basis bytes and final triangle count are
explicit diagnostics. The reserve is conservative geometry accounting, not a
measurement of every private kernel allocation.

Integration REDs: f4-native-finalization-green-build.log exposed an accidental
anonymous-namespace link boundary (fixed in the existing owning translation
unit, without changing CMake). f4-native-finalization-linked-member.log
(exit1/91.922s) exposed duplicate temporary prefix attribute keys after full
classification. The native path now canonicalizes geometry through its existing
exact kernel instead of requiring uniqueness of attributes that it replaces.
Canonical and nonhybrid working paths retain their original attribute guard.
f4-native-finalization-canonical-member.log (exit1/87.897s) reached1926 final
triangles, classification, cook and collision; only the consumer test's closure
helper failed because it treated hard-normal render seams as topological holes.
The independent per-face-vertex outward tetrahedron reproduces this helper RED
in f4-closed-seams-red.log (exit1/143ms). The correction pairs exact cooked
positions, with no epsilon, while retaining inward/open/disconnected/malformed
negative cases. f4-closed-seams-green.log passes in102ms. The actual full
--dense-structural-member now passes in88.339s
(f4-native-finalization-final-member.log):3072 working triangles and1926 final
triangles, all authored halfspaces, final Rebuild signs, region/classification,
cook, collision, closed outward volume and support. The fresh export and native
replay results below supersede the earlier pending checkpoints.

End-to-end audit for this increment: source identity, persisted subdivision,
recipe/layer order and existing Preview/Cancel publication interfaces are
unchanged; the geometry and GPU-classification conversion is the changed seam.
Final mesh/collision/structural-field cook and deterministic source-free replay
require the current actual-member package. LOD uses the same finalization but
its actual-member quality/performance acceptance remains INTEGRATION REQUIRED.
Editor reopening/painting, full four-rock Beauty and broad hardware budgets
remain INTEGRATION REQUIRED under F4/B1. ECS, Luau, events, Sequencer, animation,
audio and particles receive no new API or data contract in this repair;
navigation/destruction/streaming continue to consume the existing cooked mesh,
collision and structural-field contracts and retain their F4 downstream gates.
No new dependency or renderer implementation is introduced.

### Verified actual-member repair and native witness01

- Final build f4-closed-seams-green-build.log:exit0/29.125s. Topology contract
  exit0/102ms; actual full member exit0/88.339s.
- f4-native-cut-witness-01-export.log:exit0/179.486s. Four independently authored
  controls cook/read/classify/close/support correctly. Only member1 uses the
  schema29 dense/hybrid repair in this witness; the other three remain canonical
  controls. A fresh Vulkan session reproduces all four rmesh/rcoll byte arrays.
- Member1:3072 working triangles;1188 render vertices and1926 final triangles;
  source8071145322725865162, recipe15719883657597454160,
  surface13138507671608526433. Native cuts/frame CPU2.7590485s, classification
  GPU9.664us, retained pre-cut field1048576bytes on this run. These are named
  substage measurements, not end-to-end generation time or cross-hardware caps.
- Package build/windows-vulkan-editor/f4-native-cut-witness-01/candidates.epak:
  19752890bytes, SHA256 EC4EAE5EAB59B8363879550EA2A89F34D1EC37725186BD3830778C15EA085871.
  The export deliberately keeps accepted=false until visual acceptance.
- Byte-identical package replay from the fresh directory
  build/windows-vulkan-editor/f4-native-cut-witness-01-runtime/Packages,
  with no loose source/cooked inputs. Front and side captures exit0, Vulkan,
  source_free=true, fallbacks=0,1188vertices/5778indices, one draw/material,
  three packaged textures. Fixed exposure/PBR_NEUTRAL; no valid GPU frame
  timing samples, so capture frame_ms is not a performance acceptance.
- Original1280x720 captures were inspected:
  [front](images/m8-f4-native-cut-witness-01/member-1-front.bmp), yaw25,
  SHA256 E018CAA75BB316558BF291D6176EB1D5572D20FB93D054C638E87872E230A933;
  [side](images/m8-f4-native-cut-witness-01/member-1-side.bmp), yaw135,
  SHA256 B6007E4DBE00E31F3035CE12B6D396621CDFCF5BE90EE98280117FB8DF843EC6.
  Both use scale1, y-1.4, camera-z5/pitch-15/FOV40. Logs front-capture.log and
  side-capture.log live in the isolated replay directory.

Status: PASS for this actual-member generation/cook/closed-geometry defect.
Beauty remains RED: the captures show broad fracture facets but still a plain,
overly smooth structural block, without convincing geological surface detail.
No four-rock, editor, painting, LOD or full F4 acceptance is implied. Next use
this stable actual-member route for matched structural/detail/material views
and editor publication; do not reopen the hidden-root solver or change global
voxel size without a newly reproduced failure. Persistent Goal remains ACTIVE.
