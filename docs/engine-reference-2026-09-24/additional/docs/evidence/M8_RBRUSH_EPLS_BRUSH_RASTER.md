# M8 RBrush EPLS — shared scalar brush raster

Date: 2026-08-14

State: **Task 3 PASS; RBrush Slice 25, C26 EPLS, M8 and JericoEngine 1.0 remain ACTIVE**

This checkpoint proves that one real Brush Engine stroke produces deterministic
scalar coverage and that EPLS routes those exact bytes to either Manual Include
or Manual Exclude. It does not prove `.epls` persistence, editor preview, C04
job ownership, cook, MeshBlend, runtime integration or source-free replay.

## Product behavior

- `JericoPaintingBrush` owns the neutral U16 raster. It consumes the canonical
  Brush Engine, admitted R8 tips, projection, overrides and seed; no EPLS or PBR
  concept leaks into that seam.
- The literal stroke from `(2,2,0)` to `(6,2,0)` with diameter `2`, spacing `1`
  produces exactly three dabs at X `2`, `4` and `6`.
- The irregular `2x2` tip `[255,128;64,0]` produces `32` U16 texels, exactly
  `9` non-zero texels, dirty rect `[1,1]..[7,3)` and FNV-1a hash
  `0xDDE8C814ED949DC7`.
- Exact converted coverages are `65535`, `32896` and `16448`; repeated runs are
  byte-identical. Removing the intermediate Brush Engine dab breaks both the
  dab count and literal hash assertions.
- EPLS sends the same raster to only one destination. Include leaves Exclude
  zero; Exclude leaves Include zero. Full Exclude at a full procedural texel
  composes to exact zero under the previously proven final-mask contract.
- U16 layer strength is deterministic: `32768` scales full coverage to exact
  `32768`.
- `CancellationToken` now exposes the same cancellation state as a native
  `std::stop_token`. Job cancellation requests both views, and the raster checks
  the stop token per row before publishing a candidate.
- SurfacePaint's scalar path calls the Painting raster instead of retaining a
  second tip/projection loop. PBR payload sampling and material blending remain
  SurfacePaint-owned and its recipe/evaluator/cooker regressions remain green.
- Projection validation rejects finite axes whose squared length overflows;
  they now fail closed with `painting.mask.projection_invalid` instead of
  reaching an unnormalizable dab.

## RED retained

The Task 3 product tests were written before the new APIs. The causal failures
were:

```text
BrushMaskRasterizerTests.cpp(1,10): fatal error C1083:
Cannot open include file 'Jerico/Painting/Brush/BrushMaskRasterizer.hpp'

EplsBrushEvaluatorTests.cpp(1,10): fatal error C1083:
Cannot open include file 'Jerico/Epls/EplsBrushEvaluator.hpp'
```

Review added one adversarial projection case before hardening. Its authentic RED
was:

```text
BrushMaskRasterizerTests.cpp:172: check failed: !result.hasValue()
painting.brush_mask_rasterizer: FAILED
```

The CTest inventory changed from `272` to `278` because this task registered the
four pre-existing Painting/SurfacePaint executables that had no `add_test`, plus
the two new product tests. The scheduling contract records the complete new
inventory hash `c9fe1293a60bf06cc23a9e516d208232beaad53d262d1f6cc39f6ad30dbf47f2`.

## Verification

Fresh commands and results:

```powershell
cmake --build --preset windows-vulkan-editor --target JericoSurfacePaintEvaluatorTests JericoSurfacePaintCookerTests JericoBrushMaskRasterizerTests JericoEplsBrushEvaluatorTests
# PASS in 10.5 s

ctest --test-dir build/windows-vulkan-editor -C Debug -R "^(foundation\.jobs|epls\.|painting\.|surface_paint\.)" --output-on-failure
# 9/9 PASS in 0.49 s

cmake --build build/windows-vulkan-editor --config Release --target JericoJobSystemTests JericoBrushEngineTests JericoBrushMaskRasterizerTests JericoSurfacePaintRecipeTests JericoSurfacePaintEvaluatorTests JericoSurfacePaintCookerTests JericoEplsMaskTests JericoEplsRecipeTests JericoEplsBrushEvaluatorTests
# PASS in 80.4 s

ctest --test-dir build/windows-vulkan-editor -C Release -R "^(foundation\.jobs|epls\.|painting\.|surface_paint\.)" --output-on-failure
# 9/9 PASS in 0.76 s

ctest --test-dir build/windows-vulkan-editor -C Debug -R "^(repository\.test_scheduling_contract|architecture\.)" --output-on-failure
# 21/21 PASS in 2.42 s

ctest --test-dir build/windows-vulkan-editor -C Debug -R "^repository\.(runtime_only_target_discovery|shipping_source_exclusion)$" --output-on-failure
# 2/2 PASS in 45.60 s

cmake --build --preset windows-vulkan-editor
# integrated Debug build PASS in 224.6 s

git diff --check
# PASS; only existing line-ending conversion warnings from user-owned files
```

Product commit:

```text
39a9bd3 feat(painting): rasterize shared scalar brush masks
```

## Boundaries and next action

- No `.epls`, `.rbrush`, `.abr`, stroke, Qt or authoring graph reader was added
  to runtime or Shipping.
- This checkpoint contains no visual proof and makes no visual/editor/runtime
  claim.
- MeshBlend remains a later Materials/Renderer backend; it was not implemented
  inside EPLS or PWAS.
- Next legal behavior is Task 4: bounded `.epls` persistence, the World facet
  and one reversible ProjectAsset transaction with Preview/Cancel/Apply/
  Undo/Redo and stale dependency rejection.
