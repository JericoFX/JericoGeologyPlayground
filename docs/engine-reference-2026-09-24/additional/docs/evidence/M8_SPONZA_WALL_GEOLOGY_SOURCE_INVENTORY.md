# M8 Sponza Wall Geology Source Inventory

Date: 2026-08-24
Slice: `M8-SPONZA-WALL-GEOLOGY-LAYER-VISUAL-SHOWCASE`
State: read-only inventory completed before derived showcase generation; no
source file was copied, converted or cooked by this slice

## Provenance and license

The available Intel Sponza 2022 source roots were inspected read-only:

```text
F:\JericoSourceAssets\main_sponza
F:\JericoSourceAssets\pkg_a_curtains
F:\JericoSourceAssets\pkg_c_trees
```

All three roots retain byte-identical `credits_license.txt` files with SHA-256
`3F981E31F8DE04F3754E63D7C259A0A78F71DB8F01B5155C58CAC52FB79CC2B9`.
They identify the Sponza 2022 Scene, its Intel Sample Library provenance and
Sponza Addon contributors, and contain the Creative Commons Attribution 4.0
International license. Attribution and the license notice remain required for
distributed derivatives. Source files remain external, immutable and outside
the repository commit.

The previously retained selected-source manifest
[`M8_HIGH_DETAIL_SPONZA_SOURCE.sha256`](M8_HIGH_DETAIL_SPONZA_SOURCE.sha256)
was revalidated against the external roots before this slice performed any
copy or cook:

```text
checked=93
bytes=2691984695
mismatches=0
```

## Available formats

The complete roots contain the following files. Presence does not imply that
Jerico supports or selects the format.

| Component | Files | Bytes | Formats |
|---|---:|---:|---|
| Architecture | 153 | 6,714,394,584 | `.bin` 1, `.fbx` 2, `.gltf` 1, `.hdr` 1, `.max` 2, `.png` 143, `.txt` 1, `.usda` 2 |
| Curtains | 27 | 1,581,114,826 | `.bin` 1, `.fbx` 2, `.gltf` 1, `.jpg` 6, `.max` 2, `.png` 12, `.txt` 1, `.usda` 2 |
| Tree | 18 | 3,695,228,974 | `.bin` 1, `.fbx` 2, `.gltf` 1, `.max` 2, `.png` 8, `.tga` 1, `.txt` 1, `.usda` 2 |

Only the existing glTF 2.0 `.gltf + .bin + relative PNG` closure is admitted:

```text
JericoGltfCook
  .gltf/.glb input
  -> canonical .rmesh/.rmat/.rtext
  -> deterministic .epak publication
  -> package-only source-free replay
```

FBX, USDA and MAX are not selected. The unrelated `JungleRuins` source root is
outside this slice.

The user-owned untracked `F:\JericoEngine\SponzaPBR_textures.zip` was also
inventoried without extraction or mutation:

```text
bytes=116573876
sha256=CEEEC5405BC8D9CB798833D102D3B32ED08EA07694397BA13814C610B6B3B241
entries=78
payload=.mtl x1 + .tga x75
uncompressed_bytes=228652955
```

It does not contain a model or a retained license/provenance record and is not
an admitted input for this slice. It remains immutable and outside commits.

## Referenced PBR images and channels

All images referenced by the selected glTF documents were reopened read-only:

| Component | Images | RGB | RGBA | Dimensions |
|---|---:|---:|---:|---|
| Architecture | 72 | 47 | 25 | 4096x4096 for all 72 |
| Curtains | 8 | 6 | 2 | 4096x4096 for all 8 |
| Tree | 4 | 1 | 3 | 4096x4096 for all 4 |

The architecture closure retains BaseColor, tangent-space Normal and packed
Roughness/Metalness inputs. The selected wall uses the already-cooked real PBR
material and texture dependencies; no external shader is copied or executed.

## Selected real wall

The selected surface is from the immutable architecture glTF:

```text
source glTF       NewSponza_Main_glTF_003.gltf
source glTF SHA   E04C4C540C74BDDDCBD3F590A85C14119BFD5839702246A23B3A737C0CCE2400
node index        55
node/mesh name    arch_wall_2ndfloor
mesh index        51
primitive index   2
source vertices   313
source indices    1137
source material   brickwall_01
cooked submesh    156 of 405
material GUID     FE605B89A020E71D90C6BE6783BCC79C
```

`JericoGltfCook` preserves each source primitive as one cooked submesh while
flattening the default-scene node transform into its vertices. The selected
submesh mapping is therefore deterministic and travels through the actually
supported Jerico route. The derived showcase may refine this open surface for
displacement fidelity, but its base view must retain the selected wall shape,
material identity and UVs. It must never replace the source wall with an AABB,
closed box or unrelated analytic showcase object.

The derived showcase selects the dominant vertical `+X` face within submesh
156 rather than taking its horizontal cap or inventing a closed volume. That
selection retains 50 source vertices / 189 source indices and 35 open boundary
edges. It is explicitly `surface_form=open_planar_wall` and
`negative_space=none`: no subtractive primitive, tunnel, chamber or cave is
present.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
