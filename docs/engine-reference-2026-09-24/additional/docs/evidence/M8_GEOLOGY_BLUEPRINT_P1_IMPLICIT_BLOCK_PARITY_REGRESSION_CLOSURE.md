# M8 Geology Blueprint P1 ImplicitBlock Parity Regression Closure

Date: 2026-08-29

Slice: `M8-GEOLOGY-BLUEPRINT-P1-IMPLICIT-BLOCK-PARITY-REGRESSION`

Local status: `PASS`

Blueprint status: unchanged `PARTIAL`; only the repeated complete-blueprint exit
audit may decide blueprint `PASS`.

## Observable result

The Release and Debug Vulkan ImplicitBlock gates now satisfy the unchanged
`deviation <= 1e-4 m` contract for all five archetypes. The accepted field and
mesh hashes, vertex counts and triangle counts remain exact. No recipe, CPU
reference, shader formula, generated field, Surface Nets topology, cook schema
or runtime semantic changed.

## Preserved RED

Before the repair, the existing Release binary exited `1` at the unchanged
parity assertion for all five archetypes:

| Archetype | Field/mesh hash | RED max m |
|---:|---:|---:|
| 1 | `1203374479842651972` | `0.480034` |
| 2 | `18421722622004146555` | `0.0695623` |
| 3 | `13594365889757201430` | `0.429167` |
| 4 | `5797452099299631219` | `0.444146` |
| 5 | `13561244679937275703` | `0.483242` |

The failing discriminating request was authored at
`(-0.9, -0.4, 0.5) m` and reported the quantized sample
`(-0.96, -0.36, 0.48) m`.

## First causal divergence

Bounded instrumentation traced the request through both stage values, the CPU
reference, the layer shader and the selected-sample readback. It demonstrated:

```text
requested x              = -0.9 m
voxel size               = 0.12 m
scaled x                 = -7.5
rounded voxel x          = -8
old Release leaf/local   = -2 / 0
old field word           = 43564
old sampled GPU world x  = -1.92 m
correct leaf/local       = -1 / 0
correct field word       = 44076
correct sampled world x  = -0.96 m
```

The first incorrect state was therefore the host selected-sample voxel split at
an exact negative 8-voxel leaf boundary. The observation path read the previous
leaf while continuing to publish the requested quantized world position. The
productive Vulkan field was never changed, which explains why field and mesh
hashes remained stable while selected CPU/GPU parity was red.

Independent localization checks showed that disabling layer-shader
optimization did not change any RED value, a one-sample inspection reproduced
the same wrong value, and a byte-for-byte readback of the uploaded leaf-origin
buffer matched the canonical host layout. Those checks were removed after the
cause was proven.

## Minimal repair

`GeologyGpuSessionDiligent.cpp` now resolves a signed voxel coordinate using an
explicit 64-bit floor division and derives the local coordinate as
`voxel - leaf * 8`. This maps every finite admitted `int32` voxel, including
exact negative multiples of eight, into one canonical leaf and local range
`0..7`. The existing ImplicitBlock test already contains the exact `-8` boundary
that reproduced the defect, so no tolerance or test semantic was changed.

## GREEN witnesses

Release and Debug both exited `0` with the same results:

| Archetype | Field/mesh hash | Vertices | Triangles | GREEN max m |
|---:|---:|---:|---:|---:|
| 1 | `1203374479842651972` | 1,508 | 3,016 | `1.41254e-07` |
| 2 | `18421722622004146555` | 1,032 | 2,064 | `1.84393e-07` |
| 3 | `13594365889757201430` | 1,482 | 2,960 | `3.91667e-08` |
| 4 | `5797452099299631219` | 1,653 | 3,304 | `1.07288e-08` |
| 5 | `13561244679937275703` | 1,540 | 3,076 | `3.98957e-08` |

The irregular seed-change witness remains
`13561244679937275703 -> 7958105428677596897` with `11,963` dirty voxels.
Repeated evaluation remains clean, and the five productive Surface Nets meshes
remain non-empty with their accepted material stream.

## Regression and runtime boundary

All commands below exited `0` after the repair:

- complete `JericoGeologyRecipeTests` CPU recipe/reference suite;
- Release and Debug `JericoGeologyGpuSessionTests --implicit-block`;
- Release `--joint-set`, `--fold-domain`, `--gradient-detail-warp`,
  `--directed-crack`, `--durability-graph`, `--rockfall` and `--colluvium`;
- representative P2 `--multi-scale-microstructure`;
- source-free `JericoStaticMeshRenderAssetTests --geology-generated` using the
  accepted cooked multi-scale microstructure `.rmesh`.

The source-free replay loaded `6,569` vertices, `41,328` indices and `52,552`
classification bytes into `533,176` geometry device bytes, preserved source
field `18089867140996499955` and final/surface
`2282109602832013339`, measured `0.070656 ms` for the StaticPbr frame and
exited without fallback, render error or fatal diagnostics.

The formerly reported Debug `0xC0000005` did not reproduce after a fresh Debug
build: the complete focused gate published all five metrics and exited `0`.

## End-to-end system completeness audit

Data lineage for this repair is:

```text
requested world sample
-> rounded voxel coordinate
-> canonical signed leaf/local split
-> deterministic linear field word
-> bounded GPU buffer copy
-> selected stage sample
-> CPU/GPU parity and debugging result
```

Applicable integrations were CPU/GPU observation, debugging, validation,
failure handling, deterministic identity, test coverage and documentation.
Source recipes, asset generation, serialization, ECS/runtime representation,
renderer shaders/materials, physics/collision, cook/package, streaming/LOD,
destruction, navigation, scripting, Sequencer, particles, audio and editor UI
were not modified. Their existing blueprint results and documented
`INTEGRATION REQUIRED` boundaries remain owned by the repeated exit audit.

No upstream semantic geology data is added, removed, compressed or
reinterpreted. Product hashes plus the P1/P2/source-free witnesses demonstrate
that the repaired observation reaches existing downstream consumers without a
new system, allocation, pass, dependency or schema.

## Scope and continuity

The 182 foreign default-status entries remain untouched with SHA-256
`2e996b1669df5ad1dd4e0cb9c3cbaf1b8bfc73ba6bc734abbad0a49419954030`.
Transport, recrystallization, VDB/UI, bespoke presets, new operators and the
post-Geology production-expansion document were not opened.

Next slice: exactly one `M8-GEOLOGY-BLUEPRINT-EXIT-AUDIT` successor repeats the
complete audit and alone decides the eventual blueprint status.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
