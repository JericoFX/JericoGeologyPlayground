# M8 Geology Blueprint P0 FractureCells Closure

**Slice:** `M8-GEOLOGY-BLUEPRINT-P0-FRACTURE-CELLS`
**Status:** `PASS`
**Backend:** Diligent/Vulkan, NVIDIA GeForce RTX 3090
**Schema:** 9

## Result

Jerico now evaluates `FractureCells` as the deterministic structural consumer
of the published `AnisotropicVoronoiCells` output. It converts the retained
anisotropic boundary metric to a metre-domain proxy, produces a composed
fracture mask and stable owner ID, and admits either a real scalar carve or a
scalar-preserving weakening output.

The admitted implementation:

- consumes the existing cell ID, nearest, second, boundary and structural-mask
  values without recomputing sites or introducing another field, lattice,
  noise or channel framework;
- derives `boundaryProxyMeters = sqrt(boundaryMetricMetersSquared)` and the
  frozen `1 - smoothstep(width, width + softness, boundaryProxy)` response;
- treats cell ID `0`, zero upstream mask and any non-finite structural input as
  explicit background with zero mask and owner ID;
- preserves the published anisotropic metrics and reuses the existing separate
  structural mask for `fractureMask`;
- exposes `fractureId` as the stable nearest-cell owner when the composed mask
  is positive and `0` otherwise;
- exposes `fractureNormalApprox` on demand as a normalized centred
  finite-difference gradient, with zero at background, exact cusps, non-finite
  stencils and gradients at or below the finite epsilon;
- admits exactly `Carve` and `Weakening`: Carve produces real void under the
  existing negative-inside SDF convention, while Weakening preserves the
  scalar field and publishes only reusable structural data;
- includes width, softness, mode, strength, mask and producer ordering in
  canonical validation, serialization and hash identity;
- preserves source material GUIDs and the existing `8x8x8` dirty-leaf route.

No VDB/OpenVDB/NanoVDB/PNanoVDB route, replacement framework, removed geology
UI, `CavityCarve`, weathering/durability consumer or later P0/P1/P2/M9/Task23
work was introduced. The implementation is original Jerico code and copies no
external source.

## TDD and review evidence

The slice began with mathematical RED contracts before production support:

1. the recipe-test target could not compile the not-yet-existing
   `GeologyFractureCells.hpp` reference;
2. schema tests then lacked the `FractureCells` kind, typed payload, schema-9
   route and canonical parser/writer/hash state;
3. the Vulkan test lacked an admitted consumer of the published anisotropic
   structural buffers and therefore could not produce the required mask,
   scalar effect or bounded dirty work.

The resulting GREEN contracts prove low crack response at cell centres, high
response at a known boundary, widening under larger width, finite monotonic
softness, deterministic IDs/background, mask composition and distinct Carve
versus Weakening scalar semantics. Schema regressions cover every semantic
input, schema 1-8 rejection, missing or already-consumed anisotropic producers,
non-positive/non-finite width and softness, represented endpoint overflow,
unknown mode and non-identity transform.

READ_ONLY review then exposed three missing proof obligations. The final code
now rejects non-finite upstream mask and all three structural metrics on CPU and
HLSL, provides the required on-demand normal approximation with cusp,
background, non-finite-neighbour and below/above-epsilon regressions, and emits
both Carve/Weakening hashes plus explicit position/index count/value deltas in
the visible proof. Final spot-check reported no Critical, Important or Minor
finding and `Ready=yes`.

## Mathematical and runtime proof

The frozen formula, units, endpoints, mode, output and normal conventions are
recorded in:

`docs/evidence/M8_GEOLOGY_BLUEPRINT_P0_FRACTURE_CELLS_CONVENTIONS.md`

Focused real Vulkan command:

```text
JericoGeologyGpuSessionTests.exe --fracture-cells-proof docs/evidence/M8_GEOLOGY_BLUEPRINT_P0_FRACTURE_CELLS_VULKAN.svg
```

Observed result:

```text
fracture_cells|backend=Diligent/Vulkan|device=NVIDIA GeForce RTX 3090|field_hash=15244624087024613006|structural_hash=18373946749623678984|scalar_parity_max_m=5.36442e-09|mask_parity_max=4.50683e-08|normal_parity_max=0|width_dirty_voxels=243|softness_dirty_voxels=162|mode_dirty_voxels=81
```

Selected Vulkan samples meet the frozen CPU reference within `2e-4 m` scalar
and `2e-5` mask tolerances; observed deviations are materially smaller.
Identical reevaluation preserves scalar and structural hashes and reports zero
dirty work. Width, softness and mode changes each produce non-empty bounded
voxel/leaf work. All extracted triangles retain the source document material
GUID.

Visible proof:

- `docs/evidence/M8_GEOLOGY_BLUEPRINT_P0_FRACTURE_CELLS_VULKAN.png`;
- `docs/evidence/M8_GEOLOGY_BLUEPRINT_P0_FRACTURE_CELLS_VULKAN.svg`;
- Carve hash `15244624087024613006` versus Weakening hash
  `14199050555395717152`;
- `2408/2184` triangles, position delta `108/1093`, index delta `672/6552`
  and `16` positive bounded mask samples;
- one real `8x8` Vulkan inspection panel and meshes extracted from evaluated
  fields, with no synthetic geometry, image generation or whole-field
  intermediate readback.

The PNG/SVG pair was published separately at
`bbdaac7e079fc594ff0aeff971a17654d1ac61db` so it is directly visible from
GitHub and mobile clients.

## Verification

Coherent Release targets were rebuilt from the existing
`windows-vulkan-editor` tree:

```text
JericoProceduralAuthoring                    PASS
JericoGeologyHeightfieldErosion              PASS
JericoGpuGeologyDiligent                     PASS
JericoGeologyRecipeTests                     PASS
JericoGeologyGpuSessionTests                 PASS
```

The heightfield library rebuild was required only to remove an inherited stale
binary/header mixture discovered during verification; no heightfield source or
contract changed in this slice. Its direct CPU/GPU gate then passed before the
official matrix was rerun.

Focused executables:

```text
JericoGeologyRecipeTests.exe                         PASS
JericoGeologyGpuSessionTests.exe --fracture-cells    PASS
JericoGeologyGpuSessionTests.exe --fracture-cells-proof
                                                       PASS
```

The final inherited five-gate matrix passed without widening its `600 s`
timeout:

```text
repository.cancelled_vdb_geology_absent  PASS
repository.gpu_geology_boundary          PASS
procedural.geology_recipe                PASS
procedural.geology_gpu_topology          PASS
procedural.geology_gpu_session           PASS (591.54 s)
100% tests passed, 0 tests failed out of 5
Total Test time (real) = 591.93 sec
```

## Scope boundary

Completing FractureCells does not complete geology P0, the geology blueprint or
JericoEngine 1.0. Blueprint Step 10 (`CavityCarve`) remains the next P0 slice
and is not implemented here.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
