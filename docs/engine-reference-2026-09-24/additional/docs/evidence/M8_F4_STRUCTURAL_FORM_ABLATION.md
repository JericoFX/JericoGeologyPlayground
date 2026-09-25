# M8 F4: matched Strata relief ablation

Date:2026-09-05. Baseline: `047664e9a76254705b684ab6aa17eb9fce9f45b0`.
Slice: `M8-F4-GEOLOGY-STRUCTURAL-AUTHORING-AND-ROCK-FAMILY-BEAUTY`.
State: diagnostic comparison verified; natural-rock Beauty RED, F4 PARTIAL.

## Scope correction and observable question

The owner explicitly reiterated: "recuerda que la UI es en la M10.5".
New UI work is deferred to M10.5. The uncommitted phase-control increment and
its tests were withdrawn selectively; the three source/test files match HEAD.
Previously published controls remain intact. JericoEditor was rebuilt after
withdrawal: `f4-strata-phase-withdrawal-editor-build.log`, exit0/48.996s.
The withdrawn patch/logs remain under the configured build directory for audit;
their passing tests are not a delivered UI capability or F4 acceptance.

This comparison asks whether periodic geometric Strata relief creates the visible
tiers and whether the broad structural facets survive without that relief.
It changes no implementation, renderer, material settings, density or cut plane.

## Inputs and executable checks

All working paths/logs below are under `build/windows-vulkan-editor/`.
A: fresh `JericoGeologyRockFamilyTests --export-dense PIPELINE.rmat
f4-form-ablation-no-relief-01`. Log `f4-form-ablation-no-relief-export.log`,
exit0/162.768s, including fresh-session mesh/collision repeat, closedness/support
and the existing four-member export guards. All twelve recipe/mesh/collision
files match `f4-native-cut-witness-01` byte for byte.

B: retained `f4-strata-relief-witness-01` geometry, whose deterministic generation
and rejection/recovery evidence is in `M8_F4_STRATA_RELIEF.md`. It is not reported
as a new GPU generation in this comparison. Both cases use member1, voxel0.06m,
working subdivisions16 (3072 triangles), the same IrregularPolyhedral definition,
disabled DirectionalWarp and six identical exact PlaneCuts with roughness0.

A has zero shape/groove amplitude. B has shape0.045m and groove0.015m,
spacing0.50m, phase warp0. The existing B recipe also moves Strata before all
cuts, as required for this admitted pre-cut relief route. This is not an
independent test of layer ordering; zero-amplitude A Strata does not change
distance, while classification/durability can depend on order. No claim of
identical semantic fields is made. The checked recipe diff contains only the
Strata move and the two relief amplitudes; all other layer payloads are unchanged.

Both retained-material exports use the same Poly Haven package/GUID and current
canonical profile from `M8_F4_AUTHORED_MATERIAL_WITNESS.md`. The `.rmat` and three
`.rtext` outputs match byte for byte across A/B. Members0,2,3 retain identical
recipe/mesh/collision bytes. Material exports exit0: A1.320s, B1.302s; logs
`f4-form-ablation-no-relief-material.log` and `f4-form-ablation-relief-material.log`.

Packages `f4-form-ablation-{no-relief,relief}-material-01/candidates.epak`:

| Case | Bytes | SHA256 | Member1 vertices / triangles |
| --- | ---: | --- | --- |
| A no relief | 23948874 | `380444804C86D2E23496C1D949D3563F36954E017D4257CD64307FD5179365CF` | 1188 / 1926 |
| B relief | 24009706 | `3671C9CB17FC357585800BAE234F4417F5B9A9C09D4CB1F4E2A0EE65F3EA9D82` | 1760 / 2840 |

The B package is byte-identical to the published authored-material witness02.
A support passes above-ground and uniform-density center support, contact area
1.56673 and footprint1.99961 square metres. This is the existing geometric
support contract, not a new dynamic Jolt acceptance or a performance budget.

## Native comparison and result

Each `f4-form-ablation-{no-relief,relief}-replay-01/` contains only its package
under `Packages/` plus output BMP/log files. Package copy hashes match.
The CMake-provided `Samples/CookedAssetProof/Debug/JericoCookedAssetProof.exe`
uses `--package Packages:/candidates.epak`, member GUID
`B8130000000000000000000000000106`, scale1,worldY-1.4,cameraZ5,pitch-15,FOV40,
yaw25 front and135 side. All four captures exit0, Vulkan1280x720,
source_free=true,fallbacks0,1 material/3 textures, fixed exposure/PBR_NEUTRAL.
Logs are `front-capture.log` and `side-capture.log` in each replay directory.
An initial post-capture log check missed the ANSI color prefix before the success
marker; matching the actual marker verified all four. No capture was rerun or
accepted without its successful source-free/no-fallback marker.

| Case | Front | Side |
| --- | --- | --- |
| A no relief | [Original](images/m8-f4-form-ablation-01/no-relief-front.bmp) | [Original](images/m8-f4-form-ablation-01/no-relief-side.bmp) |
| B relief | [Original](images/m8-f4-authored-material-witness-02/member-1-front.bmp) | [Original](images/m8-f4-authored-material-witness-02/member-1-side.bmp) |

A front SHA256 `C29E37F582645AE815FEECC6C4BE3B19ECAF6291379A9945B52957E2BE06A252`;
A side `1F3B4FE36D222634B59B821D53DA54DAB66C28264D8F46FFEA8BF05FED0E70D3`.
Fresh B captures exactly match the two retained original BMP hashes from
`M8_F4_AUTHORED_MATERIAL_WITNESS.md`; no duplicate images are added to Git.

All four originals were inspected at native resolution. A has broad straight
facets and no repeated geometric shelves. B introduces conspicuous periodic
steps on the silhouette while the large cut faces remain. Thus the authored
pre-cut Strata relief creates the visible tiering; material texture alone does
not explain it. The block/cut structure accounts for the facets that survive A.
Neither case is an accepted natural rock; turning relief off is a diagnostic
control, not a proposed final repair or a waiver of structural detail.

## Completeness and next action

The existing recipe -> dense source -> field -> extraction/exact cuts -> classified
mesh/collision -> material/package -> native replay chain is exercised. There is
no new algorithm, semantic conversion, dependency, schema or consumer interface.
Existing serialization, source identity, topology, cut regions, support, cook and
failure guards remain active. ECS, animation, scripting, events, Sequencer, audio
and particles receive no changed interface. The actual four-rock collision,
painting, multiface/source correspondence, LOD/streaming and hardware-budget
gates remain INTEGRATION REQUIRED in F4; new UI belongs to M10.5.

The bounded units review found no hidden scale/conversion: IrregularPolyhedral
uses plane distances and smooth maximum in metres, followed by min(scale), which
is1 for this member. GPU Strata uploads the amplitudes directly and changes the
field by `(-shape+groove)*saturate(1-abs(d)/B)*regionalWeight`. Its conservative
field-change bound is0.060m here. This is not a0.060m geometric displacement
guarantee: smoothed corners/interpolation may have gradient magnitude below1.
No admitted gradient floor exists, and this visual comparison does not establish
a numerical distance defect. Do not invent a threshold or declare one repaired.
Relevant formulas: `GeologyImplicitBlock.hpp`, `GeologyGpuImplicitBlock.hpp`,
and the existing Strata branch in `GeologyGpuShaders.hpp`.

Next: inspect the blueprint's shape-aware weathering model and its existing
operator against the actual dense member. Identify a specific missing
form-producing behavior and its field/region/cut consumers, then retain a RED
before changing the bounded working-surface route. Preserve the six cuts,
source correspondence, support and topology; a new operator that lacks admitted
correspondence must reject explicitly. Global periodic relief or more phase/noise
tuning is not a demonstrated repair of this block-like structural form.

Persistent Goal: ACTIVE until full JericoEngine 1.0 closure.
