# M8 Sponza Wall Geology Layer Visual Showcase

Date: 2026-08-24
Slice: `M8-SPONZA-WALL-GEOLOGY-LAYER-VISUAL-SHOWCASE`
State: `PASS`

## Result

A real Sponza 2022 wall primitive now has one deterministic, source-immutable
comparison of base, domain warp, macro, meso/ridged, strata, erosion and the
combined result. Every stage uses the same real `brickwall_01` PBR material,
the same camera and the same raking key light. The primary evidence is seven
original-resolution `1280x720` Vulkan Beauty/final-shaded captures; topology is
supporting evidence only.

The wall is not an analytic box and is not a cave. The derivation selects the
dominant vertical `+X` face of Sponza node `arch_wall_2ndfloor`, primitive 2,
cooked submesh 156. It remains an open surface with 35 boundary edges and
declares `surface_form=open_planar_wall|negative_space=none`. No negative SDF
primitive, tunnel, chamber or external shader is present.

The complete read-only provenance, license, source hash, format, channel and
supported-load-path inventory is retained in
[`M8_SPONZA_WALL_GEOLOGY_SOURCE_INVENTORY.md`](M8_SPONZA_WALL_GEOLOGY_SOURCE_INVENTORY.md).

## RED first

The focused verification existed before the builder accepted
`--sponza-wall-geology`. Its first execution failed at the missing cancellation
contract, before implementation:

```text
CMake Error at Tests/Render/VerifySponzaWallGeologyLayerShowcase.cmake:45
sponza.wall_geology.cancellation_failed|exit=2
Usage: JericoHighDetailSponzaPlaygroundBuilder
  <architecture.rmesh> <curtains.rmesh> <tree.rmesh> <output.rmesh>
```

The accepted manifest validator also has five negative mutations and rejects:

```text
color-only change
identical stage position hash
missing erosion stage
camera/material/light drift
debug-only proof
```

## Selected geometry and deterministic recipe

```text
source glTF SHA-256  e04c4c540c74bdddcbd3f590a85c14119bfd5839702246a23b3a737c0cce2400
source node           arch_wall_2ndfloor
source primitive      2
cooked submesh        156 / 405
source primitive      313 vertices / 1137 indices
axis selection        +X vertical face
selected face         50 vertices / 189 indices / 35 open boundary edges
refinement            8x8 per source triangle
derived mesh          12096 vertices / 12096 indices / 4032 triangles
material GUID         FE605B89A020E71D90C6BE6783BCC79C
material              SponzaHighDetail/Architecture/Material/016-brickwall_01
textures              BaseColor + MetallicRoughness + Normal
seed                  0x53504F4E5A415741
negative space        none
```

The visible base is the selected Sponza surface with its source UVs and
material. The Jerico-native geology evaluator supplies bounded displacement;
it does not replace the wall with its auxiliary distance field. Publication is
atomic, preview does not mutate accepted state, cancellation publishes zero
files, and undo/redo return exact byte identities.

| Stage | Max displacement m | Mean displacement m | `.rmesh` SHA-256 | Beauty vs base changed ratio |
|---|---:|---:|---|---:|
| base | 0.000000 | 0.000000 | `10277736befcbbdb1f001ca640dbf4cab984b0031f91e5cf4a0238d18a37adfb` | 0.000000 |
| domain warp | 0.239773 | 0.102421 | `235f6aaab585593665ad77cdfcb0c1215fb41cdca4f7f64c6c7bfdf21ecf961a` | 0.108008 |
| macro | 0.190020 | 0.111403 | `3b7118829d9d62156e49e1692aecc57457461aaaec71500f936a754e3472a04f` | 0.105660 |
| meso/ridged | 0.137701 | 0.019078 | `87690cfcd1e559396fd7b839ec920a8f5cac500d9039aa8126b80e86a1efcaca` | 0.102174 |
| strata | 0.055000 | 0.034390 | `646b414c311edb66258f8b61bf8a7d10186148f0e10061631c19d3ae1724f7cf` | 0.103798 |
| erosion | 0.115541 | 0.075621 | `326d35277d72f19c8a6d3cbd2c13b25663fea058d383f1213ab611c290795a30` | 0.105592 |
| combined | 0.320000 | 0.180170 | `488203c10fa2d7d27e98abd80faa6c60c78c5593b2730d29a7ff0355d1820e26` | 0.107626 |

All seven position hashes and all seven cooked mesh SHA-256 identities are
unique. A repeated clean build byte-compares the seven meshes, manifest and
topology image exactly.

## Beauty/final-shaded proof

Fixed render identity:

```text
backend              Vulkan, validation enabled
resolution           1280x720
camera               x=0.25, z=4.8, yaw=0, pitch=0, vertical FOV=48
world                 scale=0.80, yaw=-25, y=0
material             FE605B89A020E71D90C6BE6783BCC79C
light                 identical directional + raking point key for all stages
directional shadows  High, Beauty debug=None
```

The committed PNG files are lossless, mobile-viewable mirrors of the inspected
runtime BMP captures. The exact BMP files remain reproducible in the build
evidence root and are not needed by Shipping.

### Base

![Sponza wall base](images/m8-sponza-wall-geology-base.png)

### Domain warp

![Sponza wall domain warp](images/m8-sponza-wall-geology-domain-warp.png)

### Macro

![Sponza wall macro geology](images/m8-sponza-wall-geology-macro.png)

### Meso/ridged

![Sponza wall meso ridged geology](images/m8-sponza-wall-geology-meso-ridged.png)

### Strata

![Sponza wall strata](images/m8-sponza-wall-geology-strata.png)

### Erosion

![Sponza wall erosion](images/m8-sponza-wall-geology-erosion.png)

### Combined rock wall

![Sponza wall combined geology](images/m8-sponza-wall-geology-combined.png)

Each capture passes the repository's structural image validator. Object
coverage is `0.098982..0.103866`, luminance range is `197..234`, and color
standard deviation is `30.917374..33.062125`. Compared with base, every
geology stage changes `0.102174..0.108008` of the full frame. Original-resolution
inspection confirms that strata and erosion affect shaded surface relief rather
than only color.

## Supporting topology

![Sponza wall base and combined topology](images/m8-sponza-wall-geology-topology.png)

The left view is base topology and the right view is combined topology. It
supports the Beauty proof by exposing the refined open surface and displaced
silhouette, but it is not used as the primary acceptance image.

## Cook, package and replay

```text
sponza.wall_geology.contract_pass
  accepted=1|rejections=5|stages=7
  source_immutable=true|deterministic=true

sponza.wall_geology.runtime_pass
  stages=7|beauty=7|source_free=true|vulkan=true
  negative_space=none

manifest SHA-256  70c76d8e97bf98b19d2964a79d63670d8996764181d10dec5905177c4de33de5
package SHA-256   b68e9fbb6e3659af38f1dc73091c5db74ff3b021a6201388325d09ac73836195
combined BMP SHA  af2359297a35b32a2aa985491f73840c59e873e59fae9ae4625655d342fc9f78
combined PNG SHA  da02738b131947372fa20ff458518ca592af2212f82fb8e3d0902a7847647ecd
package entries   11
```

The deterministic package contains seven `.rmesh` assets, the real Sponza
`.rmat` and its three `.rtext` dependencies. It passes Shipping validation and
is replayed after the loose cooked directory and provenance input are removed.
All seven runtime proofs report `backend=Vulkan`, `source_free=true` and no
`render.diligent.error/fatal`. The combined capture repeats byte-identically.

## Boundary of this PASS

This closes only the user-directed Sponza wall geology layer showcase. Geometry
Technology Showcase criteria 11-13 remain `PARTIAL`; the matrix remains
`10 PASS / 3 PARTIAL / 0 NOT STARTED`. The Persistent Goal remains `ACTIVE`.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
