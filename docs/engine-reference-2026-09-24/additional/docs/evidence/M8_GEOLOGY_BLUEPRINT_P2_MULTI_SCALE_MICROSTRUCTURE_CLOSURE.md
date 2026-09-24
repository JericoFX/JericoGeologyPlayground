# M8 Geology Blueprint P2 Multi-Scale Microstructure Closure

**Slice:** `M8-GEOLOGY-BLUEPRINT-P2-MULTI-SCALE-MICROSTRUCTURE-SYNTHESIS`

**State:** `PASS`

**Selected boundary:** `BOUNDED_TWO_SCALE_PORE_SYNTHESIS`

**Persistent Goal:** `ACTIVE until full JericoEngine 1.0 closure`

## Result

Jerico now admits one schema-27 cumulative geology layer, kind 29, after the
closed schema-26 mineral-dissolution stage. It evaluates exactly two independent
bounded pore distributions (fine and mesoscopic), carves their union into the
existing signed-distance field, raises only the existing packed cavity response
where geometry is removed, and reuses the existing auxiliary float buffer for a
separately identified pore-distance inspection field.

This is a finite authoring operator, not a general porous-material system. It
does not add an unlimited scale hierarchy, transport, recrystallization,
multiphase chemistry, distributed simulation, machine learning, OpenVDB, a new
runtime format, a second field/mesh/cooker path, runtime destruction, remesh,
retopology or save state. Preview, pure diff, cancellation and canonical recipe
acceptance leave the designer in control.

## Research-first provenance and independent design

The research record is
`M8_GEOLOGY_BLUEPRINT_P2_MULTI_SCALE_MICROSTRUCTURE_RESEARCH.md`. It studies the
primary paper and project materials for Venu, Bosak and Padrón-Griffe,
*Procedural Multiscale Geometry Modeling using Implicit Surfaces* (Computer
Graphics Forum 44(7), 2025; DOI `10.1111/cgf.70233`), plus only primary sources
needed to bound the problem. Jerico retained the problem-level ideas of explicit
scale separation, implicit evaluation and spatial distribution, then discarded
the reference implementation and designed independently for the existing typed
recipe and Diligent/Vulkan field path.

The frozen contract is
`docs/superpowers/specs/2026-08-29-m8-bounded-two-scale-pore-synthesis-design.md`.
No reference code, shader, data structure or asset was copied or ported.

## Observable RED chronology

The CPU contract was written first and the focused Release build failed with
MSVC C1083 because `GeologyMultiScaleMicrostructure.hpp` did not exist. The
schema/order/hash, validation, pure diff, fixed-neighborhood reference and work
admission were then implemented until `JericoGeologyRecipeTests` passed.

The Vulkan test was also written before backend support. It failed because the
public result lacked microstructure work/byte diagnostics, a dedicated field
hash and pore-distance samples. The minimal backend/shader implementation then
made cancellation, CPU/GPU parity, deterministic replay, seed sensitivity,
scalar/cavity change, prior-stage preservation, cook and runtime consumption
green. A later evidence-only compile error exposed a test-local helper scope and
was corrected without changing production behavior.

## Frozen exactly-two-scale contract

Schema 27 owns exactly six binary32-exact payload values:

```text
fine cell width     4..8 voxels
fine radius         1..2 voxels
fine density        1/256..1
meso cell width     8..32 voxels
meso radius         2..8 voxels
meso density        1/256..1
fine cell/radius < meso cell/radius
radius <= cell width / 2 for each scale
```

Each scale queries exactly the eight cells touching the sample's containing
cell corner lattice. Full-width recipe seed, an independent scale domain and
signed cell coordinates produce deterministic candidate centers in the inner
half of each cell. Density rejects candidates; a finite positive sentinel owns
rejected or empty cells. There is no expanding nearest-neighbor search.

For one admitted voxel:

```text
fine = minimum signed sphere distance over 8 fine candidates
meso = minimum signed sphere distance over 8 meso candidates
pore = min(fine, meso)
carved = max(host, -pore)
weight = saturate(strength * mask)
output = lerp(host, carved, weight)
```

When `carved > host`, the existing packed cavity byte becomes the maximum of its
prior value and `weight`; every other classification byte is preserved. The
shared auxiliary float stores `pore` only after the schema-26 field has already
been inspected and hashed. Surface Nets, collision, GEOL classification,
materials, cook and static runtime remain the established consumers.

## Finite admission, identity and storage

```text
maximum active voxels             4,194,304
candidate evaluations per voxel   16
maximum candidate evaluations     67,108,864
maximum shared auxiliary bytes    16,777,216
additional bytes                  0
dispatch count                    1
iteration count                   0
```

Admission checks payload, finite ordered bounds, active-voxel count and signed
cell-coordinate safety before allocation or dispatch. Cancellation is checked
before preparation, dispatch and publication; invalid, cancelled or failed work
publishes no result. Canonical schema-27 serialization covers kind, payload,
strength, mask, active state and full-width seed. Equal inputs replay exactly;
scale, density or seed changes alter recipe and produced-field identity.

## Implementation and vertical integration

- `GeologyMultiScaleMicrostructure.hpp` owns validation, pure diff, fixed work
  admission, stable hashing and the CPU point reference.
- The cumulative recipe admits exactly one active kind-29 layer in final order.
- Existing layer constants and scalar ping-pong resources carry the operation;
  no descriptor, buffer or global device limit was added.
- The Diligent/Vulkan shader implements the same 16 fixed candidate evaluations
  and publishes a dedicated microstructure-field hash plus nine CPU/GPU samples.
- The existing packed classification retains all channels except monotonic
  cavity response where carving occurs.
- Existing Surface Nets, JRMS/JRCL cook, GEOL material resolution and StaticPbr
  replay consume the output without a parallel route.

## Release verification

All focused Release builds and executables returned zero on Diligent/Vulkan with
an NVIDIA GeForce RTX 3090. The consumed witness is:

```text
recipe_hash=2590024913169852556
source_field_hash=18089867140996499955
field_hash=2282109602832013339
surface_hash=2282109602832013339
microstructure_field_hash=889530289141831535
work_items=147456
candidate_evaluations=2359296
shared_auxiliary_bytes=589824
additional_buffer_bytes=0
mesh_bytes=533954
collision_bytes=244398
cpu_gpu_inspection_samples=9
```

The cooked SHA-256 identities are
`310f42bd74ab1de0788a1a17d09a3f8ca0b50db6165968a8b5ce1d61b5f11f2f`
for `.rmesh` and
`751b71bbf4454c5d61a4f299aeb830b2084ca95b04d4a629548651c9d55cb822`
for `.rcoll`.

The established source-free StaticPbr replay consumed 6,569 vertices and 41,328
indices, allocated 533,176 geometry-device bytes, reported one-frame GPU time
`0.069632 ms`, zero fallback textures and no Diligent error/fatal. Its three
distinct 960x720 captures are retained beside the cooked products. Visual
inspection confirms a connected host silhouette, visibly distinct fine and
mesoscopic cavities, and a non-empty classified material response under
IBL/shadow; this is original-resolution Beauty, not a parser/debug-only proof.

Exact commands, hashes and metrics live in
`data/m8-geology-blueprint-p2-multi-scale-microstructure-release.txt`.

## End-to-end system completeness and data lineage

```text
typed two-scale controls + strength/mask/full-width seed
-> canonical schema-27 recipe and identity
-> checked finite CPU/Vulkan evaluation
-> existing scalar ping-pong + pore auxiliary + packed cavity classification
-> Surface Nets + collision + GEOL material
-> deterministic JRMS/JRCL cook
-> source-free StaticPbr replay and original-resolution Beauty
```

| Category | Closure result |
|---|---|
| Source/input and authoring | `PASS`: six typed scale/distribution controls, seed, strength, mask, pure diff, preview/cancel and explicit designer acceptance. Dedicated UI remains deferred. |
| Asset pipeline/serialization | `PASS`: canonical schema-27 round-trip/hash and deterministic JRMS/JRCL products with recipe provenance. |
| CPU/GPU processing | `PASS`: fixed 16-candidate reference, checked budgets, one dispatch, zero iteration, nine CPU/GPU witnesses. |
| Runtime representation/ECS | `PASS` for existing cooked static mesh/collision/material representation; no new runtime component or mutable field is required by this offline boundary. |
| Renderer/shaders/materials | `PASS`: compute production plus existing GEOL/StaticPbr source-free replay, no fallback/error and original-resolution Beauty. |
| Physics/collision | `PASS`: deterministic static-triangle `.rcoll`; scale-dependent physical properties remain future integration. |
| LOD/streaming | Existing static cooked product remains valid; preservation of semantic pore metadata across simplification/world cells is `INTEGRATION REQUIRED`. |
| Destruction/runtime modification/save | `INTEGRATION REQUIRED`; no mutable field, runtime carve, remesh, collision replacement or persistence was admitted. |
| Scripting/gameplay/navigation/events | `INTEGRATION REQUIRED` only if a later product requires read-only cooked cavity queries. |
| Animation/Sequencer | `NOT APPLICABLE` to this static authoring operator. |
| Particles/audio | `INTEGRATION REQUIRED` only behind later exposure/cavity events demanded by a product. |
| Debugging/profiling/failure | `PASS`: stable rejection stages, dedicated hash/samples, exact work/memory counters, cancel/no-publication and finite ceilings. |
| Testing/documentation | `PASS`: observable CPU/GPU RED, regressors, cook/collision/material/runtime/Beauty proof, research/design/closure/audit alignment. |

### INTEGRATION REQUIRED

| Subsystem | Required input/interface | Owner/dependency | Milestone |
|---|---|---|---|
| Dedicated editor UI/transaction | Typed two-scale controls, cavity/pore preview, pure diff and existing preview/cancel/apply transaction | JericoEditor geology authoring | Later Geology editor integration |
| LOD/world-cell streaming | Deliberate preservation, aggregation or discard rule for cavity semantics | Geometry LOD and World Cell owners | Production streaming |
| Physical properties | Cooked cavity/host classification to Jolt material-property contract | Geology plus Jolt integration | Production Geology/physics |
| Runtime modification/save | Stable modification identity, bounded field ownership, remesh/collision replacement and persistence | Future runtime modification owner | Post-blueprint production expansion |
| Gameplay/Luau/navigation/events/VFX/audio | Read-only cooked cavity query/event seam if demanded by a real product | Respective downstream owners | Feature-driven future slices |

All eight ordered P2/research entries are now individually closed under their
published bounded evidence. That closes the ordered P2 list; the complete
Geology blueprint is eligible only for a dedicated repeated exit audit against
all sections and appendices. This slice does not start the later production
expansion and does not complete JericoEngine 1.0.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`
