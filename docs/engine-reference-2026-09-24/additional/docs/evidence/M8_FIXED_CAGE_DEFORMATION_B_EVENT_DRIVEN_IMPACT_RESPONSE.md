# M8 fixed-cage deformation Stage B — event-driven impact response

Date: 2026-08-31
Slice: `M8-FIXED-CAGE-DEFORMATION-B-EVENT-DRIVEN-IMPACT-RESPONSE`
Baseline: `9f53cd95a9a1123bc9666c9364ded549384f5ee8`
Status: `PASS` for the bounded Stage B contract.
Persistent Goal: `ACTIVE until full JericoEngine 1.0 closure`.

## Result

Jerico now admits one generic `Jerico::Destruction::ImpactDescriptor` and one
per-object `FixedCageImpactResponse`. A valid event carries a world-space point in
meters, a normalized world-space direction, deposited energy density in `MJ/m^3`,
an effective radius in meters and a stable source-event GUID. The response binds
one object GUID, its current transform, one validated Stage A `FCAG` mesh and its
cooked Physical Material.

The response is synchronous and bounded: it evaluates exactly 27 rest points,
updates only the bound object's direct cage state and publishes that state once
when bytes actually change. It has no World scan, CPU vertex rewrite, mesh rebuild,
continuous deformation tick or asynchronous backlog. An ordinary mesh with the
same Physical Material remains ineligible and performs no cage work or upload.

No parallel dent-material authority was introduced. The existing cooked
`compressiveStrengthMegapascals` is the smallest sufficient quantitative seam.
It is owned by `CollisionPhysicalMaterial`, remains serialized in `.rcoll`, and is
consumed here as a direct resistance energy density because:

```text
1 MPa = 1 MJ/m^3
```

## Frozen deterministic response

For each canonical rest point `q_i`, object transform `(T, R, S)`, event point
`p`, event direction `v`, radius `r`, maximum cage displacement `m`, deposited
energy density `e` and compressive strength `c`:

```text
world(q_i) = T + R(S q_i)
distance_i = |world(q_i) - p|
linear_i = clamp(1 - distance_i / r, 0, 1)
falloff_i = linear_i^2 (3 - 2 linear_i)
drive = clamp(e / c, 0, 1)
object_direction = normalize(S^-1 R^-1 v)
delta_i = object_direction * m * drive * falloff_i
```

The accumulated direct offset is clamped to the Stage A maximum displacement.
The inverse scale is required to preserve an arbitrary world direction under a
non-uniform object transform. The focused fixture uses translation `(2,3,4)`, a
90-degree Z rotation, scale `(2,1,0.5)` and an oblique normalized direction. Its
point 26 receives `(0.14142136, 0.14142136, 0)` meters while the opposite point
receives exactly zero. Identical event, transform, mesh and material inputs produce
byte-identical 27-point state; a stronger unclamped event produces a larger result,
and doubling compressive strength produces a smaller result.

The response is independent of display names. Object, source-event and Physical
Material identities plus the material definition hash survive into application
statistics/publication provenance.

## Authentic RED to GREEN

The retained development sequence demonstrated these failures before the minimum
productive changes:

- the renderer test could not compile because no generic impact descriptor or
  fixed-cage response API existed;
- the first response header pulled the monolithic renderer header into
  `DestructionRuntime`; extracting the unchanged state POD into
  `StaticMeshFixedCageState.hpp` restored the intended small contract boundary;
- an oblique event under non-uniform scale exposed a rotation-only direction
  conversion; the test failed both expected point-26 components until the inverse
  scale was applied;
- an almost singular positive scale was initially accepted and could overflow the
  inverse transform; the retained malformed-transform test forced explicit finite
  scale limits;
- the first no-event metric confused the one-time 464-byte zero-state GPU
  residency upload with a deformation upload. The retained test now measures the
  subsequent unchanged frame: response work `0`, upload `0`, backlog `0`.

Malformed descriptors reject non-finite/out-of-range positions, non-unit
directions, zero/out-of-budget radii, zero/out-of-budget energy density and zero
event identity with `destruction.impact.invalid`. Invalid binding, transform,
canonical cage or structural material data has a separate typed error. Rejections
leave state clean and publish nothing.

## Event, dirty publication and inactivity

The focused event produces:

```text
evaluated points          27 exactly
affected points           15
publication generation    1
dirty publications        1
pending work after event  0
unchanged next-frame work 0
unchanged upload bytes    0
```

The event state uploads 464 bytes once. Repeating the same render state uploads
zero bytes. A never-deformed response has no publication and no work; the renderer
may still allocate the Stage A zero-state cage buffer on first residency, which is
not reported as deformation work. No-event inactivity is not presented as zero GPU
cost: the normal static-PBR pass still executes.

## Exact source-free runtime application

The Stage B test writes one complete in-memory package with exactly these cooked
runtime entries:

```text
M8/fixed-cage-impact.rmesh
M8/fixed-cage-impact.rmat
M8/fixed-cage-impact.rcoll
```

It reads only the three package payloads, reconstructs the response from the
packaged fixed cage and the packaged Physical Material, reapplies the same generic
event, uploads the result and renders it on Vulkan.

```text
package content hash       16663408142763578608
package bytes              2133023
package role               Complete
entry count                3
source fallback reads      0
resident/replay cage state byte-identical
resident/replay pixels     bit-identical
```

The package payload is intentionally not committed as a duplicate binary artifact.
This is a focused Debug Vulkan package-only proof, not a new Shipping-wide
certification.

## Vulkan visual, shadow, AO, bounds and LOD evidence

All retained BMPs are native 640x480 captures from Vulkan on an NVIDIA GeForce RTX
3090 with validation enabled. The event-produced state is passed directly into the
existing Stage A test; no second deformation calculation creates the renderer,
shadow, AO, bounds or LOD inputs.

| Capture | SHA-256 | Finding |
| --- | --- | --- |
| [`fixed-cage-impact-event.bmp`](images/m8-fixed-cage-stage-b/fixed-cage-impact-event.bmp) | `FA128F33B9DC91B43317E7F7E3C241EE12065BD2522BA84CEEA295BFD2E9A5BA` | Resident event-produced silhouette. |
| [`fixed-cage-impact-source-free.bmp`](images/m8-fixed-cage-stage-b/fixed-cage-impact-source-free.bmp) | `FA128F33B9DC91B43317E7F7E3C241EE12065BD2522BA84CEEA295BFD2E9A5BA` | Package-only event response is pixel-identical. |
| [`fixed-cage-ordinary.bmp`](images/m8-fixed-cage-stage-b/fixed-cage-ordinary.bmp) | `D704733AA5678DB4A035FA4C99CFD748D317A9DA3B0F47ADD986E446F1C115D8` | Ordinary static-PBR baseline. |
| [`fixed-cage-rest.bmp`](images/m8-fixed-cage-stage-b/fixed-cage-rest.bmp) | `D704733AA5678DB4A035FA4C99CFD748D317A9DA3B0F47ADD986E446F1C115D8` | Rest cage is bit-identical to ordinary. |
| [`fixed-cage-deformed.bmp`](images/m8-fixed-cage-stage-b/fixed-cage-deformed.bmp) | `FA128F33B9DC91B43317E7F7E3C241EE12065BD2522BA84CEEA295BFD2E9A5BA` | The exact event state changes the expected hit-side silhouette. |
| [`fixed-cage-shadow-rest.bmp`](images/m8-fixed-cage-stage-b/fixed-cage-shadow-rest.bmp) | `C2A7EDCAB9150C60DB06E1538B51728E6030D420A7942BD45EA7CDD6F742CF09` | Rest state through directional shadow and AO depth. |
| [`fixed-cage-shadow-deformed.bmp`](images/m8-fixed-cage-stage-b/fixed-cage-shadow-deformed.bmp) | `A86D5C3DC8B8EE93BA5383E1944306584A22384B049753EE7A86F4644C55111F` | Event state changes main, directional-shadow and AO-depth output coherently. |
| [`fixed-cage-source-free-rest.bmp`](images/m8-fixed-cage-stage-b/fixed-cage-source-free-rest.bmp) | `D704733AA5678DB4A035FA4C99CFD748D317A9DA3B0F47ADD986E446F1C115D8` | Stage A package replay at rest. |
| [`fixed-cage-source-free-deformed.bmp`](images/m8-fixed-cage-stage-b/fixed-cage-source-free-deformed.bmp) | `FA128F33B9DC91B43317E7F7E3C241EE12065BD2522BA84CEEA295BFD2E9A5BA` | Stage A package replay of the exact event state. |

The captures were inspected at original resolution. They prove direct geometric
and downstream-path coherence, not the separate M8 criterion-13 Beauty gate.
The retained Stage A checks additionally prove directional shadow pages, contact-AO
depth, conservative frustum bounds and conventional LOD0/LOD1 (12/4 triangles)
consume this exact state.

## Separate measured costs

The retained 640x480 evidence run reports the event CPU calculation, dirty upload
and GPU duration independently:

| Metric | Measured value |
| --- | ---: |
| Event response CPU | `0.0168 ms` |
| Evaluated / affected controls | `27 / 15` |
| Dirty cage upload CPU | `0.0586 ms` |
| Dirty cage bytes | `464` |
| Fixed-cage resident bytes | `464` |
| Fixed-cage `StaticPbr` GPU duration | `0.028672 ms` |
| Ordinary `StaticPbr` GPU duration | `0.028672 ms` |
| Settled response work / upload / backlog | `0 / 0 / 0` |

These are single-frame fixture measurements, not p50/p95 or a scene-scale budget.
They do not claim zero GPU cost in an inactive frame.

## Collision and physical authority

Stage A remains `VisualOnly`. Jolt collision, contact shape ownership and material
attachment are unchanged. This slice adds no proxy, triangle recook, Jolt shape
update, Fracture handoff, bond/support traversal or second damage authority. The
generic descriptor is a consumer boundary; a product-specific Physics or gameplay
producer may publish it without creating a cage-specific event type.

## End-to-end system completeness and data-lineage audit

```text
physics/gameplay source event + stable provenance
-> generic world-space ImpactDescriptor with explicit units
-> one object identity + current T/R/S transform
-> package-only FCAG eligibility + maximum displacement
-> package-only Physical Material compressive strength + identity/hash
-> deterministic bounded evaluation of exactly 27 rest points
-> per-object direct offsets + generation + one dirty publication
-> existing 464-byte Stage A GPU state upload only on change
-> PBR + bounds + directional shadow + contact-AO depth + conventional LOD
-> retained Vulkan pixels, hashes and separate CPU/upload/GPU metrics
```

Useful upstream information retained downstream is the source-event GUID, object
GUID, Physical Material GUID/definition hash, world point/direction/radius/energy
units, object transform, FCAG budget, deterministic state bytes and publication
generation. No event/material/object identity is replaced by a display name.

| Area | Stage B result and boundary |
| --- | --- |
| Source/input and event ownership | One generic descriptor is frozen in `Jerico::Destruction`; this fixture is a direct admitted producer. Automatic Jolt contact interpretation remains producer policy, not cage logic. |
| Asset pipeline and runtime formats | Stage A `.rmesh` FCAG and existing `.rcoll` Physical Material are reused unchanged; no source schema or duplicate material field is added. |
| ECS/components and runtime representation | One bound runtime response owns one object's 27 offsets, generation and dirty bit. No World scan or generic health component. |
| CPU processing | One accepted event evaluates exactly 27 points synchronously; settled frames schedule no response work. |
| GPU, renderer and shaders | Existing Stage A fixed-cage pipelines consume the direct publication. No shader or renderer response calculation is duplicated. |
| Materials and lighting | Existing compressive strength supplies quantitative resistance. PBR, directional shadow and AO depth consume the same result. |
| Physics/collision | Physical Material input is cooked physics-owned data; Jolt collision remains explicitly `VisualOnly` and unchanged. |
| Animation | No skeletal/vertex animation ownership is introduced; fixed-cage state remains a separate static-mesh path. |
| Luau, events and Sequencer | The generic descriptor is callable from future admitted producers; no Luau/Sequencer API or event subscription surface is added in this bounded runtime proof. |
| Serialization and save/load | Rest cage/material serialize through package entries. Live offsets are not persistent until Stage C. |
| Streaming and LOD | Existing conventional LOD path consumes the event state. No GPU-driven visibility or new streaming residency policy is admitted. |
| Destruction/runtime modification | Direct offsets change; topology and Fracture authority do not. Stage D owns causal Fracture handoff. |
| Navigation/gameplay queries | Unchanged because collision/topology are unchanged; no deformed-surface query is claimed. |
| Particles and audio | No effect dispatch is added; source-event provenance remains available for later admitted consumers. |
| Debugging/visualization | Typed diagnostics, event/object/material provenance, generation, point counts, captures and exact hashes are retained. |
| Profiling/performance budgets | Event CPU, upload CPU/bytes and `StaticPbr` GPU duration are separate. Scene-scale distributions remain Stage F hardening. |
| Tooling/UI/authoring | No editor UI is added. Preview, policy authoring, undo/redo, batch and reimport belong to Stage F. |
| Testing/failure handling | Locality, determinism, strength ordering, material distinction, ordinary/inactive behavior, malformed/budget/transform negatives, package replay and Vulkan downstream paths are executable. |

## INTEGRATION REQUIRED after Stage B

| Milestone | Subsystem and reason | Required input/data | Expected interface | Owner | Dependency |
| --- | --- | --- | --- | --- | --- |
| Stage C — plastic persistence and optional elastic recovery | Live direct offsets need an explicit durable lifecycle; recovery must not become a permanent tick. | Stage B state/publication, event classification, plastic target, optional recovery policy/rate and stable object identity. | Versioned save/load for persistent offsets plus bounded scheduled recovery that sleeps at rest. | World/runtime deformation-state and serialization owners. | Stage B deterministic state and a separately frozen plastic/elastic rule. |
| Stage D — Fracture handoff | A sufficiently strong causal impact must enter the existing localized structural authority without parallel health/damage. | Same descriptor/provenance, Physical Material, deformation result and existing bond/support graph. | One typed handoff into existing `DestructionCoordinator` localized load/support evaluation; detached results become Jolt bodies. | Existing Destruction/Fracture and Physical Material owners. | Stage B event identity and Stage C classification where required; current admitted Fracture graph. |
| Stage E — measured collision prototype | Product contact may require a cheapest bounded approximation, but Stage B evidence does not justify a default. | Dirty cage state, representative contact cases, object transform and Jolt shape ownership. | Update a small primitive/compound proxy only on state change; measure count, CPU update time and contact error. | Engine/Physics Jolt owner. | Stage B visual state plus representative product evidence and benchmark thresholds. |
| Stage F — editor, producer wiring and production hardening | Designers need transactional policy authoring and real product producers need one observable route to the generic descriptor. | Eligibility/budget, response/recovery/collision policies, stable source producers, profiling distributions and diagnostics. | Inspector and cage visualization; Preview/Cancel/Apply/Undo/Redo; cook/reimport/batch; one producer adapter per admitted event source; source-free validation and scene-scale budgets. | JericoEditor, asset-cook, Physics/gameplay-event and profiling owners. | Stable Stage B-E runtime policies and representative product scenes. |

Stage G remains optional: deformation-aware normals, authored poses, compact
network replication or higher-fidelity collision require product evidence and
measured cost. None is implied by this PASS.

## Verification

```text
cmake --build build/windows-vulkan-editor --config Debug --target JericoStaticMeshRenderAssetTests --parallel 8
PASS

build/windows-vulkan-editor/Tests/Render/Debug/JericoStaticMeshRenderAssetTests.exe --fixed-cage-impact-contract-only
PASS on Vulkan / RTX 3090

build/windows-vulkan-editor/Tests/Render/Debug/JericoStaticMeshRenderAssetTests.exe --fixed-cage-impact-evidence docs/evidence/images/m8-fixed-cage-stage-b
PASS on Vulkan / RTX 3090, nine 640x480 captures written and inspected

ctest --test-dir build/windows-vulkan-editor -C Debug -R "^(runtime_formats\\.rmesh|render\\.static_mesh_asset)$" --output-on-failure
PASS 2/2, full static-mesh renderer regression 86.37 s
```

Expected Vulkan informational output remains present. No
`render.diligent.error`, fatal diagnostic, fallback or validation test failure
occurred.

Persistent Goal: ACTIVE until full JericoEngine 1.0 closure.
