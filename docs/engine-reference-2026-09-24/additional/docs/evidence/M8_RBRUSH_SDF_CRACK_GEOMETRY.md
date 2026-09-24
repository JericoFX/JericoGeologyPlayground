# M8 RBrush Slice 30 — Sparse SDF Crack Geometry Evidence

Date: `2026-08-14`

Status: `PASS` for RBrush Slice 30 only. Sparse SDF 2.0, the complete RBrush
family, M8 and the JericoEngine 1.0 master Goal remain incomplete.

## Observable contract

`SparseSdfSession::previewBrushCrack` composes the accepted RBrush path instead
of introducing a parallel sculpt pipeline:

```text
R8/R16 mask -> Slice 27 signed-distance field -> Slice 29 projection
             -> Slice 28 CrackCutter stamp -> closed remesh/transition
             -> ordinary SdfChangePreview -> Apply / Undo / Redo
```

The operation accepts only `CrackCutter`, reads the current accepted
`replacementField`, leaves Preview immutable and publishes a new generation
only through the existing session transaction. Applying a second preview made
from an older generation is rejected. Undo restores the exact prior pointers
and hashes; Redo restores the exact accepted after generation without
recomputation.

The meshing integration also preserves ownership at new sign-crossing edges and
expands only the outer transition boundary. This prevents an interior crack
loop from being treated as the exterior selected patch.

## RED

Registering `procedural.sdf_brush_crack_geometry` first changed the complete
inventory from `288` to `289`:

```text
repository.test_scheduling.full_inventory_changed
expected_count=288
actual_count=289
actual_sha256=62bf955d27c1b66f744a6eeb049567a49fd74264c1f2d4d5114225bf61ddf3f8
```

After admitting that exact inventory, the focal build failed because
`MaskSignedDistanceField2D`, `SdfBrushCrackRequest` and
`SparseSdfSession::previewBrushCrack` did not exist. The first deep-cut GREEN
attempt then exposed two real product defects in sequence:

```text
procedural.sdf.ownership_ambiguous
procedural.sdf.transition_inverted
```

Both were fixed in the production ownership/transition path before acceptance.

## Real ABR proof

The retained local input was read through the admitted ABR decoder and never
copied into Git or Shipping:

```text
File: Free Cracks Photoshop Brushes 10.abr
Bytes: 23435296
SHA-256: 0D8875E5E19228A828F3A259348B8B94DB4584849F33BFC66DD8287FB65FEE9D
Decoded presets: 15
Selected preset: 0, 2500 x 2500, R8
```

For the bounded visual transaction, the selected mask is deterministically
box-averaged to a `33 x 33` footprint before Slice 27 derivation. This preserves
thin crack coverage at the transaction's finite SDF resolution; it is not a
claim that an arbitrary 2500² dense stamp can be remeshed directly in one
operation.

Release execution produced:

```text
samples=15
sample_index=0
source_width=2500
source_height=2500
mask_extent=33
projected=2352
changed=733
depth_pixels=32807
before_field=17306942977181637381
after_field=16179023300168531359
before_mesh=16836154411106329525
after_mesh=14258772364067016975
```

The retained three-panel image shows the decoded crack footprint, the unchanged
surface and the same surface after the localized geometric cut:

![RBrush crack mask, before Sparse SDF surface and after geometric cut](images/m8-rbrush-sdf-crack-geometry.bmp)

```text
Image: docs/evidence/images/m8-rbrush-sdf-crack-geometry.bmp
Bytes: 2137014
SHA-256: 71ED1EB3C8FB3D960680C11B9670588BBA11E13E97E82D92FA486FADB8515869
```

## GREEN and verification

Product commit:

```text
3ddfa57 feat(procedural): transact rbrush cracks on sparse sdf
```

Fresh results:

```text
Debug focal:                        1/1 PASS, 3.60 s
Slice 27/28/29 + Sparse SDF chain: 11/11 PASS, 9.78 s
Release focal with real ABR:            PASS, exit 0
Complete Debug build:                   PASS, 120.9 s
Scheduling contract:                1/1 PASS, 0.07 s
Runtime/Shipping boundary matrix:  12/12 PASS, 726.97 s
git diff --cached --check:               PASS
```

The boundary matrix covers runtime-only discovery, Shipping source exclusion,
procedural authority, Editor/runtime boundaries and the source-free JericoBSP,
PWAS, Spline and Sparse-SDF authoring/Shipping replays. No `.rbrush`, ABR bytes,
Qt code or crack-authoring implementation entered Shipping.

## Explicit limits and non-claims

- Classic Marching Cubes can still encounter geometric vertex-touch ambiguity
  for extreme dense/high-resolution cutter fields even when index topology
  reports zero open and non-manifold edges. Slice 30 proves the bounded admitted
  footprint above, not every dense stamp.
- No Qt panel, World picking, material companion, persistent DDC, C04 UI owner,
  RBrush cook/runtime format or Layout 3D implementation was added.
- Procedural 3D Layout remains `NOT STARTED`; no tab, workspace, World session
  or second procedural framework was created.
- Sparse SDF 2.0, the RBrush family, M8 and the master Goal remain active and
  incomplete.

The next legal RBrush capacity is Slice 31: combine the accepted geometric
stamp with material detail while preserving the same transaction and ownership
boundaries.
