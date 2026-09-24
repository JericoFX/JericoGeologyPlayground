# M8 P1 JointSet closure

## Status

`M8-GEOLOGY-BLUEPRINT-P1-JOINT-SET`: `PASS`.

This closes only the second item of blueprint section 88. It does not complete
P1, the geology blueprint or JericoEngine 1.0.

## Result

Canonical geology schema 14 admits one reusable `JointSet` layer with one to
three explicit structural plane families. The contract serializes and hashes
each local unit dominant normal, every ordered metre offset, bounded angular
and offset jitter, joint half-width, full layer transform and the seed exactly
when irregularity makes it semantically active.

The finite CPU reference resolves local planes through the layer's full
inverse-transpose transform. Productive Diligent/Vulkan receives only those
resolved world-space planes and subtracts their thin joint slabs from the
resident negative-inside source. Existing strength, regional masks, ping-pong
buffers, bounded inspection, dirty tracking, cancellation, workload/device
guards and Surface Nets are reused. No JointSet generator, preset-specific
entry point, bespoke proof mesh, new mesher or later P1 operator was introduced.

Research provenance and the independent Jerico mapping are recorded in
`M8_GEOLOGY_BLUEPRINT_P1_JOINT_SET_RESEARCH.md`.

## RED contracts

The first recipe/CPU build failed because the canonical
`GeologyJointSet.hpp` contract did not exist. After CPU/schema GREEN, both
productive Vulkan proof variants failed with
`procedural.geology.gpu_layer_unsupported`. These were the intended missing
behavior boundaries before their respective production changes.

Validation additionally freezes the `1..3` family, `2..16` planes-per-family
and `32` total-plane bounds, strict offset order, unit normals, angular jitter
in `[0, 15]` degrees, separation-safe half-width/offset jitter and canonical
seed inactive state.

## Canonical proof inputs

Both proof cases use the same canonical BSP document, source recipe, layer
strength `1.0`, joint half-width `0.08 m`, translation `[-0.10, 0.05, 0.0]`,
quaternion `[0.0, 0.0, 0.13052619222005157, 0.9914448613738104]` and scale
`[1.0, 0.95, 1.05]`.

| Variant | Family | Dominant local normal | Plane offsets (m) | Angular jitter | Offset jitter (m) | Seed |
|---|---|---|---|---:|---:|---:|
| 0 | A | `[0, 1, 0]` | `[-1.0, -0.3, 0.4, 1.1]` | `4°` | `0.03` | `0x1020304050607080` |
| 0 | B | `[1, 0, 0]` | `[-1.4, -0.4, 0.6, 1.6]` | `3°` | `0.02` | same layer seed |
| 1 | A | `[0, 1, 0]` | `[-1.0, -0.3, 0.4, 1.1]` | `4°` | `0.03` | `0x1020304050607081` |
| 1 | B | `[0.7071067811865475, 0, 0.7071067811865476]` | `[-1.2, -0.4, 0.4, 1.2]` | `5°` | `0.025` | same layer seed |
| 1 | C | `[0, 0, 1]` | `[-0.75, 0.0, 0.75]` | `2°` | `0.02` | same layer seed |

These are generic structural compositions, not named final cliff or rock
presets.

## Productive Vulkan evidence

Observed on `Diligent/Vulkan`, NVIDIA GeForce RTX 3090. Every proof required
`10,035,104` device bytes, below the session's allowed device budget. Bounded
CPU/GPU parity is measured in metres.

| Variant | Recipe hash | Field / mesh hash | Vertices | Triangles | Maximum parity deviation (m) |
|---|---:|---:|---:|---:|---:|
| Two families | `908055519451103148` | `16163176516410499243` | 10,004 | 19,936 | `6.82302e-08` |
| Three families | `16823238850938977866` | `4802703339955210947` | 11,262 | 22,564 | `1.44368e-08` |

Both hashes are non-zero and distinct. Immediate reevaluation preserved each
field hash with zero dirty leaves and zero dirty voxels. Changing only the
layer seed changed field hash from `16163176516410499243` to
`2723190147181742338` and reported `11,594` bounded dirty voxels. An
always-cancelled request returned `procedural.geology.gpu_cancelled`. Every
extracted triangle retained the canonical BSP document material GUID.

Visible evidence:

- `M8_GEOLOGY_BLUEPRINT_P1_JOINT_SET_VULKAN.svg`, SHA-256
  `4f27eec83010d03e74f9040abba77a2c6e1653cda33abae85eac4e5d33d5e055`;
- `M8_GEOLOGY_BLUEPRINT_P1_JOINT_SET_VULKAN.png`, SHA-256
  `6baee781caa904bb488e9f97b3f91c21d2c7c866e07f58a9efdcfbd16420ee5f`.

The SVG is rendered only from the two evaluated GPU Surface Nets meshes. The
PNG is a `2000x560` rasterization of that SVG, not a separate generated image.

## Verification

- focused schema/CPU build and executable: `PASS`;
- focused productive Vulkan JointSet gate after final assertions: `PASS`,
  `98.22 s`;
- inherited gates in the first nine-gate matrix: recipe `0.14 s`, topology
  `0.08 s`, Bubble Weathering `75.19 s`, Curvature Weathering `73.88 s`, Detail
  `73.69 s`, Proof Recipes `73.74 s`, ImplicitBlock `73.65 s` and JointSet
  `74.56 s`, all `PASS`;
- the inherited full GPU session was initially cut only by its old `600 s`
  CTest timeout. The exact registered command then exited `0` in `674.560 s`.
  After raising only that test's timeout to `900 s`, CTest reported `PASS` in
  `763.71 s`; no assertion or production code changed for this correction;
- effective current matrix: `9/9 PASS`;
- exact canonical round-trip, invalid-state rejection, parameter/seed/transform
  identity, finite CPU behavior, bounded inspection, cancellation, dirty work,
  material ownership and real Surface Nets assertions: `PASS`;
- global CMake generation remains blocked by unrelated pre-existing missing
  `Qt6::Core` and `Jerico::EditorProjectAuthority` targets. Focused MSBuild
  targets compile and link successfully.

## Boundary

The slice did not implement `FoldDomain`, gradient-based implicit detail warp,
`DirectedCrack`, durability, rockfall, colluvium, imported mesh structural
operators, P2/M9/Task23, OpenVDB, editor UI or a final cliff system.

The later production-expansion document
`docs/JERICO_1_0_PRODUCTION_EXPANSION_GEOLOGY_PWAS_EPLS_ANIMATION_DESTRUCTION.md`
must not be started until the complete Geology blueprint is actually finished,
published and closed. At that later boundary, work begins with the required
read-only repository audit; this JointSet slice does not authorize it.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
