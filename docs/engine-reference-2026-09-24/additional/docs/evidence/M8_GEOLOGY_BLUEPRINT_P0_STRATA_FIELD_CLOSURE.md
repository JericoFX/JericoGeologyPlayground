# M8 Geology Blueprint P0 StrataField Closure

**Slice:** `M8-GEOLOGY-BLUEPRINT-P0-STRATA-FIELD`
**Status:** `PASS`
**Backend:** Diligent/Vulkan, NVIDIA GeForce RTX 3090
**Schema:** 8 (unchanged)

## Result

Jerico now evaluates one deterministic reusable structural StrataField on the
existing world-space `8x8x8` leaf lattice. The field exposes a periodic scalar
and composed mask independently from its optional narrow-band SDF response.
Future geometry, durability, material, weathering and fracture-bias stages can
therefore consume one structural meaning without this slice implementing those
later consumers.

The admitted implementation:

- normalizes the authored world axis;
- treats spacing in meters and phase in cycles;
- folds the full-width seed into the existing three-octave Jerico FBM path;
- makes zero warp an exact analytic path independent of seed/frequency;
- composes profile, strength and the existing regional mask into `[0,1]`;
- preserves source material GUIDs;
- keeps scalar-field background/sampling semantics unchanged;
- exposes only bounded selected structural samples plus a structural hash;
- tracks optional SDF response through the existing dirty voxel and `8x8x8`
  leaf path.

No VDB/OpenVDB/NanoVDB/PNanoVDB route, new field/noise/channel framework, UI,
FaultDisplace or later consumer was introduced. The implementation is Jerico
code and copies no external source.

## TDD evidence

The slice began with mathematical RED contracts. Recorded failures were:

1. the CPU test target failed because `GeologyStrataField.hpp` did not exist;
2. the periodic test exposed that absolute `phaseCycles` must not be compared
   as periodic output after a whole-cycle translation;
3. recipe validation accepted an ambiguous non-identity Strata transform;
4. the Vulkan test failed to compile because bounded inspection exposed no
   `structuralFieldHash`, `structuralScalar` or `structuralMask`.

Read-only review then found that raw squared-axis validation could overflow for
an otherwise finite extreme vector and make CPU normalization diverge from the
robust Vulkan path. Validation and the CPU reference now use `hypot`, with an
extreme-finite-axis regression test.

The minimum GREEN implementation added one CPU reference, one resident Vulkan
`float2` structural buffer, one deterministic structural hash and the bounded
inspection extensions required by those tests.

## Mathematical and runtime proof

The frozen formula and units are recorded in
`docs/evidence/M8_GEOLOGY_BLUEPRINT_P0_STRATA_FIELD_CONVENTIONS.md`.

Focused real Vulkan command:

```text
JericoGeologyGpuSessionTests.exe --strata-field-proof <evidence.svg>
```

Observed result:

```text
strata_field|field_hash=12696178019175010622|structural_hash=3141405104561149360|parity_max=1.5731e-07|orientation_dirty_voxels=21601|orientation_dirty_leaves=96|total_voxels=73728|total_leaves=144
```

`parity_max=1.5731e-07` is below the frozen `1e-4` CPU/GPU tolerance. Repeating
identical input preserved structural identity and reported zero dirty work.
Zero warp remained invariant after changing both seed and warp frequency.
Rotating the axis changed the structural identity and produced non-empty,
bounded dirty work: `21601/73728` voxels and `96/144` leaves.

Visible proof:

- `docs/evidence/images/m8-geology-blueprint-p0-strata-field/strata-field-vulkan-inspection.svg`
- three real bounded Vulkan structural-mask inspections over the same `8x8`
  voxel-center slice: horizontal zero-warp, diagonal zero-warp and diagonal
  warped;
- 64 selected samples per evaluation, never a whole-field readback.

## Verification

Focused coherent MSVC targets were built in dependency order:

```text
JericoProceduralAuthoring
JericoGpuGeologyDiligent
JericoGeologyRecipeTests
JericoGeologyGpuTopologyPlanTests
JericoGeologyGpuSessionTests
```

Focused executables:

```text
JericoGeologyRecipeTests.exe                    PASS
JericoGeologyGpuTopologyPlanTests.exe           PASS
JericoGeologyGpuSessionTests.exe --strata-field-proof ...  PASS
```

Integrated matrix:

```text
repository.cancelled_vdb_geology_absent        PASS
repository.gpu_geology_boundary                 PASS
procedural.geology_recipe                       PASS
procedural.geology_gpu_topology                 PASS
procedural.geology_gpu_session                  PASS (531.76 s)
100% tests passed, 0 tests failed out of 5
```

The generic `dev` regeneration remains blocked by the pre-existing Terrain
configuration references to absent DiligentFX targets. The Vulkan preset
regeneration also reaches the pre-existing AssetPipeline Qt reference while
the editor is disabled. Neither path belongs to this slice; the already
generated authoritative Vulkan projects were rebuilt directly and the full
five-test CTest matrix passed from that tree.

## Scope boundary

Completing StrataField does not complete geology P0, the geology blueprint or
JericoEngine 1.0. Blueprint Step 7 (`FaultDisplace`) remains the next P0 slice.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
