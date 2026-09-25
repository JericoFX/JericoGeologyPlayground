# F4 structural export coverage

Date:2026-09-05. Baseline:`935d3b84343c822db73735ced5740e5b3fbaee94`.
Slice:`M8-F4-GEOLOGY-STRUCTURAL-AUTHORING-AND-ROCK-FAMILY-BEAUTY`.
State: **PASS for bounded structural export and native regeneration** in the
actual member. F4 remains **PARTIAL**; LOD and natural-rock Beauty remain RED.
Persistent Goal: ACTIVE until full JericoEngine 1.0 closure.

## Defect, contract and implementation

The actual four-iteration post-cut weathering fixture in
`M8_F4_POST_CUT_WEATHERING.md` evaluates512000 resident samples. Productive
GSTR export previously rejected that simulation lattice before examining its
contents, despite the structural body fitting the existing262144-sample carrier.
The fresh pre-change `f4-structural-coverage-red.log` fails the export assertion
with `procedural.geology.structural_export_budget`, and separately fails the
unchanged LOD feature-angle assertion. It is an executed RED, not an expected
failure annotation or a replacement easier fixture.

The current spatial consumer is `sampleFractureField` in
`Editor/JericoEditor/Source/FractureEditorSession.cpp`. It uses the serialized
Float32 voxel size and eight trilinear neighbours for generated cut vertices and
bond centroids. Those positions lie within the final source mesh bounds.
`RUNTIME_FORMATS_AND_CHUNKS.md` and ADR-031 now explicitly define structural
coverage and the source ownership/field-lineage distinction. The codec already
supports a rectangular lattice with arbitrary signed origin; no binary layout,
schema, version, runtime cap or consumer coordinate convention changed.

Export reads and validates the entire resident distance/classification/durability
field. It retains the union of all nonpositive samples plus one voxel of
neighbours and the final Float32 mesh bounds plus interpolation neighbours.
An already bounded full lattice retains exactly its previous layout. An oversized
lattice retains the smallest rectangle enclosing that union. Missing evaluated
neighbours or excess required samples reject; export never lowers resolution,
clamps coordinates, discards solids or fabricates samples. Validation still
rejects invalid samples in the otherwise discardable positive-distance margin.

The full staging byte count remains charged to admission before GPU allocation.
The sample vector is allocated only after coverage admission, with no active
mapping; cancellation unmaps on both per-leaf passes. Physical readback diagnostics
include all three copies and both mappings. No full additional CPU field is
allocated. The private buffer/leaf helpers exist to test exact assembly and
excluded-air validation without a separate evaluator or GPU fault-injection API.

The actual required rectangle has minimum`(-13,-11,-10)`, extent`(31,26,25)`:
20150 samples at the unchanged0.06m authored voxel size. Full evaluated-field,
recipe, source and final-surface lineage are unchanged. `finalFieldHash` continues
to identify the canonical field; the enclosing cooked content hash covers GSTR's
exact retained bytes. No improvement in frame time is inferred from sample count.

## Executed verification

Logs and products are under `build/windows-vulkan-editor/`.

- `f4-structural-coverage-red-build.log` and `f4-structural-coverage-red.log`:
  build exit0; actual downstream run exit1, exactly export-budget and LOD failures.
- `f4-structural-coverage-green01.log`: export/mesh coverage/cook pass; aggregate
  exit1 solely for LOD76.882729deg versus35deg. Thresholds unchanged.
- `f4-structural-coverage-parity01.log`: structural mode exit0 in121.6666403s;
  64 selected GPU samples match retained distance, durability and both packed
  classification words exactly; repeated snapshot is exact, dirty voxel count0;
  one fewer allowed sample rejects. Cook roundtrip preserves GSTR and exact
  collision. This first parity run preceded the exhaustive leaf test below.
- `f4-structural-coverage-final-build.log`: GPU tests, topology tests and native
  ImGui tool build exit0. The user-launched preview PID531216 was confirmed
  absent before relinking the tool; it was not terminated by this work.
- `f4-structural-coverage-final-topology.log`:1/1 PASS,2.06s total. Includes full
  layout preservation, exact negative-coordinate crop, cap/overflow/coverage
  rejection, and all728 retained samples from a4096-sample synthetic resident
  field. A remote negative and a zero sample enlarge the required region beyond
  the initial mesh bounds. All retained distances/durability/classifications and
  neighbours compare exactly. Invalid distance, durability or classification in
  excluded positive air reject.
- `f4-structural-coverage-final01.log`: final structural mode exit0,124.8012237s.
  Includes the exhaustive-tested assembly, probes using the serialized Float32
  step, and exact comparison against the complete original GSTR from
  `f4-strata-warp-material-01/member-1.rmesh`. Fresh parity01/final01 cooked files
  match: mesh SHA256`D15DADE9518C077E130C29950593BA01D77C8A4A23002048F3A817666306005E`,
  collision`12B0311508DB7C56FFDFA8DD081005CF9191A233758A97D87AD7729EEF2906B2`,
  recipe`1D93583060B21B4751DBB9D6AD098946175FC26A85ABAC47D6200DECBCE39350`.
  Collision and recipe also remain identical to the pre-export weathered product.
- `f4-structural-coverage-native01.log`: native ImGui verification exit0,
  99.8646845s. Real phase input changes geometry; Capture retains the exact
  generated recipe/mesh/collision, Cancel preserves the accepted candidate,
  Restore recovers byte-identical inputs and identical pixels.12626 busy frames
  prove the frame loop remained responsive; this is not a frame-rate benchmark.
  Vulkan validation enabled; no validation errors. The original/restored image
  SHA256`B1893EC5DA24B3598FC35672A68F7024D4A52C57DA715A6EC081BFE58D5AE5DB`
  also matches the previous post-cut weathering image exactly.
- `f4-structural-coverage-curvature.log`: ordinary convex/concave curvature,
  CPU parity, durability/mask/iteration/structural retention regression exit0,
  75.2577753s. The geometry field/dirty-count identities remain unchanged.

Original1280x720 native images were inspected and retained under
`images/m8-f4-structural-export-coverage-01/`: `panel.bmp` shows the loaded5458
triangle member and real controls (SHA256`5D276E6D9051C54D1DFA8D9FB862DDD088E1F99CFD347E7CF86E75C159CECA60`);
`generated.bmp` is the UI-free phase-adjusted result
(`67E8EB03E750BF3989C53E44C31BFA390DA84522FDCC99CE5DE18F5C393FD917`).
The clean capture has recipe hash17214987525715108663 and source
8071145322725865162, yaw25/pitch-15/distance5/worldY-1.4/FOV40.
Broad planar facets and a prismatic mass remain; these images are not Beauty
acceptance. The full four-member/source-free Shipping witness remains open.

Reproduce the final producer test:

```text
cmake --build --preset windows-vulkan-editor --target JericoGeologyGpuSessionTests JericoGeologyGpuTopologyPlanTests JericoGeologyPreview -- /m:2
ctest --test-dir build/windows-vulkan-editor -C Debug -R ^procedural.geology_gpu_topology$ --output-on-failure
build/windows-vulkan-editor/Tests/ProceduralAuthoring/Debug/JericoGeologyGpuSessionTests.exe --actual-working-weathering-structural Tests/ProceduralAuthoring/Fixtures/F4WorkingWeatheringControl.geology.toml NEW-OUTPUT build/windows-vulkan-editor/f4-strata-warp-material-01/member-1.rmesh
```

For the native gate, follow `Tools/GeologyPreview/README.md` with `--verify`,
the final01 candidate recipe/mesh/collision, existing authored.rmat and texture
directory in f4-strata-warp-material-01, and a new output directory. Retained
native01/inputs.txt records every absolute input path. Final READ_ONLY review
found no blocking mapping/arithmetic/coverage/contract defects.

The separate `--actual-working-weathering-structural` mode requires structural
export, parity and cook without asserting LOD. The existing
`--actual-working-weathering-downstream` still requires both, and remains RED
until LOD is repaired. Neither mode is full F4/Shipping/Beauty acceptance.

## End-to-end completeness and data lineage audit

Source parameters -> unchanged canonical GPU generation -> complete resident
distance/durability/classification fields -> bounded exact GSTR plus unchanged
classified mesh -> cook/read -> fracture sampling and native renderer.
Classification, curvature/exposure and authored durability remain explicit;
physical strength still belongs to Physical Material. Removed simulation air is
not a source of inferred runtime material or strength.

| Integration | Disposition for this change |
|---|---|
| Source/input data; asset pipeline; serialization; save/load | Same immutable actual recipe, canonical evaluator/cooker and existing GSTR reader/writer. Exact samples and full lineage retained. |
| CPU processing; GPU processing; performance budgets; profiling | Full resident GPU work unchanged; bounded CPU validation/assembly and unchanged carrier cap. Staging remains budgeted. Timings are test wall time on this machine, not frame time or cross-hardware acceptance. |
| Runtime representation; ECS/components; events | Existing immutable mesh/GSTR and prepared-body publication authority unchanged. No new component or event. |
| Physics/collision; destruction/runtime modification | Exact cooked collision remains required. Fracture source-bound trilinear coverage is guaranteed; actual-member full convex-body Fracture publication is INTEGRATION REQUIRED, owned by procedural authority/Fracture in M8 F4. |
| Renderer; shaders; materials; visualization | Geometry and classified material inputs unchanged. Native final replay is a diagnostic gate; Beauty remains RED. No shader/material policy changed. |
| Editor/authoring workflow; tooling/UI | Canonical export removes the producer blocker. Native Generate/Cancel/Restore real UI replay passes. Final UI remains M10.5; this change adds no controls. |
| LOD; streaming | LOD remains INTEGRATION REQUIRED in M8 F4: same0.12m,3000 triangles,0.12m deviation,35deg gate. GSTR stays owned by the source asset, not duplicated into generated chunks. Streaming representation is unchanged. |
| Scripting/Luau; navigation/gameplay queries | No new consumer/API. Future spatial queries must respect explicit coverage and reject outside; exposing geological gameplay queries remains a subsystem integration decision. |
| Animation; Sequencer; particles; audio | No consumer of this snapshot or behavior change in this slice. No fabricated integrations or new data contracts. |
| Debugging; testing; documentation; failure handling | Coverage/budget/coordinate/readback/invalid-field diagnostics; pure exhaustive assembly and actual Vulkan tests. Failed/cancelled export cannot publish the evaluator's dirty cache. |

Future growth, fluid or other volume consumers needing positive simulation space
outside structural coverage are **INTEGRATION REQUIRED**: their owner must define
the required spatial domain and canonical producer/carrier before implementing
that feature. They cannot assume this snapshot contains arbitrary simulation air.
This is outside F4's structural body/Fracture contract and does not remove the
source recipe or complete canonical field from generation.

Remaining F4 work includes actual-member LOD, natural form in four independent
rocks, localized RBrush, multimaterial response and affected criteria6-12
source-free Shipping/publication acceptance. This increment closes none of those.
