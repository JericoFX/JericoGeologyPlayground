# M8 Fixed-Cage Stage D — Fracture Handoff Evidence

Date: 2026-09-01
Slice: `M8-FIXED-CAGE-DEFORMATION-D-FRACTURE-HANDOFF`
Local status: `PASS`
Persistent Goal: `ACTIVE until full JericoEngine 1.0 closure`

## Scope proved

This slice joins the accepted fixed-cage deformation response to the existing
PreparedBreakable structural authority. One `ImpactDescriptor` now carries the
same stable event identity through the direct 27-point deformation solve,
Physical Material resistance selection, one canonical local bond request,
support traversal and the existing Jolt detached-body activation path.

It does not add a damage scalar, health meter, fracture graph, renderer-owned
decision or World scan. It does not implement deforming collision, editor
authoring, Stage G or M9.

## Authentic RED

The focused RED was written first in
`Tests/Physics/PreparedBreakableBondGraphRuntimeTests.cpp`. The initial build
exited `1` because the test named production contracts that did not exist:
`FixedCageFractureHandoffRequest`, `applyFixedCageImpact`, the deformation-state
disposition and retired-owner query.

The package RED was then added to
`Tests/Physics/PreparedBreakablePackageTests.cpp`. Its first execution exited `1`
with 11 failed checks because a package replay did not yet expose fixed-cage
source availability, eligibility, state transition, disposition or idempotent
same-event behavior.

The completed RED proves:

1. a weak event deforms while structural topology and Jolt body count remain
   unchanged;
2. a stronger event forwards the exact source identity and Physical Material
   lineage to the existing structural path once;
3. replaying the same event evaluates zero bonds, visits zero chunks and does not
   activate another body;
4. local graph and activation budgets remain authoritative and reject atomically;
5. invalid impact, object, material, state and prepared-graph lineage reject
   without publishing the candidate cage state;
6. deformation-only and ordinary non-cage assets remain supported and do not
   enter Fracture;
7. successful replacement retires the intact fixed-cage owner instead of copying
   its state onto detached chunks.

## Causal contract

`DestructionCoordinator::applyFixedCageImpact` is the only Stage D adapter. It
first applies the accepted Stage B/C response to a candidate copy, validates the
live intact entity and ECS snapshot, then forwards exactly one structural load
to `applyStructuralLoad`. Deposited energy density is forwarded without a new
tuning scalar because `1 MJ/m3` and `1 MPa` are the same SI pressure unit.

The existing structural path remains authoritative for:

- Physical Material tensile/compressive/shear strength;
- cooked bond resistance and exact canonical endpoint lookup;
- prepared graph topology and support anchors;
- per-event bond/traversal/activation/body limits;
- detached-component materialization and Jolt activation.

Event identities that cause a topology mutation are retained in a sorted,
bounded per-graph ledger. Its size cannot exceed the cooked bond count because
only a newly failed bond records an identity. Stage C's last event identity
handles immediate replay; the graph ledger handles delayed structural replay.

## Atomic state ownership

The updated Stage C component is published before structural evaluation only so
one transaction can be observed by the existing World seams. If structural
evaluation rejects or identifies a delayed replay, the exact previous component
is restored and the candidate response is discarded.

After a topology-changing failure replaces the intact entity:

```text
intact fixed-cage response   retired; no recovery work or dirty publication
intact ECS entity            PendingDestroy
detached chunk entities      existing PreparedBreakable/Jolt authority
fixed-cage state on chunks   absent
same event replay            deterministic no-op
later event on old owner     typed owner-retired rejection
```

A structural survivor and a deformation-only asset retain the Stage C owner and
saveable state. There is never a live intact cage state and copied chunk cage
state claiming the same deformation.

## Data lineage

```text
ImpactDescriptor {position, direction, energy density, radius, event identity}
+ cooked .rmesh fixed cage and Stage C policy/state
+ cooked .rcoll Physical Material identity/hash/strength
+ PreparedBreakable source mesh, chunks, bonds, anchors and budgets
-> direct 27-control-point deformation candidate
-> exact ECS lineage/snapshot validation
-> one PreparedBreakableStructuralLoadRequest with the same event identity
-> Physical Material strength vs cooked bond resistance
-> only the named causal bond may fail
-> existing bounded support traversal
-> existing detached-component entity/Jolt body activation
-> retained intact deformation OR consumed intact owner
```

Useful upstream semantics preserved downstream are object, mesh, material and
event identity; material definition hash; current/plastic cage state; selected
load mode; bond endpoints; topology hash; support classification and explicit
work counts.

## Source-free package proof

The package fixture now cooks one fixed cage into the same source `.rmesh` used
by the existing PreparedBreakable package. Replay mounts package bytes only,
resolves the mesh, collision, Physical Material, chunks and graph from cooked
entries, runs the handoff, then repeats the same event.

```text
weak package bytes                    17298
weak stable payload hash              0xa8e61338dc33abb9
strong package bytes                  17298
strong stable payload hash            0xb0cb6523ef749bf6
weak effective shear resistance       5 MPa
strong effective shear resistance     30 MPa
applied event stress                   20 MPa
weak result                            bond failed, 2 bodies
strong result                          survived, 1 body
fixed-cage evaluations                 27 in both packages
exact replay bond/chunk work           0 / 0 in both packages
source-bearing strings                 rejected by package test
Shipping source-free proof             PASS
```

The weak and strong packages are byte-identical across one-worker and
four-worker cook runs respectively, while their distinct Physical Materials
produce distinct deterministic hashes and structural outcomes.

## Measured work

One retained Debug fixture sample reports actual metric kinds separately:

```text
deformation classification CPU        0.0083 ms
complete handoff CPU                   0.6615 ms
structural evaluation CPU              0.1158 ms
topology evaluation CPU                0.0268 ms
Jolt body activation CPU               0.2784 ms
evaluated cage controls                27
evaluated bonds                        3
visited chunks                         4
newly detached chunks                  2
```

These are single-fixture samples, not p50/p95 or a scene-scale performance
approval. The counters prove bounded local work; they do not claim zero renderer
or physics-frame cost.

## Verification

```text
focused fracture-handoff runtime contract                       PASS
focused package-only weak/strong/replay contract                 PASS
scene.world_save                                                 PASS 0.01 s
scene.component_registry                                         PASS 0.01 s
physics.prepared_breakable_runtime                               PASS 0.05 s
physics.prepared_breakable_bond_graph_runtime                    PASS 0.34 s
physics.prepared_breakable_package                               PASS 0.14 s
physics.prepared_breakable_source_free                           PASS 103.22 s
runtime_formats.rmesh                                            PASS 0.02 s
render.static_mesh_asset                                         PASS 67.28 s
integration total                                                PASS 8/8, 171.12 s
git diff --check                                                 PASS
```

The broad render target again invalidated the pre-existing MediumHigh atmosphere
LUT fixture. Its unrelated long recook was cancelled; the admitted render test
project and affected tools were rebuilt directly. An initial source-free run
then exposed one stale `AssetDump` binary from that interruption. After rebuilding
the tool, the proof passed, followed by the retained clean 8/8 run above. No
atmosphere source, policy or product output belongs to this slice.

## End-to-end system completeness audit

| Area | Stage D result |
|---|---|
| Source/input data | Reuses the single Stage B descriptor and stable event identity |
| Asset pipeline/runtime format | Reuses admitted `.rmesh`, `.rcoll`, Physical Material and PreparedBreakable payloads; no new format |
| Editor/authoring | `INTEGRATION REQUIRED` in Stage F |
| ECS/components | Candidate Stage C state is transactionally published/rolled back; fractured owner is retired |
| Runtime/CPU | One explicit coordinator seam; no World scan or background queue |
| GPU/renderer/shaders | Existing cage rendering is unchanged; renderer does not decide fracture |
| Materials | Cooked Physical Material and bond resistance remain quantitative truth |
| Physics/collision | Existing detached Jolt activation is exercised; deforming intact collision remains Stage E |
| Animation | Not applicable to this static-mesh handoff |
| Luau/events | No second gameplay damage event; future exposure must preserve the same causal identity |
| Sequencer | Not an owner of impact, fracture or recovery state |
| Serialization/save-load | Surviving/deformation-only state remains the Stage C component; consumed intact state cannot be republished |
| Streaming/package | Deterministic source-free weak/strong package replay proved |
| LOD/shadow/AO/bounds | Existing Stage A/C paths remain unchanged and render regression passes |
| Destruction/runtime modification | Existing bond/support/materialization authority is reused directly |
| Navigation/gameplay queries | No new collision/nav claim while intact deformation remains `VisualOnly` |
| Particles/audio | Not causal owners; reactions are outside this bounded slice |
| Debug/visualization | Typed diagnostics, work counters and retained test output; editor views deferred to Stage F |
| Profiling/budgets | Five CPU metric kinds and exact local work counts retained |
| Failure handling | Event/object/material/state/graph/budget/replay paths are typed and atomic |
| Documentation/testing | Authentic RED, package/runtime/integration tests, hashes and this audit retained |

## Required future integrations — not implemented here

### Stage E — measured deforming-collision prototype

`INTEGRATION REQUIRED`

- subsystem: Physics/Jolt collision;
- reason: the intact Stage A-D cage remains intentionally `VisualOnly`;
- required input: current bounded cage state, contact stability requirements and
  representative products;
- expected interface: cheapest measured proxy/update policy with truthful
  fallback and no per-frame triangle recook;
- owner/milestone: Physics + Destruction, Stage E;
- dependency: this completed causal ordering and explicit consumed/retained
  state disposition.

### Stage F — editor and production hardening

`INTEGRATION REQUIRED`

- subsystem: JericoEditor, Asset Pipeline and Destruction tooling;
- reason: designers need validated authoring, preview and telemetry rather than
  direct binary editing;
- required input: fixed-cage topology/policy, Physical Material lineage,
  fracture endpoints/budgets and Stage E policy;
- expected interface: Inspector, visualization, Preview/Diff, cancellation,
  transaction, Undo/Redo, deterministic cook, package validation and profiling;
- owner/milestone: Editor + Asset Pipeline + Destruction, Stage F;
- dependency: stable Stage A-D contracts plus measured Stage E decision.

Stage G remains optional only if product evidence requires deformation-aware
normals, skinned poses, networking or further collision behavior. M9 does not
start automatically. The mandatory M8 exit audit still retains criterion 13 as
`PARTIAL`; four Beauty products remain unresolved.

Persistent Goal: ACTIVE until full JericoEngine 1.0 closure.
