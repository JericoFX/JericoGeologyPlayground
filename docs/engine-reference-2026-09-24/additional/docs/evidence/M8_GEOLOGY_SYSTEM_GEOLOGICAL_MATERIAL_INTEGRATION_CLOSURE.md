# M8 Geological Material Integration Closure

**Slice:** `M8-GEOLOGY-SYSTEM-GEOLOGICAL-MATERIAL-INTEGRATION`

**Local status:** `PASS`

**Date:** 2026-08-28

**Backend/device:** Diligent/Vulkan, NVIDIA GeForce RTX 3090

This closure is limited to the static Geological Material vertical. It does not
complete P1, the Geology blueprint, the later production expansion, or the
JericoEngine 1.0 Persistent Goal.

## Result

The same admitted geology evaluation now owns the complete product lineage:

```text
BSP document + typed geology recipe
-> resident Vulkan scalar field + packed geological classification field
-> Surface Nets geometry + per-vertex classification
-> GeologySurfaceMesh
-> canonical .rmesh 1.1 GEOL chunk + exact triangle collision
-> AssetRegistry / package dependencies
-> existing Diligent static-mesh renderer
-> canonical PBR geology permutation
-> world-space triplanar material appearance
```

No parallel geology evaluator, mesher, mesh format, material database, shader
family, renderer, editor workflow or proof framework was added.

## Selected representation and measured cost

Ordinary meshes retain the existing 48-byte `MeshVertex` and continue to write
JRMS 1.0 byte-for-byte. Geological meshes use JRMS 1.1 and one optional `GEOL`
chunk containing an eight-byte classification per unique vertex plus one bounded
descriptor and a palette of at most four existing PBR material GUIDs.

The representative fractured/weathered boulder produced:

| Measurement | Result |
|---|---:|
| Unique vertices | 7,076 |
| Indices | 41,544 |
| Classification bytes | 56,608 |
| Ordinary `.rmesh` comparison | 506,398 bytes |
| Geological `.rmesh` | 563,210 bytes |
| File overhead | 56,812 bytes, 11.22% |
| Ordinary vertex fetch | 48 bytes/vertex |
| Geological vertex fetch | 56 bytes/vertex, 16.67% increase |
| Vertex buffer | 339,648 bytes |
| Index buffer | 166,176 bytes |
| Classification buffer | 56,608 bytes |
| Total geometry device bytes | 562,432 bytes |
| Device-buffer overhead | 56,608 bytes, 11.19% |
| PBR texture samples | 9/pixel: base color, normal and ORM over three axes |
| Focused static PBR GPU metric | 0.131072 ms for one Release measured frame |

The optional stream is smaller and more deterministic than shader-side replay of
the full authoring recipe. History-dependent fracture, erosion, deposition,
cavity and exposure witnesses survive cook; normal and world position stay in the
ordinary vertex; slope/height may be reconstructed from those values; palette and
descriptor identity replace source/editor recipes at runtime.

## Semantic contract

The eight-byte record carries:

- strata class and composed strata strength;
- fracture, erosion, deposition and cavity responses;
- signed curvature;
- surface exposure plus one `fresh` bit.

Values use deterministic UNORM/SNORM quantization with exact disabled identity.
Malformed reserved state is rejected. The GPU layer dispatches update the same
classification field while modifying the scalar field; Surface Nets samples that
classification at emitted vertices. The lineage test proves repeated canonical
classification equality and a distinct disabled structural/weathering result.

The productive cook reported:

```text
source_hash=17187918773788934726
recipe_hash=3273502845010353272
final_field_hash=13911905980116102005
surface_hash=13911905980116102005
vertices=7076
indices=41544
classification_bytes=56608
mesh_bytes=563210
collision_bytes=251346
```

Collision is generated from the same positions and indices as the rendered
surface. The cook gate proves immediate mesh/collision reread and deterministic
bytes without an intermediate duplicated/flat-normal BSP render mesh.

## Renderer and material result

The existing static-mesh upload path recognizes optional geological data, binds
the eight-byte stream as vertex slot 1 and resolves all palette GUID dependencies
through the ordinary Asset Registry. The canonical `PBR_Opaque` archive contains
the geology permutation; the ordinary pipeline remains unchanged.

The shader uses world position and the cooked world frame for triplanar mapping,
selects one palette material deterministically, and samples base color, normal and
ORM on three axes. It does not use the mesh UV channel, manual unwrap, emissive,
height maps or unrelated decorative geology noise. Existing direct lighting,
normal mapping, shadow and IBL paths remain shared.

The runtime `fresh` update replaces the classification generation without
re-extracting geometry and changes the admitted material response. Producing new
topology or newly exposed vertices remains outside this slice.

## RED and GREEN evidence

The change started with failing semantic codec/lineage, JRMS, cook, renderer and
Shipping contracts. Two downstream failures were especially useful:

1. Source-free replay initially reported one fallback because the ordinary PBR
   dependency audit required emissive for geology palette materials although the
   geology permutation never binds emissive. The focused RED asserted
   `fallbackTextureCount == 0`; the production fix excludes only that inapplicable
   geology-palette dependency and the same gate is GREEN with `fallbacks=0`.
2. The first package validation rejected `generated-geology.rmesh` with
   `mesh_contract` because mesh dependency enumeration included only the base
   material. Asset Registry, AssetDump and cooked-directory verification now use
   the same bounded palette dependency contract; the eight-entry package validates.

During final verification a newly rebuilt test briefly linked an older configured
GPU library. Rebuilding the owning GPU target exposed the real current code and
all `full`, `repeated` and `disabled` evaluations completed with `success=true`.
No production change was made for that build-tree mismatch.

## Focused verification

Fresh final runs produced exit code zero for:

- `JericoMeshAssetTests`;
- `JericoSdfCookTests`, including `GeologyCookTests`;
- `JericoGeologyGpuSessionTests --material-lineage`;
- `JericoGeologyGpuSessionTests --material-lineage-cook`;
- `JericoStaticMeshRenderAssetTests --geology-only`;
- `JericoStaticMeshRenderAssetTests --geology-generated`;
- `JericoPackageOverlayTests`, including the geology Shipping boundaries;
- `JericoAssetDump --shipping-validate geology-material.epak`;
- two consecutive isolated `JericoCookedAssetProof` package replays plus a fresh
  post-build Release replay with identical Beauty hashes.

The final generated-render gate reported:

```text
geology_material_renderer.loaded|vertices=7076|indices=41544|ordinary_mesh_bytes=506398|geology_mesh_bytes=563210|vertex_bytes=339648|index_bytes=166176|classification_bytes=56608|geometry_device_bytes=562432|source_field_hash=17187918773788934726|final_field_hash=13911905980116102005|surface_hash=13911905980116102005
geology_material_renderer.gpu|metric=single_frame_static_pbr_gpu_ms|milliseconds=0.131072
```

## Source-free Shipping proof

The isolated replay directory contains only:

```text
Packages/geology-material.epak
```

The package is 12,219,676 bytes with SHA-256
`8BFF51147978179C1A1F8D418EA9CDD9C343454E19E4ECD8980539338890F16F`.
It contains exactly one `.rmesh`, four ordinary `.rmat` palette entries and three
`.rtext` resources. AssetDump reported:

```text
asset_dump.shipping_valid|entries=8|no_abr=true|no_brush_source_images=true|no_editor_rbrush=true|no_editorassets=true
```

Two consecutive package-only Vulkan Beauty replays and a fresh post-build Release
replay produced the identical image hash below. The Release replay reported 7,076
vertices, 41,544 indices, one draw, four materials, three textures, zero
fallbacks, one point light, `source_free=true`, `gpu_available=true` and
`gpu_ms=0.135`.

One preceding cold capture returned only the background even though the CLI
reported success. It is rejected evidence and demonstrates that the proof CLI's
success record alone is not Beauty acceptance. The two visually inspected,
byte-identical repetitions below are the accepted evidence; the cold-capture
validation weakness remains a proof-harness limitation, not a material fallback.

## Poly Haven provenance

The Shipping Beauty uses the Poly Haven `rock_surface` asset by Amal Kumar under
CC0:

- asset page: <https://polyhaven.com/a/rock_surface>;
- base color:
  <https://dl.polyhaven.org/file/ph-assets/Textures/jpg/1k/rock_surface/rock_surface_diff_1k.jpg>,
  SHA-256 `07EDDA2BBE4715B01AAE2E4BAC9F21F204CF4AAE37B4733EF01DE33281E5E60E`;
- OpenGL normal:
  <https://dl.polyhaven.org/file/ph-assets/Textures/jpg/1k/rock_surface/rock_surface_nor_gl_1k.jpg>,
  SHA-256 `C645AF7F6A20BF77421765E6F998873834821E1950A3A0A21C5EE9CED32CECC1`;
- packed ARM/ORM source:
  <https://dl.polyhaven.org/file/ph-assets/Textures/jpg/1k/rock_surface/rock_surface_arm_1k.jpg>,
  SHA-256 `8DE45A3AA0BE70F8BAF5C609603BB6BBA0274EE4EB8C8E47DAD455258D5D7F2D`.

The downloaded source images remain ignored build inputs. Only Jerico-cooked
`.rtext` products are present in the Shipping package.

## Visual evidence

All images are original 960x720 BMP captures:

| Evidence | SHA-256 |
|---|---|
| `images/m8-geology-system-geological-material-integration/geological-material-weathered-960x720.bmp` | `ADF2010D163F0EFF277EDA382CF97C0DB775431924D17495639EEF8F584A190B` |
| `images/m8-geology-system-geological-material-integration/geological-material-fresh-960x720.bmp` | `E92A2F499568209FB75A7173E5264C58905B2E87AFFC39231AB442B8BC494412` |
| `images/m8-geology-system-geological-material-integration/geological-material-fresh-ibl-shadow-960x720.bmp` | `134DBB2D026373347D2FCBEA25983E65755ED745718074B5ACF91E77EF3084E1` |
| `images/m8-geology-system-geological-material-integration/geological-material-polyhaven-shipping-960x720.bmp` | `223CAE3C8308E7D5FC7E609F9F0EF8EFF843109DEC469EBE849C12E14D004EFA` |

The direct captures prove classification and fresh/IBL/shadow response. The
package-only Poly Haven capture proves real triplanar PBR texture detail and
visible class-dependent appearance without source access or UV unwrap.

## End-to-end completeness re-audit

| Category | Result after this slice |
|---|---|
| Source/input data | `PASS` for BSP plus canonical typed recipe. |
| Asset pipeline | `PASS` for classified Surface Nets cook, `.rmesh`, collision, palette dependencies and package. |
| Editor/authoring workflow | `PASS` for the existing generic cooked viewport consumer; semantic mask UI remains `INTEGRATION REQUIRED`. |
| ECS/components | `PASS` for static `MeshComponent` consumption without a geology-specific component. |
| Runtime representation | `PASS` for optional bounded JRMS 1.1 `GEOL`; ordinary JRMS 1.0 is unchanged. |
| CPU processing | `PASS` for codec/reference validation and cook. |
| GPU processing | `PASS` for same-dispatch classification, Surface Nets emission and repeated identity. |
| Renderer | `PASS` through the existing Diligent static-mesh path. |
| Shaders | `PASS` for canonical world/triplanar PBR geology permutation. |
| Materials | `PASS` for bounded palette of existing PBR materials and zero fallback. |
| Physics/collision | Geometry identity `PASS`; Jolt physical properties are `INTEGRATION REQUIRED`. |
| Animation | `NOT APPLICABLE` to this static material vertical. |
| Scripting/Luau | `INTEGRATION REQUIRED` for bounded geology queries, not recipes. |
| Events | `INTEGRATION REQUIRED` with the later runtime-modification owner. |
| Sequencer | `NOT APPLICABLE` until geological parameters are authorized as animated state. |
| Serialization | `PASS` for deterministic static products and rejection contracts. |
| Save/load | Static package load `PASS`; mutable geology state is `INTEGRATION REQUIRED`. |
| Streaming | Package dependencies `PASS`; geology-aware world-cell residency is `INTEGRATION REQUIRED`. |
| LOD | `INTEGRATION REQUIRED`; classified simplification/topology levels are not implemented. |
| Destruction/runtime modification | Consumer-side `fresh` replacement `PASS`; topology/remesh producer is `INTEGRATION REQUIRED`. |
| Navigation/gameplay queries | `INTEGRATION REQUIRED`. |
| Particles | `INTEGRATION REQUIRED` after classified change/impact events exist. |
| Audio | `INTEGRATION REQUIRED` after stable contact geology/physical class exists. |
| Debugging | Stable errors and focused traces `PASS`; editor mask views are deferred. |
| Visualization | Runtime/Beauty evidence `PASS`; no forbidden VDB/UI restoration was made. |
| Profiling | Exact device bytes, sample budget and GPU metric recorded: `PASS`. |
| Performance budgets | Bounded stream/palette and fixed nine-sample shader path: `PASS` for this vertical. |
| Tooling/UI | Generic cooked consumer `PASS`; bespoke geology UI intentionally absent. |
| Testing | Codec, cook, renderer, collision, package and source-free Beauty gates `PASS`. |
| Documentation | Design, closure, audit and handoff reflect actual implementation: `PASS`. |
| Failure handling | Invalid codec/descriptor/dependency/fallback/source boundaries are explicit: `PASS`; first-frame pixel validation remains a proof-harness limitation. |

## Remaining `INTEGRATION REQUIRED`

1. **Structural fracture ownership:** stable fracture-cell/plane sidecars and
   imported-mesh structural operators. Owner: future authorized structural
   geology slice. Dependency: classified surface identity.
2. **Jolt physical materials:** per-triangle/region friction, restitution and
   breakability. Owner: Physics/RuntimeFormats. Dependency: stable query/palette.
3. **Geology LOD and streaming:** deterministic semantic reduction, topology
   levels and world-cell page dependencies. Owner: ProceduralAuthoring,
   VirtualizedWorld and Render.
4. **Runtime modification/destruction/save:** coordinated remesh, collision,
   residency and generation replacement; newly exposed producer; durability,
   rockfall and colluvium. Owner: later explicitly authorized runtime geology and
   destruction slices.
5. **Runtime consumers:** bounded query service, Luau values, typed result/change
   events, navigation invalidation/rebuild, particles and audio. Owners: their
   respective runtime subsystems after the query seam exists.
6. **Editor semantic visualization:** bounded class/mask inspection through the
   admitted generic workflow. Owner: a future explicitly authorized editor slice;
   no VDB restoration or bespoke generator is implied.

The next applicable Geology slice is
`M8-GEOLOGY-BLUEPRINT-P1-GRADIENT-BASED-IMPLICIT-DETAIL-WARP`. This document does
not authorize its implementation in the current task.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
