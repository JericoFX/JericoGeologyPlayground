# M8 SimpleBake 2.9.11 — Technical Research Input

**Milestone:** M8  
**Category:** texture baking / material flattening / high-to-low / UDIM / channel packing  
**Status:** source inspected; research input ready; native Jerico implementation not started  
**Owner timing:** read and act on this only when M8 is the legal `ACTIVE_SLICE`  
**Runtime dependency:** none intended  
**Source archive SHA-256:** `e63170de4be42b477aa1b2d9ce53de72163142062cd6d88272071b122d1c7754`

## 1. Package inventory

The supplied SimpleBake package reports version `2.9.11`.

Inspected package size:
- 93 files;
- 43 Python files;
- ~20,495 Python lines;
- additional `.blend`/resource files and cached Python artifacts.

Relevant modules include:

```text
object_preparation
image_management
material_management
external_save
UV management
channel_packing
cleanup
presets
auto_cage
AOV handling
preview
bake operators
common_bake_support
PBR bake support
```

The useful value for Jerico is the **orchestration architecture around baking**,
not merely a call to Blender's bake operator.

## 2. Why this belongs to M8

Generated/imported M8 assets need a reproducible route from authoring materials and
high-detail geometry to cooked runtime textures.

This includes:
- generated geology;
- imported environment props;
- high-to-low workflow;
- normal/AO/curvature/thickness masks;
- packed material channels;
- UDIM;
- decals;
- LOD material consistency.

SimpleBake is therefore useful as a reference for a Jerico bake pipeline even
though it does not primarily create geometry.

## 3. Inspected pipeline pattern

A mature bake operation is treated as a transaction:

```text
validate scene/request
-> record mutable state
-> identify source/target objects
-> prepare temporary objects/materials
-> prepare UV/image targets
-> configure selected-to-active / cage
-> normalize material graph for desired maps
-> execute one or more bake passes
-> post-process
-> channel-pack
-> external save
-> restore/cleanup temporary state
```

Jerico should preserve this transactional idea but remove dependency on Blender
state.

## 4. Native Jerico bake-job model

Recommended contract:

```text
BakeJob {
    source_revision
    source_meshes[]
    target_mesh
    projection_mode
    cage
    ray_distance
    uv_set
    udim_tiles[]
    output_maps[]
    resolution
    supersampling
    dilation
    color_space_policy
    channel_pack_recipe
    deterministic_seed
}
```

Execution result:

```text
BakeResult {
    images[]
    metrics
    warnings
    source_lineage
    job_hash
}
```

Never make the renderer/runtime reconstruct authoring materials to determine what
was baked.

## 5. Preflight / state isolation

The source goes to substantial effort to save and restore Blender state. Jerico can
avoid most of that fragility by making bake inputs immutable snapshots.

M8 requirements:
- validate mesh/UV/material revisions before execution;
- create temporary bake resources under job ownership;
- publish only after all requested outputs validate;
- cancellation destroys temporary results;
- failed jobs cannot mutate the source asset;
- job hash/cache includes every quality-affecting input.

## 6. Material graph normalization

The inspected PBR path can temporarily reinterpret several Blender shader types
through a normalized Principled/PBR representation.

The reusable idea is a **MaterialBakeIR**:

```text
MaterialBakeIR {
    base_color
    metallic
    roughness
    normal
    emission
    opacity
    ao
    displacement/height
    custom_aov[]
}
```

Jerico's importer/Blender adapter should convert external material graphs into this
IR before baking. The bake core then does not need special cases for every DCC node.

Do not copy Blender node semantics into Engine runtime.

## 7. Automatic map requirement analysis

The inspected add-on recursively examines shader graphs and only schedules maps
that are actually needed.

Jerico should provide a graph walker that determines:

```text
Constant
TextureDependent
GeometryDependent
ViewDependent / UnsupportedForOfflineBake
```

for each PBR output.

Examples:
- constant roughness does not need a texture unless policy requests one;
- linked roughness needs baking/transfer;
- normal/bump path needs tangent-space handling;
- emission only needs output when nonzero/linked;
- procedural graph requires flattening.

This reduces bake time and texture memory.

## 8. High-to-low / selected-to-active projection

A native high-to-low baker should formalize projection:

For each target texel sample:
1. reconstruct target triangle and barycentric position from UV rasterization;
2. derive target geometric/shading frame;
3. generate ray origin using bias/cage;
4. raycast into the high-detail source set;
5. choose hit according to front/back/max-distance policy;
6. evaluate source attributes/material;
7. encode requested output.

Required diagnostics:
- miss count/ratio;
- backface hits;
- max/mean projection distance;
- overlapping-candidate ambiguity;
- cage intersections.

## 9. Cage generation

The add-on can create a duplicate target cage and displace it outward.

Jerico should separate:
- authored cage;
- generated normal-offset cage;
- adaptive cage.

A simple generated cage starts with:

```text
p_cage = p_target + n * d
```

but production needs self-intersection checks and potentially a spatially varying
`d`.

An adaptive option can choose minimum safe offset from local source/target
proximity, constrained by max ray distance.

Cage quality must be measurable before the expensive bake starts.

## 10. Normal baking

Jerico must explicitly define tangent-space convention:
- tangent basis source;
- bitangent sign;
- handedness;
- UV set;
- texture Y convention;
- normalization/encoding.

The normal bake should calculate the high-detail normal in the target tangent basis:

```text
n_tangent = transpose(TBN_target) * n_high
encoded = n_tangent * 0.5 + 0.5
```

Exact matrix convention must match Jerico renderer tests.

This is directly useful to the previously discussed:
**dense mesh -> lighter mesh -> bake normals** workflow.

## 11. Specialist maps

The source package includes workflows for maps such as:
- AO;
- curvature;
- thickness;
- vertex color;
- color ID;
- lightmap-style outputs;
- custom node/AOV data.

Jerico should not depend on template `.blend` materials for these.

Native kernels:

### Ambient occlusion
Hemisphere ray integration:

```text
AO = 1 - visible_fraction
```

with distance/falloff policy.

### Curvature
Use mesh differential/normal variation or signed dihedral accumulation, with an
explicit scale/radius.

### Thickness
Reuse the M8 mesh-analysis ray thickness service where possible rather than
implementing a second incompatible thickness estimator.

### Color ID
Use stable semantic/material/region IDs, not random color identity as the canonical
data. Render a deterministic debug color only at export.

## 12. Channel packing

The inspected source uses NumPy arrays to read image channels and compose new RGBA
outputs. It also contains gamma-related transformations.

Jerico should make this safer and explicit.

Define each packed channel as:

```text
PackedChannel {
    source_image
    source_channel
    source_color_space
    invert
    scalar/remap
}
```

For typical linear data maps:

```text
R = AO
G = Roughness
B = Metallic
A = optional mask
```

Never apply `pow(x, 1/2.2)` by heuristic. Conversion must be driven by declared
color spaces:

```text
sRGB color -> linear before numeric processing
linear data map -> remains linear
display/export encoding -> explicit at file output
```

Normal/roughness/metal/AO are data, not color.

## 13. UDIM

The bake job should enumerate tiles explicitly.

Per tile:
- determine occupied target triangles;
- allocate requested image(s);
- rasterize only relevant geometry;
- preserve tile number;
- validate padding at tile edges;
- publish a tile manifest.

Cache granularity should allow rebaking one changed tile.

## 14. Padding / dilation

Every bake output needs edge dilation to avoid mip/linear-filter seams.

Possible native approach:
- build valid-texel mask;
- nearest-valid propagation / jump-flood;
- copy data outward `N` pixels;
- for normals, renormalize after filtering/propagation as required.

Margin must be specified in **pixels**, then converted based on each output
resolution.

## 15. Anti-aliasing / supersampling

Prefer deterministic supersampling in the rasterizer:
- NxN stratified samples or fixed pattern;
- resolve in linear space;
- special handling for normal vectors;
- ID maps use coverage-aware discrete policy, not numeric averaging.

A denoiser may be added for ray maps such as AO, but it must be optional and its
parameters included in the job hash.

## 16. Background jobs / progress / cancel

The source includes explicit background/progress management. Jerico M8 should make
baking a normal job-system citizen:

```text
Queued
Preparing
Rasterizing
RayTracing
PostProcessing
Encoding
ReadyForReview
Published
Cancelled / Failed
```

Report:
- map/pass;
- tile;
- samples completed;
- ETA only if robust;
- cancel latency;
- memory use.

No UI thread blocking.

## 17. Output formats and cook

Recommended distinction:

```text
authoring/intermediate images
  -> validated BakeResult
  -> channel policy
  -> compression/format selection
  -> Jerico texture cook
```

The bake service should not hard-code final GPU compression.

Output metadata should preserve:
- source job hash;
- map semantic;
- color space;
- normal convention;
- resolution;
- UDIM tile;
- channel packing recipe.

## 18. M8 integration with other research inputs

This work can share services with the other M8 addon studies:

- **Smart Segmentation:** semantic region IDs -> Color ID/material/bake masks;
- **Zen UV:** UV validation, texel density, pack/UDIM -> bake targets;
- **Smart Remesh:** low-poly result -> normal/AO/high-to-low bake;
- **Auto-Rig Pro:** skinned asset prep may need texture bake before final cook.

Avoid four separate implementations of mesh ray queries, UV metrics or job
orchestration.

## 19. Tests Jerico should require

### Projection
- plane high->low;
- sphere;
- intersecting source surfaces;
- cage too small / too large;
- deliberate miss zones.

### Normals
- tangent basis roundtrip;
- mirrored UV;
- negative object scale;
- known sloped high-poly surface;
- renderer reconstruction error.

### Channels/colors
- linear roughness remains unchanged;
- sRGB base color conversion;
- pack/invert/remap;
- no gamma on data maps.

### UDIM/padding
- multiple tiles;
- empty tiles;
- border padding;
- independent tile rebake.

### Transaction/cancel
- cancel each pipeline phase;
- source unchanged;
- stale source revision rejected;
- deterministic cache key.

## 20. License boundary

The current marketplace documentation lists SimpleBake as GPL.

For Jerico:
- do not copy its source into production;
- do not make Blender/SimpleBake a Shipping dependency;
- reimplement the bake orchestration and math from first principles;
- resource `.blend` files remain reference-only unless separately licensed and
  admitted.

Official reference:
- https://superhivemarket.com/products/simplebake---simple-pbr-and-other-baking-in-blender-2/docs

## 21. M8 decision

**HIGH-VALUE M8 ASSET-PIPELINE INPUT.**

The strongest concept to retain is:

```text
immutable bake job
-> material normalization
-> projection/ray kernels
-> specialist maps
-> postprocess
-> explicit channel/color-space packing
-> deterministic cook
```

That gives Jerico a Blender-addon-independent bake pipeline suitable for a future
open-source engine.
