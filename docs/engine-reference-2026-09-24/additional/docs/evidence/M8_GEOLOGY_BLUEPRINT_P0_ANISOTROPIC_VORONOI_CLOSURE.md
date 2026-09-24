# M8 Geology Blueprint P0 AnisotropicVoronoi Closure

**Slice:** `M8-GEOLOGY-BLUEPRINT-P0-ANISOTROPIC-VORONOI`
**Status:** `PASS`
**Backend:** Diligent/Vulkan, NVIDIA GeForce RTX 3090
**Schema:** 8 (unchanged)

## Result

Jerico now evaluates `AnisotropicVoronoiCells` as a reusable deterministic
structural primitive. An explicit ordered set of world-space sites owns local
frames, positive ellipsoidal metric weights and positive relative scales. The
result publishes stable cell IDs, nearest metric, second metric, boundary
metric and separately composed structural mask without changing the scalar SDF
or source materials.

The admitted implementation:

- accepts `2..64` explicit distinct world-space sites with no hidden density
  generator or replacement lattice;
- folds both halves of the 64-bit layer seed into stable IDs `1..N`, reserving
  ID `0` for background;
- evaluates `(alpha*x^2 + beta*y^2 + gamma*z^2) / g` in each normalized local
  frame;
- resolves exact ownership ties by lower stable cell ID;
- publishes ordered nearest/second metrics and `boundary = second - nearest`;
- exposes a finite zero-mask background rather than infinity or NaN;
- uses one dedicated 16-byte Vulkan structural value plus the existing
  separately composed strength/regional-mask weight;
- preserves scalar field identity, extracted source material GUIDs and the
  existing StrataField/FaultDisplace route;
- exposes only bounded selected structural samples and stage hashes;
- compares cell/metric/mask bits through exact voxel dirtiness and existing
  `8x8x8` leaf aggregation.

No VDB/OpenVDB/NanoVDB/PNanoVDB route, replacement field/lattice/noise/channel
framework, removed UI, `FractureCells`, `CavityCarve`, weathering consumer or
later P0/P1/P2/M9/Task23 work was introduced. The implementation is original
Jerico code and copies no external source.

## TDD evidence

The slice began with fixed-site mathematical RED contracts. The authentic
failures were:

1. the CPU recipe-test target could not find the not-yet-created
   `GeologyAnisotropicVoronoi.hpp` reference;
2. schema tests then failed to compile because the layer kind and typed payload
   did not exist;
3. the Vulkan test failed to compile because bounded inspection exposed none of
   `structuralCellId`, nearest, second or boundary metric.

The resulting GREEN contracts prove analytically expected ownership, lower-ID
tie resolution, anisotropic-axis effect, positive relative scale, rotated local
frames, full-width seed folding and determinism, `nearest <= second`,
`0 <= boundary <= second`, finite background and separate strength/mask
composition. Schema tests cover canonical round-trip/hash identity, every
semantic field, schema 1–7 rejection, count bounds, non-finite/degenerate
frames, non-positive weights/scales, float-domain preservation, duplicate
packed sites and non-identity layer transforms.

Read-only review then exposed an additional RED: individually representable
`FLT_MAX` weights and `FLT_MIN` relative scale made the represented ratio
overflow, so the shader could publish cell ID `0` with a positive mask and NaN
boundary while the CPU double reference selected a real site. The regression
now proves canonical finite saturation and lower-ID ownership for a direct CPU
reference, while schema validation rejects non-finite/non-normal represented
ratios. Productive Vulkan evaluation also rejects a site/topology combination
whose conservative metric bound exceeds the finite structural domain; the
shader retains finite saturation as defense in depth.

The first runtime fixture used a Bounds mask, which the published Vulkan route
does not admit. The fixture was corrected to the already-published RBrush local
field contract; production did not gain another mask implementation.

## Mathematical and runtime proof

The frozen formula, units, tie, background, packing and dirty contracts are
recorded in
`docs/evidence/M8_GEOLOGY_BLUEPRINT_P0_ANISOTROPIC_VORONOI_CONVENTIONS.md`.

Focused real Vulkan command:

```text
JericoGeologyGpuSessionTests.exe --anisotropic-voronoi-proof docs/evidence/M8_GEOLOGY_BLUEPRINT_P0_ANISOTROPIC_VORONOI_VULKAN.svg
```

Observed result:

```text
anisotropic_voronoi|backend=Diligent/Vulkan|device=NVIDIA GeForce RTX 3090|field_hash=14199050555395717152|structural_hash=1887424290186436780|metric_parity_max_m2=7.53021e-07|dirty_voxels=729|dirty_leaves=8|total_leaves=64
```

The bounded CPU/GPU metric deviation is below the frozen `5e-5 * (1+|d2|)`
tolerance. Every selected cell ID matched exactly. Identical reevaluation
preserved scalar and structural hashes and reported zero dirty voxels/leaves.
Changing one x-axis weight from `4` to `2` preserved the scalar hash, changed
the structural hash and produced non-empty bounded work: `729` voxels and
`8/64` leaves. All extracted triangles retained the source document material
GUID.

Visible proof:

- `docs/evidence/M8_GEOLOGY_BLUEPRINT_P0_ANISOTROPIC_VORONOI_VULKAN.svg`;
- one real bounded `8x8` Vulkan inspection; cell-ID hue shows ownership and pale
  values approach the curved anisotropic boundary;
- 64 selected samples, never a whole-field intermediate readback.

## Verification

Focused coherent MSVC targets were compiled and linked from the existing
`windows-vulkan-dev` tree:

```text
JericoProceduralAuthoring
JericoGpuGeologyDiligent
JericoGeologyRecipeTests
JericoGeologyGpuSessionTests
```

Focused executables:

```text
procedural.geology_recipe                                      PASS
JericoGeologyGpuSessionTests.exe --anisotropic-voronoi         PASS
JericoGeologyGpuSessionTests.exe --anisotropic-voronoi-proof   PASS
```

The final inherited five-gate matrix passed without widening its `600 s`
timeout:

```text
repository.cancelled_vdb_geology_absent  PASS
repository.gpu_geology_boundary          PASS
procedural.geology_recipe                PASS
procedural.geology_gpu_topology          PASS
procedural.geology_gpu_session           PASS (549.68 s)
100% tests passed, 0 tests failed out of 5
Total Test time (real) = 550.19 sec
```

The final READ_ONLY re-review marked the numeric-domain correction `READY`:
the represented ratio rejection, CPU/HLSL finite saturation, bounded topology
preflight and exact extreme-value regressions close the prior P1 without a new
high-severity finding.

## Scope boundary

Completing AnisotropicVoronoi does not complete geology P0, the geology
blueprint or JericoEngine 1.0. Blueprint Step 9 (`FractureCells`) remains the
next P0 slice and is not implemented here.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
