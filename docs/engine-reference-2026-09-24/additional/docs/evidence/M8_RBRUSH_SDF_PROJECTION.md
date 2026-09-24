# M8 RBrush Slice 29 — Sparse SDF Projection Evidence

Date: `2026-08-14`

Status: `PASS` for RBrush Slice 29 only. Sparse SDF 2.0, the complete RBrush
family, M8 and the JericoEngine 1.0 master Goal remain incomplete.

## Observable contract

`SdfBrushProjection` converts the existing Slice 27 derived 2D signed-distance
mask into bounded canonical `SdfBrushStampPoint` values for the accepted Slice
28 Sparse SDF stamp path. The source `SparseSdfField` and mask remain immutable.

All eight required modes produced one point for the same selected real field
sample and then changed/reopened that field through `CrackCutter`:

| Mode | Stamp distance | Projection hash | Stamp hash | Result field hash |
|---|---:|---:|---:|---:|
| Surface Tangent | -0.5 | `13817642073726687266` | `6083319531850472617` | `16793949789983081724` |
| Planar | -0.25 | `17487735343598694680` | `2326789694892004326` | `1073898290668858828` |
| Triplanar | -0.5 | `7887795675046450012` | `17252442637778935177` | `6662853671934300462` |
| Spherical | -0.25 | `10249552655738690842` | `11172782275780433558` | `1628700614455872665` |
| Cylindrical | -0.25 | `11063847535817692817` | `5215430016404449430` | `3568786324295590609` |
| Radial | -0.25 | `5292249510176051741` | `15083004182536926790` | `14279142293346469914` |
| Object Local | -0.25 | `18421976764197922715` | `12471144495272227606` | `8972217495641998694` |
| World | -0.4 | `13794124877933490879` | `8187116488201511375` | `5392537640065236603` |

The different distances are intentional: conformal modes use the sampled
field distance, planar/object/radial modes use their explicit projection depth,
and the tiled World mode can be bounded by the bilinear mask distance.

## Product boundary

- The public module is C++20, Qt-free and remains in
  `Jerico::ProceduralAuthoring`.
- The frame is finite, unit, orthogonal and right-handed; invalid frames are not
  silently repaired.
- `worldSizeMeters` preserves the source mask aspect ratio and isotropic texel
  distance. Sampling is bilinear; periodic axes wrap and bounded axes reject.
- Surface Tangent, Planar, Triplanar, Spherical, Cylindrical, Radial, Object
  Local and World follow the accepted coordinate/depth contracts.
- The one-sided extruded operand combines mask distance and slab distance, then
  clamps to the source narrow band.
- Candidate bricks are copied, sorted and validated. Reversed input order gives
  identical points and projection hash; duplicates and missing bricks fail.
- The caller can lower the Slice 28 hard cap of `1,048,576` output points.
  Budget overflow fails without returning partial output.
- Invalid mask identity/shape/finiteness, mode, bounds, settings, radial metric,
  triplanar normal and cancellation fail with stable
  `procedural.sdf.projection_*` errors.
- The projector emits points only. Slice 28 remains the sole owner that applies
  them to a new validated field generation.

## RED

Registering `procedural.sdf_brush_projection` first changed the complete test
inventory from `287` to `288` and produced the required scheduling RED:

```text
repository.test_scheduling.full_inventory_changed
expected_count=287
actual_count=288
actual_sha256=4ff88eeb4e54040a321cf3d5a76c8bcd8cc8537ce7e9532f11a373a0518e36c9
```

After admitting that exact inventory, the focal product build failed for the
intended reason:

```text
SdfBrushProjectionTests.cpp(1,10): fatal error C1083:
Jerico/ProceduralAuthoring/SdfBrushProjection.hpp: No such file or directory
```

No projection implementation existed at that point.

## GREEN and verification

Product commit:

```text
a12a8fc feat(procedural): project rbrush masks onto sparse sdf
```

Fresh results:

```text
Debug focal:                       1/1 PASS, 0.09 s
Slice 27/28 + Sparse SDF chain:  10/10 PASS, 6.26 s
Release focal:                     1/1 PASS, 0.09 s
Complete Debug build:                  PASS, 34.7 s
Scheduling contract:               1/1 PASS, 0.08 s
Runtime/Shipping boundary matrix: 12/12 PASS, 424.45 s summed test time
git diff --cached --check:              PASS
```

The boundary command's outer shell reached its 244-second limit while the
single `editor.boundary_audit` test was rebuilding runtime proofs. The original
`ctest` process continued rather than being duplicated. Its completed
`LastTest.log` records all twelve selected tests as `Test Passed`, including the
270.17-second boundary audit, and ends after the final Spline root guard.

The matrix proves runtime-only target discovery, Shipping source exclusion,
procedural authority, Editor/runtime boundary, source-free JericoBSP/PWAS/
Spline/Sparse-SDF replays and their authoring-root guards. Shipping acquired no
RBrush authoring parser, `.rbrush` format or projection implementation.

## Procedural 3D Layout authority

Before changing Sparse SDF, these documents were read completely after safe
Git reconciliation:

- `docs/JERICO_PROCEDURAL_3D_LAYOUT_GENERATOR.md`;
- `docs/superpowers/specs/2026-08-14-procedural-3d-layout-generator-design.md`.

This projector is a bounded materialization/refinement primitive. It is not the
canonical layout model and does not generate layout by directly editing SDF
bricks. Procedural 3D Layout remains `NOT STARTED`; later it must create a
logical GUID-based `Procedural3DLayout` in PWAS `Layout` mode, reuse the shared
World/Spline/SDF/BSP/Mask/Inspector surfaces and materialize only after
validation plus Preview/Accept. No new tab, workspace, World session or
procedural framework was added.

## Explicit non-claims

Slice 29 does not implement World picking, Qt controls, the C04 projection job
owner, persistent DDC, Undo/Redo, the uploaded-style visible crack showcase,
general Sparse SDF 2.0 operations, RBrush runtime data or Procedural 3D Layout.

The next legal RBrush capacity is Slice 30: demonstrate an uploaded-style crack
that cuts actual Sparse SDF rock geometry and survives the admitted authoring
transaction/Undo/Redo path. M8 and the master Goal remain active.
