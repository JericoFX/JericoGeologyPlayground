# M8 Slice31 — RBrush Sparse SDF + Material Stamp Evidence

**Date:** 2026-08-14

**State:** PASS for Slice31 Combined Geometry/Material Stamp

**Implementation checkpoint:** `0eb4cb1 feat(rbrush): coordinate sdf and material stamps`

## Observable product result

One RBrush action, identified by one action GUID, brush asset/preset, seed and
centered projection frame, produced both:

- a real Sparse SDF `CrackCutter` change followed by closed mesh regeneration;
- coordinated BaseColor, Roughness/ORM, Normal and Material Height changes.

Preview did not mutate accepted state. Apply, stale-preview rejection, Undo and
Redo preserved exact combined geometry/material generations. Invalid action
GUID, brush, seed, projection and cancellation left both states unchanged.

The coordinator is Qt-free and reuses `SparseSdfSession::previewBrushCrack` and
`SurfacePaint::evaluateMaterialPaint`; it adds no second geometry, paint,
transaction or dirty-region evaluator.

## RED

After adding `editor.rbrush_sdf_material_stamp_session`, scheduling first failed
with the expected inventory change from 289 to 290 tests. Once only that policy
change was admitted, the focal build failed at:

```text
fatal error C1083: cannot open include file:
'Jerico/Editor/RBrushSdfMaterialStampSession.hpp': No such file or directory
```

This was the expected missing-product RED before the coordinator existed.

## GREEN metrics

Direct focal execution reported:

```text
projected=27
geometry_changed=15
material_changed=572
before_field=4433146001320906727
after_field=11396295289668741972
before_mesh=16987138097501352060
after_mesh=3706071143325303788
before_material=9832528875841396675
after_material=16632830667693390192
before_combined=16124229639774239841
after_combined=16515244761345651981
dirty_min=-0.45,-0.45,0.25
dirty_max=0.45,0.45,0.75
dependencies=127
BaseColor changed texels=143
MetallicRoughness changed texels=143
Normal changed texels=143
Height changed texels=143
```

`dependencies=127` is the exact union of:

- Geometry Pages;
- Collision;
- Navigation;
- Surface Pages;
- Shadow Pages;
- GI Bricks;
- World Cell Dependencies.

## Retained visual evidence

![Combined real SDF and material stamp](images/m8-rbrush-sdf-material-stamp.bmp)

The four panels, left to right, are generated from the real focal fixture:

1. derived signed-distance crack mask;
2. Sparse SDF signed-distance delta at the edited surface;
3. resolved BaseColor result from the multi-channel material evaluation;
4. combined material response shaded with real resolved Normal, Roughness,
   Height and the SDF depth delta.

The geometry delta covers 39,632 pixels in its retained diagnostic panel. Two
independent executions produced the same BMP SHA-256:

```text
F817A24A1977222A814DE8FDB582CDA53B8BB6E21A73E5249C113D0C2C58A153
```

This BMP is retained deterministic product evidence, not concept art.

## Verification

### Focal product test

```text
ctest --preset windows-vulkan-editor
  -R ^editor\.rbrush_sdf_material_stamp_session$
  --output-on-failure

1/1 PASS
0.48 s test time
```

### SDF + Surface Paint regression chain

```text
editor.surface_paint_transaction
editor.rbrush_sdf_material_stamp_session
surface_paint.recipe
surface_paint.evaluator
surface_paint.cooker
procedural.sdf_brush_stamp
procedural.sdf_brush_projection
procedural.sdf_brush_crack_geometry

8/8 PASS
6.60 s real time
```

### Scheduling and runtime/editor boundary

```text
repository.test_scheduling_contract PASS, 0.03 s
editor.boundary_audit PASS, 272.27 s
2/2 PASS
```

The boundary audit rebuilt and verified its seven isolated runtime proof
executables. Its 4m32s duration was nested compilation, not a cook or a frozen
Qt event loop.

### Complete editor build

```text
cmake --build --preset windows-vulkan-editor
PASS
86.1 s
```

## Explicit boundary and remaining work

- No UI was added in Slice31.
- No new top-level tab/workspace or Procedural 3D Layout implementation was
  added.
- No material payload was stored in `.rbrush`.
- RBrush remains editor-only authoring input.
- This slice does not claim static SDF cook/package or source-free Shipping;
  those remain the explicit Slice34/35 gates in the accepted RBrush roadmap.
- Slice31 does not complete M8 or the JericoEngine 1.0 master Goal.
