# M8 Geology Blueprint P1 — Gradient-Based Implicit Detail Warp Closure

Date: 2026-08-28
Slice: `M8-GEOLOGY-BLUEPRINT-P1-GRADIENT-BASED-IMPLICIT-DETAIL-WARP`
Local status: `PASS`

This closes only the bounded Gradient-Based Implicit Detail Warp operator. It
does not complete P1, the Geology blueprint, the later PWAS/EPLS production
expansion or JericoEngine 1.0.

## Result

Jerico now has one typed `GradientDetailWarp` geology layer in schema 16. The
operator reuses the existing recipe, deterministic fBm, resident Vulkan
ping-pong field evaluator, mask/strength/seed identity, dirty tracking, Surface
Nets, classified cook, collision and geological PBR material path. It does not
introduce a second noise, gradient, SDF, mesher, material or renderer system.

The accepted composition is:

```text
BSP Source
-> existing IrregularPolyhedral ImplicitBlock
-> GradientDetailWarp
-> classified Surface Nets
-> JRMS 1.1 GEOL + exact triangle collision
-> eight-entry source-free package
-> existing Diligent/Vulkan world/triplanar PBR renderer
```

The input block remains closed and recognizable. The detail layer changes the
field and silhouette without removing input occupancy: with Jerico's
negative-inside convention the final value is `min(source, candidate)`. This
composition rule prevents the operator from creating holes in the admitted
parameter domain. The output is an occupancy-preserving scalar field, not a
claim of exact signed-distance magnitude.

## Research and provenance boundary

Research preceded implementation and is recorded in
`M8_GEOLOGY_BLUEPRINT_P1_GRADIENT_DETAIL_WARP_RESEARCH.md`. The sources were:

- the official Paris et al. project page;
- the official paper/DOI;
- the public MIT `Rock-fracturing` repository at upstream commit
  `b91965b3011ed269cbc3a051b00c9b284aaa2e36`;
- the author's later explanatory post about implicit detail.

The reference implementation was inspected only to understand the problem,
data flow and limits. No source, shader, API or implementation was copied,
translated or ported. Jerico's implementation was designed independently around
its existing negative-inside field, typed recipe, fBm, GPU dispatch, masks,
bounded topology plan and Surface Nets contracts.

## Frozen contract

The layer payload is:

```text
maximumDisplacementMeters
surfaceBandMeters
frequencyCyclesPerMeter
octaves
lacunarity
gain
```

The evaluator:

1. estimates the input gradient with central differences at one voxel;
2. normalizes it in world space;
3. transforms the position and normal into the layer's local domain;
4. evaluates existing seeded fBm with local triplanar weights;
5. displaces the sample point inward along the input normal;
6. samples the existing field at that point;
7. applies a cubic surface-band window and the existing strength/mask weight;
8. composes with `min(source, candidate)`.

Validation rejects non-finite/degenerate transforms, a band narrower than two
voxels, displacement greater than half the band and frequencies above the
declared voxel Nyquist ceiling. Disabled, zero-strength and zero-displacement
states are exact input identity. A canonical all-zero RBrush mask is also exact
input identity in the admitted representative topology.

## RED chronology

The slice began with observable failures before implementation:

1. the CPU reference header did not exist;
2. schema 16, kind 20 and the typed payload were absent;
3. the topology plan did not include bounded detail displacement;
4. the real RTX 3090 Vulkan gate rejected the unsupported GPU layer.

During final composition hardening, two further rejected states were retained as
engineering evidence rather than hidden:

- non-grid-aligned inspection points produced a false 5.16 cm CPU/GPU parity
  deviation because the GPU inspector returns quantized voxel witnesses; the
  physical tolerance stayed at `1e-4 m` and the witnesses were aligned to the
  declared 12 cm grid;
- a 16 cm representative displacement crossed a topology cell and caused an
  all-zero mask to lose exact field-hash identity; that fixture was rejected and
  the accepted 12 cm maximum (`10.8 cm` after strength) stays below one voxel.

## Productive GPU and topology proof

The accepted Release run on Diligent/Vulkan and an NVIDIA GeForce RTX 3090
reported:

```text
implicit_block_field_hash=13561244679937275703
detail_field_hash=1812677832387639658
mesh_hash=1812677832387639658
vertices=1710
triangles=3416
parity_max_m=5.07839e-08
gpu_ns=84544
seed_dirty_voxels=11745
localized_dirty_voxels=186
```

The focused gate proves:

- CPU/GPU witnesses stay below `1e-4 m` and the observed maximum is
  `5.07839e-08 m`;
- every inspected output is negative-inside monotonic relative to its admitted
  ImplicitBlock input;
- structural scalar, mask and cell identity pass through unchanged;
- repeated field, mesh and classification hashes are exact and repeated dirty
  work is zero;
- disabled, zero-strength, zero-displacement and all-zero-mask states are exact
  input identity;
- seed and localized RBrush changes produce non-zero bounded dirty work;
- the Surface Nets result is a closed two-manifold by exact edge incidence;
- bounds change stays within displacement plus one voxel;
- immediate cancellation reports the existing stable cancellation error.

## Cook, package and Beauty

The accepted cooked JRMS 1.1 mesh contains:

```text
vertices=1710
indices=10248
geologyVertices=1710
sourceHash=0xFB0C1E520D1B89F3
recipeHash=0xE3E0175A6982BCF3
vertexHash=0xCF6E418700AE6B1A
indexHash=0x1F07420F935FAC31
contentHash=0x3F9BEC5207FFCD98
bytes=137530
sha256=091A7FE54FEA9918C8568CD8E178C621793A8D9283832C9D175BB66AA43ADC30
```

The authoritative detail package is 11,794,284 bytes with SHA-256
`9F1444DA3E2C2A1E3BD46F9A62BF56C693753A8A963FB4AF91198A0D598B520A`.
It contains exactly one classified mesh, four already-published geology PBR
materials and three already-published cooked Poly Haven `rock_surface` textures.
Shipping validation reported:

```text
asset_dump.shipping_valid|entries=8|no_abr=true|no_brush_source_images=true|no_editor_rbrush=true|no_editorassets=true
```

Two consecutive isolated package-only replays produced byte-identical original
960x720 captures with SHA-256
`DB1D4E437309C53BA7DB6EA504A9D2DEF3EA84143DBF3A8ED0D1F3E8F5C85D01`.
Both reported one draw, four materials, three textures, zero fallbacks,
`source_free=true`, `gpu_available=true` and approximately `0.118 ms` GPU time.

The comparison input is the exact preceding IrregularPolyhedral ImplicitBlock,
packaged with the same material/camera/light contract. It contains 1,540
vertices and 9,228 indices; its Beauty SHA-256 is
`5FC9C142969EFEC342E128D0C93014B7F703286D6C8E5E08B807696E22B02954`.
Visual inspection of the paired grazing-light captures confirms that the block
mass and characteristic cut remain recognizable while the detail result adds
bounded surface/silhouette variation.

Stable evidence:

| Capture | SHA-256 |
|---|---|
| `images/m8-geology-blueprint-p1-gradient-detail-warp/implicit-block-input-shipping-beauty-960x720.bmp` | `5FC9C142969EFEC342E128D0C93014B7F703286D6C8E5E08B807696E22B02954` |
| `images/m8-geology-blueprint-p1-gradient-detail-warp/gradient-detail-warp-shipping-beauty-960x720.bmp` | `DB1D4E437309C53BA7DB6EA504A9D2DEF3EA84143DBF3A8ED0D1F3E8F5C85D01` |

## Focused verification

Fresh exit-code-zero gates:

- Debug and Release `JericoGeologyRecipeTests`;
- Debug and Release `JericoGeologyGpuTopologyPlanTests`;
- Debug and Release `JericoGeologyGpuSessionTests --gradient-detail-warp`;
- Debug cook of both the input and detailed classified products;
- JRMS semantic reread and exact triangle collision cook;
- authoritative PackageTool build;
- detail and comparison-input `AssetDump --shipping-validate`;
- two consecutive detail package-only Release Vulkan Beauty replays;
- `git diff --check` before publication.

The new focused CTest registration is present. Full CMake regeneration remains
blocked by preserved foreign/global configuration defects (`Qt6::Core` and
`Jerico::EditorProjectAuthority` targets unavailable), so the configured focused
MSBuild projects were used. This slice does not alter or absorb those blockers.

## End-to-end completeness result

Applicable and proved here:

- source/input: typed BSP + ImplicitBlock + GradientDetailWarp recipe;
- CPU/GPU processing: one deterministic reference/evaluator contract;
- serialization: schema 16 round-trip and validation;
- masks/cancellation/cache/dirty work: existing mechanisms deepened, not cloned;
- topology: bounded admission plus closed classified Surface Nets;
- asset/collision/material/rendering: existing cook and PBR lineage preserved;
- debugging/profiling/testing: field inspections, stable errors, GPU duration,
  dirty metrics and paired Beauty evidence;
- Shipping: one source-free package with no fallback or editor/source payload.

Not applicable to this static scalar operator: animation and Sequencer.

Still `INTEGRATION REQUIRED` under their existing owners and milestones:

1. semantic geology mask visualization/editor UI;
2. physical geology properties in Jolt/contact queries;
3. classified LOD reduction and geology-aware world-cell streaming;
4. runtime remesh/destruction/save-load transactions and fresh-surface events;
5. navigation, gameplay/Luau, particles and audio query consumers;
6. the remaining P1 operators (`DirectedCrack`, durability, rockfall, colluvium
   and imported-mesh structural operators);
7. all eight P2/research features in blueprint section 89.

The later PWAS/EPLS/Animation/Destruction production expansion remains forbidden
until all remaining Geology blueprint work is implemented, published and closed.

## Boundary

No DirectedCrack, later P1/P2 operator, OpenVDB/UI restoration, bespoke C++
generator/preset, M9, Task23 or production-expansion work was implemented.

The next legal Geology slice is `M8-GEOLOGY-BLUEPRINT-P1-DIRECTED-CRACK-SWEPT-VOLUME`
in exactly one new top-level successor after this slice is committed, pushed and
ownership is explicitly transferred.

Persistent Goal: `ACTIVE until full JericoEngine 1.0 closure`
