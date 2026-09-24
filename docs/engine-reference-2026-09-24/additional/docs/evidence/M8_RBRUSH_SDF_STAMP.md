# M8 RBrush Slice 28 — Sparse SDF Stamp Evidence

Date: `2026-08-14`

Status: `PASS` for RBrush Slice 28 only. Sparse SDF 2.0, RBrush as a complete
family, M8 and the JericoEngine 1.0 master Goal remain incomplete.

## Observable contract

`SdfBrushStamp` accepts an immutable, validated `SparseSdfField` plus canonical
already-projected points and applies exactly these authoring operations:

| Operation | Source | Stamp / target | Influence | Output | Operation hash | Field hash |
|---|---:|---:|---:|---:|---:|---:|
| Add | 1 | -2 | 1 | -2 | `13819501352865884505` | `4478657834312672723` |
| Subtract | -1 | -2 | 1 | 2 | `17289738553750012232` | `5432769257977669204` |
| SmoothAdd | 0.2 | 0.3 | 1 | 0.14375 | `14760236680866618829` | `3814269132897798069` |
| SmoothSubtract | 0.2 | -0.3 | 1 | 0.35625 | `14990342151316517628` | `2625446274221995991` |
| SurfaceDisplace | 1 | displacement 0.5 | 0.5 | 0.75 | `7647952108198072445` | `9898553365190078106` |
| CrackCutter | -1 | -2 | 1 | 2 | `3705671232875340348` | `11513157665436217872` |
| ChipCutter | -1 | -2 | 1 | 2 | `13955552304177119487` | `9240233963105208507` |
| Smooth | 2 | six neighbours 0 | 1 | 0.285714 | `15583829297322223462` | `15427234134671456365` |
| Flatten | 1 | -1 | 0.5 | 0 | `3106828343726580556` | `15782636976898745485` |
| MaskOnly | 1 | mask 0.625 | 0.625 | field remains 1 | `5611770689379776952` | source `11068065209510513868` |

An additional `Smooth` case crosses from brick `{1,0,0}` into its neighbour
and obtains the same `2/7 = 0.285714` result. Its operation/field hashes are
`437325729519897671` / `1477616105812625351`.

The half-strength Add case produces `1 -> -1`, and a displacement beyond the
narrow band clamps `-3.8 -> -4` for a four-cell band.

## Product boundary

- The source field is never mutated.
- Input order is not semantic; a private brick/sample sort produces identical
  operation and result hashes.
- Duplicate keys, missing bricks, sample index `>=729`, non-finite values,
  invalid settings/identity/provenance and empty input fail closed.
- Requests are bounded to `1,048,576` points.
- Any positive-influence `Locked` or `NoGenerate` sample rejects the entire
  request. Zero influence leaves protected authoring untouched.
- Changed samples receive `stampRegionGuid`. Inserting a lexicographically
  earlier region remaps every existing owner by GUID before reopening through
  `SparseSdfField::create`.
- `MaskOnly` accepts no geometry region, preserves every source field hash and
  returns sorted positive mask values.
- Cancellation is checked before allocation and through validation,
  evaluation, ownership copying, application and canonical hash traversal.

## RED

After registering `procedural.sdf_brush_stamp` and updating the exact test
inventory from 286 to 287, configuration succeeded. The focal product build
failed for the intended reason:

```text
SdfBrushStampTests.cpp(1,10): fatal error C1083:
Jerico/ProceduralAuthoring/SdfBrushStamp.hpp: No such file or directory
```

No product implementation existed at that point.

## GREEN and verification

Product commit:

```text
a2a8b8789ede853e81361f15d2aa190297a7f77b
feat(procedural): apply rbrush sdf stamp operations
```

Fresh results:

```text
Debug focal:                 1/1 PASS, 0.06 s
Sparse SDF neighbours:       6/6 PASS, 3.30 s
Release focal:               1/1 PASS, 0.04 s
Integrated Debug build:      PASS, 45.1 s
Runtime/Shipping boundaries: 12/12 PASS, 167.19 s
git diff --cached --check:   PASS
```

The boundary matrix includes runtime-only target discovery, source exclusion,
JericoBSP/Spline/PWAS/Sparse-SDF Shipping, procedural authority and scheduling.
The new module remains in `Tools/ProceduralAuthoring`; Shipping acquired no
RBrush authoring format or stamp implementation.

## New Procedural 3D Layout authority

Before changing Sparse SDF, both of these were read completely after the safe
Git reconciliation:

- `docs/JERICO_PROCEDURAL_3D_LAYOUT_GENERATOR.md`
- `docs/superpowers/specs/2026-08-14-procedural-3d-layout-generator-design.md`

This slice creates no top-level tab, workspace, World session or procedural
framework. Procedural 3D Layout remains `NOT STARTED` and, when legally opened,
must be a PWAS `Layout` mode reusing World, Spline, Sparse SDF, JericoBSP,
Masks, Inspector and the existing procedural viewport.

## Explicit non-claims

Slice 28 does not implement 2D-mask projection, Surface Tangent/Planar/
Triplanar/Spherical/Cylindrical/Radial/Object Local/World mapping, World picking,
Qt UI, C04 ownership, DDC persistence, crack/chip geometry showcase, cook of an
RBrush format, runtime procedural generation or Procedural 3D Layout.

The next legal RBrush capacity is Slice 29 projection. Slice 30 visible crack
geometry, Sparse SDF 2.0 completion, M8 and the master Goal remain pending.
