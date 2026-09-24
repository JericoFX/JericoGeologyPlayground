# M8 Geology Blueprint P1 — Directed Crack Swept Volume Closure

**Slice:** `M8-GEOLOGY-BLUEPRINT-P1-DIRECTED-CRACK-SWEPT-VOLUME`

**Status:** `PASS`

**Date:** 2026-08-28

Completing this operator does not complete P1, the Geology blueprint, the later
production expansion or JericoEngine 1.0.

## Result

Jerico schema 17 kind 21 now provides one typed `DirectedCrack` layer over the
existing canonical geology recipe, spline storage, negative-inside field,
resident Vulkan evaluator and classified Surface Nets product. The operator:

```text
canonical ProceduralSpline branch graph
-> deterministic activation / propagation clipping / arc-length resampling
-> implicit-surface closest-point projection and planar chart
-> explicit width-depth profile
-> bounded swept carving volume
-> max(source, -volume) subtraction
-> existing classified Surface Nets / cook / PBR path
```

No reference implementation, parallel graph/SDF/CSG/mesher/material system,
bespoke generator or preset was copied or introduced. Research provenance and
the independent design are recorded in:

- `M8_GEOLOGY_BLUEPRINT_P1_DIRECTED_CRACK_RESEARCH.md`;
- `../superpowers/specs/2026-08-28-m8-directed-crack-swept-volume-design.md`;
- `../superpowers/plans/2026-08-28-m8-directed-crack-swept-volume.md`.

## Frozen contract

- maximum 32 branches, 128 prepared segments and 8 profile keys;
- maximum 268,435,456 admitted active-voxel/segment distance tests;
- positive uniform layer scale and `ImplicitSurfacePlanarChart` projection;
- deterministic seed activation and branch propagation;
- bounded resampling with cancellation inside arc-table and sample loops;
- exact rejection of degenerate, crossing, T-junction and overlapping graph
  configurations admitted by the independent Jerico design;
- exact zero/disabled identity and structural-classification pass-through;
- centered one-voxel gradient CPU/GPU parity;
- subtractive negative-inside composition `max(source, -volume)`.

## RED chronology

The initial productive Vulkan gate returned the expected unsupported-layer error
before kind 21 existed. During implementation, placing the segment traversal in
the monolithic layer shader exposed a driver pipeline-creation stall; systematic
isolation proved the cause. The final implementation uses a dedicated compute
entry point inside the same `GeologyGpuSession`, sharing its field buffers,
classification buffers, binding rules, cancellation and dirty tracking. This is
not a parallel geology system.

The topology gate then produced an observable measurement RED because its first
depth window included the block's opposite face. Restricting the measurement to
the admitted upper crack band corrected the test without changing the operator or
fixture.

## Productive Vulkan proof

Debug and Release on the NVIDIA GeForce RTX 3090 produced identical identities:

```text
source_field_hash=13542752315804488866
crack_field_hash=13887919847164091
mesh_hash=13887919847164091
segments=27
dirty_voxels=1052
parity_max_m=2.83122e-09
opening_width_m=0.289553
depth_m=0.415148
```

The result is deterministic on repeat, changes fewer than all active voxels,
preserves the source structural field hash and returns the exact source field
when disabled. Cancellation is observed both during graph preparation and the GPU
session.

## Topology and cook

The admitted proof is the existing BSP box transformed by typed recipe data; it
is not a dedicated proof mesh. The cracked product proves:

- literal opening width and depth bands;
- one connected solid surface component;
- exact two-incidence closed manifold edges;
- a local 64-sample exterior flood fill with no disconnected empty component;
- no disconnected solid fragment;
- exact classified JRMS 1.1 reread;
- exact triangle collision indices and vertex positions;
- byte-identical geology classifications after cook/reread.

The cracked JRMS contains:

```text
vertices=5588
indices=33516
geologyVertices=5588
sourceHash=0xEE87C10F2BF50246
recipeHash=0x2E182D0053156677
vertexHash=0x42C88D60C2745F8E
indexHash=0xE6B3CC7F04A175E1
contentHash=0xC7564F7CCDD77A86
sha256=D36D36D4141E7AF5F633BD4935FF6B1F902A57821A225737FD927B3B80E459C6
```

## Source-free package and Beauty

The authoritative cracked package has eight entries, 12,104,492 bytes and
SHA-256 `AA500E2B0B0E883F8A6C38C42D0E99CE8F82E73DDFD3026ADFEB87D17220D161`.
The exact comparison package also has eight entries and SHA-256
`21CECA65D110D197A7CE2F7F537CABBEDCDF77EF1136B569FC44760F0CA88655`.
Both pass:

```text
asset_dump.shipping_valid|entries=8|no_abr=true|no_brush_source_images=true|no_editor_rbrush=true|no_editorassets=true
```

Package-only Release Vulkan replay reports one draw, four materials, three
textures, zero fallbacks, `source_free=true` and `gpu_available=true`. Two
consecutive crack captures are byte-identical with SHA-256
`AC2E7EAF8D3844A0AC410CD3A95D6FF40206D67C3AD389A00A09AA212FEB02CE`.
The exact control capture has SHA-256
`1C824E4FF2737EB4EF68635158C6510FFD08A705C02EC1235B6F7035A228D8B5`.

Visual inspection at the original 960x720 resolution confirms the same block,
camera, light and triplanar rock material in both captures. The cracked result
contains a bounded open mouth, visible inner wall and dark void without a
disconnected hole or material fallback.

| Capture | SHA-256 |
|---|---|
| `images/m8-geology-blueprint-p1-directed-crack/directed-crack-source-shipping-beauty-960x720.bmp` | `1C824E4FF2737EB4EF68635158C6510FFD08A705C02EC1235B6F7035A228D8B5` |
| `images/m8-geology-blueprint-p1-directed-crack/directed-crack-shipping-beauty-960x720.bmp` | `AC2E7EAF8D3844A0AC410CD3A95D6FF40206D67C3AD389A00A09AA212FEB02CE` |

## Verification

- schema round-trip, canonical hashing and rejection contracts: `PASS`;
- CPU reference and bounded graph preparation: `PASS`;
- Debug and Release productive Diligent/Vulkan evaluation: `PASS`;
- deterministic repeat, disabled identity, dirty work and cancellation: `PASS`;
- width/depth, manifold, connected solid and exterior flood-fill gates: `PASS`;
- classified cook, JRMS reread and exact triangle collision: `PASS`;
- two eight-entry source-free Shipping packages: `PASS`;
- paired original-resolution package-only Beauty and repeat hash: `PASS`;
- focused `git diff --check`: `PASS`.

Global CMake regeneration remains blocked outside this slice by the preserved
test-scheduling inventory mismatch and the previously observed missing global
`Qt6::Core` / `Jerico::EditorProjectAuthority` targets. Focused configured
Debug/Release MSBuild compilation, link and execution pass.

## End-to-end boundary

The operator preserves all pre-existing surface classification instead of
inventing crack-only runtime semantics. A distinct “fresh crack surface” channel
could benefit material response, particles, audio, gameplay and destruction, but
adding it would change shared semantic lineage and is therefore explicitly:

```text
INTEGRATION REQUIRED
subsystem: geological semantic lineage and runtime consumers
input: prepared crack graph plus newly exposed Surface Nets faces
expected interface: stable classified surface semantic carried through GEOL
owner: future authorized Geology/material/runtime integration slice
dependency: shared classification schema and consumer contracts
milestone: after the applicable blueprint operator and consumer slices are authorized
```

Physical properties, classified LOD/streaming, runtime remesh/destruction and
save/load, navigation/gameplay/Luau/events, particles, audio and editor semantic
visualization remain `INTEGRATION REQUIRED` under the existing system audit.
Durability graph, rockfall, colluvium and imported-mesh structural operators are
still unimplemented P1 work.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
