# F4 actual-member LOD cause audit, 2026-09-06

Status: diagnosis CONFIRMED LOCALLY; attempted control INSUFFICIENT; M8 PARTIAL.
Production behavior is restored. The actual-member gate remains112.727115/35deg.
PlaneCut evidence at `c029e87b` remains CLOSED and unchanged.

## Decision

The worst112.7-degree pair is not just a global correspondence problem or a
normal accumulation bug. Its dense reference contains a locally folded face.
Independent per-cell QEF placement within an AABB does not ensure that the
connected Surface Nets faces have a consistent geometric orientation.

One bounded mass-point control removes all faces opposed to the sampled field
in this member, but still fails the real angular LOD gate at46.547574deg.
It is withdrawn, not shipped as a repair. No threshold, region policy, recipe,
shader cache, PlaneCut constraint or runtime format was changed by this result.
No further presets, rank thresholds or position blends were tried.

## Separate measurements

Same actual recipe, weathering, field, regions, dense spacing0.06m, LOD spacing
0.12m,3000-triangle cap,0.12m spatial gate and35deg angular gate.
A is current bounded-QEF placement. B retains the gathered edge-crossing mass
point after weathering only. Native/pre-weathering cuts remain on the A route.
The QEF is still evaluated in B; only its returned placement is not used there.

| Measurement | A: current QEF | B: mass-point control |
| --- | ---: | ---: |
| Actual same-region nearest-vertex normal maximum |112.727115deg|46.547574deg|
| Same-region closest-triangle interpolated normal maximum |90.934290deg|46.819505deg|
| Bidirectional vertex-to-triangle distance maximum |0.063546m|0.051588m|
| LOD-to-dense distance, restricted to same region |0.120461m|0.108682m|
| Dense faces opposed to raw sampled-field normals |149|0|
| LOD faces opposed to raw sampled-field normals |38|0|
| Stored vs reconstructed area-weighted normal maximum |0.00000121deg|0.00000121deg|
| Dense / LOD triangles after finalization |5458 /1348|5460 /1348|
| Authored regions including source region |7|7|

Distances above are vertex-to-triangle samples, not a certified continuous
Hausdorff bound. Raw field normals are an independent diagnostic frame, not an
analytic oracle. Face opposition alone is not a proof of global intersection.
The inspected worst quad supplies concrete local folding evidence.

**Geometry:** A's worst dense vertex is (0.497325271,0.464012325,0.419999987).
Its sole incident triangle in region E2178D96A95A2C403CBCFF8B9B720ACC has normal
(0.257239,0.257912,-0.931295), whereas its raw field normal has positive Z.
The quad's original diagonal gives face/field angles8.724deg and105.872deg;
the other gives169.795deg and9.063deg. A diagonal flip alone cannot repair it.

**Normals:** finalization computes the area-weighted geometric normals correctly.
Replacing those normals with a smoother field frame could hide the fold, but
would not repair its geometry. No such replacement was made.

**Correspondence/regions:** comparing nearest points on triangles of the same
region reduces A's maximum to90.934deg, still RED.179/853 coarse vertices have
their unrestricted closest face in another region; dropping region ownership
would mix surfaces and is not accepted. In B the worst46.55deg vertex lies only
0.112mm from the same-region dense surface; the angular difference remains
46.56deg there. This residual is not explained away by vertex spacing or by
choosing an unrelated region. A local coarse geometric frame still differs.

## Bounded controls and limits

The captured scalar field is byte-identical in A and B, SHA256
`de7ac459280a580674e0d530140cb2fe6bf4f1bead838d2e04418b5d313933a2`.
Active dense cells and oriented raw connectivity matched after translating GPU
atomic vertex IDs back to cell IDs. Comparing raw vertex IDs across dispatches
would be invalid. Final connectivity differs by two degenerate A triangles
which become nondegenerate in B; there is no claim of identical final topology.
B moves corresponding dense vertices by at most55.0615mm. Its GPU mass placement
matches independent CPU reconstruction within0.000073mm.

At the worst A cell(8,7,6), the unit-cell QEF position is
(0.288755,0.733539,1.0), versus crossing mass(0.646919,0.650043,0.662117).
The sampled-normal matrix singular values are2.44457,0.153584,0.0216664.
This does not demonstrate a floating-point precision failure. The sampled
tangent-plane objective and its box constraint do not enforce face validity.

A fixed twelve-step offline projection of QEF vertices toward the trilinear
scalar zero reduces scalar residual RMS1.141mm to0.00392mm, yet leaves132 raw
faces opposed to the field (153 originally, including raw degenerates). This
is a negative diagnostic, not a candidate production algorithm or guarantee.
The mass control was the only alternate placement run through the full GPU
actual-member downstream test. It fails35deg and has no Beauty acceptance.

## Cluster/meshlet assessment

The current geology LOD does not run a global mesh simplifier. It runs the same
GPU extraction with a larger grid stride and projects regions independently.
The first defect exists in the dense mesh before that operation.

A cluster that merely groups these triangles cannot repair their placement.
A cluster-based, boundary-aware simplifier could be evaluated if replacing
this LOD producer becomes necessary: it would have to preserve regional seams,
closed collision geometry and angular error. That is a different producer and
requires cost/integration evidence, not just a meshlet buffer. No reasonable-cost
general repair is established by this experiment, so none is introduced now.
The user's condition for a global simplification/correspondence diagnosis was
not met by the112.7deg witness.

## Minimum repair and next blocker

Confirmed repair target: valid local face geometry during sampled extraction,
followed by angularly faithful coarse geometry. Normal summation alone and the
correspondence metric alone are insufficient. A mass-point replacement is also
insufficient. No production-ready minimum repair has been demonstrated.

The current independent-placement/fixed-stride route cannot guarantee these two
properties through its existing constraints. This is a limit of the current
algorithm, not proof that SDFs, GPU meshing or Jerico cannot reach the objective.
Do not spend another preset loop trying to satisfy an unconstrained property.

Next executable RED remains the actual-member LOD35deg gate, with the retained
folded quad as a local geometry regression. A subsequent repair must address
face validity and angular preservation together, within existing M8 contracts;
otherwise it needs an explicit bounded approach change. Do not repeat this audit.
Then resume structural Beauty, the four real rocks and remaining M8 gates.

## Reproduction, evidence and withdrawal

Base: `c029e87b845b9426f17347d27bd3ecb8b45758f4`, plus the preserved shader-cache
WIP already present in the checkout. That WIP was fixed across both variants,
not repaired, accepted or included in this checkpoint. Runtime Vulkan/RTX3090.

Evidence in `data/m8-f4-lod-cause-01/`:

- `snapshots.zip`: A/B raw/final meshes, sampled fields, cell maps, region
  reference, analysis JSON and complete run/build/topology logs. SHA256
  `627d021ea04f467b8a79a786407aa00c5b2f0ebd6c0eba07b55c752f1e2f748a`.
- `manifest.json`: SHA256 of each archived input/log.
- `capture-only.patch`: withdrawn, opt-in capture instrumentation against the
  base; no persistent buffers or capture environment switch remain in source.
- `mass-control.patch`: the withdrawn two-line B delta, applied after capture.
- `A-analysis.json`, `B-analysis.json`, `A-qef-analysis.json`,
  `control-identity.json`: measurements accessible without unpacking meshes.

Both patches pass `git apply --check --cached` against the base index. On a
CRLF checkout use `--ignore-space-change` for patch application. They are
experiment records, not an instruction to resume generation or overwrite WIP.
Replay each variant with a fresh output directory and diagnostic directory:

```powershell
$env:JERICO_GEOLOGY_LOD_DIAGNOSTICS = '<absolute diagnostic directory>'
./build/windows-vulkan-editor/Tests/ProceduralAuthoring/Debug/JericoGeologyGpuSessionTests.exe --actual-working-weathering-downstream Tests/ProceduralAuthoring/Fixtures/F4WorkingWeatheringControl.geology.toml '<fresh output directory>'
```

Build target `JericoGeologyGpuSessionTests` in `build/windows-vulkan-editor`,
Debug. After unpacking, offline measurement needs Python/NumPy only:
`Tests/ProceduralAuthoring/AnalyzeGeologyLod.py <A-or-B-directory>` and
`Tests/ProceduralAuthoring/AnalyzeGeologyLodQef.py <A-directory>`.

Actual downstream A reproduced112.727115deg twice; B reproduced46.547574deg.
All three processes exited1 at the angular gate. The structural export retained
20150 samples;64 probes, repeat identity and dirty0 passed in both variants.
Capture and B source changes were surgically withdrawn. Restored native target
build passed; `procedural.geology_gpu_topology` passed1/1 in2.31s. No restored
actual-member GREEN or new native Beauty is claimed.

Preservation verified after withdrawal: the original193 dirty entries remain,
status digest `14e32188c2c563e61006ea6835d989c59a6a1e4a2760b9a963083d372fbae489`
(UTF8 short-status lines joined with LF, no final newline). No production source
is included in this checkpoint. Both offline analyzers executed successfully;
all24 archived file hashes and archive integrity were verified.

Completeness/lineage audit: authored recipe -> sampled GPU field -> fitted mesh
-> region split/geometric normals -> LOD gate is traced with real data. Existing
materials/collision/cook depend on those final meshes; runtime LOD, streaming
and M8 performance remain INTEGRATION REQUIRED (owner: geology/cooker, M8;
input: accepted closed classified topology levels). Debugging evidence changes
only. No source assets, editor/UI, ECS, runtime schemas, physics, animation,
scripting/events, save/load, destruction, navigation, particles, audio or other
downstream behavior is changed or newly accepted. UI remains M10.5.

Persistent Goal: ACTIVE until full JericoEngine1.0 closure.
