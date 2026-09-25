# F4 actual-member post-cut weathering integration

Date:2026-09-05. Baseline:`f33b3930974e0abad887e7ec37084f514f9c885e`.
Slice:`M8-F4-GEOLOGY-STRUCTURAL-AUTHORING-AND-ROCK-FAMILY-BEAUTY`.
State: **PARTIAL**. Dense geometry/regions/exact collision and native diagnosis
work; structural export and LOD have explicit RED contracts. Beauty remains RED.
Persistent Goal: ACTIVE until full JericoEngine 1.0 closure.

## Input and admitted behavior

Uses the immutable actual-member fixture from
`M8_F4_POST_CUT_WEATHERING_RED.md`: density16, voxel0.06m, working source3072
triangles, implicit block, sampled Strata shape0.045/groove0.015/warp0.15m,
all six authored cuts and terminal Rebuild. No easier no-relief substitution.
The candidate adds one identity/full-strength/unmasked **Convex** curvature
weathering layer after the cuts: spacing0.06m, response0.12m, wear0.06m per
iteration, durability0, four iterations. Concave, transformed, partial-strength
and multiple active post-cut weathering operations remain rejected in this path.
Disabled and zero-iteration controls retain the original geometry exactly.

The public evaluator first evaluates the canonical recipe with this layer
disabled. That produces the actual sampled-Strata/native-cut region reference,
including the existing whole-triangle source/block dominance certificate.
It then evaluates the **original source and complete recipe** on the same GPU
session. The reference mesh is only a provenance BVH; it never initializes this
second field. Strata and cuts are therefore not applied twice to the field.
The final surface consumes the final resident field using sampled Hermite
roots/normals; it does not consume `nativeCutBasis` or reapply the exact cuts.

Region ownership after wear follows the existing **unique nearest-centroid**
projection against the actual prefix geometry. Ambiguity still rejects; every
region present in the reference must survive projection and final topology.
This is not exact pointwise partitioning of every triangle along former face
boundaries. Exact localized RBrush correspondence remains an open F4 integration.
Nonintersecting cuts need not create a region; the actual fixture explicitly
proves all six intersecting cut IDs in the control and weathered result, and
checks the internal reference hash against that control.

Canonical float32 closed geometry and region-separated frames now share the
existing native finalization implementation. The normalized mesh is actually
returned, and its classifications are resampled from the final GPU field before
cook. Render and collision use the identical normalized geometry.

Internal reference evaluation does not publish the dirty cache. Public cache
publication occurs after structural export and LOD validation. Retained cache
memory is conservatively charged during the internal evaluation, and reference
wall time, device requirement, readback, triangle count/hash and final topology
costs have separate diagnostics. This two-evaluation implementation is explicitly
marked as a shortcut pending an identity-keyed bounded reference cache.

## RED and verification sequence

All logs/directories below are under `build/windows-vulkan-editor/`.

- Published initial RED:`f4-working-weathering-final-red.log`, exit1/84.870705s:
  region-operator admission rejected before productive integration.
- `f4-working-weathering-run-03.log`, exit1/90.8990456s: work accounting included
  BVH Hermite traversals that the sampled-field branch does not execute. The
  536870912 primitive-test cap was retained; source traversal, all allocated
  geometry bytes and actual sampled extraction remain bounded.
- `f4-working-weathering-run-04.log`, exit0/90.9020005s: first closed geometric
  result,5460 triangles, six regions; exact disabled recovery. This preceded
  exact collision validation and is not the final cooked result.
- `f4-working-weathering-cache-red-02.log`, exit1/107.5332734s: exact collision
  rejected two zero-area triangles; repeated evaluation also reported nonzero
  dirty voxels because the internal prefix overwrote the public cache.
- `f4-working-weathering-green-06.log`, exit0/108.6271344s: canonical5458-triangle
  result; exact collision roundtrip, all six regions, changed field/geometry,
  packed curvature, repeat equality and zero dirty voxels, cancellation,
  zero-iteration and disabled recovery. Structural export rejection was logged
  separately, not accepted as structural delivery.
- Fresh-process `f4-working-weathering-green-07.log`, exit1/114.5180212s: dense
  result and all three serialized files match06 exactly; the newly requested
  LOD profile fails its normal-deviation guard. Original control cook vertices
  and indices also match the retained pre-change material witness exactly.
- Final core mode:`f4-working-weathering-final.log`, **exit0/102.1946488s**;
  includes all four admission negatives, unchanged retained control geometry,
  exact collision and reference-hash/dirty-cache assertions. Its three outputs
  match06/07 byte for byte.
- Explicit complete acceptance:`f4-working-weathering-downstream-red.log`,
  **exit1/115.0580005s**, exactly two failed assertions: structural export and
  LOD normal deviation. These are desired successful behavior, not EXPECTED_FAIL
  cases; their current failures remain mandatory work.
- `procedural.geology_gpu_topology`: **1/1 PASS,2.04s**, including unchanged
  workload/adapter/memory admission contracts.
- Final publication build and `f4-working-weathering-publish.log`:
  **exit0/102.233398s** after adding the shared finalizer's explicit host-capacity
  guard. All candidate bytes still match06. Final capture-only replay
  `f4-working-weathering-publish-photo.log` exits0 and its original bitmap matches
  the retained weathered image byte for byte; capture mode ignores desktop input.
- Existing independent `--curvature-weathering` regression,
  `f4-working-weathering-regression.log`: **exit0/75.2856542s**. Convex11.7581,
  Concave-10.6201, response1 each, dirty1249/1561, localized mask54; CPU parity,
  durability, iteration, no-op/disabled, structural retention and cancellation
  remain covered. Concave still works in that existing non-working path.

Dense control: recipe4289218905036011389, field7166596416233778912,3512 triangles.
Weathered: recipe13215961280259925244, field1203036994618608041,5458 triangles.
Source identity remains8071145322725865162. Prefix reference has3512 triangles;
its measured wall time was4.9532485s in06. These runs do not certify hardware
budgets or interactive generation latency.

Reproducible commands use the configured Debug `JericoGeologyGpuSessionTests`
target. Core mode:

```text
--actual-working-weathering Tests/ProceduralAuthoring/Fixtures/F4WorkingWeatheringControl.geology.toml NEW-DIRECTORY build/windows-vulkan-editor/f4-strata-warp-material-01/member-1.rmesh
```

The last argument is optional; it preserves the original authored material
descriptor and additionally checks the unchanged control's cooked geometry.
The complete downstream acceptance mode is deliberately separate and must exit
nonzero until both missing integrations are repaired:

```text
--actual-working-weathering-downstream Tests/ProceduralAuthoring/Fixtures/F4WorkingWeatheringControl.geology.toml NEW-DIRECTORY build/windows-vulkan-editor/f4-strata-warp-material-01/member-1.rmesh
```

Neither explicit mode is disguised with EXPECTED_FAIL or registered through the
preexisting foreign CMake changes. An exit0 from core does not accept downstream.

Retained candidate06 (also identical in fresh07), SHA256:

| File | SHA256 |
|---|---|
| candidate.geology.toml | `1D93583060B21B4751DBB9D6AD098946175FC26A85ABAC47D6200DECBCE39350` |
| candidate.rmesh | `274353BFA1986C626F00CD85721C3FE893E97CBEB52BAEB64C648B5CFFDA0147` |
| candidate.rcoll | `12B0311508DB7C56FFDFA8DD081005CF9191A233758A97D87AD7729EEF2906B2` |

## Original native photographs

The native Diligent ImGui diagnostic host now has `--capture-only`: load matched
cooked inputs, render four hidden-window frames, save clean original pixels plus
exact recipe/mesh/collision/view metadata and exit. It does not regenerate or
claim Generate/structural-export acceptance. The user-authorized native tool
exception remains separate from final UI scheduled for M10.5.

`f4-working-weathering-{control-photo-06,photo-06}.log` both exit0:1280x720,
one original authored material, three textures, zero fallbacks, no validation
error. Camera yaw25,pitch-15,distance5,worldY-1.4,FOV40 is identical. No image
editing/resizing or synthetic rendering was used.

- [Control](images/m8-f4-post-cut-weathering-01/control.bmp), SHA256
  `8F0E44390F487F2E8435088975799F292B2E876CFAAA8E86ADA93A458074903E`.
- [Weathered](images/m8-f4-post-cut-weathering-01/weathered.bmp), SHA256
  `B1893EC5DA24B3598FC35672A68F7024D4A52C57DA715A6EC081BFE58D5AE5DB`.

Original-resolution inspection: visible rounded edges and softened strata tiers,
but the overall mass remains too prismatic, with broad planar regions and weak
natural geological structure. **Beauty RED**. This is one matched diagnostic view,
not acceptance of four rock-family products or a source-free Shipping package.

## End-to-end completeness and next action

Lineage: original recipe/dense source -> canonical block/Strata/cuts -> actual
region reference; original source/full ordered GPU field -> sampled final-field
extraction -> unique region transport -> canonical closed topology -> final-field
classification -> dense cook/exact collision -> original native material view.
No erosion-byte claim is made for Convex; its existing packed curvature and
exposure semantics are retained.

| Integration | State / owner / required work |
|---|---|
| Source, authoring data, serialization | Original schema/parameters/identities preserved; no source asset edits. |
| CPU/GPU, renderer, shaders, materials | New orchestration/provenance and final-field consumption; same admitted operators/materials. No dependency or runtime-format change. |
| Physics/collision | Exact normalized static triangle collision verified. Broader Jolt/product support remains F4. |
| Structural field / asset pipeline | **INTEGRATION REQUIRED, F4, ProceduralAuthoring + RuntimeFormats.** Expanded simulation halo exceeds the262144-sample runtime export cap (`procedural.geology.structural_export_budget`). Define and prove bounded retained field coverage/query semantics or the appropriate existing representation; do not raise the cap or silently drop downstream semantic data. Native Generate requests this export and still rejects this four-iteration candidate. |
| LOD | **INTEGRATION REQUIRED, F4, GPU topology owner.** Cell0.12m, deviation0.12m,3000-triangle budget,35-degree feature limit rejects with76.882729-degree normal deviation. Preserve the limit; repair correspondence/topology with the actual reference and rerun the explicit downstream RED. |
| RBrush, tooling/UI, transactions | Stable region IDs retained, but localized projection/UV RED remains. Final UI M10.5; only the authorized native diagnostic capture mode changes here. |
| Debugging, visualization, profiling, tests | Explicit core/downstream modes, cancellation, separate reference cost/hash, original captures. No cross-hardware or FPS PASS. |
| ECS, gameplay/Luau/events, Sequencer, animation | No API change; existing F4 semantic consumers remain required before broader completeness. |
| Save/load, streaming, destruction, navigation, particles/audio | Cooked mesh/collision roundtrip covered; structural snapshot coverage must be settled before claiming procedural runtime-query/destruction/material consumers. Existing milestone integrations remain open. |
| Failure handling | Ambiguity, lost regions, topology, allocation/work and runtime-format guards remain hard failures; internal evaluation does not publish a new public dirty reference. |

Next: close structural export coverage first with the complete actual recipe,
then the measured LOD feature-loss RED. Continue natural-form work against the
retained native comparison; do not declare F4 or the persistent Goal complete.

Preservation target remains the exact189 preexisting short-status entries,
SHA256 `e0407c64c57944500fa01272d526c21f9a248cc7f784799588b3a72ffbf28d08`.
