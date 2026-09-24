# M8 Fixed-Cage Stage C — Plastic/Elastic Persistence Evidence

Date: 2026-09-01
Slice: `M8-FIXED-CAGE-DEFORMATION-C-PLASTIC-ELASTIC-PERSISTENCE`
Local status: `PASS`
Persistent Goal: `ACTIVE until full JericoEngine 1.0 closure`

## Scope proved

This slice extends the accepted Stage B fixed-cage response without adding a
second impact authority. Stage B still owns the unit-bearing impact descriptor,
the cooked Physical Material compressive-strength response and the direct
27-point spatial solve. Stage C only classifies that result into a persistent
plastic target and an optional bounded linear recovery.

It does not implement fracture handoff, deforming Jolt collision, authoring UI,
wrinkles, simulation tensors, FEA or M9 work.

## Authentic RED

The RED was written before production changes in:

- `Tests/RuntimeFormats/MeshAssetTests.cpp`;
- `Tests/Render/StaticMeshRenderAssetTests.cpp`.

The first build of `JericoMeshAssetTests` and
`JericoStaticMeshRenderAssetTests` exited `1` because the executable contract
required production symbols that did not exist: `MeshFixedCageResponsePolicy`,
`MeshFixedCageRecoveryMode` and `MeshFixedCage::responsePolicy`. The RED also
named the missing recovery, save/load and rejection behavior before those paths
were implemented.

The completed contract proves:

1. a non-zero plastic dent survives after recovery work unregisters;
2. disabled recovery schedules no work and publishes no unchanged state;
3. linear recovery moves monotonically to the cooked target and settles;
4. equal asset/event/timestep inputs reproduce exact state bytes and publication
   order;
5. malformed policy, state, timestep and atomic work budget reject without
   mutation;
6. an ordinary non-cage asset, including one without a persistent mesh identity,
   remains unaffected;
7. the real `WorldSaveSnapshot` saveable-component seam round-trips the state.

## Cooked policy and explicit limits

The `.rmesh` owns the response policy. A default fixed cage remains byte-compatible
JRMS 1.3. A non-default policy writes JRMS 1.4 with one required 16-byte `FCRP`
chunk, schema version 1.

| Field | Meaning and unit | Admitted range |
|---|---|---|
| `plasticRetentionRatio` | Dimensionless fraction of the Stage B event delta retained as the plastic target | `[0, 1]`; disabled recovery requires a non-zero retained result |
| `recoveryMode` | Explicit recovery algorithm | `Disabled` or `Linear` |
| `recoverySpeedMetersPerSecond` | Maximum object-space cage-point travel per second | `0` when disabled; `[0.0001, 100] m/s` when linear |
| recovery timestep | Caller-supplied bounded step | `[0.0001, 0.25] s` |
| work budget | Atomic cage update | exactly 27 control points |
| settle tolerance | Exact-target snap threshold | `0.00001 m` |
| maximum displacement | Existing FCAG object-space radius | unchanged and enforced for current and target state |

There is no permanent per-frame tick or World-wide scan. A response reports one
pending work item only while its current state differs from its plastic target;
settlement removes that object from active recovery.

## Persistence contract

`Jerico.Destruction.FixedCageDeformationState` is an Engine-owned saveable
component with stable type ID `0x4A44464341474531`, schema version 1 and exactly
752 canonical bytes. Its normalized FNV-1a content hash covers:

- stable object identity;
- cooked mesh identity;
- cooked Physical Material identity and definition hash;
- source impact event identity;
- publication generation;
- 27 current object-space offsets;
- 27 plastic-target object-space offsets.

Restore rejects corrupt hashes, unsupported versions, non-finite/out-of-bound
offsets, incoherent provenance and mismatched object/mesh/material lineage before
mutating the live response. A restored state republishes only when its current
render bytes differ from the live state.

## Data lineage

```text
Stage B ImpactDescriptor
+ cooked .rmesh FCAG/FCRP
+ cooked .rcoll Physical Material identity/hash/compressive strength
-> deterministic Stage B 27-point impact result
-> Stage C plastic target classification
-> optional per-object bounded linear recovery
-> current cage state + generation + one dirty publication when bytes change
-> existing Stage A GPU cage upload
-> vertex-shader cage interpolation for all mesh vertices
-> bounds / shadow / contact-AO depth / conventional LOD consumers
-> saveable deformation component
-> WorldSaveSnapshot capture/restore
-> source-free package replay
```

The useful upstream semantics retained downstream are asset policy, object/mesh
identity, Physical Material identity/hash, source event, generation, current
shape and plastic target. Jolt collision remains explicitly `VisualOnly` and is
not presented as following the visual deformation.

## Source-free and Vulkan proof

The focused evidence command builds a complete in-memory package from exactly
three cooked payloads (`.rmesh`, `.rmat`, `.rcoll`), discards resident input
pointers at the package boundary, decodes only package entry bytes and repeats
the same event plus six recovery steps.

```text
package role                         Complete
package entries                      3
package bytes                        2133092
package content hash                 4376141820902678232
state bytes                          752
active state normalized hash         14641777700188145091
resident publications                7
package publications                 7, byte/order identical
resident/package settled pixels      bit-identical
Vulkan device                        NVIDIA GeForce RTX 3090
capture resolution                   640x480 original
```

The retained successful measurement separates the relevant work:

```text
event/classification CPU             0.0073 ms
six active recovery steps CPU        0.0120 ms total
dirty cage upload CPU                 0.0089 ms
dirty cage upload bytes               464
StaticPbr GPU duration                0.105472 ms
settled response work/upload/backlog  0 / 0 / 0
```

The settled CPU result is not described as zero GPU cost. The GPU measurement is
reported independently.

### Original-resolution captures

| Capture | SHA-256 | Observation |
|---|---|---|
| `fixed-cage-recovery-peak.bmp` | `fa128f33b9dc91b43317e7f7e3c241ee12065bd2522ba84ceea295bfd2e9a5ba` | Immediate elastic peak differs from the settled result |
| `fixed-cage-plastic-settled.bmp` | `50d34b6df8a6a9bad138d50565f928ee47d5a1df2d73afade734bcc72d1bb896` | Non-zero retained plastic shape after CPU sleep |
| `fixed-cage-persistence-source-free.bmp` | `50d34b6df8a6a9bad138d50565f928ee47d5a1df2d73afade734bcc72d1bb896` | Package-only replay is pixel-identical |
| `fixed-cage-shadow-rest.bmp` | `c2a7edcab9150c60db06e1538b51728e6030d420a7942bd45ea7cdd6f742cf09` | Existing Stage A rest shadow path |
| `fixed-cage-shadow-deformed.bmp` | `8212e7a5510af1674d4ed19c40e3ec832e6f479fb7764646735715632fa73c7f` | Shadow path consumes the Stage C current state |

All ten retained 640x480 captures are under
`docs/evidence/images/m8-fixed-cage-stage-c/`. This neutral box witness is
contract evidence, not Beauty credit and not an example of fine wrinkles.

## Verification

```text
focused JRMS policy/negative contract                         PASS
focused plastic/elastic/save contract                         PASS
focused source-free + Vulkan evidence                         PASS
scene.world_save                                              PASS 0.04 s
scene.component_registry                                      PASS 0.03 s
runtime_formats.rmesh                                         PASS 0.02 s
render.static_mesh_asset                                      PASS 68.23 s
integration total                                             PASS 4/4, 69.58 s
git diff --check                                              PASS
```

One unrelated build-graph problem was observed: building the broad render test
target after CMake regeneration also invalidated the MediumHigh atmosphere LUT
fixture. Its retained metrics predict roughly 35 minutes on four workers. That
recook was cancelled and the admitted render/runtime targets were rebuilt
directly. No atmosphere source or CMake policy was changed in this slice.

## End-to-end system completeness audit

| Area | Stage C result |
|---|---|
| Source/input data | Uses the existing Stage B descriptor; no second event source |
| Asset pipeline/runtime format | JRMS 1.4 `FCRP`; default JRMS 1.3 remains byte-compatible |
| Editor/authoring | `INTEGRATION REQUIRED` in Stage F; no direct binary asset editing |
| ECS/components | Versioned Engine-owned saveable deformation component registered with Destruction |
| Runtime/CPU | Direct 27-point classification and active-object-only recovery |
| GPU/renderer/shaders | Existing Stage A cage buffer and shader interpolation consume current state unchanged |
| Materials | Existing cooked Physical Material quantitative seam remains the only resistance input |
| Physics/collision | Jolt remains `VisualOnly`; deforming collision is deliberately not claimed |
| Animation | Not applicable to the static-mesh Stage C contract |
| Luau/events | Stage B event authority unchanged; gameplay-facing authoring/read API deferred to Stage F if required |
| Sequencer | Not required for deterministic static recovery; no hidden timeline ownership added |
| Serialization/save-load | Schema-1 component round-trips through `WorldSaveSnapshot` with exact lineage |
| Streaming/package | Complete source-free package replay proved from three cooked entries |
| LOD/shadow/AO/bounds | Existing Stage A downstream paths exercised with the settled Stage C state |
| Fracture/destruction | Plastic state is complete locally; same-event Fracture handoff remains Stage D |
| Navigation/gameplay queries | No collision/nav claim while policy is `VisualOnly` |
| Particles/audio | Not causal owners of deformation; future Fracture reactions belong after Stage D |
| Debug/visualization | Original-resolution captures and typed diagnostics retained; editor views deferred to Stage F |
| Profiling/budgets | Event, recovery, upload and GPU costs measured separately; exact atomic budget enforced |
| Failure handling | Policy/state/timestep/budget/hash/lineage negatives are typed and mutation-free |
| Documentation/testing | Executable RED, focused/integration tests, hashes and this audit retained |

## Required future integrations — not implemented here

### Stage D — same-event localized Fracture handoff

`INTEGRATION REQUIRED`

- subsystem: existing localized Fracture authority;
- reason: a product may need a Stage B impact to exceed visual cage response and
  enter real fracture without creating a second damage event;
- required input: the same stable event identity, admitted impact descriptor,
  Physical Material lineage and current cage state;
- expected interface: one explicit handoff into the existing Fracture owner;
- owner/milestone: Destruction/Fracture, Stage D;
- dependency: this completed Stage C state and the already accepted Stage B event.

### Stage E — measured deforming-collision prototype

`INTEGRATION REQUIRED`

- subsystem: Jolt collision/physics;
- reason: the current contract is intentionally visual-only;
- required input: bounded current cage state and representative product scenes;
- expected interface: measured prototype with update cost, stability and fallback;
- owner/milestone: Physics + Destruction, Stage E;
- dependency: Stage D ordering and explicit product evidence.

### Stage F — editor and production hardening

`INTEGRATION REQUIRED`

- subsystem: JericoEditor/asset cook/debug/profiling;
- reason: designers need policy authoring rather than binary editing;
- required input: schema-1 retention/mode/speed limits and live state telemetry;
- expected interface: preview/diff, validation, cancellation, transaction,
  undo/redo, deterministic cook, debug views and scale testing;
- owner/milestone: Editor + Asset Pipeline + Destruction, Stage F;
- dependency: stable Stage C/D/E contracts.

Stage G remains optional only if product evidence requires deformation-aware
normal correction, skinned poses, networking or further collision behavior.
M9 does not start automatically. The mandatory M8 exit audit still retains
criterion 13 as `PARTIAL`; four Beauty products remain unresolved.

Persistent Goal: ACTIVE until full JericoEngine 1.0 closure.
