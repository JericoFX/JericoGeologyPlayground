# F4 actual working-member LOD repair, 2026-09-06

Status: **PASS for the actual-member geometry/LOD gate. M8 and Beauty remain
PARTIAL/RED.** This is a protected technical baseline, not permission to relax
the gate during Beauty work. PlaneCut diagnostic `c029e87b` remains closed.

## Result and frozen acceptance

The real `F4WorkingWeatheringControl.geology.toml` downstream test completed
with exit 0 using the production Vulkan field/extractor and native finalizer:

| Measurement | Previous production | Repaired production | Gate |
|---|---:|---:|---:|
| Maximum same-region LOD normal deviation |112.727115 degrees|17.014496 degrees|<=35 degrees|
| Dense triangles |5458|5460|Valid closed topology|
| LOD triangles |1348|3000|<=3000 and fewer than dense|
| Bidirectional vertex-to-surface maximum distance |63.546 mm|2.953137 mm|<=120 mm|
| Raw/final dense faces opposing sampled field normals |153/149|0/0|0|
| LOD faces opposing retained sampled field normals |38|0|0|
| Authored regions |7|7|All retained|
| Region-boundary edges, dense / LOD |Independent grids|397 / 397|Exact positions and ownership|
| Closed mesh, mesh cook, collision cook |LOD rejected|PASS|Required|

The geometric distance is the existing bidirectional **vertex-to-triangle**
metric, not a certified continuous Hausdorff bound. Shading normals are rebuilt
from area-weighted geometric face normals, separately per region. Stored vs
independently accumulated frames differ by at most 0.00000121 degrees.
Closest same-region surface normal comparison also measures17.014496 degrees.
All LOD vertices retain exact dense positions; no normal-map transfer or dense
normal substitution was used to pass. No legitimate sharp edge is erased by an
adjacent-face 35-degree filter:35 degrees measures cross-LOD fidelity.

The first native Debug run measured400507 microseconds of **CPU** endpoint
reduction. It is not GPU generation time, frame time, a hardware-independent
budget, or a startup-cache result. Existing finalization resamples material
classification on the GPU and includes host work in post-cut topology timing.
The fresh automatic run measured523905 microseconds; timing is deliberately
excluded from deterministic content identity.

## Cause and minimum repair

1. **Dense geometry:** numerical rank alone fitted weak directions in nearly
   parallel reconstructed Hermite constraints. A bounded QEF can still fold a
   face without leaving its cell. The post-weathering sampled path now applies
   a0.1 squared-singular-value feature-rank floor to unit-normal constraints.
   Weak directions retain the mass point. Exact/native and the other sampled
   paths keep their numerical rank policy. A dense orientation guard rejects
   post-weathering faces opposing their sampled field before publication.
2. **Normals:** accumulation was correct. Correcting the geometry fixes its
   input; no shading override is needed.
3. **Correspondence/regions:** reprojection from a newly extracted coarse grid
   shifts regional boundaries and local neighborhoods. Using the dense mesh as
   the coarse region reference alone did not solve the residual.
4. **LOD production:** post-weathering working surfaces now reduce the accepted
   dense mesh. Deterministic endpoint QEM ordering locks region boundaries,
   checks the manifold link condition, rejects degenerate/inverted local faces
   and rejects any collapse whose affected geometric vertex frames exceed the
   original dense region's angular limit. The existing native finalizer,
   classification resampling, spatial/normal/closed/cook gates remain in force.

This is backend-only authoring processing. It adds no runtime schema, permanent
GPU buffer, dependency, generic provenance, adaptive grid or cluster subsystem.
It never reimposes pre-weathering cut planes. Other geology LOD producers retain
their existing stride extraction. For the repaired path, the profile's triangle
and error budgets govern reduction; cell spacing no longer defines a fresh
coarse sample lattice. Endpoint-only reduction can refuse an unachievable budget
instead of removing a required seam or weakening angular fidelity.

Primary sources studied as algorithm descriptions:
[Schaefer and Warren, Dual Contouring implementation report](https://people.engr.tamu.edu/schaefer/research/techreport02408.pdf)
for feature rank and mass-point null directions;
[Garland and Heckbert, Surface Simplification Using Quadric Error Metrics](https://www.cs.cmu.edu/~garland/quadrics/quadrics.html)
for accumulated plane-error ordering. No donor implementation was copied.

## Bounded experiments and stopping decisions

All controls used the retained actual weathered field; no preset tuning or
PlaneCut experiment was reopened. Offline finalization was a diagnostic proxy;
the final17.014496 result was independently reproduced by the native pipeline.

| Control | Dense / LOD opposed faces | Max LOD angle | Decision |
|---|---:|---:|---|
| Legacy replay |153 /38|112.727115|RED reproduced|
| Published mass control |0 /0|46.547574|Withdrawn, previous audit|
| Feature rank, coarse re-extraction |0 /0|44.641822|Dense repair candidate; LOD still RED|
| Feature rank + all fine Hermite constraints in coarse QEF |0 /1|56.229967|Rejected|
| Feature rank + dense-region reference for coarse grid |0 /0|46.947772|Rejected as sufficient repair|
| Feature rank + field-aligned diagonal selection |0 /0|52.909491|Rejected as sufficient repair|
| Feature rank + locked-region endpoint reduction |0 /0|17.014496|Native downstream PASS|

An earlier edge-root triangle-fan geometry control produced251 opposed faces
and6 degenerate triangles; no production fan was introduced. Better triangle
field alignment alone did not guarantee cross-LOD fidelity. No cluster/meshlet
unit was needed: the bounded whole member reached the budget in the existing
host finalization stage. Revisit partitioning only for a measured scale/cost
problem, not for this closed112.7-degree witness.

## Automated regression and reproduction

Retained evidence: `docs/evidence/data/m8-f4-lod-repair-01/proof.zip`,
SHA256`4d9cbb3312163302ebf8c47f17c92277af9eacf1d1dca53ad0f6fe52bbf36666`
(65 files,4630505 bytes), with per-file hashes in `manifest.json`. Includes
actual scalar input/origins, all bounded controls, their withdrawn test-only
source, native cooked products, raw/final meshes, analyses and logs. The native
analysis and397-edge identity proof are also stored unpacked beside the archive.

Final verification: the four focused CTest regressions passed4/4 in187.60s
(actual member154.67s). The repository scheduling contract passed1/1. Ordinary
`--classified-lod1` passed. A fresh process reproduced dense/LOD text and cooked
mesh/collision bytes exactly; see `fresh-process-identity.json`, `ctest.log`,
`fresh-native.log` and `scheduling.log`. Shader-cache WIP was present in the
checkout during testing and remains uncommitted/unaccepted; this repair does
not depend on or claim that separate cache change.
After the loose-cap RED/GREEN correction, the final working-LOD and actual-member
CTest run passed2/2 in135.63s. All six geometry/cook hashes remained exact.
Final extraction replay also demonstrates legacy153/38 opposed faces (expected
exit1) versus repaired0/0 (exit0), independently of the shader-cache WIP.
Readable unpacked Vulkan logs have ANSI color/trailing blanks removed; their raw
bytes remain in the archives. `verification-raw.zip` SHA256 is
`275b4a601f297a8b0ce6b7461b2161e1afc1c37b29bb87d5079d1dda07a25fc0`.
Owned code/evidence passes staged whitespace checks. The separately imported
owner research retains its seven original Markdown hard-break lines verbatim;
its staged SHA256 matches the original99206ef8...f1bdc47.

- `procedural.geology_working_lod`: closed subdivided box, region boundary
  identity, geometric frames, reduction, determinism, impossible budget and
  cancellation. A RED/GREEN case also protects profiles whose triangle cap is
  above the dense count: the result must still be cheaper. This does not change
  the actual member's3000-triangle target.
- `procedural.geology_gpu_hermite`: existing analytic/numerical known answers
  plus weak-direction geometric rank and retained strong-edge constraints.
- `procedural.geology_actual_working_lod`: the complete real weathered member,
  not a cube proxy. Fresh output each run; requires<=35 degrees,<=0.12m,<=3000
  triangles, closed mesh, every authored region, exact regional boundary edges,
  dense structural export/parity, mesh/collision cook and round-trip agreement.
- `procedural.geology_gpu_topology` and ordinary `--classified-lod1`: existing
  unaffected producer/profile regressions.

Build the configured Debug `JericoGeologyGpuSessionTests`,
`JericoGeologyGpuTopologyPlanTests` and optional `JericoGeologyLodFieldReplay`.
Run the named CTest regressions. The real test writes its measured result and
proof directory to `Tests/ProceduralAuthoring/actual-working-lod-proof/latest.txt`
under the configured build tree, with full output in `latest.log`.
The source fixture is committed; the actual regression has no retained-field
or local material dependency. The optional field replay isolates extraction:
two directory arguments are retained field input and fresh output;`--legacy`
selects the previous sampled mode and must fail its orientation guard.

## End-to-end completeness and remaining M8 scope

Lineage: unchanged source/recipe -> existing GPU scalar weathering -> repaired
dense Hermite extraction -> authored region transport -> closed geometric
frames -> reduced LOD with exact regional boundaries -> GPU classification ->
mesh/material/collision cook. Region and geological classification remain the
existing semantic carriers; no intermediate generic provenance is required.

| Integration | Disposition |
|---|---|
| Source, generation, intermediate/final mesh, CPU/GPU, LOD, error/cancel, metrics |Implemented and exercised in actual downstream test|
| Materials, frames, serialization, asset pipeline, collision |Existing finalization/resampling/cook reused; dense and LOD artifacts retained|
| Editor workflow, transactions, undo/redo, tooling/UI |Existing recipe/session entry point; no new controls. Final UI M10.5|
| Renderer/shaders, visualization, runtime representation |Cooked attributes unchanged. Original-resolution Beauty acceptance still required; technical LOD PASS does not replace it|
| Runtime LOD selection/streaming, destruction/modification, gameplay/navigation queries |Existing downstream contracts consume accepted cooked data. This gate does not claim new runtime level selection or streaming proof; remaining M8 gates must exercise them where required|
| ECS, simulation CPU/GPU, animation, Luau, events, Sequencer, save/load, particles, audio |No new inputs or behavior from this backend-only mesh reduction; existing cooked mesh/physical consumers remain owners|
| Testing/documentation/failure handling |Focused deterministic/negative/GPU/actual regressions; unreachable budget fails closed|
| High-to-low normal bake |INTEGRATION REQUIRED, deferred after Beauty/four rocks by user choice. Needs stable UV/parameterization and projection, authored material binding, cook and runtime sampling; owner procedural authoring/material pipeline. Current geology cook writes zero UV0; no high-to-low baker found|

Next: Beauty structural review, then the four real M8 rocks and remaining gates.
The research note `docs/research/JERICO_FAST_EROSION_MACROFORM_RESEARCH_FOR_ASTRA.md`
is preserved unchanged, not implemented. Normal bake remains a future visual
improvement; it is not needed to repair the geometry validated here.
Persistent Goal: **ACTIVE until full JericoEngine1.0 closure**.
