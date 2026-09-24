# M8 Fixed-Cage Deformation Stage F — Editor Production Hardening

Date: 2026-09-01
Slice: `M8-FIXED-CAGE-DEFORMATION-F-EDITOR-PRODUCTION-HARDENING`
Status: `PASS`
Persistent Goal: `ACTIVE until full JericoEngine 1.0 closure`

## Result

Stage F integrates the already accepted fixed 3x3x3 cage into the ordinary
JericoEditor Inspector. It adds project-owned scalability, exact project source
resolution, deterministic non-destructive impact preview, selected-object cage
visualization, and official project transaction Apply/Undo/Redo.

The authoring surface is deliberately `VisualOnly`. It neither exposes nor
serializes the rejected Stage E `SingleHull`, `CageCells`, or `CoarseProxy`
policies. Existing prepared-breakable entities remain fracture-dominant and are
rejected by this authoring session. Preview deformation state is transient and
is never serialized as authored content.

## Authentic RED and production discoveries

The focused editor gates initially failed because no fixed-cage property
provider, project policy transaction, Inspector widgets, viewport preview state,
or project render profile existed. The first integration link also rejected the
missing project-source resolver.

The exercised UI workflow then exposed two real downstream omissions:

1. the test World collision lacked the structural Physical Material required by
   the existing fracture authority; the fixture was corrected with a separate
   exact cooked collision instead of weakening production validation;
2. an ordinary hydrated World admitted entities but not its single resident
   cooked render mesh, so visual preview failed with
   `editor.viewport.fixed_cage.cage_mismatch`. World admission now hydrates the
   exact catalog mesh/material/texture set when the current viewport's existing
   single-mesh contract applies.

The final integrated UI gate passes through a real project catalog, admitted
World, selected entity, cooked mesh, structural Physical Material, viewport,
project writer, catalog handoff, World reload, and exact replay.

## Implemented ownership and seams

- `ProjectRenderProfile` reads the selected project render profile and owns
  separate fixed-cage visibility distance, activity distance, and bounded
  preview work. No reference-engine distance is embedded.
- `FixedCageProjectSource` reuses the exact fracture project authority for mesh,
  collider, structural Physical Material, exterior Visual Material, transform,
  catalog identity, and stale-data checks. It then requires an existing
  canonical fixed cage and rejects prepared-breakable ownership.
- `FixedCageEditorSession` owns only the authoring draft and transient preview.
  It reuses `FixedCageImpactResponse`, retains the fixed 27-point work bound,
  caches exact repeated preview, and produces canonical candidate mesh bytes.
- The ordinary Inspector displays eligibility, cage identity, read-only
  Physical Material lineage, `VisualOnly` collision truth, selected project
  profile, maximum displacement, plastic retention, recovery mode/speed,
  selected cage visibility, diff, and actual metrics.
- `FixedCagePolicyPublication` uses the official project publication
  coordinator and bridge. Preview writes zero bytes. Apply, Undo, and Redo replay
  exact before/after bytes and admit the resulting catalog handoff before the
  World is reloaded and the selection restored.
- `FixedCageViewportPreviewState` fences entity identity, selection revision,
  mesh identity, and content hash. The selected object uses the existing
  fixed-cage render state; other instances remain on the ordinary instanced
  path. The 27-handle overlay is cached and is cleared on selection/revision
  change or Cancel.
- Inspector metrics report preview CPU, evaluated control points, uploaded state
  bytes, visible vertices, cage/overlay memory, main/depth/shadow consumers,
  active/settled state, handle uploads, and explicit absence of Jolt preview
  mutation.

## Editor workflow proof

`JericoFixedCageEditorPanelTests` proves:

- ordinary Inspector widgets exist and are disabled without an eligible
  selection;
- the selected project entity resolves 27 cage points, structural Physical
  Material, `VisualOnly`, and the project profile values `40 / 20 / 27`;
- unchanged Preview reports a transient impact, writes zero source/cooked bytes,
  and Cancel restores exact state;
- an authored maximum-displacement change produces a bounded diff while the
  admitted mesh remains byte-identical;
- Apply changes the exact project `.rmesh`; one Undo restores the exact baseline
  bytes; one Redo restores the exact candidate bytes;
- every catalog handoff reloads the owning World and restores selection;
- the viewport preview state is selected-only, revision-fenced, cached for exact
  repeats, and has a fixed 27-handle overlay.

## Editor performance sweeps

These are actual Debug single samples. They are not p50 or p95 claims.

| Sweep | Vertices | Objects | State | Metric kind | Preview CPU (ms) | Evaluated points | Uploaded bytes |
|---|---:|---:|---|---|---:|---:|---:|
| ordinary baseline | 8 | 1 | rest | exact zero-work contract | 0 | 0 | 0 |
| vertex | 8 | 1 | active | single sample | 0.0086 | 27 | 324 |
| vertex | 512 | 1 | active | single sample | 0.0085 | 27 | 324 |
| vertex | 4096 | 1 | active | single sample | 0.0091 | 27 | 324 |
| object | 8 | 1 | active | aggregate single sample | 0.0086 | 27 | 324 |
| object | 8 | 16 | active | aggregate single sample | 0.1365 | 432 | 5,184 |
| object | 8 | 64 | active | aggregate single sample | 0.5595 | 1,728 | 20,736 |

State sweep result:

```text
states=rest,active,repeated,cancelled,settled
active_evaluations=1
repeated_additional_evaluations=0
active_uploaded_bytes=324
cancel_restored=true
settled_active_count=0
settled_count=1
```

The invariant is visible in the sweep: authoring evaluation and state upload
remain fixed at 27 points / 324 bytes per active object and do not scale with
mesh vertex count. Exact repeat, rest, Cancel, and settled state add no repeated
authoring evaluation.

## Renderer and package proof

The complete `render.static_mesh_asset` Vulkan suite passed. Fresh focused
evidence modes also passed:

```text
fixed_cage.performance
ordinary_static_pbr_gpu_ms=0.217088
cage_static_pbr_gpu_ms=0.21504
cage_cpu_upload_ms=0.0197
cage_uploaded_bytes=464
cage_resident_bytes=464
cage_visible_vertices=8

fixed_cage_impact.inactive
response_work=0
uploaded_bytes=0
pending_work=0

fixed_cage_impact.response
cpu_ms=0.0101
evaluated_points=27
affected_points=15
pending_work=0
generation=1

fixed_cage_persistence.performance
event_cpu_ms=0.0074
active_recovery_cpu_ms=0.0119
dirty_upload_cpu_ms=0.0075
dirty_uploaded_bytes=464
static_pbr_gpu_ms=0.135168
settled_response_work=0
settled_uploaded_bytes=0
settled_backlog=0
```

The renderer evidence exercised main, depth, and shadow consumers. It emitted no
Diligent error/fatal; the only validation noise was the pre-existing
`WARNING-Shader-OutputNotConsumed` warning.

Package/source-free proof:

```text
fixed_cage.source_free
package_hash=8929095179939204720
package_bytes=2132567
mesh_format_minor=3
fixed_cage_points=27

fixed_cage_impact.source_free
package_hash=16663408142763578608
package_bytes=2133023
entries=3
source_free=true

fixed_cage_persistence.source_free
package_hash=4376141820902678232
package_bytes=2133092
state_hash=14641777700188145091
state_bytes=752
entries=3
publications=7
pixels=bit_identical
source_free=true

fixed_cage_editor.source_free
package_bytes=1310
entries=1
source_assets=false
fallback=false
```

The editor package fixture removes its temporary `SourceAssets` tree before
replay. Shipping reads the accepted cooked mesh policy from one package and has
no editor/source fallback.

## Capture identity

All captures are original 640x480 32-bit BMPs (1,228,854 bytes each). They are
technical runtime evidence, not a Beauty acceptance claim.

| Evidence | SHA-256 |
|---|---|
| render rest / ordinary / source-free rest | `d704733aa5678db4a035fa4c99cfd748d317a9da3b0f47add986e446f1c115d8` |
| render deformed / source-free deformed | `c4e6f9f93158d509b4cdeb21eae5991ad2b7c84aa53780f70046b3b7c5f5ae60` |
| render shadow rest | `c2a7edcab9150c60db06e1538b51728e6030d420a7942bd45ea7cdd6f742cf09` |
| render shadow deformed | `8c78bfa5218ab9892715ffedd2b8157e870165a2a89ff65067c0539890214a55` |
| impact event / deformed / package replay | `fa128f33b9dc91b43317e7f7e3c241ee12065bd2522ba84ceea295bfd2e9a5ba` |
| persistence settled / deformed / package replay | `50d34b6df8a6a9bad138d50565f928ee47d5a1df2d73afade734bcc72d1bb896` |
| persistence shadow deformed | `8212e7a5510af1674d4ed19c40e3ec832e6f479fb7764646735715632fa73c7f` |

The impact event and its package-only replay are bit-identical. The settled
plastic state and its package-only replay are also bit-identical.

## Regression gates

```text
JericoFixedCageEditorPanelTests                         PASS
JericoEditorShellTests --fixed-cage-inspector-contract PASS
editor.project_session                                 PASS 13.41 s
repository.test_scheduling_contract                    PASS 0.02 s
physics.world_jolt                                     PASS 3.14 s
physics.prepared_breakable_runtime                     PASS 0.05 s
physics.prepared_breakable_bond_graph_runtime          PASS 0.35 s
physics.prepared_breakable_package                     PASS 0.15 s
physics.prepared_breakable_source_free                 PASS 101.48 s
render.static_mesh_asset                               PASS 65.94 s
```

These retain Stage D fracture behavior and provide no selectable or serialized
Stage E collision-following policy.

## End-to-end system completeness audit

| Area | Stage F disposition |
|---|---|
| source/input data | Existing canonical `.rmesh`, World entity, collision, structural Physical Material, and project render profile are exact inputs. No new SourceAsset is created. |
| asset pipeline / cook | Existing cage only; policy Apply produces a canonical mesh candidate through the official project asset transaction. No cage generation or preview-pose cook. |
| editor / tooling UI | Ordinary Inspector, selection eligibility, read-only lineage, Preview/Diff/Cancel/Apply/Undo/Redo, metrics, and visible rejection are exercised. |
| ECS / runtime representation | Existing mesh, collider, rigid body, fixed-cage render state, and deformation persistence components are reused. No new gameplay component or collision policy. |
| CPU processing | Event-driven fixed 27-point evaluation; zero rest/repeat/settled work is measured. |
| GPU / renderer / shaders | Existing main/depth/shadow fixed-cage consumer is reused; selected object separates from ordinary instanced draws; overlay is editor-only. |
| materials | Exterior Visual Material is catalog-resolved; structural Physical Material is one authoritative cooked collision reference. No display-name branching. |
| physics / collision | Preview and small dents are explicitly `VisualOnly`; Jolt is absent from editor preview; Stage D prepared-breakable remains authoritative. |
| scripting / events | Runtime impact descriptor/response authority is reused. No new Luau API is required for an editor-only policy authoring slice. |
| serialization / save-load | Only accepted cage policy fields change in canonical `.rmesh`; deformation preview is transient. Existing state persistence/package replay remains bit-identical. |
| streaming / LOD | Project visibility/activity settings are separated and validated; general runtime delivery is recorded below. No LOD schema is invented. |
| destruction / runtime modification | Existing fixed-cage response, recovery/plastic persistence, and Stage D handoff remain unchanged and regressions pass. |
| navigation / gameplay queries | N/A for VisualOnly editor preview; collision/navigation are intentionally unchanged. |
| particles / audio / Sequencer / animation | N/A: Stage F authors policy and previews an existing response; it creates no new downstream event or timeline contract. |
| debugging / visualization / profiling | Selected cage/handles and actual metric kinds cover vertex, object, and state sweeps plus main/depth/shadow, upload, memory, active, and settled counts. |
| failure handling | Empty/stale/ineligible/fracture-dominant/material-mismatch/budget/transaction failures reject atomically and visibly. |
| testing / documentation | Focused UI, session, transaction, package, physics, scheduling, full Vulkan render, evidence, design, and plan are retained. |

## Semantic data lineage

```text
project render profile + selected World entity
-> exact mesh/collider/rigid-body/material catalog resolution
-> existing canonical MeshFixedCage + accepted policy draft
-> deterministic FixedCageImpactResponse preview
-> transient 27-point render state + selected handle overlay + actual metrics
-> optional canonical policy-only .rmesh candidate
-> official Preview (0 writes) / Apply / Undo / Redo project transaction
-> catalog handoff + World reload + selected renderer consumer
-> source-free package replay of accepted runtime policy/state
```

Useful upstream information retained downstream includes mesh/content identity,
selection revision, cage schema/points/maximum displacement, structural Physical
Material identity/definition/strength, Visual Material identity, object
transform, response recovery/plastic policy, impact event identity/generation,
project visibility/activity/work budget, and active/settled/upload/memory metrics.

## INTEGRATION REQUIRED

### General runtime delivery of project render-profile scalability

- subsystem: general runtime configuration / streaming;
- why required: Stage F proves editor/project ownership and validates separate
  visibility/activity settings, but Shipping currently has no general project
  runtime-settings asset carrying them to every World;
- required input: the admitted project render profile and selected profile
  identity;
- expected interface: one versioned, source-free project/runtime settings asset
  consumed by World visibility/activity scheduling, not by fixed-cage authoring;
- owner: general runtime configuration and streaming;
- dependency: a repository-wide project settings schema decision;
- milestone: a future explicitly designated runtime-foundations slice, not
  Stage G and not an implicit M9 start.

### General per-entity multi-mesh World viewport residency

- subsystem: JericoEditor World renderer;
- why required: the current pre-existing World viewport resident set owns one
  mesh asset and can now hydrate repeated instances of that exact mesh. General
  Worlds require per-entity mesh/material/texture residency independent of Stage
  F;
- required input: the already admitted World render dependency set and per-entity
  MeshComponent GUIDs;
- expected interface: typed per-entity render-asset lookup with the existing
  ordinary instancing path preserved and fixed-cage selected draws layered on
  top;
- owner: World viewport/render dependency integration;
- dependency: completion of the existing M7G renderDependencySet seam;
- milestone: a future explicitly designated editor/World-rendering slice.

Neither integration authorizes Stage G, Stage E collision modes, or M9.

## Limits and next legal action

Stage F does not add authored/baked poses, deformation-aware normals,
networking, arbitrary cage resolution, soft bodies/FEA, or higher-fidelity
collision. Stage G remains optional and requires new product evidence. M9 remains
ineligible while mandatory M8 exit gates, including the retained Beauty audit,
are not all `PASS`.

Persistent Goal: ACTIVE until full JericoEngine 1.0 closure.
