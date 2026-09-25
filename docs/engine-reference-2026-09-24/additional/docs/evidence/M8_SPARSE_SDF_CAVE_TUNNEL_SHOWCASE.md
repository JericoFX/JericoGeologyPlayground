# M8 Sparse SDF Cave/Tunnel Showcase

Date: 2026-08-24
Slice: `M8-SPARSE-SDF-CAVE-TUNNEL-SHOWCASE`
State: `PASS` for Geometry Technology Showcase criterion 4 only

## Result

The retained cave is a real indexed iso-surface extracted from the final
Jerico-native Sparse SDF field. The AABB is allocation/debug bounds only. With
`Show SDF Bounds` disabled, the generated mesh remains visible and unchanged.

Shape generation is independent from extraction:

```text
BaseField
-> GeologicalModifierStack
-> OptionalNegativeSpaceStack
-> Final Sparse SDF
-> Surface Classification
-> existing mesh/collision/LOD/cook
```

`SparseSdfGeologyGenerator` is the shared owner of this flow. Rock and cliff
recipes use it with `NegativeSpace=None`; the cave recipe first creates a solid
host mass and then explicitly subtracts the selected BSP opening, spline tunnel,
chamber and branch. `SparseSdfCaveShowcase` is a consumer, not the general
geology generator. No subdivision, QEF or remesh parameter was used to repair
the silhouette.

The accepted cave reports:

```text
vertex_count=56724
triangle_count=113444
visible_landmarks=6
tunnel_length_m=9.9070
minimum_ceiling_clearance_m=2.0031
ceiling_relief_m=1.4536
chamber_width_m=8.3999
branch_length_m=6.9161
decisive_bend_degrees=37.9518
minimum_route_void_distance_m=0.3678
maximum_probe_zero_crossings=6
rbrush_changed_samples=27252
```

The stable final-product record is
[`m8-sparse-sdf-cave-metrics.txt`](data/m8-sparse-sdf-cave-metrics.txt).

## RAW field proof: recipe before meshing

The shared generator retained three neutral, pre-material iso-surfaces:

```text
Rock   1082 vertices / 2160 triangles / 0 subtract primitives
Cliff  2526 vertices / 5048 triangles / 0 subtract primitives
Cave   6776 vertices / 13564 triangles / 4 subtract primitives
```

Rock and cliff are closed solid fields without negative space. The cave differs
because negative space is an explicit second stack over a solid host. These
facts and field/surface hashes are retained in
[`m8-sparse-sdf-raw-geology-metrics.txt`](data/m8-sparse-sdf-raw-geology-metrics.txt).

The rock was also regenerated one field stage at a time:

```text
Base -> Anisotropy -> Domain Warp -> Macro FBM -> Meso/Ridged
     -> Strata -> Shallow Fractures -> Erosion
```

All eight stages have distinct field and surface hashes, remain closed, and
contain zero negative primitives. The record is
[`m8-sparse-sdf-rock-stages-metrics.txt`](data/m8-sparse-sdf-rock-stages-metrics.txt).
This is the regression oracle for a future blob: inspect the first changed RAW
stage instead of compensating in the mesher.

The final-field horizontal and vertical diagnostics render the solid host on
the left and the post-RBrush final field on the right. They expose the irregular
BSP entrance, continuous tunnel, chamber, branch and remaining rock wedge. The
RBrush crack pass is surface-localized: it changes 27,252 samples rather than
globally deforming 165,564 narrow-band samples as the rejected recipe did.

## BSP opening and spline negative space

The authored entrance is an exact selected subtractive BSP convex field. Its
outer and inner rings form a tapered irregular eight-sided opening, so it is not
an AABB or circular portal. It blends into a spline tunnel whose control points
carry position, radii, blend and local warp, then reaches a chamber and a
separate curved/vertical branch.

The RED gate rejects:

- an exterior recess or debug-only arch;
- a missing entrance, tunnel, chamber, natural ceiling or branch;
- an oversized box vestibule;
- an AABB-only BSP field without mixed-axis planes;
- a branch that removes the central rock wedge;
- a view that cannot inspect all six required landmarks.

## Surface classification, collision and navigation

Classification happens after final geometry. It partitions one continuous
vertex/index surface into four material zones without defining the shape:

```text
Floor          14348 triangles
Wall            6500 triangles
Ceiling/Rock    15867 triangles
Exterior Rock  76729 triangles
```

Normals come from the final SDF gradient and tangents are cooked with the mesh.
The `.rcoll` is the exact generated triangle surface, not bounds or a separate
shell. Recast retains 30 polygons; Detour returns a two-point
entrance-to-chamber path, while the navigation debug product also exposes the
branch.

## Resolution, LOD and documented limit

The final surface uses a `0.14 m` voxel size. LOD1 regenerates the same recipe at
`0.20 m`:

```text
Final  56724 vertices / 113444 triangles / 4084604 bytes
LOD1   27522 vertices /  55040 triangles / 1982064 bytes
```

LOD1 retains all six landmarks and continuous route clearance. Its critical
feature error is `0.0222 m`; the measured approximation error is `0.2123 m`.
Final/LOD1 maximum edge lengths are `0.24189410 m` and `0.34508416 m`.

This slice does not claim the missing general adaptive-remesh/retopology
pipeline. The extractor remains uniform within active cells; min/max edge and
approximation error are measured rather than mislabeling LOD as adaptive
topology. No new dependency was added.

## Cook and source-free Vulkan replay

The final `.epak` contains 15 runtime entries: final and LOD meshes, four cave
materials, four PBR textures, editor selection material/texture, level, exact
triangle collision and navigation. Shipping validation rejects source/editor
formats. Package-only Vulkan reports:

```text
backend=Vulkan
resolution=1280x720
draws=4
materials=4
textures=4
fallbacks=0
source_free=true
```

Two final-quality replays are byte-identical at channel tolerance `0`; a third
replay directly selects LOD1 and is visually distinct. No Vulkan validation,
`VUID`, Diligent error or Jerico renderer-fatal marker was present. Package
SHA-256:
`A5AA0FB6E88EA14A49ADBDC7D840342D46E54C70A159EA97BD5A22BA65BAB7D9`.

## Original-resolution visual review

All retained captures were reopened at original resolution after the passing
Vulkan gate.

| Capture | Native size | SHA-256 | Accepted fact |
|---|---:|---|---|
| [`m8-sparse-sdf-raw-rock.bmp`](images/m8-sparse-sdf-raw-rock.bmp) | 1280x720 | `7B754AAB762B568699499A487315848304E75E22CC8B6D47EF55828263BCABA4` | Neutral RAW shared-generator rock, closed and with no negative space. |
| [`m8-sparse-sdf-raw-cliff.bmp`](images/m8-sparse-sdf-raw-cliff.bmp) | 1280x720 | `C772849A2E8DB660FDB2CFDCA9ECCB9A7A2F93A69E25A8D895E2708E24E5B539` | Neutral RAW shared-generator directional cliff/macizo, closed and with no negative space. |
| [`m8-sparse-sdf-cave-final-field-horizontal.bmp`](images/m8-sparse-sdf-cave-final-field-horizontal.bmp) | 1284x480 | `E494ECE417AC85A2CBD40BC8032E2BC45C61FA886A6A5B6964D21C8376717329` | Host-only field beside the final horizontal negative-space field. |
| [`m8-sparse-sdf-cave-final-field-vertical.bmp`](images/m8-sparse-sdf-cave-final-field-vertical.bmp) | 1284x480 | `B4F908DC66A4A6A2697E074DF008C4E14260594673AC9547B0F1290C8B1257F8` | Host-only field beside the final vertical tunnel/chamber field. |
| [`m8-sparse-sdf-cave-entry-final.bmp`](images/m8-sparse-sdf-cave-entry-final.bmp) | 604x890 | `C3A0BF96F19B351B8C2C01EC220EE7F540158B5F3F80B01B6DE1E51BF941BCA1` | JericoEditor shaded host mass and generated entrance with no bounds cube. |
| [`m8-sparse-sdf-cave-collision-debug.bmp`](images/m8-sparse-sdf-cave-collision-debug.bmp) | 604x890 | `3FDD3683D2D6F704143E2250D24DD5F60EA27115D6A597E7B5C2DE3504A74DC5` | Dense collision derivative of the same generated surface. |
| [`m8-sparse-sdf-cave-navigation-debug.bmp`](images/m8-sparse-sdf-cave-navigation-debug.bmp) | 604x890 | `FA7F422A7983FD6634773837DD423A24CBA94D1E24E70A1564E478CBA2EAAEFB` | Recast/Detour route with a readable branch. |
| [`m8-sparse-sdf-cave-shipping.bmp`](images/m8-sparse-sdf-cave-shipping.bmp) | 1280x720 | `8500BB68767D6018256E28CA30D53520B75B5F959657EA3F343DB2C6695C909E` | Package-only interior view: tapered opening, tunnel, chamber, ceiling, floor and rock wedge. |
| [`m8-sparse-sdf-cave-lod1-shipping.bmp`](images/m8-sparse-sdf-cave-lod1-shipping.bmp) | 1280x720 | `B7B0E576AE31A6900D28A60A2EBB41FE287F499474602E9318FE1BF7A7FF8B3E` | Direct LOD1 replay retaining the critical negative space. |

The old box/recess images and the global RBrush star-patch result are rejected
and receive no gate credit.

## Verification

```text
JericoSparseSdfGeologyTests                 PASS
JericoSparseSdfBuilderTests                 PASS
JericoSparseSdfMesherTests                  PASS
JericoBspWorkbenchTests                     PASS
JericoSparseSdfCaveShowcaseTests            PASS
editor.sparse_sdf_cave_showcase             PASS, 35.57 s
editor.sparse_sdf_cave_showcase_vulkan      PASS, 81.21 s
AssetDump --shipping-validate               PASS
source-free final replay                    PASS x2, byte-identical
source-free LOD1 replay                     PASS, visually distinct
Vulkan validation / renderer errors         none detected
```

## Gate effect

Geometry Technology Showcase criterion 4 changes to `PASS`. The matrix becomes
`4 PASS / 8 PARTIAL / 1 NOT STARTED`; criterion 5 is the first ordered gap.
The overall M8 Geometry Technology Showcase remains `PARTIAL`, M8 Render
Foundation remains `PASS` 11/11, and the JericoEngine Persistent Goal remains
`ACTIVE`.

## Criterion 10 regeneration reconciliation

The original package values remain the historical publication snapshot. The
canonical fresh `Release` / MSVC `19.44.35228.0` regeneration identity is cave
`126FB56529E5BE57`, mesh `571C36D04467CA84`, collision
`35E407FA79B81038`, navigation `2E9C0FD989106D82`, package SHA-256
`6DC405E0C320A3389EC8574F3412A4DF0D4038C6A852EE7342FD2F6FC992E245`.
Two clean full Vulkan executions are byte-identical. See
[`M8_GEOMETRY_DETERMINISTIC_REGENERATION_CLOSURE.md`](M8_GEOMETRY_DETERMINISTIC_REGENERATION_CLOSURE.md).
