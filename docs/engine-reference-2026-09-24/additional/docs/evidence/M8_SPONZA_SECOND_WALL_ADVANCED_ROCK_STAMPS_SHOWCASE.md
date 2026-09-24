# M8 Sponza Second Wall Advanced Rock Stamps Showcase

Date: 2026-08-24
Slice: `M8-SPONZA-SECOND-WALL-ADVANCED-ROCK-STAMPS-SHOWCASE`
State: `PASS`

## Result

A second real Sponza 2022 wall now demonstrates the existing Jerico-native
surface-stamp path independently from, and then combined with, a more advanced
rock-layer recipe. The aligned progression is base, one isolated stamp, three
accumulated stamps, advanced layers without stamps and combined. All five use
the same camera, real `brickwall_01` PBR material and lighting.

The primary evidence is five inspected original-resolution `1280x720` Vulkan
Beauty/final-shaded captures. The source is node `walls_floor_03`, primitive 3,
cooked submesh 349; it is not the first showcase's
`arch_wall_2ndfloor` primitive 2 / submesh 156. The selected product remains an
open Sponza stone wall with two authored openings and
`negative_space=none`—not a cave, tunnel, chamber or analytic box.

Read-only source, candidate, license, hash, format, channel, load-path, stamp
owner and noise-backend details are retained in
[`M8_SPONZA_SECOND_WALL_ROCK_STAMPS_SOURCE_INVENTORY.md`](M8_SPONZA_SECOND_WALL_ROCK_STAMPS_SOURCE_INVENTORY.md).

## RED first

Before the builder accepted the new mode, the focused verifier executed the
cancellation path and failed at the absent behavior:

```text
sponza.second_wall_rock_stamps.cancellation_failed
exit=2
Usage: JericoHighDetailSponzaPlaygroundBuilder ...
```

The accepted verifier now retains one positive deterministic A/B run and eight
negative mutations that reject:

```text
reuse of first wall submesh 156
missing stamp identity/provenance
color-only stamp output
identical stage geometry
missing required stage
camera/material/light drift
cave-like negative space
debug-only evidence
```

## Geometry, stamps and advanced recipe

```text
source glTF SHA-256  e04c4c540c74bdddcbd3f590a85c14119bfd5839702246a23b3a737c0cce2400
source node           walls_floor_03 (node 110, mesh 105)
source primitive      3
cooked submesh        349 / 405
source primitive      359 vertices / 1428 indices
axis selection        -X vertical face
selected face         24 vertices / 60 indices / 24 open boundary edges
refinement            adjustable 2..32; retained proof uses 12
derived mesh          8640 vertices / 8640 indices / 2880 triangles
material GUID         FE605B89A020E71D90C6BE6783BCC79C
seed                  0x53504F4E5A415342
preset GUID           F0F1F2F3F4F5F6F7F8F9FAFBFCFDFEFF
RBrush asset GUID     E0E1E2E3E4E5E6E7E8E9EAEBECEDEEEF
stamp owner           Jerico.ProceduralAuthoring.applySdfBrushStamp
stamp operation       SurfaceDisplace
negative space        none
```

The three ordered stamp operations have stable operation GUID, region GUID,
transform, displacement, mask derivation key, operation hash and content hash.
They modify 1041, 840 and 474 projected sparse-field samples respectively.
The advanced no-stamp recipe combines anisotropy, domain warp, macro FBM,
meso ridged noise, strata, shallow Voronoi/directional fractures, erosion and
three material layers. No negative SDF primitive is present.

| Stage | Stamps | Advanced layers | Max / mean displacement m | `.rmesh` SHA-256 | Beauty changed ratio |
|---|---:|---|---:|---|---:|
| base | 0 | no | 0.000000 / 0.000000 | `58b94612120b555a121577a281057d2a274efef86961bfeda7f75b03d7c49cf1` | 0.000000 |
| isolated stamp | 1 | no | 0.178314 / 0.006679 | `ebb07b7bf33406b3486668660882570e0a563dd33f9052709c1a7e6a1915abcc` | 0.045666 vs base |
| accumulated stamps | 3 | no | 0.178314 / 0.012759 | `7d3a19611d89b65110a70d96c058f3cf3e24ea80d596a249c1a4264b7c6f1a6f` | 0.110390 vs base |
| advanced layers | 0 | yes | 0.205249 / 0.061977 | `ce71a3f37c7d3815612b43fdb41c2645e3915beb77593bddc982e43690c0cedc` | 0.274589 vs base |
| combined | 3 | yes | 0.207705 / 0.062329 | `8b15477a486d72ac8f609b7d4ca74dd3fcbefa1fcd72cf97c88fc2f570cd9197` | 0.274831 vs base; 0.104895 vs advanced |

All five position identities and mesh byte identities are unique. A clean A/B
generation byte-compares every mesh, the manifest and topology exactly.
Preview does not mutate accepted state, cancellation leaves zero outputs, and
undo/redo restore exact byte identities.

Subdivision is an explicit deterministic input, rather than a hard-coded mesh
property. The retained Beauty proof uses 12 for legible relief; the behavioral
gate generates and validates 2, 12 and 32, proves that 2 and 32 differ, and
rejects 1 and 33 without publication. This slice does not claim meshoptimizer
admission or a new node editor. The existing inspector already exposes numeric
geology controls and an ordered recipe stack; a later UX slice may present
quick sliders/presets and an
advanced node/stack view over that same recipe without creating two authorities.

## Beauty/final-shaded proof

Fixed render identity:

```text
backend              Vulkan, validation enabled, NVIDIA GeForce RTX 3090
resolution           1280x720
camera               x=0.25, z=4.8, yaw=0, pitch=0, vertical FOV=48
world                 scale=0.80, yaw=-25, y=0
material             FE605B89A020E71D90C6BE6783BCC79C
light                 same directional + raking point key for all five stages
directional shadows  High, Beauty debug=None
```

### Base

![Second Sponza wall base](images/m8-sponza-second-wall-rock-stamps-base.png)

### Isolated Jerico-native stamp

![Second Sponza wall isolated stamp](images/m8-sponza-second-wall-rock-stamps-isolated-stamp.png)

### Three accumulated stamps

![Second Sponza wall accumulated stamps](images/m8-sponza-second-wall-rock-stamps-accumulated-stamps.png)

### Advanced rock layers without stamps

![Second Sponza wall advanced layers](images/m8-sponza-second-wall-rock-stamps-advanced-layers.png)

### Combined layers and stamps

![Second Sponza wall combined result](images/m8-sponza-second-wall-rock-stamps-combined.png)

Every non-base capture passes the structural image validator. Object coverage
is `0.268141..0.273173`, luminance range is `208..232`, and color standard
deviation is `40.203556..42.100144`. Original-resolution inspection confirms
real shaded relief and silhouette movement while preserving the recognizable
wall and its openings. The combined-vs-advanced `0.104895` ratio proves that
the stamps remain visible after the advanced layers are applied.

## Supporting topology

![Second Sponza wall base and combined topology](images/m8-sponza-second-wall-rock-stamps-topology.png)

The left half is base topology and the right half is combined topology. It
supports the Beauty proof and demonstrates a displaced open wall surface; it
is not the primary acceptance view.

## Cook, package and source-free replay

```text
sponza.second_wall_rock_stamps.contract_pass
  accepted=1|rejections=8|stages=5|subdivisions=2,12,32|limit_rejections=2
  source_immutable=true|deterministic=true

sponza.second_wall_rock_stamps.runtime_pass
  stages=5|beauty=5|source_free=true|vulkan=true
  negative_space=none

manifest SHA-256  e11ed5fb8c94740367e07dbd21ff66bac0dab39313634150e5b6454aaf5c4d28
package SHA-256   9cf92bf30fbce7f0e45e7d79dbef94c47a105f78cf20fc169c15d747da935d6d
combined BMP SHA  a52c0f5af47ad27bd6fae44f73d842bf43a1477644cc04e71ce69292b3a7e94c
combined PNG SHA  f43e4e3b38fdba5b70da6a618d0b809b869eb046646475f03bf2d91a5b5aadbb
package entries   9
```

The deterministic package contains the five `.rmesh` stages, the real Sponza
`.rmat` and its three `.rtext` dependencies. It passes Shipping validation and
is replayed only after loose cooked inputs and provenance are removed. Every
runtime proof reports `backend=Vulkan`, `source_free=true`, the real material
pipeline and non-empty geometry. The combined capture repeats byte-identically.

## Boundary of this PASS

This closes only the direct second-wall advanced rock/stamp showcase. Geometry
Technology Showcase criteria 11-13 remain `PARTIAL`; the matrix remains
`10 PASS / 3 PARTIAL / 0 NOT STARTED`. FastNoise2, Delabella, meshoptimizer,
quick-preset UX and a visual geology node graph remain outside this slice and
are not implied complete.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
