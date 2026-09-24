# M8 Geological Material Integration Design

**Slice:** `M8-GEOLOGY-SYSTEM-GEOLOGICAL-MATERIAL-INTEGRATION`

**Status:** Selected on 2026-08-28 after the repository's 30-second recommended-option window.

## Observable outcome

One Vulkan-evaluated geological field must own all four stages of the product:

```text
SAME GEOLOGICAL FIELD
-> GEOMETRY
-> SURFACE CLASSIFICATION
-> MATERIAL APPEARANCE
```

The resulting `.rmesh` must retain smooth Surface Nets normals, compact
classification, deterministic identity and collision parity. JericoEditor and
runtime must render the cooked product through the existing Diligent static-mesh
path with world-space/triplanar PBR and no manual UV unwrap.

## Existing break in lineage

- `GeologyGpuSession` evaluates the scalar field and extracts unique Surface Nets
  vertices, normals and indices, but `GeologySurfaceMesh` carries no usable
  geological classification.
- `GeologyCook` accepts `HardSurfaceResult`, not `GeologySurfaceMesh`.
- The reused BSP cooker duplicates every triangle vertex, replaces smooth normals
  with face normals and creates dominant-axis projected UVs.
- JRMS 1.0 has only the ordinary 48-byte `MeshVertex`; semantic geology is lost.
- `Surface/PBR_Opaque` samples UV-space textures and has no geological binding.
- No productive `GeologySession` consumer exists. The generic cooked viewport is
  nevertheless reusable: `CookedViewportScene` loads `.rmesh` through the Asset
  Registry and uploads it through `createStaticMeshRenderAsset`.

## Selected representation

Keep the ordinary 48-byte `MeshVertex` and all ordinary JRMS 1.0 output unchanged.
Only a geological mesh carries an optional eight-byte semantic record per unique
vertex:

| Byte | Meaning | Quantization |
|---:|---|---|
| 0 | strata class | UNORM8 |
| 1 | strata/composed strength | UNORM8 |
| 2 | fracture response | UNORM8 |
| 3 | erosion response | UNORM8 |
| 4 | deposition response | UNORM8 |
| 5 | cavity response | UNORM8 |
| 6 | signed curvature | SNORM8 encoded into one byte |
| 7 | surface exposure plus `fresh` | exposure UNORM7, high bit is `fresh` |

Quantization clamps first and rounds to the nearest representable integer with a
fixed half-up rule. Disabled contributors write the exact identity value. Reserved
states are rejected, not guessed.

The GPU evaluator owns one in-place `uint2` classification volume. Existing layer
dispatches update the channels from the same samples that modify geometry; they do
not re-evaluate a second recipe. Surface Nets trilinearly samples the continuous
channels at its generated vertex, chooses strata class deterministically and emits
one `uint2` record beside each unique vertex. The buffer admission and diagnostics
include exactly eight bytes per voxel plus eight bytes per possible surface vertex.

For the published FoldDomain surfaces, direct unique-vertex cooking changes the
vertex payload from `triangleCount * 3 * 48` to `vertexCount * 56`. The existing X
proof therefore falls from 1,168,704 bytes to 227,472 bytes and the Y proof from
1,187,136 bytes to 230,944 bytes: about 80.5% less vertex payload while retaining
classification.

## Cooked format

A geological `.rmesh` uses JRMS minor version 1 and one optional geology chunk.
The chunk contains:

- schema version and exact vertex count;
- the packed semantic records;
- world-space origin/up frame, metres-per-tile and triplanar rotation;
- up to four existing PBR material GUIDs;
- deterministic channel-to-palette routing and thresholds;
- source field, final field and final surface identity hashes.

The ordinary writer continues to emit JRMS 1.0 byte-for-byte, including its golden
fixture. The reader accepts both minors, skips unknown optional chunks as before,
and rejects malformed geology counts, non-finite frames, invalid palettes and
non-zero reserved state. The geology chunk never contains recipes, source paths or
editor-only payloads.

`GeologyCook` consumes `GeologySurfaceMesh` directly and builds the existing
`CookedMeshAsset` without another mesh format or cooker. It preserves unique
positions, smooth normals, tangents and indices; base UV values are zero and unused.
Collision is generated from exactly those positions and indices and must prove
identity after immediate reread.

## Material and renderer integration

The integration is a permutation of the canonical surface PBR family, not a new
renderer or material database. A geological mesh causes the existing static-mesh
asset path to:

- upload the optional eight-byte stream as vertex slot 1;
- resolve the descriptor's GUIDs through the existing material/texture registry;
- select a geology permutation stored in the canonical PBR shader archive;
- retain the existing lighting, normal mapping, shadow and IBL implementation.

The shader selects one dominant material deterministically. Strata establishes the
base class; fracture, erosion, deposition and cavity may redirect to a configured
palette entry; curvature and exposure apply bounded factor modulation. Ties use a
fixed documented channel order. The first vertical samples base color, normal and
ORM for the selected material over three world-space axes: nine texture samples per
pixel. It does not blend four full materials, sample emissive/height maps or use the
base UV channel.

The `fresh` bit is part of the runtime stream and may be updated through the
existing static-mesh asset without re-extracting geometry. This slice proves that
the bit changes the bound material appearance. Producing newly exposed vertices is
owned by a later admitted runtime-modification/destruction slice.

## Editor and runtime path

No geology UI or VDB workflow is restored. The existing generic path remains:

```text
cooked package / level
-> AssetRegistry::loadMesh and material dependencies
-> CookedViewportScene::upload
-> RenderDevice::createStaticMeshRenderAsset
-> canonical Diligent PBR draw
```

The same upload contract is used by runtime tests and packaged Shipping replay.
This is sufficient for productive consumption of a cooked geology asset without a
bespoke editor generator or parallel scene system.

## Failure contracts

Stable errors cover classification-size mismatch, invalid codec state, invalid
descriptor frame, missing/duplicate palette GUIDs, unsupported PBR material modes,
stale field/surface hashes, renderer resource mismatch, collision divergence and
Shipping source leakage. No failure silently falls back to ordinary UV PBR.

## Explicitly deferred integration

The following remain `INTEGRATION REQUIRED` and are not silently considered done:

- stable fracture-cell ownership and structural sidecars;
- physical material binding for Jolt beyond collision geometry identity;
- geology-aware LOD and streaming;
- producer logic for newly exposed/runtime-modified surfaces;
- destruction, durability, rockfall and colluvium;
- gameplay queries, Luau and typed events;
- navigation, particles and audio consumers.

Completing this design does not complete P1, the Geology blueprint or the
JericoEngine 1.0 Persistent Goal.
