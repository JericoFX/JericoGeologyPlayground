# M8 Sparse SDF Rock Family Showcase

Date: 2026-08-23
Slice: `M8-SPARSE-SDF-ROCK-FAMILY-SHOWCASE`
State: `PASS`

## Result

Criterion 2 of the independent M8 Geometry Technology Showcase gate is closed.
The retained family contains four deterministic, asymmetrical Sparse SDF rock
forms with different width/depth/height profiles, flat placement contacts and a
real RBrush Crack stamp. The final Editor contact view is the visual variation
proof; the collision/debug view and package-only replay are supporting product
evidence, not substitutes for it.

This closes only the rock-family criterion. Sparse SDF cliffs, caves and the
BSP/SDF/Terrain transition remain outside this slice.

## Product path

```text
controlled family seed + four bounded profiles
-> existing RBrush rock brush-set resolver (feature=Crack)
-> canonical RBrush 2D SDF mask
-> SparseSdfSession + real crack Preview/Apply
-> SparseSdfBuilder / Marching Cubes / transition ownership
-> SdfCookCandidate
-> .rmesh + exact StaticTriangleMesh .rcoll
-> real PBR_Opaque material + TextureCook BC7/BC5 textures
-> 10-entry .epak
-> JericoEditor Vulkan views
-> isolated package-only Vulkan replay twice
```

The family seed is `0x524F434B46414D31`; the RBrush selection seed is
`0x435241434B534554`. The four requested profiles are:

| Member | Width x depth x height (m) | Asymmetry |
|---:|---:|---:|
| 0 | `2.60 x 1.50 x 1.15` | `-0.55, +0.20` |
| 1 | `1.45 x 1.35 x 2.25` | `+0.25, -0.45` |
| 2 | `2.15 x 2.05 x 1.65` | `+0.62, +0.52` |
| 3 | `1.85 x 2.65 x 1.35` | `-0.18, -0.68` |

## Variation and editability

The first focused RED constructs four copies of one asymmetric mesh with only
translation, rotation and uniform scale differences. The transform-invariant
shape histogram rejects them with:

```text
editor.sparse_sdf.rock_family.indistinguishable
```

The accepted family has minimum pairwise histogram distance `0.137500`, above
the gate threshold `0.04`. Two independent generations produce the same family
hash, member hashes and byte-identical mesh/collision products.

The meaningful edit changes one member's bounded subtract operation:

```text
Preview discarded -> accepted state unchanged
Preview repeated  -> identical afterHash
Apply             -> edited member changes
Owned output      -> another family member remains unchanged
Undo              -> exact prior content hash
Redo              -> exact applied content hash
```

All four accepted members have a real crack displacement and a flat contact
region:

| Member | Content hash | Crack-changed samples | Contact vertices |
|---:|---:|---:|---:|
| 0 | `67CB3D95736B9EA2` | 19 | 65 |
| 1 | `2ACA6B195DB30E8D` | 47 | 82 |
| 2 | `000F00B4570AFF43` | 60 | 78 |
| 3 | `19AF04269C6FD714` | 47 | 75 |

## Original-resolution visual review

| Capture | Native size | SHA-256 | Accepted use |
|---|---:|---|---|
| [`m8-sparse-sdf-rock-family-final-shaded.bmp`](images/m8-sparse-sdf-rock-family-final-shaded.bmp) | 604x890 | `20BE82D7FBB115BC14DA305D364B00BFD52331B290901FB8D50BC2BF3524C57C` | Primary final-shaded contact view: four recognizably different silhouettes and flat placement contacts. |
| [`m8-sparse-sdf-rock-family-collision-debug.bmp`](images/m8-sparse-sdf-rock-family-collision-debug.bmp) | 604x890 | `1298C20D8186F0E3221BE0FBF261EC10CFF4FA2AC728ABBCB1AF8299B813D587` | Collision/topology inspection. Each separated orange triangle is derived from the exact packaged collision triangle mesh. |
| [`m8-sparse-sdf-rock-family-shipping.bmp`](images/m8-sparse-sdf-rock-family-shipping.bmp) | 1280x720 | `DB88CE56BCA81BCE8B581FF8804409DED729D59D0C9CEB6B35497252430AD1D3` | Isolated package-only Vulkan replay. Perspective occlusion makes this supporting Shipping evidence, not the variation contact sheet. |

All three captures were reopened at original resolution after the final passing
run. No conceptual image, empty frame, parser output or field-only visualization
received criterion credit.

## Cook, material, collision and Shipping

- Combined contact mesh: 15,624 vertices, 15,624 indices, 5,208 triangles,
  812,910 bytes.
- Collision: exact vertex/index correspondence with the accepted mesh,
  `StaticTriangleMesh`, 250,227 bytes.
- Material: real `Surface/PBR_Opaque` pipeline, BC7 BaseColor, BC7 ORM, BC5
  TangentNormal and BC7 Emissive cooked by the admitted TextureCook route.
- Package: 10 entries, 3,891,759 bytes, SHA-256
  `855440311B15BDC5646B0AD26FDE83659597392BDFBD7559E3B88E512FE200CE`.
- Inventory contains mesh, material, four rock textures, collision and level;
  `AssetDump --shipping-validate` passes and reports no authoring/source format.
- Runtime reports `textures=4`, `fallbacks=0`, `source_free=true`.
- Two isolated Vulkan replays are byte-identical with zero differing pixels.

## Representative metrics

| Metric | Result |
|---|---:|
| Family generation plus member cook | 2,865 ms |
| Active replacement bricks | 90 |
| Retained sparse field bytes | 1,584,128 |
| Runtime draw calls | 1 |
| Runtime triangles | 5,208 |
| Runtime proof frame | 7.304 ms |
| FXAA GPU p95 | 0.016 ms over 120 valid samples |
| Proof peak working set | 204,193,792 bytes |
| Package replay | Vulkan, 1280x720, source-free |

The generic proof did not expose a reliable total GPU frame duration for this
run (`gpu_available=false`), so none is claimed. No separate LOD derivative is
claimed: the current admitted Sparse SDF path supplies the final mesh derivative
and exact collision, while the general LOD/remesh pipeline remains a later
capability.

## Verification

```text
editor.sparse_sdf_rock_family             PASS 1/1
editor.sparse_sdf_rock_family_showcase    PASS 1/1, 50.66 s
JericoEditor executable                   real deployed Debug executable
Editor backend                            Vulkan, embedded viewport
Vulkan validation/errors                  none detected
AssetDump Shipping validation             PASS
Package-only replay                       PASS x2
Replay capture comparison                 byte-identical, 0 differing pixels
Test inventory                            345 total / 92 scheduled
```

## Closure

```text
Criterion 2 Sparse SDF rock variation = PASS
M8 Geometry Technology Showcase       = PARTIAL
M8                                    = ACTIVE / PARTIAL
M9                                    = BLOCKED
Persistent Goal                       = ACTIVE
```

## Criterion 10 regeneration reconciliation

The original values above remain the historical publication snapshot. The
criterion-10 audit found that they did not declare build/compiler identity and
that a later cave change had replaced the accepted `SparseSdfSession` output
with a second geology generation. The bounded repair restores the accepted
owner and pins the canonical fresh `Release` / MSVC `19.44.35228.0` identities:
family `88BC36BC4F0758DF`, mesh `F817068B72BB58D8`, collision
`0855C446878F1CC0`, package SHA-256
`5E5A6430D6BE26D11AF3C87ABB55B1A2A8F4949CD6E252A72FC5B782CF800C4C`.
Two clean generations are byte-identical and the broad structural wrapper
passes without weakening its threshold. See
[`M8_GEOMETRY_DETERMINISTIC_REGENERATION_CLOSURE.md`](M8_GEOMETRY_DETERMINISTIC_REGENERATION_CLOSURE.md).
