# M8 P1 ImplicitBlock archetypes closure

## Status

`M8-GEOLOGY-BLUEPRINT-P1-IMPLICIT-BLOCK-ARCHETYPES`: `PASS`.

This closes only the first item of blueprint section 88. It does not complete
P1, the geology blueprint or JericoEngine 1.0.

## Result

Canonical geology schema 13 admits one reusable `ImplicitBlock` layer with
Tabular, Columnar, Equidimensional, Rhombohedral and IrregularPolyhedral
definitions. The contract serializes and hashes the archetype, metre extents,
edge smoothing, prism side count, obliqueness, irregularity, full layer
transform and the seed where it is semantically active.

The CPU reference and productive Diligent/Vulkan shader use finite
negative-inside scalar fields and intersect them with the existing resident
field. Existing strength, regional masks, ping-pong buffers, bounded
inspection, dirty tracking, cancellation, workload/device guards and Surface
Nets are reused. No block-specific mesh generator, named final preset, new
field/mesher or later P1 operator was introduced.

Research provenance and the independent Jerico mapping are recorded in
`M8_GEOLOGY_BLUEPRINT_P1_IMPLICIT_BLOCK_RESEARCH.md`.

## RED contracts

The first recipe/CPU build failed because the canonical
`GeologyImplicitBlock.hpp` contract did not exist. After CPU/schema GREEN, the
productive Vulkan test failed for all five cases with
`procedural.geology.gpu_layer_unsupported`. These were the intended missing
behavior boundaries before their respective production changes.

## Canonical proof inputs

All proof cases use the same canonical BSP document, source recipe, layer
strength `1.0`, translation `[-0.15, 0.05, 0.0]`, quaternion
`[0.0, 0.0, 0.25881904510252074, 0.9659258262890683]`, scale
`[1.0, 0.9, 1.1]` and edge smoothing `0.06 m`.

| Archetype | Half extents (m) | Active structural input | Seed |
|---|---:|---:|---:|
| Tabular | `[1.4, 0.30, 0.9]` | canonical short local Y | `0` |
| Columnar | `[0.55, 1.2, 0.55]` | `radialSideCount=7` | `0` |
| Equidimensional | `[0.85, 0.85, 0.85]` | bounded aspect ratio | `0` |
| Rhombohedral | `[0.85, 0.85, 0.85]` | `obliqueness=0.30` | `0` |
| IrregularPolyhedral | `[0.85, 0.85, 0.85]` | `irregularity=0.24` | `0x1020304050607080` |

Inactive structural parameters are required to remain canonical neutral values.

## Productive Vulkan evidence

Observed on `Diligent/Vulkan`, NVIDIA GeForce RTX 3090. Every proof required
`10,035,104` device bytes, below the session's allowed device budget. Bounded
CPU/GPU parity is measured in metres.

| Archetype | Recipe hash | Field / mesh hash | Vertices | Triangles | Maximum parity deviation (m) |
|---|---:|---:|---:|---:|---:|
| Tabular | `1553182577270673992` | `1203374479842651972` | 1508 | 3016 | `1.41254e-07` |
| Columnar | `7526147775144184657` | `18421722622004146555` | 1032 | 2064 | `1.84393e-07` |
| Equidimensional | `17599341297033135598` | `13594365889757201430` | 1482 | 2960 | `3.91667e-08` |
| Rhombohedral | `6399341029778058959` | `5797452099299631219` | 1653 | 3304 | `1.07288e-08` |
| IrregularPolyhedral | `3757966610032993958` | `13561244679937275703` | 1540 | 3076 | `3.98957e-08` |

All five hashes are non-zero and pairwise distinct. Immediate reevaluation
preserved each field hash with zero dirty leaves and zero dirty voxels. Changing
only the irregular seed changed field hash from `13561244679937275703` to
`7958105428677596897` and reported `11,963` dirty voxels. An always-cancelled
request returned `procedural.geology.gpu_cancelled`. Every extracted triangle
retained the canonical BSP document material GUID.

Visible evidence:

- `M8_GEOLOGY_BLUEPRINT_P1_IMPLICIT_BLOCK_VULKAN.svg`, SHA-256
  `25f942fffdc481aa2e1d0b7c860a066339b95b8f8ab2473d267ba9e4464e1573`;
- `images/m8-geology-blueprint-p1-implicit-block-vulkan.png`, SHA-256
  `46752e78ac78fb01493b776fe4024319bfaee068231936b8e56bcbe10b8234bb`.

The SVG is rendered only from the five evaluated GPU Surface Nets meshes. The
PNG is a rasterization of that SVG, not a separate generated image.

## Verification

- focused schema/CPU gate: `procedural.geology_recipe`, `PASS`;
- focused productive Vulkan ImplicitBlock gate: `PASS`;
- inherited matrix: `7/7 PASS`, `899.79 s` total;
  - recipe `0.12 s`;
  - topology `0.08 s`;
  - full GPU session `598.83 s`;
  - Bubble Weathering `75.12 s`;
  - Curvature Weathering `75.20 s`;
  - Detail `75.26 s`;
  - Proof Recipes `75.14 s`;
- final focused build: `PASS`;
- exact canonical round-trip, invalid-state rejection, parameter/seed/transform
  identity, finite CPU behavior, bounded inspection, cancellation, dirty work,
  material ownership and Surface Nets assertions: `PASS`.

## Boundary

The slice did not implement JointSet, FoldDomain, gradient-based implicit
detail warp, DirectedCrack, durability, rockfall, colluvium, imported mesh
structural operators, P2 research, OpenVDB, editor UI or a final cliff system.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
