# Rock v2 — backend evidence and visual rejection

Date: 2026-09-08. Scope: `Tools/BlenderWorker/**` only. **PARTIAL**, no native
integration or Beauty admission. The user-approved target is equal or better
than the purchased addon references. That target has **not** been achieved.

## Delivered code

- Five original v2 graphs and definitions, separately versioned from seven v1
  generators. Existing catalogue, worker, schema and layer routes are reused.
- Irregular convex support hull, independent macro/meso fields, differential
  bedding, finite inclined fracture cutters, individually capped/segmented columns,
  volumetric rounding and generated-island surface-area filtering.
- Own procedural preview shader; not copied addon materials and not runtime bake.
- Actual worker execution of each v2 graph, with GLB, LOD0/1, semantic masks and
  static collision **candidates**. These still require native Jerico cook/admission.
- Conservative Y/Z indexing inside the X-sweep self-intersection validator.
  Original exact triangle predicate and 5M limits remain. Bin occupancy visits and
  pair tests are separately bounded; no threshold was increased to pass a job.

## Verification

| Check | Actual result | Evidence |
| --- | --- | --- |
| CPU suite | 129 tests passed | `tests/test_*.py`, excluding Blender-only worker/transport/groups modules |
| Spatial-index RED | Old X-only scan rejected 400 separated triangles under a 20K work cap | `test_dense_x_overlap_does_not_compare_distant_y_bands` |
| Spatial-index oracle | Matches exhaustive triangle predicate on 40 seeded dense/sparse cases | `test_spatial_bins_match_exhaustive_triangle_oracle` |
| Initial five v2 worker jobs | FAILED: intersection pair budget exceeded | `_local/rock-v2-20260908-1821/replay.json` |
| First successful five | 5/5 complete, visual defects retained | `_local/rock-v2-20260908-1827/replay.json` |
| Refined five | 5/5 complete; 22.4–37.5 s, final profile | `_local/rock-v2-20260908-1833/replay.json` |
| Repeat seed | Two clean processes: equal geometry, semantics and job key | `_local/rock-v2-variants-20260908-1836/variants.json` |
| Changed seed | Different geometry | Same variants record |
| Nonempty keep-clear | Protected box with 0.1 m margin admitted by worker's surface and containment tests | Same variants record |
| Legacy v1 | Fresh v1 job completes | Same variants record |
| Cancellation | Expected cancellation; no output/result.json | Same variants record |
| Main comparison | 80 PNG: 5 families × v1/v2 × 4 passes × 2 views, 1024² | `rock-v2-20260908-1835/manifest.json` |
| Populated keep-clear views | 16 PNG including baseline and constrained candidate | `rock-v2-keep-clear-20260908-1838/manifest.json` |

`audit_rock_v2_capture.py` decodes originals, validates image SHA-256, source-result
SHA-256 and GLB SHA-256, and writes `audited-lineage.json` with the actual product
and LOD identity. The initial capture manifests used result-bundle geometry hashes,
not individual LOD hashes; their fields are **not** a substitute for the audited
binding. `schemaHash` is accurately named and is **not** a graph digest. A full
serialized node-graph digest is not implemented by this slice.

Camera/framing is fixed in source-space for the v1/v2 comparison. Both use the same
original preview material per style. V1 results come from the exact retained paths
in the older manifest, never a newest-file scan. These are historical v1 recipes,
not proof that every parameter/seed/quality setting matches the new v2 jobs; the
images are a controlled presentation comparison, not a causal per-parameter test.
The keep-clear comparison intentionally changes the protected-volume input.

Original-resolution inspections sampled the principal clay/PBR three-quarter
views, v1 fracture clay and populated keep-clear front. Not all 96 originals were
individually artist-reviewed. Pixel decoding and hash validation are integrity
checks, never visual acceptance.

## Paid addon: actually evaluated, kept private

The user's installed Nature Generator preset `NG_Fractured_Rock_Formation` was
loaded with its **original nodes**, evaluated (17,987 vertices) and rendered in
factory/background/offline/disable-autoexec Blender. No addon Python registration,
node export, graph renaming, source copying or asset repackaging occurred.

Private evidence: `_local/private-nature-fractured-20260908-1836/` with two original
1024² PNGs and `private-manifest.json`, including source-library hash. Do not stage
that directory or paid assets. `private_addon_preview.py` is a generic private
preview harness, not an addon-to-original-code converter or production adapter.
The preset uses its own authored input and normalized framing; it is **not** an
equal-BSP benchmark against Jerico's ellipsoid fixture.

Buying a product and changing the Python that constructs its graph does not by
itself establish redistribution rights. The published Nature Generator listing
links a license that disallows redistribution/repackaging without creator
permission. Historical purchase terms must be reviewed separately. Original
Jerico graphs have no dependency on those paid groups. Sources consulted:
[Nature Generator](https://superhivemarket.com/products/the-nature-generator),
[license](https://superhivemarket.com/page/royalty-free-license),
[RockForm author listing](https://artofbrianyoung.artstation.com/store/6VWPz/rockform-designer-blender-addon).

## Visual assessment and next bounded correction

**Beauty remains rejected.** Bedding is too periodic and shelf-like; fracture
cutters still create slots/notches rather than a convincing network of intersecting
rock faces. Column joints produce floating-looking fragments, including fragments
above the area filter. Rounded rocks lack the reference's material/erosion detail.
The preview material remains too uniform. Noise and contrast scores cannot turn
these observations into PASS.

The next correction should prioritize independently authored polygonal cell/face
construction and extrusion with variable fracture depth, spatially varying bed
thickness and truncated beds, and attached rather than detached column joint
grooves. Use standard Blender operations and the private original preset as a
visual comparator; do not translate its exact graph and call it original. First
write geometric/visual RED fixtures for these specific defects, then evaluate in
the next owner-coordinated window. No additional Blender window is currently open.

M10.5 connection details are in `../M10_5_GENERATOR_CONTROLS.md`. Helper-building
controls require full `rebuildCandidate`; there is no new Qt frontend here. Loop
selection, native lifecycle, incremental DDC, high→low bake, native materials,
navigation/Jolt, source-free Shipping and native Beauty remain with their proper
owners and are not admitted by this evidence.

## Coordination

Main owner: `01a081f1-022e-7e30-aad7-b72e9544d10c`. Window announced for 18:21 UTC;
closed at **18:38:41 UTC**, verified zero Blender processes. Serial executions,
two threads, no extension/restart. Engine/Editor/shared docs/Git were not mutated
by this task. Files remain local for main-owner review/publication; no commit or
push is claimed.
