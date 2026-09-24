# M8 P1 FoldDomain closure

## Status

`M8-GEOLOGY-BLUEPRINT-P1-FOLD-DOMAIN`: `PASS`.

This closes only the third item of blueprint section 88. It does not complete
P1, the geology blueprint or JericoEngine 1.0.

## Result

Canonical geology schema 15 admits one bounded deterministic `FoldDomain`
layer. Its local unit fold axis, local unit displacement direction,
world-metre amplitude, cycles-per-world-metre frequency, phase, envelope,
falloff and full transform all participate in validation, exact TOML
round-trip and recipe identity.

The finite CPU reference freezes inverse-transpose axis transformation,
forward direction transformation, physical non-uniform envelope scaling and
world-unit amplitude/frequency. Productive Diligent/Vulkan kind 19 packs the
resolved frame into the existing layer constants and trilinearly samples the
resident negative-inside source at the displaced position. Existing regional
masks, strength composition, ping-pong resources, bounded inspection, dirty
tracking, cancellation, workload/device guards and Surface Nets are reused.

No stochastic warp, generator, final preset, new field architecture, custom
mesher, proof-only geometry or later P1 operator was introduced. The ownership
audit and independent mapping are recorded in
`M8_GEOLOGY_BLUEPRINT_P1_FOLD_DOMAIN_RESEARCH.md`.

## RED contracts

The first recipe/CPU build failed because the canonical
`GeologyFoldDomain.hpp` contract did not exist. After CPU/schema GREEN, the
productive Vulkan test failed with
`procedural.geology.gpu_layer_unsupported`. These were the intended missing
behavior boundaries before their respective production changes.

Validation freezes unit axes, zero seed, positive physical values, phase and
falloff ranges, exact envelope identity, canonical transform state and the
complete derived GPU representation. Extreme valid-double inputs that would
underflow a scaled envelope or overflow angular frequency in `float` are
rejected canonically rather than reaching shader division or NaN behavior.

## Canonical proof inputs

Both proof variants use the canonical BSP source, strength `1.0`, envelope
falloff fraction `0.30` and zero seed.

| Variant | Fold axis local | Displacement local | Amplitude | Frequency | Phase | Envelope (m) | Translation / Z rotation / scale |
|---|---|---|---:|---:|---:|---|---|
| 0 | `[1, 0, 0]` | `[0, 1, 0]` | `0.42 m` | `0.38 cycles/m` | `0.125` | `[1.7, 1.25, 0.9]` | `[-0.10, 0.05, 0]` / `15 degrees` / `[1, 0.95, 1.05]` |
| 1 | `[0, 1, 0]` | `[1, 0, 0]` | `0.33 m` | `0.52 cycles/m` | `0.35` | `[1.45, 1.35, 0.85]` | `[-0.10, 0.05, 0]` / `-22.5 degrees` / `[0.9, 1.1, 1]` |

These are generic structural witnesses, not named final rock or cliff presets.

## Productive Vulkan evidence

Observed with Diligent/Vulkan on NVIDIA GeForce RTX 3090. Bounded CPU/GPU
parity is measured in metres.

| Variant | Recipe hash | Field / mesh hash | Vertices | Triangles | Maximum parity deviation | Device bytes |
|---|---:|---:|---:|---:|---:|---:|
| Localized X fold | `9118176348480163319` | `5386317975944435887` | 4,062 | 8,116 | `1.31884e-07 m` | 16,721,312 |
| Localized Y fold | `4109718907081042942` | `4445589947315620632` | 4,124 | 8,244 | `6.30919e-08 m` | 12,542,432 |

Both field hashes are non-zero and distinct. Immediate repeat preserves the
field hash with zero dirty leaves and voxels. Changing phase changes the field
hash from `5386317975944435887` to `2650952795681934025` with `5,321` bounded
dirty voxels. Changing non-uniform scale changes it to
`7328310748432103084` with `2,516` bounded dirty voxels. An always-cancelled
request returns `procedural.geology.gpu_cancelled`; zero strength is field-
identical to the source-only recipe. Every extracted triangle retains the BSP
document material GUID.

Visible evidence:

- `M8_GEOLOGY_BLUEPRINT_P1_FOLD_DOMAIN_VULKAN.svg`, SHA-256
  `b18a8ae9c5e32077e932c7b195135e9e5b9d829c9fb8e9d74192538167b2edb2`;
- `M8_GEOLOGY_BLUEPRINT_P1_FOLD_DOMAIN_VULKAN.png`, SHA-256
  `5564ad0d8e9fa1d3935e430b7a2066f2cd2b0a0c1a41bc21b82a6776158cb438`.

Both are generated only from the two evaluated GPU Surface Nets meshes. The
PNG is a `2000x560` rasterization of the SVG, not separate generated geometry.

## System completeness audit

The mandatory audit was performed before implementation in
`M8_GEOLOGY_SYSTEM_COMPLETENESS_AUDIT.md` and repeated for FoldDomain after
implementation.

| Category | FoldDomain disposition |
|---|---|
| source/input, canonical recipe, serialization | implemented in schema 15; exact parse/write/hash and invalid-state tests pass |
| CPU processing | finite reference, full-transform witness and exact identity behavior implemented |
| GPU processing | resident Diligent/Vulkan layer kind 19, trilinear source sampling, inspection and parity implemented |
| runtime representation, renderer, shaders | final scalar field flows through the existing Surface Nets mesh and normal render-mesh contract; no parallel representation |
| materials | existing triangle material GUID ownership is preserved and tested; geological semantic material evaluation remains `INTEGRATION REQUIRED` below |
| physics/collision | no FoldDomain-specific collision type applies; downstream cooked collision consumes the resulting mesh through the existing geometry path |
| debugging, visualization, profiling, budgets | stage inspection, hashes, dirty statistics, parity, SVG/PNG evidence and device/workload guards pass |
| failure handling, testing, documentation | canonical validation, cancellation, RED/GREEN tests, research mapping and this closure are present |
| asset pipeline, editor/authoring, ECS, animation, Luau, events, Sequencer, save/load, streaming, LOD, destruction, navigation, particles, audio, tooling UI | no new FoldDomain-specific integration applies in this scalar-operator slice; the repository-wide geology audit records the vertical gaps instead of silently treating them as complete |

Data lineage for this slice is:

```text
canonical FoldDomain parameters + BSP source
    -> resolved CPU/GPU fold frame
    -> displaced resident SDF sampling
    -> final scalar field
    -> Surface Nets positions/normals/indices + source material GUID
    -> existing mesh consumers
```

FoldDomain introduces no new persistent geological classification field. Its
canonical parameters survive serialization and identity, but FoldDomain
provenance—like other geology-layer semantics—is not carried through SDF to
mesh attributes. The retroactive system audit identifies that loss.

### INTEGRATION REQUIRED

- **Subsystem:** Geological Surface/Material integration.
- **Why:** geometry-affecting strata, fractures, erosion, sediment, cavity and
  related semantic fields currently collapse before material evaluation.
- **Required input:** canonical geology recipe/seed, world position and normal,
  appropriate retained or reconstructable geological fields, slope/curvature/
  depth and runtime exposure state where applicable.
- **Expected interface:** one Jerico-owned geological surface representation
  feeding the existing material/PBR path with world-space/triplanar mapping;
  no manual UV dependency and no parallel material architecture.
- **Owner:** the existing geology extraction plus Jerico material/renderer
  contracts, coordinated in one dedicated vertical slice.
- **Dependency:** the current system completeness audit and productive scalar/
  Surface Nets paths.
- **Milestone:** next authorized Geology slice,
  `M8-GEOLOGY-SYSTEM-GEOLOGICAL-MATERIAL-INTEGRATION`.

This required integration does not invalidate the local FoldDomain operator
PASS, but it prevents any claim that the complete geology system is finished.

## Verification

- focused schema/CPU build and executable: `PASS`;
- focused topology build/executable, including conservative FoldDomain
  amplitude expansion and zero-strength identity: `PASS`;
- focused productive Vulkan FoldDomain gate after final assertions: `PASS`,
  `75.22 s`;
- inherited Bubble Weathering, Curvature Weathering, Detail, Proof Recipes,
  ImplicitBlock and JointSet productive Vulkan gates: all `PASS`;
- inherited full GPU session: `PASS`, direct executable exit `0` in
  approximately `614 s` by process start/end observation;
- exact canonical round-trip, every payload/transform identity input,
  invalid/overflow/underflow rejection, oblique non-uniform transform,
  cancellation, dirty work, material ownership and Surface Nets assertions:
  `PASS`;
- global CMake generation remains blocked only by unrelated preserved missing
  `Qt6::Core` and `Jerico::EditorProjectAuthority` targets. Focused MSBuild
  targets compile and link successfully.

The effective current matrix is `10/10 PASS`: recipe, topology, full inherited
GPU session, Bubble Weathering, Curvature Weathering, Detail, Proof Recipes,
ImplicitBlock, JointSet and FoldDomain.

## Boundary

The slice did not implement gradient-based implicit detail warp,
`DirectedCrack`, durability, rockfall, colluvium, imported mesh structural
operators, later P1/P2/M9/Task23, VDB/UI, final presets or the geological
material integration itself.

It did not start
`docs/JERICO_1_0_PRODUCTION_EXPANSION_GEOLOGY_PWAS_EPLS_ANIMATION_DESTRUCTION.md`.
That later expansion remains forbidden until the complete Geology blueprint is
actually finished, published and closed and its user-ordered read-only audit
begins.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
