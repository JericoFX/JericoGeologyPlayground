# M8 Geology Blueprint P2 — Bounded Aperiodic-Tiling Source Closure

**Date:** 2026-08-29
**Slice:** `M8-GEOLOGY-BLUEPRINT-P2-APERIODIC-TILING`
**State:** `PASS`
**Selected boundary:** `BOUNDED_DETERMINISTIC_CORNER_LATTICE_SOURCE`

## Result

Jerico now admits one cumulative schema-22 `AperiodicTiling` source whose exact
diagnostic identity is `BoundedDeterministicCornerLattice`. It constructs a
finite colored-corner lattice from the existing layer seed, derives one
strictly cell-confined additive box per tile, and sends those boxes through the
existing guarded Vulkan scalar/classification field, Surface Nets, JRMS/JRCL
cook, package and static PBR runtime.

This is intentionally a bounded deterministic stochastic construction. It does
not claim a theorem-backed infinite aperiodic set. The primary-source boundary,
terminology limits and independent Jerico design precede the implementation in:

- `M8_GEOLOGY_BLUEPRINT_P2_APERIODIC_TILING_RESEARCH.md`;
- `docs/superpowers/specs/2026-08-29-m8-bounded-aperiodic-tiling-design.md`;
- `docs/superpowers/plans/2026-08-29-m8-bounded-aperiodic-tiling.md`.

No reference implementation, code, shader, asset or tile library was inspected,
copied or ported.

## Frozen authored and construction boundary

The source owns only:

```text
finite positive local root half extent
+ tile counts 1..16 on each axis
+ existing full-width source-layer seed
```

Admission is capped at 4,096 tiles and 4,913 shared lattice vertices. Vertex
colors are a domain-separated full-coordinate hash in X-fastest lexicographic
order. Each cell reads its canonical eight shared corner bits; neighboring
cells therefore agree on all four bits of every common face. Opposing-face
balance and per-axis edge transitions independently map the signature to a
bounded center offset and half extent. Every published box satisfies
`abs(offset) + halfExtent <= 0.94 * cellHalfExtent`.

The implementation fails closed on non-finite or non-positive authored and
derived geometry, invalid counts, checked-arithmetic overflow, reserved-zero
identity, cancellation and insufficient work or retention. Default/hard limits
are exactly 9,009 work items and 1 MiB retained bytes; no partial preview or
geometry escapes a failure.

Source content identity covers definition plus construction/hash version. The
evaluated identity additionally covers seed, ordered colors, signatures, boxes
and exact counters. Stable brush GUIDs derive from the evaluated source content
identity plus tile coordinate and signature, rather than ordinal alone.

## Designer choice and authoring semantics

Evaluation returns one immutable preview containing colors, signatures, boxes,
work, retained bytes and content identity. The focused diff reports authored
extent/count/seed changes and changed vertices, tiles and boxes without mutating
either input. The designer accepts or rejects the canonical recipe/revision;
only the accepted recipe reaches the existing explicit GPU evaluation request.

This slice adds no dedicated UI, automatic Apply, preset, general tiling
framework, solver, optimizer, grammar or second field/cook path.

## RED chronology

The observable Vulkan vertical was written first and failed to compile because
schema 22, the source kind and evaluator did not exist. Focused recipe and CPU
RED then exposed missing schema admission/round-trip/hash and construction
contracts. Topology RED exposed missing source materialization. Vulkan routing
RED exposed the absent consumed field path. Review-derived RED subsequently
proved fail-closed `DBL_MAX`/subnormal derived geometry, source-sensitive brush
GUID identity and validator/evaluator agreement.

The implementation was added only until those contracts passed. One generic
field-inspection sample exactly on a negative leaf seam produced a readback
false negative while neighboring interior probes, host planes and generated
field/mesh remained correct. The slice did not alter the shared field or
inspection implementation: the source test inspects every generated box at a
strict interior point, still comparing all 24 GPU samples with the CPU source
distance.

## Verification

Focused Release tests:

```text
procedural.geology_recipe          PASS  0.06 s
procedural.geology_gpu_topology    PASS  0.05 s
2/2 passed                         0.16 s total
```

The clean-rebuilt real Vulkan vertical passed on an NVIDIA GeForce RTX 3090:

```text
recipe_hash=150879947720210687
source_hash=4770534469121386187
field_hash=14590439447042778467
vertices=4568
indices=27120
work_items=84
retained_bytes=1620
gpu_ns=2325728
```

The dedicated cook repeated the same identities and produced:

```text
surface_hash=14590439447042778467
mesh_bytes=365066
collision_bytes=163554
material_bytes=1864571
textures=3
source_gpu_ns=2338432
```

`JericoPackageTool` sealed exactly six entries into a 2,395,567-byte package:

```text
F22E4FF3D4A6CCE7596CF011FCBED312080BFACE22E79EAAF00BEDE22898D9BC
```

Two isolated package-only Shipping replays used only
`Packages:/aperiodic-tiling.epak`. Both reported Vulkan, the cooked pipeline,
4,568 vertices, 27,120 indices, one draw, one material, three textures,
`fallbacks=0` and `source_free=true`. Both original-resolution 1280x720 frames
were byte-identical:

```text
ADC863D4AA1564D3624F26A55ABA58293FBB8F3EAA7821F5030A5F908ACCD3E9
```

The inspected accepted frame is retained at
`docs/evidence/images/m8-geology-blueprint-p2-aperiodic-tiling/aperiodic-tiling-vulkan.bmp`.
Stable machine-readable metrics are retained in
`docs/evidence/data/m8-geology-blueprint-p2-aperiodic-tiling-release.txt`.

## Data lineage

```text
schema-22 finite definition + existing source-layer seed
-> canonical definition/source/recipe identities
-> shared lattice vertex colors
-> ordered eight-bit tile signatures
-> cell-confined additive boxes + stable brush GUIDs
-> existing guarded Vulkan scalar/classification field
-> existing classified Surface Nets
-> existing JRMS/JRCL/material/texture cook
-> six-entry source-free package
-> existing static PBR Vulkan consumer
```

Corner colors and signatures are authoring/generation identity, not geological
material classes. They remain in the immutable source preview and source hash;
runtime deliberately consumes only cooked Jerico assets. Existing geological
classification remains authoritative through extraction and cook. The source
does not discard or fabricate strata, fractures, erosion, deposition, cavities,
durability, wetness or physical properties.

## End-to-end completeness result

| Category | Result |
|---|---|
| Source/input and recipe | `PASS`: schema 22, finite extent/counts, seed, canonical round-trip/hash and validator/evaluator parity. |
| Authoring workflow | Immutable preview/diff and explicit accepted recipe `PASS`; dedicated UI/transaction wrapper/presets remain `INTEGRATION REQUIRED`. |
| CPU/GPU | Finite CPU construction, exact limits/cancel/failure and 24-point Vulkan parity `PASS`. |
| Field/mesh/cook/runtime | Existing classification, Surface Nets, exact collision, six-entry package and package-only replay `PASS`. |
| Renderer/materials | Existing PBR Vulkan path, one material, three textures and zero fallback `PASS`. |
| Physics | Exact cooked static triangles `PASS`; geological physical properties remain `INTEGRATION REQUIRED`. |
| Serialization/save-load | Static recipe/products `PASS`; mutable runtime tiling/save identity is deliberately absent. |
| Streaming/LOD | Package participation `PASS`; classified LOD/world-cell consumption remains `INTEGRATION REQUIRED`. |
| Destruction/runtime modification | `INTEGRATION REQUIRED`; no runtime tiling, remesh, retopology or collision mutation was added. |
| Gameplay/Luau/events/navigation/particles/audio | `INTEGRATION REQUIRED` behind later bounded geology query/result seams. |
| Debug/profiling/failure handling | Stable errors, identities, exact counters, GPU timing, deterministic frames and visible failures `PASS`. |
| Documentation | Research, independent design, plan, closure, metrics and repeated system audit match implementation. |

## Explicit remaining boundary

The five later ordered P2/research features remain open: contact-preserving
erosion, Dorsey narrow-band mineral weathering, moisture transport, mineral
dissolution and multi-scale microstructure synthesis. Neighborhood erosion,
dedicated editor UI/transactions, geology physical properties, classified
LOD/world cells, mutable runtime geology/destruction/save-load and navigation/
gameplay/Luau/events/particles/audio remain `INTEGRATION REQUIRED` with their
existing owners and dependencies.

Completing this bounded source does not complete P2, the Geology blueprint, the
post-Geology production expansion or JericoEngine 1.0.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`
