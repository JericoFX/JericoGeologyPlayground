# M8 Geology Blueprint P0 CurvatureWeathering Closure

**Slice:** `M8-GEOLOGY-BLUEPRINT-P0-CURVATURE-WEATHERING`
**Status:** `PASS`
**Backend:** Diligent/Vulkan, NVIDIA GeForce RTX 3090
**Schema:** 12

## Result

Jerico now evaluates CurvatureWeathering as bounded, signed-curvature-aware
erosion of the real sparse scalar field. The admitted CPU reference and
productive Diligent/Vulkan route use the same centred physical stencil,
negative-inside sign convention, convex/concave selection, exposure,
durability, strength, regional mask and scalar update.

The implementation:

- extends the existing `SurfaceWeathering` payload with an explicit schema-12
  `Curvature` model while preserving schemas 1–10 legacy weathering and the
  exact schema-11 Bubble model;
- validates, serializes and hashes mode, physical spacing, response length,
  maximum wear, durability and iterations without admitting a stochastic seed
  or transformed domain;
- computes `div(normalize(gradient(F)))` from centred finite differences in
  metres, with a frozen zero-normal epsilon and positive sparse background;
- uses positive curvature for `Convex`, negative curvature for `Concave`, and a
  dimensionless clamped response;
- reuses BubbleWeathering's Moore exposure and the existing mask, cancellation,
  workload guard, memory preflight, scalar ping-pong and exact dirty tracking;
- exposes only bounded selected-sample gradient magnitude, signed curvature,
  response and exposure for CPU/GPU proof;
- changes actual scalar values and Surface Nets positions for both modes while
  preserving source material GUIDs and structural scalar/mask channels exactly.

No VDB/UI route, independent persistent DurabilityField, mineral chemistry,
material transport, rockfall, colluvium, sediment, Detail, P1/P2/M9/Task23 or
later consumer was introduced. No external implementation was copied and no
dependency or licence record changed.

## RED to GREEN evidence

The slice began with mathematical RED contracts on small analytic fields. The
recipe target first failed because neither the curvature model nor evaluator
existed. The tests froze plane-near-zero behavior, opposite convex/concave
signs, physical scaling, finite zero-gradient behavior, determinism and safe
control composition before implementation.

The first productive Vulkan contract then failed because selected field samples
did not expose curvature metrics. After adding the productive pass, the field
and topology changed but metrics remained zero; that RED identified a stale
scalar-sized copy offset/length after the transient inspection tuple expanded
to four floats. Correct float4 accounting restored bounded readback and exact
CPU/GPU parity. No whole-field readback was added.

Final acceptance also covers durability `0.75`, two-iteration ping-pong, zero
strength, zero iterations with no scheduled pass, a localized RBrush mask,
Strata channel preservation, cancellation and required-versus-allowed device
memory.

Frozen conventions are recorded in:

`docs/evidence/M8_GEOLOGY_BLUEPRINT_P0_CURVATURE_WEATHERING_CONVENTIONS.md`

## Mathematical and Vulkan proof

Focused real Vulkan command:

```text
JericoGeologyGpuSessionTests.exe --curvature-weathering
```

Observed result:

```text
procedural.geology.p0_curvature_weathering|backend=Diligent/Vulkan|device=NVIDIA GeForce RTX 3090|convex_hash=17123893163529741849|concave_hash=1930275177814404830|baseline_hash=4703855310837332601|convex_curvature=11.7581|concave_curvature=-10.6201|convex_response=1|concave_response=1|convex_dirty_voxels=1249|concave_dirty_voxels=1561|durability_dirty_voxels=1529|iteration_dirty_voxels=92160|mask_dirty_voxels=54
```

The bounded selected samples report convex `+11.758061 1/m` and concave
`-10.620078 1/m`; their maximum CPU/GPU curvature deviation is
`7.6138155e-7 1/m`. Both responses are one at the selected high-curvature
locations. Identical convex reevaluation reports zero dirty leaves and voxels.
Durability reduces removal, the localized mask changes only 54 voxels, and two
iterations execute the existing safe ping-pong route.

Visible proof:

- `docs/evidence/images/m8-geology-blueprint-p0-curvature-weathering-vulkan.png`;
- `docs/evidence/images/m8-geology-blueprint-p0-curvature-weathering-vulkan.svg`;
- exact `1500x900` PNG plus matching vector source;
- all three panels are meshes extracted from evaluated GPU scalar fields;
- hashes and signed-curvature bars are sourced from the same Vulkan evaluation
  and bounded field samples, not synthetic geometry, debug colour, shading-only
  substitution or surface noise.

## Verification

Final Release verification used the existing Windows Vulkan build products:

- direct build of `JericoGeologyRecipeTests` and
  `JericoGeologyGpuSessionTests`: `PASS`;
- `JericoGeologyRecipeTests.exe`: `PASS` for the mathematical, schema,
  round-trip, hash and rejection contracts;
- `JericoGeologyGpuTopologyPlanTests.exe`: `PASS`;
- CTest `procedural.geology_gpu_curvature_weathering`: `1/1 PASS` in
  `56.08 s`;
- inherited BubbleWeathering, CavityCarve and FractureCells focused Vulkan
  regressions: `PASS` with their accepted hashes and parity values;
- inherited recipe/weathering/topology/full Vulkan/Bubble matrix: all five
  gates `PASS`; the full Vulkan session passed in `566.29 s` and Bubble in
  `56.07 s`;
- `git diff --check`: clean for the owned publication paths.

Full CMake generation remains blocked by unrelated preserved foreign state:
missing `Qt6::Core` and `Jerico::EditorProjectAuthority` targets in other test
subtrees. The test project builds directly and the newly generated CTest
registry contains the focused curvature gate; this slice does not alter that
foreign configuration.

## Scope boundary

Completing CurvatureWeathering does not complete geology P0, the geology
blueprint or JericoEngine 1.0. Blueprint Step 13 (`Detail`) remains the next P0
slice and is not implemented here.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
