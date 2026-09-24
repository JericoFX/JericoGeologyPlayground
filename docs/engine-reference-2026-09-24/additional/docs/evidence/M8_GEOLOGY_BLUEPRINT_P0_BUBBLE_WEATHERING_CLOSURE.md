# M8 Geology Blueprint P0 BubbleWeathering Closure

**Slice:** `M8-GEOLOGY-BLUEPRINT-P0-BUBBLE-WEATHERING`
**Status:** `PASS`
**Backend:** Diligent/Vulkan, NVIDIA GeForce RTX 3090
**Schema:** 11

## Result

Jerico now evaluates BubbleWeathering as bounded shape-aware erosion of the
real sparse scalar field. It classifies the authored-radius 26-neighbor Moore
shell, publishes selected-sample exposure, applies durability and the existing
strength/regional-mask weight, iterates through the existing safe scalar
ping-pong route and extracts genuinely changed Surface Nets topology.

The admitted implementation:

- extends the existing `SurfaceWeathering` kind with an explicit schema-11
  `Bubble` model while preserving schema-1–10 parsing, writing and productive
  legacy Vulkan behavior;
- hashes every semantic Bubble input and rejects ambiguous, non-finite,
  fractional/out-of-range radius, wear, durability, iteration, seed or
  transform state;
- uses a radius-one through radius-four Moore shell in voxel units and proves
  the CPU reference actually samples the authored radius;
- evaluates identical exposure and scalar equations on CPU and productive
  Diligent/Vulkan paths, including direct radius-one, radius-two and
  radius-four parity;
- performs one immutable-input/independent-output dispatch per iteration with
  the existing ping-pong fields and schedules no pass for zero iterations;
- reuses the compiled regional-mask route, cancellation, workload/device-
  memory guards, exact dirty tracking and bounded selected-sample inspection;
- binds one transient full-domain exposure field to the shared layer shader,
  accounts its bytes in preflight and performs no unexpected whole-field
  intermediate readback;
- changes actual scalar values, mesh positions and indices while preserving
  source material GUIDs and unrelated structural channels exactly.

No VDB/OpenVDB/NanoVDB/PNanoVDB route, replacement recipe/field/sampling/mask
framework, removed geology UI, `CurvatureWeathering`, persistent independent
DurabilityField, mineral chemistry, material transport, rockfall, colluvium,
sediment, P1/P2/M9/Task23 work or later consumer was introduced. No external
source was copied and no dependency/license record changed.

## TDD and review evidence

The slice began with mathematical RED contracts on small synthetic grids:

1. the recipe-test target failed because no BubbleWeathering reference or
   schema-11 model existed;
2. the first Vulkan contract reached the unsupported productive route;
3. a masked Vulkan RED exposed an out-of-bounds transient-exposure write on
   non-Bubble layers; restricting that UAV to explicit Bubble state restored
   inherited CavityCarve determinism;
4. structural preservation RED showed that inspection used only the current
   layer's write flag instead of the already-live Strata channel; inspecting
   the live channel preserved and proved its exact samples;
5. READ_ONLY review found four closure gaps: the productive legacy route had
   been displaced, the CPU reference did not own authored-radius sampling,
   radius-two lacked direct parity, and zero iterations still scheduled a
   no-op dispatch. New REDs reproduced each issue before the minimum fixes;
6. enabling Bubble plus an existing regional mask then exposed a model-flag
   overwrite. Composing rather than assigning the existing mask bits produced
   the final non-empty bounded masked delta;
7. the inherited Release field matrix reproduced `0xc0000409` when inactive
   passes bound a one-element exposure sink. A full-domain transient binding
   plus exact budget accounting removed the unsafe optimized-shader boundary;
   the same canonical Release route then passed.

The corrected review surface preserves legacy behavior, gives the CPU reference
explicit radius sampling through the radius-four endpoint, verifies radius-one
radius-two and radius-four CPU/GPU parity, round-trips legacy
`SurfaceWeathering` unchanged through schemas 1 and 10, schedules the focused
Bubble gate in CTest and performs no zero-iteration pass.

Final READ_ONLY review reported no Critical or Important finding and
`Ready=yes`. Its remaining minor note concerns two pre-existing foreign
formatting hunks in the shared CMake file; publication stages only the focused
Bubble CTest hunk and preserves those foreign hunks untouched.

## Mathematical and Vulkan proof

Frozen kernel, classification, units, update, durability, iteration, mask,
background and channel contracts are recorded in:

`docs/evidence/M8_GEOLOGY_BLUEPRINT_P0_BUBBLE_WEATHERING_CONVENTIONS.md`

Focused real Vulkan command:

```text
JericoGeologyGpuSessionTests.exe --bubble-weathering
```

Observed result:

```text
procedural.geology.p0_bubble_weathering|backend=Diligent/Vulkan|device=NVIDIA GeForce RTX 3090|field_hash=3373581106375854912|baseline_hash=4703855310837332601|flat_exposure=0.346154|corner_exposure=0.730769|exposure_parity_max=4.58497e-08|scalar_parity_max_m=5.20394e-09|durability_dirty_voxels=3884|strength_dirty_voxels=3884|iteration_dirty_voxels=5446|radius_dirty_voxels=6209|radius2_exposure_parity_max=4.58497e-08|radius2_scalar_parity_max_m=2.60197e-09|radius4_dirty_voxels=6989|radius4_exposure_parity_max=4.58497e-08|radius4_scalar_parity_max_m=2.60197e-09|mask_dirty_voxels=76
```

The convex corner exposure (`0.730769`) is greater than the broad-flat centre
(`0.346154`) and receives greater scalar removal. Durability `0.75` removes
less than durability zero; reduced strength, iteration count, radius and the
localized RBrush mask each produce deterministic bounded work. Identical
reevaluation reports zero dirty leaves/voxels. Zero strength and iterations
match the source hash, and zero iterations records no SurfaceWeathering pass.

Visible proof:

- `docs/evidence/images/m8-geology-blueprint-p0-bubble-weathering-vulkan.png`;
- `docs/evidence/images/m8-geology-blueprint-p0-bubble-weathering-vulkan.svg`;
- portrait `900x1500` PNG plus matching vector source;
- baseline hash `4703855310837332601` versus weathered hash
  `3373581106375854912`;
- both panels are actual evaluated GPU meshes, and the exposure bars come from
  selected GPU field samples rather than synthetic geometry, shading, debug
  color or surface noise.

## Verification

Final Release verification used the existing Windows Vulkan build products:

- direct MSBuild of `JericoGeologyRecipeTests` and
  `JericoGeologyGpuSessionTests`: `PASS`;
- `JericoGeologyRecipeTests.exe`: `PASS`, including schemas 1/10 legacy
  `SurfaceWeathering` and radius-one through radius-four mathematical tests;
- CTest `procedural.geology_gpu_bubble_weathering`: `1/1 PASS` in `55.62 s`;
- focused canonical `--field` Release regression: `PASS` after the safe
  full-domain exposure binding correction;
- inherited five-test repository/recipe/topology/full Vulkan matrix: `5/5
  PASS` in `594.88 s`;
- `git diff --check`: clean for the owned publication paths.

The existing full CMake regeneration remains blocked by unrelated preserved
foreign CMake state (`Qt6::Core` and `Jerico::EditorProjectAuthority` targets);
the already-configured project files and registered CTest gate were used
without changing that foreign scope.

Inherited real Vulkan gates retained their accepted observations:

```text
CavityCarve  PASS  field_hash=2463036689238531135 parity_max_m=7.03335e-08
FractureCells PASS field_hash=15244624087024613006 scalar_parity_max_m=5.36442e-09
```

## Scope boundary

Completing BubbleWeathering does not complete geology P0, the geology
blueprint or JericoEngine 1.0. Blueprint Step 12 (`CurvatureWeathering`) remains
the next P0 slice and is not implemented here.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
