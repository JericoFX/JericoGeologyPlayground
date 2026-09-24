# M8 Sparse SDF Cliff Showcase

Date: 2026-08-23
Slice: `M8-SPARSE-SDF-CLIFF-SHOWCASE`
State: `PASS` for Geometry Technology Showcase criterion 3 only

## Result

The retained cliff is not a heightfield wall. It is one connected Sparse SDF
product assembled from eight overlapping asymmetric convex masses, including
three projecting strata, then sculpted with four subtractive undercut bays and
an admitted RBrush detail pass. The final mesh exposes shelves and open space
below them across a broad portion of the face.

The focal RED rejects both:

- a heightfield-representable cliff, even when triangulated; and
- a connected shelf whose volumetric witness is hidden from the proof camera.

The production gate additionally rejects fewer than `8,000` triangles, fewer
than `64` inspectable witness columns, less than `0.75 m` of undercut clearance
or less than `48 px` of projected witness separation. The accepted product
reports:

```text
vertex_count=18932
triangle_count=37752
witness_columns=170
maximum_undercut_clearance_m=2.3107
projected_witness_separation_px=100.0289
rbrush_changed_samples=96392
```

The stable metric record is
[`m8-sparse-sdf-cliff-metrics.txt`](data/m8-sparse-sdf-cliff-metrics.txt).

## Authoring and deterministic product

The cliff uses the existing Jerico paths rather than a proof-only mesh parser:

```text
JericoBSP convex source masses
-> SparseSdfBuilder replacement field
-> RBrush projection
-> SurfaceDisplace + CrackCutter
-> Sparse SDF mesher with retained smooth normals
-> .rmesh + static triangle .rcoll
-> real PBR_Opaque fixture package
-> JericoEditor Vulkan + source-free Shipping replay
```

The same request, seed, RBrush choice and canonical mask are generated twice.
The content hash, `.rmesh` bytes, `.rcoll` bytes, vertex inventory and index
inventory are exact matches. The retained Sparse SDF authoring session proves a
bounded subtractive parameter edit through preview/discard, apply, undo and
redo; the pre-edit and post-redo hashes match their corresponding accepted
states exactly.

No cave, tunnel, interior-negative-space scene, BSP/SDF/Terrain hybrid or new
renderer work is credited by this slice. Navigation is not claimed: this
bounded cliff prop has exact static triangle collision, but no authored route
whose acceptance would make a Recast path applicable.

## Cook, package and source-free replay

The final `.epak` has ten runtime entries: mesh, material, four PBR textures,
selection material/texture, level and static triangle collision. Shipping
validation rejects source/editor formats and the isolated Shipping root contains
only `Packages/Cliff.epak`.

Two Vulkan package-only replays are byte-identical at channel tolerance `0` and
differing-pixel ratio `0`. The runtime marker reports:

```text
backend=Vulkan
resolution=1280x720
draws=1
materials=1
textures=4
fallbacks=0
source_free=true
```

Representative generation plus mesh/collision cook time was `17,886 ms` in the
Debug acceptance run. Runtime used the established `StaticPbr ->
ExposureToneMap -> Fxaa -> Present` route with GTAO; no Vulkan validation,
`VUID`, Diligent error or Jerico renderer-fatal marker was present. No unavailable
total GPU duration is invented.

Package SHA-256:
`92FF1512D81D51F8A6D5B789B86B1A4F252AE7D5307BA2244390A902131235F8`.

## Original-resolution visual review

All three final captures were reopened at original resolution after the passing
Vulkan run.

| Capture | Native size | SHA-256 | Accepted fact |
|---|---:|---|---|
| [`m8-sparse-sdf-cliff-final-shaded.bmp`](images/m8-sparse-sdf-cliff-final-shaded.bmp) | 604x890 | `0AD8871E80842F7FFC1413090F8FB2892A6CE6F6DDAC58B491F6151AFDF0891C` | Ordinary JericoEditor final shaded view of the asymmetric stepped silhouette and projecting ledges. |
| [`m8-sparse-sdf-cliff-collision-debug.bmp`](images/m8-sparse-sdf-cliff-collision-debug.bmp) | 600x900 | `3FD3053B8C118A64F61A096970AD77B29940A11A55B2B465F94FBA4BEB89E5CA` | Distinct collision/topology view exposing the dense retained triangle derivative over the same shape. |
| [`m8-sparse-sdf-cliff-shipping.bmp`](images/m8-sparse-sdf-cliff-shipping.bmp) | 1280x720 | `4F76F605BE5E47245E4AC25461195B375F4A1F005A789B0CB0D3DB8E706B1B31` | Framed oblique package-only replay; the stepped crown, three shelves and dark free space below the overhangs are simultaneously visible. |

The first rejected iterations were deliberately not retained: they were dense
meshes but still read as a plain wall with rows of spherical blobs. Geometry
count alone did not receive showcase credit.

## Verification

```text
editor.sparse_sdf_cliff_showcase                 PASS, 45.96 s
editor.sparse_sdf_cliff_showcase_vulkan          PASS, 110.84 s
AssetDump --shipping-validate                    PASS, final + debug
JericoEditor                                     Vulkan, embedded=true
editor final/debug captures                      distinct
source-free runtime                              PASS x2
repeat capture                                   byte-identical, 0 pixels
Vulkan validation / renderer errors              none detected
```

## Gate effect

Geometry Technology Showcase criterion 3 changes from `NOT STARTED` to `PASS`.
The thirteen-criterion matrix becomes:

```text
PASS        3
PARTIAL     8
NOT STARTED 2
BLOCKED     0
first gap   criterion 4
```

The overall M8 Geometry Technology Showcase remains `PARTIAL`, and the
JericoEngine Persistent Goal remains `ACTIVE`.

## Criterion 10 regeneration reconciliation

The original package values remain the historical publication snapshot. The
canonical fresh `Release` / MSVC `19.44.35228.0` regeneration identity is cliff
`E08DAEFC28C830A3`, mesh `DA377219B5049911`, collision
`9D8354194E0C74F5`, package SHA-256
`DA73C426C21CA4852246874CC19E8D905A7FAE59F82A0697B8143C1E6AD523EE`.
Two clean full Vulkan executions are byte-identical. See
[`M8_GEOMETRY_DETERMINISTIC_REGENERATION_CLOSURE.md`](M8_GEOMETRY_DETERMINISTIC_REGENERATION_CLOSURE.md).
