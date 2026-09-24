# M8 Geology Blueprint P0 CavityCarve Closure

**Slice:** `M8-GEOLOGY-BLUEPRINT-P0-CAVITY-CARVE`
**Status:** `PASS`
**Backend:** Diligent/Vulkan, NVIDIA GeForce RTX 3090
**Schema:** 10

## Result

Jerico now evaluates `CavityCarve` as a deterministic explicit spherical
subtraction over the existing sparse scalar field. The operation removes real
volume under the negative-inside convention, produces a new enclosed Surface
Nets wall, composes the existing strength/regional-mask path and preserves
material plus unrelated structural channels.

The admitted implementation:

- uses `transform.translation` as a world-metre centre and a positive
  `radiusMeters`, while rejecting non-identity rotation/scale and unused seed;
- reuses the shared sphere SDF and hard/polynomial-smooth field-CSG functions
  already used by sparse sphere stamps and brush subtraction;
- admits only the canonical `Sphere` shape in schema 10 and hashes every
  semantic recipe input;
- rejects pre-schema, non-finite, degenerate and ambiguous cavity state;
- evaluates the same hard/smooth subtraction on CPU reference and productive
  Diligent/Vulkan paths;
- permits enclosed sign changes required by real topological subtraction while
  retaining the existing protective sign guard for surface-only operators;
- reuses the compiled regional-mask path, selected-sample inspection, exact
  dirty tracking, `8x8x8` leaves and Surface Nets extraction;
- preserves source material GUIDs and exact structural scalar/mask/cell/metric
  samples.

No VDB/OpenVDB/NanoVDB/PNanoVDB route, replacement field/CSG/mask framework,
removed geology UI, `BubbleWeathering`, `CurvatureWeathering`, durability,
later P0/P1/P2/M9/Task23 work or later cavity consumer was introduced. The code
is original Jerico code and copies no external source.

## TDD and review evidence

The slice began with mathematical RED contracts before production support:

1. the recipe-test target failed to compile because the CavityCarve CPU
   reference did not exist;
2. schema tests then lacked the kind, payload, schema-10 writer/parser/hash and
   invalid-state route;
3. the focused Vulkan test reached
   `procedural.geology.gpu_layer_unsupported` because no productive pass was
   admitted;
4. the first productive pass exposed a real shared sign-guard defect: the
   post-pass guard reverted enclosed interior-to-exterior changes. Exempting the
   topological subtractor, as existing topological operators already are,
   produced the expected centre void and CPU/GPU parity;
5. READ_ONLY review found that an unused nonzero seed could change canonical
   identity. A new RED reproduced the acceptance, then validation and explicit
   NaN/Inf regressions closed the gap.

Final READ_ONLY review reported no code-level Critical, Important or Minor
finding. Its only remaining closure requirement was the durable conventions and
closure record now provided here. The review also verified the admitted RBrush
mask, exact structural preservation and focused scheduling.

## Mathematical and runtime proof

The frozen formula, units, endpoints, transform, mask, channel and identity
conventions are recorded in:

`docs/evidence/M8_GEOLOGY_BLUEPRINT_P0_CAVITY_CARVE_CONVENTIONS.md`

Focused real Vulkan command:

```text
JericoGeologyGpuSessionTests.exe --cavity-carve
```

Observed result:

```text
procedural.geology.p0_cavity_carve|backend=Diligent/Vulkan|device=NVIDIA GeForce RTX 3090|field_hash=2463036689238531135|baseline_hash=4703855310837332601|parity_max_m=7.03335e-08|translation_dirty_voxels=2159|radius_dirty_voxels=2670|smoothness_dirty_voxels=2629|mask_dirty_voxels=1925
```

The selected centre changes from negative solid to positive void, the analytic
boundary and exterior samples match the CPU reference within `1e-4 m`, and the
observed maximum deviation is `7.03335e-08 m`. The final mesh differs from the
source-only control in positions and indices; all extracted triangles retain
the source document material GUID. Identical reevaluation preserves the field
hash and reports zero dirty work. Translation, radius, smoothness and localized
mask changes each produce non-empty bounded work.

Visible proof:

- `docs/evidence/images/m8-geology-blueprint-p0-cavity-carve-vulkan.png`;
- `docs/evidence/images/m8-geology-blueprint-p0-cavity-carve-vulkan.svg`;
- portrait `900x1500` PNG plus matching vector source;
- baseline hash `4703855310837332601` versus carved hash
  `2463036689238531135`;
- `338` extracted cavity-wall triangles, position-count delta `296` and
  index-count delta `1764`;
- equal cutaway of both actual evaluated meshes reveals the enclosed void; no
  synthetic geometry, image generation, shading-only cavity or whole-field
  intermediate readback is used.

## Verification

Coherent Release targets rebuilt from the existing
`windows-vulkan-editor` tree:

```text
JericoGeologyRecipeTests       PASS
JericoGeologyGpuSessionTests   PASS
JericoSdfBrushStampTests       PASS
JericoSparseSdfBuilderTests    PASS
```

Focused executables:

```text
JericoGeologyRecipeTests.exe                    PASS
JericoSdfBrushStampTests.exe                    PASS
JericoSparseSdfBuilderTests.exe                 PASS
JericoGeologyGpuSessionTests.exe --cavity-carve PASS
JericoGeologyGpuSessionTests.exe --cavity-carve-proof <svg>
                                                  PASS
JericoGeologyGpuSessionTests.exe --fracture-cells
                                                  PASS
```

Adding the full focused CavityCarve sequence to the already long aggregate
Vulkan executable initially caused the inherited fixed `600 s` CTest gate to
time out. The timeout was not widened. The new acceptance remains in the
explicit focused command, while its duplicate invocation was removed from the
aggregate default route. The final inherited five-gate matrix then passed:

```text
repository.cancelled_vdb_geology_absent  PASS
repository.gpu_geology_boundary          PASS
procedural.geology_recipe                PASS
procedural.geology_gpu_topology          PASS
procedural.geology_gpu_session           PASS (547.06 s)
100% tests passed, 0 tests failed out of 5
Total Test time (real) = 547.48 sec
```

## Scope boundary

Completing CavityCarve does not complete geology P0, the geology blueprint or
JericoEngine 1.0. Blueprint Step 11 (`BubbleWeathering`) remains the next P0
slice and is not implemented here. Spline-driven `CaveSweep`, square/box cavity
shapes and other swept/primitive extensions also remain outside this slice.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
