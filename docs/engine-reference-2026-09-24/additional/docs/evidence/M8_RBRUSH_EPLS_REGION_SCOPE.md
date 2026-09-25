# M8 RBrush EPLS — canonical RegionMask scope

Date: 2026-08-13

State: **Task 2 PASS; RBrush Slice 25, C26 EPLS, M8 and JericoEngine 1.0 remain ACTIVE**

This checkpoint proves recipe identity and selected-region behavior. It does
not prove RBrush rasterization, editor persistence, C04 preview, cook,
MeshBlend, runtime integration or source-free Shipping.

## Product behavior

- `EplsLayerRecipe` admits the environmental semantics from the accepted
  design, a canonical UTF-8 display/custom semantic, sorted unique manual
  layers, Include/Exclude roles, non-zero U16 strength and visible/locked
  state.
- `WholeWorld` requires a zero RegionMask GUID. `RegionMask` requires exactly
  an existing non-zero `ProceduralMask` GUID; EPLS introduces no private Box,
  volume or spline schema.
- The recipe hash is stable over every admitted recipe value, including manual
  layer order/state and future RBrush stroke intent.
- `ProceduralMask` now owns point membership alongside its existing bounds
  queries. Box uses its complete XYZ bounds. SplineCorridor uses the same
  canonical sampled segments, Y interval and half-width as M7E.
- WholeWorld returns each procedural U16 sample unchanged. RegionMask retains
  samples inside the resolved Box/SplineCorridor union and writes exact zero
  outside it. Evaluation is bounded and cooperatively cancelable.

## RED retained

The product tests were written before the APIs. The two causal failures were:

```text
ProceduralMaskTests.cpp: error C3861:
'proceduralMaskContainsPoint': identifier not found

EplsRecipeTests.cpp(1,10): fatal error C1083:
Cannot open include file 'Jerico/Epls/EplsRecipe.hpp': No such file or directory
```

The CTest inventory changed from 271 to 272 tests and was reconciled separately
before accepting either failure as the product RED.

## Literal scope proof

The fixture contains both canonical primitive kinds:

- Box: `[-2,-1,-2]..[2,2,2]`;
- SplineCorridor: `[-6,0,0]..[6,0,0]`, half-width `0.5`, Y `[-1,2]`.

Literal procedural samples prove the union:

```text
position (0,0,1), source 1000   -> 1000  (inside Box)
position (4,0,0), source 20000  -> 20000 (inside SplineCorridor)
position (8,0,8), source 65535  -> 0     (outside both)
```

The same source under WholeWorld remains exactly
`[1000, 20000, 65535]`.

## Verification

Fresh commands and results:

```powershell
cmake --build --preset windows-vulkan-editor --target JericoProceduralMaskTests JericoEplsRecipeTests
# PASS in 7.8 s

ctest --test-dir build/windows-vulkan-editor -C Debug -R "^(epls\.recipe|procedural\.mask)$" --output-on-failure
# 2/2 PASS in 0.54 s

ctest --test-dir build/windows-vulkan-editor -C Debug -R "^(epls\.(mask|recipe)|procedural\.(mask|pwas_evaluator)|navigation\.)" --output-on-failure
# 6/6 PASS in 1.46 s

cmake --build --preset windows-vulkan-editor --config Release --target JericoProceduralMaskTests JericoEplsRecipeTests
# PASS in 11.2 s

ctest --test-dir build/windows-vulkan-editor -C Release -R "^(epls\.recipe|procedural\.mask)$" --output-on-failure
# 2/2 PASS in 0.13 s

ctest --test-dir build/windows-vulkan-editor -C Debug -R "^(repository\.test_scheduling_contract|architecture\.)" --output-on-failure
# 21/21 PASS in 1.43 s

git diff --check
# PASS; only existing line-ending conversion warnings from user-owned files
```

Product commit:

```text
e1a668a feat(epls): scope layers with canonical region masks
```

## Boundaries and next action

- No `ProceduralMaskChannel` was added for EPLS.
- No EPLS copy of Box/SplineCorridor exists.
- No Qt, Diligent, runtime format or Shipping reader was added.
- MeshBlend remains a later shared backend and is not demonstrated here.
- Next legal behavior is the Painting-owned scalar RBrush raster seam and the
  EPLS adapter that routes one real stroke to Include or Exclude.
