# M8 Sponza Second Wall Rock Stamps Source Inventory

Date: 2026-08-24
Slice: `M8-SPONZA-SECOND-WALL-ADVANCED-ROCK-STAMPS-SHOWCASE`
State: read-only inventory completed before derived showcase generation

## Immutable source and admitted load route

The external Intel Sponza 2022 architecture root remained read-only. Its
retained `credits_license.txt` SHA-256 is
`3F981E31F8DE04F3754E63D7C259A0A78F71DB8F01B5155C58CAC52FB79CC2B9`;
the source is distributed under CC BY 4.0 and keeps the attribution recorded in
the prior complete inventory. The selected source document is:

```text
F:\JericoSourceAssets\main_sponza\NewSponza_Main_glTF_003.gltf
SHA-256 E04C4C540C74BDDDCBD3F590A85C14119BFD5839702246A23B3A737C0CCE2400
```

The 93-file retained source manifest was revalidated before and after the
showcase work:

```text
checked=93
bytes=2691984695
mismatches=0
```

No source file was copied, converted or mutated by this slice. The complete
license, provenance, format and image-channel inventory remains in
[`M8_SPONZA_WALL_GEOLOGY_SOURCE_INVENTORY.md`](M8_SPONZA_WALL_GEOLOGY_SOURCE_INVENTORY.md).
The only admitted route used here is the already-supported Jerico closure:

```text
.gltf + .bin + relative PNG
  -> JericoGltfCook
  -> canonical .rmesh/.rmat/.rtext
  -> deterministic .epak
  -> package-only source-free Vulkan replay
```

FBX, USDA and MAX were not loaded. No shader was imported. The user-owned
untracked `SponzaPBR_textures.zip` remains unextracted, unmodified and outside
the commit (`116573876` bytes, SHA-256
`CEEEC5405BC8D9CB798833D102D3B32ED08EA07694397BA13814C610B6B3B241`).

## Candidate real walls

The architecture glTF was parsed read-only and its source primitive order was
reconciled with the 405 cooked submeshes. Brick-wall candidates included:

| Node | Mesh | Primitive | Cooked submesh | Vertices / indices | Material | Decision |
|---|---:|---:|---:|---:|---|---|
| `walls_1stfloor_01` | 23 | 2 | 68 | 844 / 2922 | `brickwall_01` | viable alternative |
| `walls_1stfloor_02` | 44 | 2 | 136 | 2763 / 5187 | `brickwall_02` | different material |
| `arch_wall_2ndfloor` | 51 | 2 | 156 | 313 / 1137 | `brickwall_01` | explicitly rejected: first showcase wall |
| `stonewalls_2ndfloor_01` | 53 | 1 | 161 | 276 / 942 | `brickwall_01` | viable alternative |
| `stonewalls_2ndfloor_02` | 92 | 1 | 314 | 64 / 114 | `brickwall_01` | too little source detail |
| `walls_floor_03` | 105 | 3 | 349 | 359 / 1428 | `brickwall_01` | selected |

The selected node is index 110, clearly distinct from node 55 / submesh 156.
Its dominant vertical `-X` face retains 24 selected vertices, 60 selected
indices and 24 open boundary edges. It is an existing Sponza wall with two
openings, not an AABB or generated replacement:

```text
surface_form=open_planar_wall
negative_space=none
material GUID=FE605B89A020E71D90C6BE6783BCC79C
material=SponzaHighDetail/Architecture/Material/016-brickwall_01
channels=BaseColor + tangent-space Normal + packed MetallicRoughness
```

## Existing Jerico-native stamp owner

Repository inventory found one applicable geometry stamp authority and reused
it directly:

```text
owner      Jerico::Tools::ProceduralAuthoring::applySdfBrushStamp
request    SdfBrushStampRequest
operation  SdfBrushOperation::SurfaceDisplace
identity   operation GUID + stamp region GUID
provenance RBrush asset GUID + preset GUID + pack content hash
mask       deterministic mask derivation key
result     immutable SparseSdfField + operation/content hashes
```

The stamp path rejects missing provenance, duplicate sample keys, protected
samples, stale regions, invalid masks and over-budget inputs. This slice maps
the selected wall vertices to deterministic sparse-field samples and projects
the accepted surface displacement back onto the refined wall. It does not
introduce another stamp/stencil implementation.

## Noise and triangulation libraries

The geology evaluation used the existing Jerico-native `valueNoise` and 3D
Voronoi/fracture implementation. `Auburn/FastNoise2` is only `SELECTED`, not
`ADMITTED`, and therefore was not linked or consumed. `Delabella` remains a 2D
Delaunay/Voronoi candidate and is not the 3D geology backend. The manifest
records:

```text
noise_backend=JericoNativeValueNoiseVoronoi3D
external_noise_packs_used=false
```

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
