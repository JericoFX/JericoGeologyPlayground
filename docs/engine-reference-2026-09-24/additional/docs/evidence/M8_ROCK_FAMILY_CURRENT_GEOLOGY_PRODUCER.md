# M8 rock-family current Geology producer checkpoint

Date: 2026-09-04. Slice: `M8-GEOMETRY-CRITERION-13-ROCK-FAMILY-BEAUTY-REPAIR`.
Status: **PARTIAL / CURRENT CANDIDATE VISUAL RED**. Producer/topology and diagnostic
consumer checks do not grant final-shaded visual acceptance.

## Repository recovery

Safely fast-forwarded `main` from `95343184659687ed5b4deaac6d0b99f4cd4f9358`
to `c7c4f6fc8b86ce647fa56cc1ff702f0bd815500c`: 30 documentation commits,
no overlap with the existing dirty paths. The 13 content-modified tracked files
had identical SHA-256 values before and after the update. The original 185-entry
preservation inventory remains binding.

Read the incoming anti-regression/capability-map directive, reconciliation audit,
approved architecture indexes and explicit Astra instructions. Approved future
architecture is not implemented capability and does not skip the active M8 slice.

## Authentic retained-product RED

The retained Shipping image was inspected at its original 1280x720 resolution.
Its coarse prism forms, occluded fourth member and uniform dark response remain
rejected. SHA-256:

```text
docs/evidence/images/m8-sparse-sdf-rock-family-shipping.bmp
DB88CE56BCA81BCE8B581FF8804409DED729D59D0C9CEB6B35497252430AD1D3
```

The actual retained cooked mesh, not regenerated legacy geometry, was decoded:

```text
build/windows-vulkan-editor/Tests/Editor/Debug/SparseSdfRockFamilyShowcase/Artifacts/RockFamily.rmesh
SHA256=67D3197F7830F7043CC8C56D8F072853B69A8E7098479D81A83D9311BF630889
vertices=15624
geology=false
geologyVertices=0
structuralSamples=0
```

After compiling the new consumer test, executed:

```powershell
build/windows-vulkan-editor/Tests/Editor/Debug/JericoGeologyRockFamilyTests.exe --inspect build/windows-vulkan-editor/Tests/Editor/Debug/SparseSdfRockFamilyShowcase/Artifacts/RockFamily.rmesh
```

Result: **expected exit 1**, `rock_family.lineage|vertices=15624|geology=0`,
failing `mesh.geology.has_value()`. This proves the missing cooked geological
lineage; it does not automate aesthetic judgment or claim all first-RED criteria.
The historical `0.0411372` coverage failure and its `0.08` threshold are unchanged.

## New bounded fixture

`Tests/Editor/GeologyRockFamilyTests.cpp` composes four explicit existing recipes:

```text
BlockGrammar -> ImplicitBlock -> FoldDomain -> Strata -> DurabilityField -> Rebuild
-> GeologyGpuSession / classified Surface Nets / GSTR
-> GeologyCook -> decoded JRMS + exact collision
```

No Detail, new evaluator, new format, public engine API or legacy generator is
introduced. Material GUID is a test fixture identity, not an admitted rendered
palette. Current checks target four identities, packed classification continuity,
closed-edge/nonzero-volume topology, collision arrays and fresh-session byte
determinism. They do not prove connectivity, ground contact, non-clone
silhouettes, no-fallback materials or Beauty.

Runtime development exposed two fixture contract errors and one missing input:

- ImplicitBlock required canonical seeds, tabular proportions and bounded
  irregularity. Fixed recipe data; validators and thresholds unchanged.
- Diligent owns one Vulkan function table/device at a time. The first session is
  now destroyed before the independent regeneration session is created.
- GSTR export requires authored durability. Added the existing Strata and
  DurabilityField recipe stages instead of dropping structural metadata.
- The fourth candidate initially exceeded the existing 262144-sample GSTR cap.
  Tightened the authored source bounds; neither the cap nor voxel size was raised.
- A test incorrectly demanded four different input-field hashes. The four source
  GUIDs and final products are independent, but the evaluated input-field hash is
  shared. The contract now checks authored source GUIDs and distinct recipe/final
  surface hashes, not artificial input-content uniqueness.

Initial failing runtime logs are retained under the configured build tree:
`rock-family-candidates-debug.log` and `rock-family-candidates-debug-green.log`
(the latter filename is historical; its result is **FAIL**, not GREEN).
`rock-family-candidates-run3.log` retains the budget rejection;
`rock-family-candidates-run4.log` retains the overconstrained source-hash rejection.

## Executed regression evidence

The current producer run completed with **exit 0**, including a second, freshly
created Vulkan session and byte-exact `.rmesh`/`.rcoll` regeneration for all four
members. Command: `JericoGeologyRockFamilyTests.exe` (Debug, current configured
Windows Vulkan Editor build); full output:
`build/windows-vulkan-editor/rock-family-candidates-run5.log`.

| Candidate | Vertices | Triangles | Recipe hash | Final surface hash |
|---|---:|---:|---:|---:|
| 0 | 2224 | 4444 | 8238901928600473551 | 13489752154880668528 |
| 1 | 2822 | 5640 | 836560380564187556 | 13884578128045486708 |
| 2 | 3512 | 7020 | 4402800243530273867 | 5042494296152204964 |
| 3 | 2630 | 5256 | 5522415631978528422 | 14631367605088036859 |

All four use input-field hash `9312090064746127685` and distinct authored source
GUIDs. These are candidate identities, **not replacement accepted aggregate rows**.
The final run has no failed assertion, Diligent ERROR, validation error or VUID
message. Diligent's staging-read `MAP_FLAG_DO_NOT_WAIT` warnings are present and
retained; this is not a warning-free renderer acceptance claim.

```powershell
ctest --preset windows-vulkan-editor -R '^(editor.sparse_sdf_rock_family|repository.test_scheduling_contract)$' --output-on-failure
```

Result: **2/2 PASS**, 7.03 seconds. The original fixture remains untouched.
Rebuilt the current `JericoGeologyGpuSessionTests` target and then executed:

```powershell
ctest --preset windows-vulkan-editor -R '^procedural.geology_gpu_(implicit_block|fold_domain)$' --output-on-failure
```

Result: **2/2 PASS**, 158.63 seconds (79.28 seconds per test). Together with the
producer run, this is one new explicit proof plus four existing regression tests.
The final owned source rebuilt successfully after formatting; the retained-input
RED was replayed and still returned the expected exit 1. `git diff --check` and
`git diff --cached --check` passed. No production Engine source changed.

The scheduling manifest remains exactly 377 tests / 112 scheduled tests with its
existing hashes. The new bounded proof runs explicitly via
`JericoGeologyRockFamilyProof`; it is not silently added to that frozen inventory.
Final vertical CTest integration remains required in this slice.

Post-change preservation recheck: exactly 185 foreign entries with normalized
SHA-256 `9a0431d3cdcb45e8e343f6c39179c90b8668f9161b2f8baa681bb737ab94ddd1`.
All 13 content-modified foreign tracked files retain their recorded hashes.

## Review and safe improvement findings

A focused read-only review and a follow-up delta review found no critical issue
in the bounded test-side change. Direct decoded identities, classification
continuity and single-device lifecycle checks were added. A second review caught
coincident-position ambiguity and incomplete coverage in the classification
oracle: it now compares full position/classification multisets and source/cooked
vertex counts, preserving multiplicity. Review does not grant full engine or
slice acceptance.

Useful follow-on work, not silently implemented:

1. Finalized CTest inventory registration across supported configuration variants.
2. Stable contact and independent anti-clone witnesses; connected/oriented indexed
   topology is now covered by the continuation below.
3. Existing coordinator-backed four-body transaction with byte-exact rollback.
4. Admitted geological material palette, native-size family/landmark Beauty and
   one-package Jolt/Shipping replay before aggregate identity replacement.
5. A future capability ledger/freshness check respecting the new anti-regression
   directive; no stale-map regeneration or unrelated engine refactor here.

The approved design and implementation plan carry the complete integration audit.
All listed downstream items remain **INTEGRATION REQUIRED** for the current M8
repair. Criterion 13 stays **1/5**, M8 stays incomplete, M9 has not started.

## Connected topology and diagnostic Vulkan continuation

The former undirected-edge-count / absolute-volume check incorrectly accepted
an inward tetrahedron, a tetrahedron with one reversed face, and two disconnected
outward tetrahedra. The new contract produced exactly those three expected RED
assertions (`rock-family-topology-red.log`, exit 1), then passed after enforcing
opposing edge uses, positive signed volume and a single edge-connected face
component. Open, out-of-range, incomplete-index and empty fixtures also reject.
All four real cooked candidates pass the stronger oracle and fresh-session
regeneration. This does not prove self-intersection freedom, stable contact or
natural shape; it is explicitly an indexed topology check.

`--export pipeline.rmat NEW-directory` now packages the four independent mesh /
collision pairs, one diagnostic PBR material and three flat texture inputs through
the existing RuntimeFormats. Canonical recipe files remain outside the package.
All twelve decoded package payloads must equal their exact inputs. Export uses a
new `.pending` directory, closes and rereads each file before publishing the final
directory through existing `GltfCook::publishDirectoryNoReplace`, and never
overwrites an existing export. The already-admitted Tool support library is linked
only by the test fixture; no new third-party dependency or engine API was added.
Failed staging is retained for inspection, not presented as an exported candidate;
the CLI reports stale staging before starting Vulkan and requests a new destination.

Review found two additional real defects. `--export-contract NEW-directory`
reproduced two RED assertions: a compute-tagged PBR archive was accepted, and a
failed file write exposed a partial final directory. The corrected contract
requires a graphics pipeline, keeps incomplete output under `.pending`, and
verifies successful publication plus byte-preserving overwrite rejection. Plain
relative destination names are normalized before checking their parent.

The first material export also correctly failed at
`runtime.rmat.invalid_texture_reference`: texture bindings were not ordered by
semantic. Sorting the fixture bindings fixed it; no format validator changed.

The initial Release proof executable was stale and rejected current mesh version
1.2. Rebuilding `JericoCookedAssetProof` in the configured Debug tree allowed the
same cooked package to render. Do not misreport the stale-binary failure as a
runtime-format regression or the Debug replay as Release recertification.

### Original-resolution visual RED

Each native 1280x720 Vulkan frame was inspected directly, without resizing,
recoloring, compositing or generated imagery:

| Member | Observed failure | SHA-256 of native BMP |
|---|---|---|
| 0 / tabular | Soft-edged rectangular slab with broad unbroken planar faces | `406AE458440B658DC1D6F5957C824B6E6B06C59A7B9A7D039BC64DE95442E45F` |
| 1 / irregular | Rounded, bent block; insufficient rock structure | `B8CD4A9DA5E2EFCCDA658EA0130D0CB1DEBE2FAAE27D7D2E08A319D4EC4F8376` |
| 2 / rhombohedral | Deformed box with soft/fabric-like side undulations | `98E87EEEB4CC7D14BFBDA2D763F2CCA0D65DA7E537B938EE735A0F841F60D322` |
| 3 / columnar | Bent extrusion with uniform section; reads as a shaped primitive | `F9878D774F83A28D25E6FA51837EEF54D5F1849BAB42895B292541527E4786EF` |

Frames: `docs/evidence/images/m8-rock-family-current-candidates/member-{0,1,2,3}-vulkan.bmp`.
All views use the same camera and flat calibrated PBR inputs: no emissive, no
Detail layer, metallic 0, roughness 0.78. Each renderer report has one draw, one
material, three textures, `fallbacks=0`, `source_free=true`, Vulkan RTX 3090.
The package contains 12 entries / 19,946,458 bytes, SHA-256:
`DEDCE128D3DDC19318A5174C0C0419C121221C876C2282315ACB5CE075643614`.
Three fresh complete producer exports yielded that exact package hash, including
the final exclusive-publication build (`rock-family-exclusive-publish-final.log`,
exit 0). An isolated replay directory for that package contained one
`Packages/candidates.epak`, with no source
recipes or loose cooked assets. All four Debug consumer replays returned exit 0
and reproduced their respective native BMPs byte-for-byte. That is package-only
diagnostic reproducibility, not final family or Release Shipping acceptance.

These are **diagnostic failures, not accepted Beauty**. There is no ground, family
composition or shadow/contact acceptance, the final palette is not authored, and
only one silhouette direction per member has been inspected. GPU timing is
unavailable in these captures; zero-valued timing fields are not measured costs.
Generic Diligent staging-map warnings remain visible in the producer logs.

Reproduction (after building the two existing Debug targets):

```powershell
# Run from build/windows-vulkan-editor; destination must not already exist.
./Tests/Editor/Debug/JericoGeologyRockFamilyTests.exe --export ./m8-aperiodic-tiling-release/aperiodic-tiling-pipeline.rmat rock-family-new-export
./Tests/Editor/Debug/JericoGeologyRockFamilyTests.exe --export-contract rock-family-new-contract
# Run the renderer from a proof directory containing Packages/candidates.epak.
F:/JericoEngine/build/windows-vulkan-editor/Samples/CookedAssetProof/Debug/JericoCookedAssetProof.exe --package Packages:/candidates.epak --mesh-guid B8130000000000000000000000000006 --capture member-0-vulkan.bmp --world-scale 1 --world-y -1.4 --world-yaw 25 --camera-z 5 --camera-pitch -15 --report-material-pbr true
```

For members 1-3 the GUID suffix is respectively `0106`, `0206`, `0306`.
The native-size proof and its package hash are durable diagnostics; none replaces
the final accepted-source, material, editor, physics or aggregate gates.

Final focused verification also passed the CPU topology/export contracts, stale
staging rejection (expected exit 2), and both CTest regressions
`editor.sparse_sdf_rock_family` / `repository.test_scheduling_contract` (2/2,
6.91 seconds after final CMake regeneration). The original retained mesh remains
the expected classified-lineage RED (exit 1). Final read-only delta review found
no remaining important issue in this bounded change; geometric self-intersection,
contact, automatic export gate registration and the complete vertical remain
outside its demonstrated scope. Foreign status still has exactly 185 entries
with the recorded normalized digest; all 13 recorded file hashes are unchanged.

## Authored support and structural ablations

The next bounded revision changes only showcase recipe data and its test-side
consumer oracle. The current path is:

```text
BlockGrammar -> ImplicitBlock -> DirectionalWarp -> two rough PlaneCut faces
-> Strata classification -> DurabilityField -> support PlaneCut -> Rebuild
-> classified Surface Nets / cook -> decoded support and topology checks
```

The support cut is intentionally before Rebuild. Its contract is checked on the
final decoded cooked mesh, not on intermediate SDF metadata: no vertex below
the member's authored plane by more than 0.02 m, projected contact triangle area
at least 15% of the XZ bounds footprint, and the uniform-density volume centroid
projected over actual contact triangles. This is a conservative geometric
diagnostic, not Jolt balance, material-density, friction, ground rendering or
self-intersection acceptance. The four local planes are -0.22, -0.55, -0.55 and
-0.70 m; a later multi-entity placement must align them to the actual scene ground.

### Support RED / GREEN

The actual prior member from the published `18be59cf` recipe/export was tested:

```powershell
build/windows-vulkan-editor/Tests/Editor/Debug/JericoGeologyRockFamilyTests.exe --support 0 build/windows-vulkan-editor/rock-family-visual-candidate-3/member-0.rmesh
```

Expected **exit 1**, reproduced after the final support build:
`rock-family-prior-support-final-red.log`. The decoded member has 2224 vertices,
contact area 0, footprint 4.22358, `above_ground=0`, and unsupported projected
centroid. Mesh SHA-256:
`4C4974F9FFC284114957C1FB350C2D306F32E441AAF62569DB34A283D4F24B0F`.
This is a retained authored-product rejection, not just a synthetic negative.

`--support-contract` covers a hand-oriented supported tetrahedron plus floating,
tipping and penetrating variants. Read-only review identified the missing
penetration regression. The added side protrusion keeps contact area 2 m2,
footprint 4.2 m2 and a supported projected centroid, but penetrates by 0.05 m.
Temporarily removing `aboveGround` reproduced exactly one expected failure
(`rock-family-below-plane-red.log`, exit 1). Restoring the predicate passes all
four cases (`rock-family-support-final-green.log`, exit 0). The temporary mutation
is not present in the final source. CLI usage includes both support modes.

### Controlled structural changes and remaining visual failure

Large FoldDomain deformation was replaced with admitted DirectionalWarp and rough
fracture-face cuts; edge smoothing was reduced to 0.035 m, and the columnar member
uses an irregular-polyhedral mass. No Detail, material or camera adjustment owns
the repair. An initial MacroWarp trial returned
`procedural.geology.gpu_layer_unsupported`; the productive allowlist and evaluator
were not changed. The rejected trial log is `rock-family-structural-repair-1.log`.

The first successful trial included Strata displacement 0.035 m and grooves
0.03 m. Native captures showed artificial terracing and thin/pinched edge regions.
Removing only those geometric Strata fields removed the periodic bands, while
keeping classification and durability. Captures in
`rock-family-structural-repair-2/Replay` still showed a thin slab edge, box-like
masses and dark pinched edges. These are explicit visual failures, not Beauty.

The subsequent member-0 trial changes only its top-cut normal from
`(0.3,0.85,0.15)` to `(0.08,0.99,0.04)`. The other three members remain controls;
the camera, flat PBR inputs, source bounds and 0.08 m voxel size are unchanged.
Native inspection confirms restored visible edge thickness on member 0 and
removal of its sharp dark pinch in that view, but the slab still reads as a soft
authored block. Members 1-3 reproduce the preceding images byte-for-byte and
remain visually rejected. This does not establish natural form or other views.

The final build and complete two-session producer/export returned **exit 0**:
`rock-family-support-final-green-build.log` and
`rock-family-structural-repair-4.log`. The export is
`build/windows-vulkan-editor/rock-family-structural-repair-3`.
Decoded final results (all four `above_ground=1`, supported projected centroid):

| Member | Vertices / triangles | Contact area / XZ footprint, m2 | Recipe hash | Surface hash |
|---|---|---|---|---|
| 0 | 1814 / 3624 | 2.896 / 4.17804 | `7292926509916498453` | `2667450392806680523` |
| 1 | 1798 / 3592 | 1.3024 / 2.06541 | `17998113048454814090` | `10511093086398217780` |
| 2 | 2692 / 5380 | 2.2976 / 4.72071 | `8531568261658843592` | `11981487702509485623` |
| 3 | 2166 / 4328 | 1.3824 / 2.20219 | `15751063477034752340` | `16702631612341361949` |

Exact classification/collision and fresh-session byte regeneration passed along
with connected outward indexed topology. Source hash remains
`9312090064746127685`. The 12-entry diagnostic package is 19,631,162 bytes,
SHA-256 `602DF14E43DE7EE44ABDA9E7092BA4FF1A3FE32913B8E23653D7FFF492AA05D6`.
Four Debug Vulkan replays from its package-only `Replay/Packages` mount returned
exit 0, each `fallbacks=0`, `source_free=true`, one draw, 1280x720. GPU timing is
unavailable; this does not recertify performance or Release Shipping.

Unmodified native frames are retained under
`docs/evidence/images/m8-rock-family-support-candidates/`:

| BMP | SHA-256 |
|---|---|
| `member-0-strata-geometry.bmp` | `76BB4A0F73D8EB77F07227889682D92D832CF0B118ADB7E30E6DFB2AFE7F5877` |
| `member-0-before-top-cut.bmp` | `CE0199B690A9AF9B89B99FC40C61735B96B82B5060529C33AE34AB593BA65648` |
| `member-0-vulkan.bmp` | `B24126D9484D041739570FDB07E0040318AEE29D4B7033BD21B22681556198AB` |
| `member-1-vulkan.bmp` | `E4786C8F25CC57C0E019995E9CF69DC29CA6AACF3FE1A7D13CA37BBFBB025905` |
| `member-2-vulkan.bmp` | `9A6CE3B980CBF634C49A5455935381184CE3791B0F1756F91E312DFC35741A33` |
| `member-3-vulkan.bmp` | `182CBAB227B68C3713D9EB0FB1CC31AAE91858763010171DF5D96C08CE2931E2` |

Each BMP is 3,686,454 bytes, original 1280x720. The before/after frames are retained
as separate images, not composites. All previous baseline images remain unchanged.

Final focused checks also passed `--topology-contract`, `--support-contract`,
`--export-contract rock-family-support-export-contract` (from the build tree),
and the two existing CTest regressions (2/2, 6.38 seconds). The external review
found no further support-oracle issue after the restored plane check and explicit
post-Rebuild evidence. No production Engine, evaluator, renderer or format changed.
All 185 foreign status entries retain their recorded digest and all 13 recorded
content-modified foreign file hashes match.

The historical legacy anti-clone inspector was inspected, not reused: it samples
at most 96 vertices by index before computing a distance histogram. Canonical
vertex reordering can change that sample, so its old distance and threshold do
not certify the new family. An independent order-invariant witness and three-view
inspection remain required. No old inspector or acceptance row was modified.

## Geological normal-mapping repair

The retained support-candidate images prompted a cooked-boundary normal probe.
All four meshes have zero opposite-hemisphere normal corners, zero opposite
average face normals and zero triangles with cross-product magnitude below
1e-10. Minimum corner-normal/face-normal dot products were respectively
0.631329, 0.508541, 0.19486 and 0.348222. Logs:
`build/windows-vulkan-editor/rock-family-frame-probe-{0,1,2,3}.log`.
This eliminates inward/degenerate frames in these inputs, not self-intersections
or all possible geometric defects. Temporary `[DEBUG-rock-frame]` instrumentation
was removed and the producer rebuilt; no probe code ships.

Tracing the actual consumer found that `EvaluateGeologyNormal` blended projected
texture normals along sharpened signed axis weights. Even a neutral tangent
normal therefore changed the macro surface direction. The correction preserves
the macro components in each projection before blending. Base-color/ORM texture
weights, classification selection, mesh data and ordinary PBR are unchanged.

The existing Vulkan material test now compares ordinary and geological PBR on
four oblique normals. It uses direct illumination, zero ambient and a non-dark
control, not two empty/black captures. An initial ambient-dominated probe returned
false confidence and was discarded. With direct lighting, the old shader had
maximum RGB differences 1, 3, 6 and 1 against tolerance 1:
`rock-family-neutral-normal-red2.log`, expected **exit 1**, two assertions.
The repaired neutral samples at strength 0 yield differences 0/0/0/0; at strength
1 they yield 0/0/0/1 (8-bit normal quantization is retained).

A deliberately tilted RGBA8 normal sample exposed a second issue: after scaling
XY to zero, its non-unit Z still biased the projected normal. That test produced
two expected failures, differences 2 and 4, in `rock-family-tilted-normal-red.log`.
Guarded normalization after strength scaling removes that bias continuously;
strength 0 now matches exactly. Strength 1 still changes the image (differences
0/4/9/2), rejecting an implementation that simply ignores all normal maps.

Read-only review identified missing BC5 positive-Z reconstruction, already present
in ordinary PBR. A real constant 4x4 BC5 block, not a mocked texture lookup,
reproduced eight failures across four normals and strengths 0/1. Differences were
100/105/109/102 per orientation, with the defective geological output black:
`rock-family-bc5-normal-red.log`, expected **exit 1**. Matching the ordinary PBR
positive-Z reconstruction before strength scaling fixes all eight comparisons,
each difference 0: `rock-family-bc5-normal-green.log`, **exit 0**. This BC5 defect
affects compressed material normals; it is not misidentified as the cause of the
current rock diagnostic's RGBA8 texture behavior.

The test is now invoked by the existing default `render.static_mesh_asset` path,
not only `--geology-only`. No CTest inventory entry/hash or timeout is changed.
The ordinary and HDR/IBL/shadow material paths remain regression consumers.
Final CTest execution passes all three gates: `render.static_mesh_asset` (77.72 s
under its unchanged 120 s limit), `editor.sparse_sdf_rock_family` (6.32 s) and
`repository.test_scheduling_contract` (0.03 s), total 84.10 s. Full log:
`rock-family-normal-final-regressions.log`. Final read-only delta review found
no outstanding important code/test issue. This is focused verification, not a
claim that the entire engine or M8 passed.

Integration/data-lineage boundary: source recipes, semantic mesh streams, collision,
editor transactions and runtime formats are unchanged. Shader cook and geological
rendering are affected; archives must be recooked. A previously cooked package is
not silently upgraded by a source shader edit. Native family/landmark Beauty and
same-run performance recertification remain **INTEGRATION REQUIRED** before final
product acceptance. The approved full integration audit remains binding; this
correction grants no editor, physics, streaming or aggregate completion credit.

### Unchanged-geometry package replay after the shader repair

Rebuilt the existing renderer/material test target to recook its `pbr-opaque.rmat`,
then used that current archive with the rock producer's existing `--export` route:

```powershell
cmake --build --preset windows-vulkan-editor --target JericoStaticMeshRenderAssetTests JericoGeologyRockFamilyTests --config Debug
build/windows-vulkan-editor/Tests/Editor/Debug/JericoGeologyRockFamilyTests.exe --export build/windows-vulkan-editor/Tests/Render/Debug/Cooked/pbr-opaque.rmat build/windows-vulkan-editor/rock-family-normal-new-export
```

The final two-session generation/export returned **exit 0** in
`rock-family-neutral-normal-final-export.log`. Actual retained output:
`build/windows-vulkan-editor/rock-family-neutral-normal-repair-2`.
All four `.rmesh` and four `.rcoll` files are byte-identical to the preceding
`rock-family-structural-repair-3` products; recipe and surface hashes remain those
listed above. Support, classification, topology and fresh-session determinism
pass unchanged. The 12-entry package is 19,934,106 bytes, SHA-256:
`35B6F63C19EB11F7559ECD394CEBE630FC202E63D06D8D1EFACBE1FBA58BD5B7`.

All four native 1280x720 Debug Vulkan replays returned exit 0 with `fallbacks=0`
and `source_free=true`. Same camera, geometry, flat material factors and texture
bytes; only the cooked shader archive changed. A second isolated replay directory
(`rock-family-normal-isolated-replay`) containing only the one package as asset
input reproduced all four BMPs byte-for-byte. This is not Release Shipping or
performance acceptance; GPU timings remain unavailable.

Original-resolution inspection no longer sees the previous sharp black pinches
in these views. Shading now follows the soft underlying forms more continuously.
The forms themselves remain overly rounded block/slab masses and **VISUAL RED**;
there is still no ground, family composition, final palette or multi-view acceptance.
No old evidence image or accepted product row was overwritten.

Native BMPs in `docs/evidence/images/m8-rock-family-normal-repair/`, each 3,686,454
bytes at original 1280x720:

| Member | SHA-256 |
|---|---|
| 0 | `98D8B63827E852198DDE1A4213BAB9728EC364483E8D169A3BC1B7B997D1222E` |
| 1 | `B809EF79489D194F33A5CF50A5ACCAD1C199BAD326D58756399BEF51A2FF438A` |
| 2 | `56853D5328405A9077925325D93E42A328BF4E099150B8B843BF9E845A6F254A` |
| 3 | `4E1D174D2235508DE950A7717AE4BD645051DF00C6BB9611AAB55FEC9503D645` |

Safe improvement delivered: neutral normal behavior and BC5 decoding now have a
default Vulkan regression, including an active-normal-map response witness, without
a new framework, dependency or test-inventory exception. Remaining improvements
are authored structural diversity and the already required full family vertical,
not an invitation to refactor unrelated renderer code.

## Shape witness and owner-requested F2 transition

On 2026-09-04 the owner requested a fresh F2 task for a whole-engine architecture
and performance audit. This rock-family repair is checkpointed **PARTIAL**, not
accepted. Criterion 13 remains **1/5**; M9 is not started.

Three explicit admitted PlaneCuts on member 1 produce a less rectangular contour,
but still artificial softened faces. The independent test-side shape witness uses
every unique **indexed** cooked position and every pair distance normalized by the
actual diameter, accumulated into 32 linearly interpolated bins. Every family pair
must exceed 0.04 total-variation distance (four percent of pair-distance probability
mass). It uses no IDs, hashes, material, normal or producer acceptance flag.
Rigid transforms, uniform scale, vertex order and split-position duplicates are
invariant within explicit limits: 8192 input vertices and 49152 indices. Invalid,
non-finite, collapsed and oversized inputs fail closed. This is **not** tessellation
invariance, a unique geometric fingerprint, a prism rejector or Beauty acceptance.

Logs under `build/windows-vulkan-editor/`:

- `rock-family-shape-red.log`: missing witness, expected exit 1 / three failures;
  `rock-family-shape-green.log`: exit 0.
- `rock-family-shape-spectrum-export.log`: actual cooked pair 2/3 fails at
  **0.0285522**, below unchanged **>0.04**; export not published.
- Corrective member 3 recipe: admitted Columnar archetype, half extents
  (0.42, 1.10, 0.50), five sides. Members 0/2 remain byte-identical controls.
  No engine operator, quality budget or legacy acceptance row was changed.
- `rock-family-shape-final-export.log`: exit 0; all six pairs, classified cook,
  exact collision, support, connected/outward topology and two fresh Vulkan
  sessions' byte determinism pass.
- Review found orphan-vertex sensitivity: `rock-family-shape-orphan-red.log`
  reproduces expected exit 1. Indexed-position filtering fixes it.
  `rock-family-shape-reviewed-contract.log` exits 0 with orphan/invalid-index,
  transform/order/duplicate, distinct-form, fractional-bin, Inf/NaN/collapsed and
  bounded-work cases. Final read-only delta review found no actionable code issue.
  The final reviewed default producer was then rebuilt and run end-to-end:
  `rock-family-shape-reviewed-proof.log`, exit 0, all six distances unchanged and
  two fresh Vulkan sessions reproduce all classified mesh/collision bytes.
  Focused CTest regressions also pass 2/2 in `rock-family-shape-regressions.log`
  (existing rock-family 6.34 s, scheduling contract 0.03 s). These do not stand in
  for a full-engine test run or final rock-family vertical acceptance.

| Pair | Final total-variation distance |
|---|---:|
| 0 / 1 | 0.265061 |
| 0 / 2 | 0.156538 |
| 0 / 3 | 0.136014 |
| 1 / 2 | 0.162642 |
| 1 / 3 | 0.180104 |
| 2 / 3 | 0.0805746 |

Retained export: `build/windows-vulkan-editor/rock-family-shape-spectrum-2`.
12-entry package SHA-256:
`A631CF5DC3CE69019E9616422ADFA04A9690782827352EAE2ABE16E73D81845F`.
Member 1: 1516 vertices / 3028 triangles, recipe 17500540138157406415,
surface 11839816666780012299. Member 3: 1306 / 2608, recipe
10285734395828212272, surface 12935066559629155998. Members 0/2 retain their
preceding recipe/surface identities and mesh/collision bytes exactly.

Native diagnostics in `docs/evidence/images/m8-rock-family-shape-witness/`:

| Image | SHA-256 |
|---|---|
| member-1-vulkan.bmp | `CB69CB0CA52B5B2F70DBA7B0C052B0CD8F8DCC587C18E201BF5CCCE6214FB6F8` |
| member-3-vulkan.bmp | `D5EB9D7FAA78BCDC57B9DAFB78D2AB382930327B14EFDA7F7025E90E79CC5ACA` |

Both are unmodified original 1280x720 Debug Vulkan output, source_free=true,
fallbacks=0, one draw each. Member 1's capture comes from `rock-family-fracture-faces-1`
and its mesh is byte-identical in the final export; member 3 uses the final export.
Camera, material and shader archive are unchanged. Inspection still rejects the
soft block and column/extrusion reads. No final ground/family scene, three-view
inventory, Release Shipping recertification or measured GPU performance is claimed.
The approved complete integration/data-lineage audit still applies unchanged.

### Deferred rock-family repair and completeness boundary

Use the normal-repaired captures as the next authentic macro-form RED. The proven
neutral/BC5 normal-mapping defects are repaired and tested; do not reintroduce
shader bias to sharpen or disguise the soft authored geometry. Improve natural
structural forms through admitted operators while preserving support, classification and outward
topology; do not mask defects with textures, lighting, framing or Detail. Establish
independent three-view anti-clone / silhouette witnesses, then compose all four through the
existing multi-entity editor/renderer and coordinator-backed atomic transaction.

`CookedAssetProof --level-guid` checks level references but still draws one selected
mesh. It must not be mistaken for the family scene consumer. This downstream gap,
final palette, Jolt package admission, Release isolated Shipping replay,
performance recertification, automatic CTest inventory integration and aggregate
criteria 6-12 remain **INTEGRATION REQUIRED** in this M8 slice. Criterion 13 stays
**1/5**; no other product row changed and M9 has not started.

Persistent Goal: ACTIVE until full JericoEngine 1.0 closure.
