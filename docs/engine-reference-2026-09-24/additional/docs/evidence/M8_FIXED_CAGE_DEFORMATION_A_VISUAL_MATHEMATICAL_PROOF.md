# M8 fixed-cage deformation Stage A — visual and mathematical proof

Date: 2026-08-31
Slice: `M8-FIXED-CAGE-DEFORMATION-A-VISUAL-MATHEMATICAL-PROOF`
Baseline: `8e1db4b1c61b9f4f2db34d7c7f8a74b3af32e9d3`
Status: `PASS` for the bounded Stage A contract.
Persistent Goal: `ACTIVE until full JericoEngine 1.0 closure`.

## Result

Jerico now has one opt-in fixed `3x3x3` object-space cage with exactly 27
x-fastest control points. An optional `FCAG` chunk extends `.rmesh` schema 1.3;
the runtime uploads a 464-byte constant buffer only for an eligible cage asset and
selects separate Diligent/Vulkan main, directional-shadow and contact-AO depth
pipelines. Ordinary meshes retain their previous bytes, buffers, pipelines and
rendered output.

The shader evaluates the tensor-product quadratic Bernstein basis over the mesh
bounds:

```text
B0(t) = (1 - t)^2
B1(t) = 2t(1 - t)
B2(t) = t^2

weight(i,j,k) = Bi(u) Bj(v) Bk(w)
index(i,j,k)  = i + 3(j + 3k)
```

The retained CPU test oracle independently proves partition of unity at five
coordinates and the locality of control point 26: weight `1` at `(1,1,1)`, `0` at
the opposite corner and `1/64` at the center. The productive runtime does not use
that CPU oracle and never rewrites vertices. With point 26 offset by `+0.35 m` on
X, the Vulkan silhouette expands only from the corresponding upper-right region.

Zero cage offsets are visually bit-identical to the ordinary mesh at 640x480:

```text
ordinary SHA-256  D704733AA5678DB4A035FA4C99CFD748D317A9DA3B0F47ADD986E446F1C115D8
rest SHA-256      D704733AA5678DB4A035FA4C99CFD748D317A9DA3B0F47ADD986E446F1C115D8
```

## Authentic RED to GREEN

The implementation retained the following productive failures before their
minimum fixes:

- `.rmesh` could not express, validate or round-trip the canonical fixed cage;
- finite same-sign extreme bounds exposed midpoint overflow in the first canonical
  rest-lattice calculation;
- render code had no opt-in cage state, GPU allocation or measured statistics;
- zero-offset and moved-point captures were identical before the new shader path;
- the HDR material archive had no fixed-cage pipeline closure;
- directional shadows initially lacked `cbFixedCage`, then reused a stale cache
  generation after the state changed;
- contact-AO depth initially rendered the undeformed position;
- explicit frame telemetry did not measure the baseline `StaticPbr` pass;
- fixed-cage IBL and GPU-driven visibility would otherwise have selected
  permutations that do not consume the cage state.

GREEN adds only the optional format seam, the fixed state/buffer, separate shader
variants, conservative bounds, cache identity, supported conventional LOD path,
metrics and focused evidence. IBL and GPU-driven visibility now reject with typed
errors instead of silently diverging. Their later admission requires measured
fixed-cage permutations.

The first broad renderer regression then caught that measuring `StaticPbr` on
every telemetry frame consumed duration-query capacity reserved by deliberate
shadow/FXAA saturation tests. The final interface makes basic `StaticPbr`
measurement explicitly opt-in. The fixed-cage measurement remains real while all
existing telemetry paths retain their prior query usage; the broad regression
passes again.

## Format, determinism and negatives

The fixed format contract is:

```text
JRMS format minor          3 when FCAG is present
FCAG schema                1
point count                27 exactly
ordering                   x-fastest i + 3(j + 3k)
payload bytes              332
runtime constant bytes     464
maximum fixture offset     0.4 m
```

Two writes of identical inputs produce identical bytes. Read/write tests prove
points 0, 1, 3, 13 and 26 against asymmetric mesh bounds, then round-trip all 27
points and the maximum displacement. A retained extreme-finite-bounds RED now
proves every derived rest point remains finite through an overflow-safe midpoint.
Negatives reject a malformed point count,
zero maximum displacement and any non-canonical rest point. Runtime negatives
reject a cage state on an ordinary mesh, non-finite or over-budget offsets, IBL and
non-fallback GPU visibility. Existing `.rmesh` inputs without `FCAG` keep their
old minor version and byte layout.

## Vulkan visual, bounds, shadow and LOD evidence

All retained BMP files are native 640x480 captures from the Vulkan test on an
NVIDIA GeForce RTX 3090 with validation enabled:

| Capture | SHA-256 | Finding |
| --- | --- | --- |
| [`fixed-cage-ordinary.bmp`](images/m8-fixed-cage-stage-a/fixed-cage-ordinary.bmp) | `D704733AA5678DB4A035FA4C99CFD748D317A9DA3B0F47ADD986E446F1C115D8` | Ordinary static PBR baseline. |
| [`fixed-cage-rest.bmp`](images/m8-fixed-cage-stage-a/fixed-cage-rest.bmp) | `D704733AA5678DB4A035FA4C99CFD748D317A9DA3B0F47ADD986E446F1C115D8` | Zero cage state is bit-identical to ordinary. |
| [`fixed-cage-deformed.bmp`](images/m8-fixed-cage-stage-a/fixed-cage-deformed.bmp) | `C4E6F9F93158D509B4CDEB21EAE5991AD2B7C84AA53780F70046B3B7C5F5AE60` | Point 26 expands the expected silhouette region. |
| [`fixed-cage-shadow-rest.bmp`](images/m8-fixed-cage-stage-a/fixed-cage-shadow-rest.bmp) | `C2A7EDCAB9150C60DB06E1538B51728E6030D420A7942BD45EA7CDD6F742CF09` | Rest state through directional shadow and AO depth. |
| [`fixed-cage-shadow-deformed.bmp`](images/m8-fixed-cage-stage-a/fixed-cage-shadow-deformed.bmp) | `8C78BFA5218AB9892715FFEDD2B8157E870165A2A89FF65067C0539890214A55` | Deformed main, shadow and depth silhouettes agree. |
| [`fixed-cage-source-free-rest.bmp`](images/m8-fixed-cage-stage-a/fixed-cage-source-free-rest.bmp) | `D704733AA5678DB4A035FA4C99CFD748D317A9DA3B0F47ADD986E446F1C115D8` | Package-only rest replay equals resident rest. |
| [`fixed-cage-source-free-deformed.bmp`](images/m8-fixed-cage-stage-a/fixed-cage-source-free-deformed.bmp) | `C4E6F9F93158D509B4CDEB21EAE5991AD2B7C84AA53780F70046B3B7C5F5AE60` | Package-only deformation equals resident deformation. |

The captures were inspected at original resolution. They are a neutral
mathematical/visual witness, not Beauty acceptance for M8 criterion 13.

The focused contract additionally proves:

- directional shadow pages render again after the cage state changes;
- contact-AO depth selects its fixed-cage pipeline;
- near LOD0 submits 12 triangles and far LOD1 submits 4 triangles for both the
  ordinary and cage-enabled assets;
- at a frustum boundary, the ordinary bounds submit zero draws while the same
  deformed cage submits one draw through the maximum-displacement expansion;
- both LODs evaluate the same normalized cage domain; identical topology between
  LODs is not required.

## Exact source-free replay

The test writes one complete package in memory with exactly two runtime entries:
`M8/fixed-cage.rmesh` and `M8/fixed-cage.rmat`. It then discards the source-side
objects as replay inputs, parses only those package payloads, uploads them to
Vulkan and renders rest plus deformed states.

```text
package content hash       8929095179939204720
package bytes              2132567
package role               Complete
entry count                2
mesh format minor          3
fixed cage points          27
fallback/source reads      0
```

The package payload is intentionally not committed as a duplicate binary artifact;
the deterministic content hash and the bit-identical resident/package capture
hashes retain the exact replay claim. This is a Debug Vulkan package-only proof,
not a new Shipping-wide certification.

## Measured cost and limitation

The retained 640x480 evidence run measured real Vulkan duration queries for the
`StaticPbr` pass and recorded the CPU cage upload separately:

| Metric | Ordinary | Deformed cage |
| --- | ---: | ---: |
| `StaticPbr` GPU duration | `0.028672 ms` | `0.028672 ms` |
| Cage CPU upload | none | `0.0442 ms` |
| Cage bytes uploaded | `0` | `464` |
| Cage resident bytes | `0` | `464` |
| Vertices evaluated by fixture | `8` | `8` |

These are single-frame fixture measurements, not p50/p95, a scalability budget or
an approval for many simultaneously deforming objects. Ordinary assets allocate
and upload zero cage bytes and execute their existing shader path. Stage B/F must
add event-frequency and scene-scale measurements before production admission.

## Collision and physical authority

The public runtime info reports `VisualOnly` for cage-enabled assets and `None`
for ordinary assets. Jolt collision stays unchanged. No proxy, triangle recook,
new damage authority, impact response, Physical Material field or Fracture rule
was introduced. Advanced deformation-aware normal correction is absent; the
existing normals/tangents remain the explicit Stage A baseline.

## End-to-end system completeness and data lineage audit

```text
cooked mesh bounds + eligibility + max displacement
-> canonical 27-point FCAG rest lattice
-> deterministic .rmesh bytes / exact package entry
-> runtime fixed-cage metadata + direct 27-offset state
-> 464-byte GPU constant upload only when dirty/eligible
-> quadratic object-space vertex deformation
-> PBR output + directional shadow + contact-AO depth
-> conservative bounds + conventional LOD selection
-> retained Vulkan pixels and measured telemetry
```

Useful upstream information is preserved as the canonical rest lattice, maximum
displacement, fixed ordering, direct offsets and explicit `VisualOnly` policy.
Stage A deliberately has no impact provenance, plastic/elastic classification,
fracture outcome or collision proxy data to lose.

| Area | Stage A result and boundary |
| --- | --- |
| Source/input and asset pipeline | Eligibility is explicit in cooked `.rmesh`; `FCAG` derives deterministically from bounds. No source asset or editor schema is added. |
| Serialization, package, save/load | Rest cage and budget round-trip through `.rmesh` and `.epak`. Live offsets are direct caller state and are not yet saved. |
| ECS/components and runtime representation | The render request owns an optional typed 27-offset view; no new World component or parallel physical authority. |
| CPU/GPU and renderer | CPU validates/uploads bounded state; GPU deforms vertices. Ordinary meshes pay no cage allocation, binding or shader execution. |
| Shaders/materials/lighting | Separate PBR, directional-shadow and contact-AO depth variants are admitted. IBL and GPU visibility reject explicitly. |
| Bounds, streaming and LOD | Maximum-displacement expansion prevents the tested frustum error; conventional supported LODs share the normalized domain. No general streaming policy change. |
| Physics/collision | `VisualOnly`; ordinary Jolt collision is intentionally unchanged. |
| Animation, Luau, events and Sequencer | Not consumed in Stage A. Event response and exposure become later bounded interfaces, not hidden here. |
| Fracture/destruction and runtime modification | No handoff or topology change. Existing Fracture authority remains exclusive. |
| Navigation/gameplay queries | Unchanged because collision and topology are unchanged; no deformed-surface query is claimed. |
| Particles/audio | No new event exists to consume; later impact/fracture events may feed existing systems. |
| Debugging/profiling | Typed errors, path counters, uploaded/resident bytes, vertex count, CPU upload time and real GPU duration are retained. |
| Tooling/UI/authoring | Evidence command only. Inspector, cage gizmo and transactional authoring are deferred to Stage F. |
| Testing/failure handling | Format, math, rest identity, visual locality, bounds, shadow, depth, LOD, package replay and malformed/budget negatives are focused tests. |

## INTEGRATION REQUIRED after Stage A

| Milestone | Subsystem and reason | Required input/data | Expected interface | Owner | Dependency |
| --- | --- | --- | --- | --- | --- |
| Stage B — event-driven impact response | Convert the existing admitted impact into a bounded cage update and sleep when settled. | Existing `ImpactDescriptor`, object transform, cage bounds/budget and quantitative Physical Material response. | One deterministic 27-point event calculation, dirty publication and no-tick settled state. | Engine impact/physical-response owner plus the fixed-cage runtime state owner. | Stage A representation and a separately frozen response function with explicit units. |
| Stage C — plastic and optional elastic response | Preserve permanent dents and schedule only bounded recovery work. | Stage B response, plastic target, optional recovery policy/rate and explicit limits. | Persistent cage-state serialization plus event-driven recovery/sleep lifecycle. | World/runtime deformation-state and serialization owners. | Stage B classification; save/load and deterministic-state contract. |
| Stage D — Fracture handoff | A stronger causal event must enter existing localized structural failure without a second health system. | Same impact, Physical Material, geometry, deformation classification and existing bond/support topology. | Existing localized Fracture evaluation and support traversal; detached results become Jolt bodies. | Existing Physical Material and Fracture/destruction owners. | Stage B/C response classification and current admitted Fracture graph. |
| Stage E — collision prototype | Determine whether a bounded coarse collision tier is worth its cost. | Dirty cage state, actual contact requirements and Jolt shape ownership. | Prototype the cheapest small primitive/compound update only on state change; benchmark count, update time and contact behavior. | Engine/Physics Jolt owner. | Stage A visual truth plus representative product cases; no default before measurements. |
| Stage F — editor authoring and production hardening | Make the asset policy usable, transactional, scalable and diagnosable. | Eligibility, max displacement, influence/recovery/collision policies and optional cage-domain override. | Inspector, cage visualization, Preview/Cancel/Apply/Undo/Redo, cook/reimport, batch, debug views and source-free validation. | JericoEditor, asset-cook and runtime-format owners. | Stable Stage B-E runtime policies and scene-scale profiling. |

Stage G remains optional, not required for this PASS: deformation-aware normal
correction, authored poses, compact networking or a higher-fidelity collision tier
must be justified by product evidence and measured cost before admission.

## Verification

```text
cmake --build build/windows-vulkan-editor --config Debug --target JericoStaticMeshRenderAssetTests --parallel 8
PASS

build/windows-vulkan-editor/Tests/Render/Debug/JericoStaticMeshRenderAssetTests.exe --fixed-cage-contract-only
PASS on Vulkan / RTX 3090

build/windows-vulkan-editor/Tests/Render/Debug/JericoStaticMeshRenderAssetTests.exe --fixed-cage-evidence docs/evidence/images/m8-fixed-cage-stage-a
PASS on Vulkan / RTX 3090, seven 640x480 captures written and inspected

ctest --test-dir build/windows-vulkan-editor -C Debug -R "^runtime_formats\\.rmesh$" --output-on-failure
PASS 1/1

ctest --test-dir build/windows-vulkan-editor -C Debug -R "^(runtime_formats\\.rmesh|render\\.static_mesh_asset)$" --output-on-failure
PASS 2/2, full static-mesh renderer regression 74.35 s
```

Expected Vulkan performance warnings note unused normal/tangent attributes in
depth-only shadow vertex shaders. No `render.diligent.error`, fatal diagnostic,
fallback or validation test failure occurred.

Persistent Goal: ACTIVE until full JericoEngine 1.0 closure.
