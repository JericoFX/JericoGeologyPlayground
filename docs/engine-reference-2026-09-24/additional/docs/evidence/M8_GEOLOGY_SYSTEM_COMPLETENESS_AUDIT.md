# M8 Procedural Geology — End-to-End System Completeness Audit

> Historical pre-FoldDomain snapshot. This audit records the system at the
> revision named below, before `M8-GEOLOGY-BLUEPRINT-P1-FOLD-DOMAIN` was
> implemented. FoldDomain closure performs its own post-implementation audit;
> the geological material and semantic-lineage gaps identified here remain
> authoritative until their dedicated vertical slice closes them.

**Audit date:** 2026-08-28

**Repository authority:** `F:\JericoEngine`, `main`

**Audited revision:** `c92732ad24bfdc38684bfb70c1a90d16581a8aa5` plus the
owned, uncommitted root `AGENTS.md` completeness rule

**Implementation status:** `PARTIAL`

**Audit mode:** repository evidence first; no geology implementation was changed

## Verdict

JericoEngine has a real deterministic procedural-geology compute core. It can
construct a scalar field from BSP, apply several geological operations on CPU and
Vulkan, extract a Surface Nets mesh, cook a static render mesh and matching static
triangle collision, and render the cooked mesh through the normal Diligent PBR
path.

That is not yet a complete geology vertical. The productive GPU surface result is
not wired into the editor/cook path, geological semantic fields collapse before
mesh extraction, and the runtime mesh/material formats expose no geological
surface classification. The current cook gives all generated triangles the
source material GUID, regenerates face normals/tangents, and converts positions
to dominant-axis planar UVs. No shader consumes strata, fracture, erosion,
sediment, cavity, curvature, geology class, or freshly exposed state. There is no
true triplanar geology material evaluation.

Consequently, the audited geology system must not be reported as complete.

## Actual current data lineage

```text
BspDocument / GeologyStackRecipe
    -> BSP source scalar field on Vulkan
    -> scalar ping-pong geology layers
       + transient strata float2 field
       + transient anisotropic Voronoi / fracture fields
       + transient exposure / curvature field
       + separate heightfield water / flow / sediment state
    -> final scalar SDF
    -> Surface Nets position + normal + index buffers
    -> GeologySurfaceMesh
       { TriangleMesh, normals, tangents, triangle material GUIDs }
    -X-> no productive Editor/JericoEditor consumer
    -X-> no direct GeologySurfaceMesh -> GeologyCook adapter

Separately demonstrated by focused tests:

HardSurfaceResult
    -> GeologyCook
    -> BspRenderMeshRequest
    -> CookedMeshAsset + CookedCollisionAsset
    -> standard StaticMesh Diligent PBR rendering
```

The `-X->` boundaries are architectural gaps, not merely missing tests.

## CURRENT

### Authoring inputs and deterministic evaluation

- `GeologyStackRecipe` is canonical, schema-versioned, validated, deterministically
  serialized, parsed, and hashed in
  `Tools/ProceduralAuthoring/Source/Geology/GeologyRecipe.cpp`.
- Layer headers retain stable IDs, names, kind, enable state, strength, seed,
  regional mask, and transform. Typed payloads exist for PlaneCut,
  DirectionalWarp, Strata, FaultDisplace, FractureCells, ImplicitBlock, JointSet,
  cavity/weathering/detail and erosion-related work.
- CPU references exist for the currently admitted mathematical contracts,
  including strata, anisotropic Voronoi/fractures, cavity carving, weathering,
  and heightfield erosion.
- `GeologyGpuSession` owns a resident Diligent/Vulkan compute path with resource
  admission, cancellation, deterministic field identities, dirty-leaf/voxel
  statistics, bounded inspection readback, and Surface Nets extraction.
- CPU/GPU parity is exercised by focused tests in
  `Tests/ProceduralAuthoring/GeologyGpuSessionTests.cpp`.

### Geological information that exists upstream

- Strata evaluation produces phase, periodic scalar, profile mask and composed
  mask (`GeologyStrataFieldSample`).
- Anisotropic Voronoi/fracture evaluation produces stable cell identity, nearest
  and second-nearest metrics, boundary metric, structural mask and fracture mask.
- Weathering evaluation produces surface exposure, gradient magnitude,
  curvature per metre and curvature response.
- Cavity evaluation produces cavity signed distance and composed weight.
- Heightfield erosion retains per-cell water, flow, discharge, sediment,
  capacity, eroded amount, deposited amount, evaporation and talus transfer in
  `GeologyHeightfieldErosionResult`.
- The canonical Strata payload serializes material-class, hardness and
  erodibility ranges.

### Mesh, cook, collision and rendering foundations

- Surface Nets emits positions, normals and indices and returns a bounded,
  content-hashed `GeologySurfaceMesh`.
- `GeologyCook` exists and uses the authoritative BSP render-mesh cooker, then
  writes and rereads a matching static triangle collision asset
  (`Tools/ProceduralAuthoring/Source/GeologyCook.cpp:138`).
- `RuntimeFormats::MeshVertex` serializes position, normal, tangent and one UV;
  submeshes carry material GUIDs
  (`RuntimeFormats/Include/Jerico/RuntimeFormats/MeshAsset.hpp:16`).
- The runtime mesh codec validates and rereads vertices, indices, submeshes,
  visibility records and bounds.
- The renderer uploads cooked meshes and materials through the existing
  `StaticMeshRenderAsset` path. The canonical PBR vertex shader already exposes
  world position, world normal, tangent and UV to the pixel shader
  (`Engine/Render/Shaders/Surface/PBR_Opaque.vsh:3`).
- The normal static PBR path already provides Diligent/Vulkan lighting, shadows,
  IBL, material factors and PBR textures. It is the integration target; geology
  does not require a second renderer.
- Shipping packaging rejects authoring-only geology/OpenVDB/FastNoise markers
  while admitting cooked `.rmesh` and `.rmat` payloads.

## PARTIAL

### Productive authoring/editor route

`GeologyGpuSession` and `GeologySurfaceMesh` are used by their implementation and
focused tests, but no direct consumer exists under `Editor/JericoEditor`. The
editor has mature BSP/SDF transactional seams, yet the GPU geology result is not
an admitted preview/apply/cook product. Bounded field inspection is a useful
debug primitive, not an editor geology workflow.

### Material identity

Triangle material GUID propagation exists generically, and `BspCook` groups
triangles into material submeshes. However, GPU geology currently assigns
`source.sourceGuid` to every generated triangle. Strata material-class ranges are
validated and round-tripped but have no evaluator consumer. This proves GUID
transport, not geological material classification.

### Surface attributes

Surface Nets computes smooth vertex normals and placeholder tangents, but
`GeologyCookRequest` accepts `HardSurfaceResult`, which cannot carry those
arrays. The downstream BSP cooker therefore duplicates vertices per triangle,
recomputes flat face normals and tangents, and synthesizes dominant-axis planar
UVs (`Tools/ProceduralAuthoring/Source/BspCook.cpp:675`).

### Collision and navigation

Static triangle collision is cooked from the same positions/indices as the
render product. It has no geological or physical-material classification.
Navigation can be baked from the resulting triangle mesh and is demonstrated in
`GeologyCookTests`, but `GeologyCook` only marks Navigation/Scatter/EPLS dirty; no
owner automatically rebuilds and publishes those derived products.

### LOD and streaming

The generic runtime has visibility records and world-cell dependencies.
Geology advertises `topologyLevels`, but the current canonical GPU test explicitly
expects that vector to be empty. Geology cook derives only base visibility, and
the current runtime mesh validation admits only `lodLevel == 0`.

### Destruction and modification

Sparse-SDF authoring transactions provide preview/apply/undo/redo and dirty
regions. This is not a runtime geology modification path: there is no shipping
field handle that can remesh a changed page, refresh semantic classification,
replace collision, invalidate navigation, or republish world-cell residency.

### Documentation

The geology blueprint and math research correctly describe shared structural
fields, material classes, durability, exposed interiors and downstream products.
Those documents are ahead of the actual surface/runtime contracts. Conversely,
the implemented bounded GPU inspection and current static geology cook are more
concrete than the high-level vertical-slice documentation makes clear.

### Documented but not implemented

- Shared geological fields driving geometry, material response, durability,
  weathering and fracture bias are architectural intent, not an end-to-end data
  path.
- Strata `materialClassMinimum/Maximum`, `hardnessMinimum/Maximum` and
  `erodibilityMinimum/Maximum` are canonical recipe data, but repository search
  finds their use only in validation, parse/write and recipe tests.
- Fracture ID/normal, exposed-interior material policy, durability volumes,
  material volumes, debug modes, LOD and streaming are described in blueprint or
  research documents but are not runtime geology products.
- The declared `topologyLevels` output is an empty capability placeholder in the
  demonstrated canonical BSP evaluation.

### Implemented but not adequately documented as a vertical

- The resident Vulkan session already exposes bounded structural/exposure/
  curvature inspection without whole-field readback.
- `GeologyCook` already writes and semantically rereads matching static render and
  collision assets, although it is only invoked by focused tests.
- Package publication has an explicit negative boundary against authoring-only
  geology/dependency markers.
- Dirty-leaf and dirty-voxel statistics, physical readback accounting, resource
  admission and cancellation are implemented compute foundations. They do not
  close downstream integration, but should not be reimplemented.

## MISSING

- A Jerico-owned geological surface representation between field evaluation and
  runtime mesh cook.
- Preservation of smooth Surface Nets normals/tangents through the cook boundary.
- Per-surface strata, fracture, erosion, sediment, cavity, curvature, geology
  class, and fresh/exposed classifications.
- Evaluation of the serialized Strata material-class/hardness/erodibility ranges.
- A runtime-format representation for optional procedural surface attributes and
  their compact mesh-level descriptor.
- World-space/triplanar sampling in the canonical PBR surface path.
- A material contract that maps geological classes/masks to existing Jerico PBR
  material layers without a manual UV unwrap.
- A productive editor preview/debug/cook route for `GeologyGpuSession` output.
- Geological debug views for individual masks/classes and data-lineage loss.
- Runtime/Luau/gameplay queries for geological class or physical properties.
- Per-triangle physical-material classification in collision.
- Multi-LOD semantic preservation and geology-aware world-cell streaming.
- Runtime-remesh provenance and automatic fresh-surface classification.
- Save/load and deterministic regeneration contracts for runtime-modified
  geological pages.
- Events for changed geology products and downstream navigation, particles,
  audio, debris or gameplay consumers.

## Geological semantic lineage

| Information | Exists during evaluation | Survives Surface Nets | Survives cook/runtime | Required disposition |
|---|---|---:|---:|---|
| Position | final scalar zero crossing | yes | yes | Keep in base mesh vertex. |
| Normal | scalar gradient / Surface Nets normal | yes | **no**; cook recomputes flat normals | Preserve the extracted normal in the productive cook adapter. |
| Slope | derivable from normal and authored up axis | not stored | not stored | Reconstruct in shader/runtime query from normalized world normal and descriptor up axis; do not spend a vertex channel. |
| Height | world position plus reference frame | implicit | implicit | Reconstruct from world position and descriptor origin/up axis. |
| Depth | requires a reference surface/field query | transient only | no | Retain only when a named consumer requires it; it is not equivalent to world height. |
| Strata | phase, periodic scalar, profile/composed masks | no | no | Bake the final composed appearance mask at extraction unless an identical admitted GPU function can cheaply reconstruct it. Never generate an unrelated material-noise mask. |
| Strata material class | parameters serialize | no evaluator | no | Implement classification from the same strata sample before exposing a material ID. |
| Fracture | mask, cell ID and distance metrics | no | no | Bake the appearance fracture mask. Retain stable cell/region identity in a query/destruction sidecar when those consumers are admitted; do not put a 32-bit cell ID on every visual vertex by default. |
| Fracture normal | requested by design docs | no implementation | no | Derive from the admitted structural field only when needed by fracture/destruction consumers. |
| Erosion | final height plus per-cell eroded amount | no | no | Bake a normalized erosion classification after simulation; it cannot be reconstructed from seed/world position alone. |
| Sediment/deposition | sediment and deposited amount per erosion cell | no | no | Bake a normalized deposition mask after simulation. Preserve simulation state only for mutable-authoring/runtime pages that must continue evolving. |
| Cavity | cavity signed distance/composed weight | no | no | Bake cavity/exposed-interior classification for stable appearance; analytic reconstruction is optional only for immutable simple cavities. |
| Curvature | exposure field has curvature and response | no | no | Quantize a stable authored curvature response when material continuity across LOD matters. Screen derivatives may supplement micro-detail but must not replace authoritative classification. |
| Surface exposure | weathering inspection channel | no | no | Bake when used by appearance/weathering. It is not the same as freshly exposed state. |
| Geology/material ID | uniform source GUID; Strata ranges only serialize | uniform source GUID | one material GUID | Add a compact local geology-class index mapped by a mesh-level palette to existing material GUIDs. |
| Fresh/exposed state | absent | no | no | Generate from edit/destruction provenance during remesh. It cannot be reconstructed reliably from position, normal or the original seed. |
| Procedural seed/parameters | canonical authoring recipe | no | recipe hash only | Cook a compact runtime appearance descriptor containing only admitted values needed for deterministic rendering/query. Do not ship the authoring recipe or forbidden authoring markers. |
| FBM/organic deformation | changes the scalar field | no semantic output | no | Export only a meaningful classification produced by that same field. Do not run a second decorative noise stack. |

## What may be reconstructed on GPU

Safe, cheap reconstruction from data already present in the renderer:

- world position;
- normalized world normal;
- slope against the descriptor's up axis;
- height against the descriptor's origin/up axis;
- triplanar projection coordinates and blend weights;
- deterministic micro-variation derived from a cooked appearance seed, provided
  that it does not claim to be a structural geological mask.

Reconstruction is conditional, not automatic, for analytic strata or simple
cavities. It is allowed only if the shader uses an admitted function with the
same coordinate frame, seed, parameters and numerical contract as geometry
evaluation, and if its cost is below the baked-stream bandwidth cost.

The following must not be reconstructed from world position and seed alone:

- erosion history;
- sediment/deposition history;
- runtime destruction provenance/freshness;
- stable fracture-cell ownership after mutable edits;
- any classification produced by a layer stack whose intermediate state is not
  present at runtime.

## Smallest coherent target representation

Do not copy a large `GeologicalSurfaceData` struct into every vertex. The smallest
coherent Jerico representation is hybrid:

1. Keep the existing 48-byte base `MeshVertex` unchanged for ordinary meshes.
2. Add one **optional** geological classification stream for geology meshes. The
   initial appearance contract can fit in two packed 32-bit words per vertex:
   strata/fracture/erosion/sediment and
   cavity/curvature/fresh-exposure/local-geology-class. Exact quantization and
   signed-curvature encoding require RED tests before being frozen.
3. Add a compact mesh-level descriptor containing schema/version, coordinate
   frame, appearance scale/rotation/seed, source/final field identity, and a
   local geology-class-to-existing-material-GUID palette.
4. Keep large/stable fracture ownership and mutable simulation state in an
   optional spatial query/page sidecar, not in the render vertex stream.
5. Compute slope, height and triplanar coordinates in the shader from world
   position/normal and the descriptor.

This adds eight optional bytes per geology vertex rather than widening every
Jerico mesh, avoids storing reconstructible values, keeps history-dependent data,
and gives runtime remeshing one deterministic classification output. It also
keeps geological semantics owned by ProceduralAuthoring/RuntimeFormats while the
renderer consumes a narrow contract.

## INTEGRATION REQUIRED

### 1. Field evaluation -> surface classification

- **Subsystem:** ProceduralAuthoring geology GPU/CPU evaluation and Surface Nets.
- **Why:** the authoritative fields disappear before surface extraction.
- **Required input/data:** final strata/fracture/weathering/cavity/erosion fields,
  extracted position/normal, source and field hashes, edit provenance.
- **Expected interface:** `GeologySurfaceMesh` carries a validated optional packed
  classification stream and descriptor; CPU reference produces identical
  classification at sampled witnesses.
- **Owning subsystem:** Tools/ProceduralAuthoring.
- **Dependency:** currently admitted field contracts and Surface Nets emission.
- **Implementation milestone:** a dedicated Geological Surface/Material
  `ACTIVE_SLICE` after the current FoldDomain slice is closed and published.

### 2. Surface -> cooked runtime mesh

- **Subsystem:** GeologyCook, BspRenderMesh, RuntimeFormats.
- **Why:** the current `HardSurfaceResult` adapter loses normals/tangents and has no
  optional semantic stream.
- **Required input/data:** `GeologySurfaceMesh`, packed classifications, descriptor,
  material palette.
- **Expected interface:** a geology-aware adapter into the existing mesh cooker;
  versioned optional mesh stream with deterministic write/read/hash/dump and
  negative validation. No second mesh format.
- **Owning subsystem:** RuntimeFormats with ProceduralAuthoring adapter.
- **Dependency:** item 1.
- **Implementation milestone:** same Geological Surface/Material slice.

### 3. Geological material -> canonical Diligent PBR

- **Subsystem:** Material runtime/cook, Render, shader cook.
- **Why:** generated caves/cliffs currently depend on planar UV and cannot consume
  geological semantics.
- **Required input/data:** world position/normal, packed classification stream,
  descriptor, existing PBR textures/material GUID palette.
- **Expected interface:** extend the existing canonical PBR surface permutations
  and shared sampling code with world/triplanar mapping and geological weighting.
  Existing `PbrMaterialData`/layer bindings remain the material authority; do not
  create a parallel geology renderer or standalone material database.
- **Owning subsystem:** Engine/Render + RuntimeFormats material contract.
- **Dependency:** item 2 and admitted shader resource/layout budgets.
- **Implementation milestone:** same Geological Surface/Material slice.

### 4. Editor authoring and debugging

- **Subsystem:** JericoEditor geology workflow.
- **Why:** no productive consumer displays or applies the GPU geology surface.
- **Required input/data:** accepted session result, material descriptor, individual
  semantic masks, diagnostics and hashes.
- **Expected interface:** reuse editor automation/transaction preview, diff,
  cancel, undo/redo and stable cook/apply seams; provide mask/class debug views.
- **Owning subsystem:** Editor/JericoEditor.
- **Dependency:** items 1-3.
- **Implementation milestone:** Geological Surface/Material vertical acceptance,
  not a bespoke generator or preset.

### 5. Collision and physical properties

- **Subsystem:** collision/Jolt physical material lookup.
- **Why:** geometry collision exists, but geology class cannot affect friction,
  restitution, breakability or impact response.
- **Required input/data:** stable local geology class and material/property palette.
- **Expected interface:** optional per-triangle/region physical-material mapping or
  bounded geology query keyed by collision witness.
- **Owning subsystem:** Physics/RuntimeFormats collision.
- **Dependency:** stable class/query identity from item 1.
- **Implementation milestone:** later explicitly authorized geology-physics slice;
  static collision geometry remains valid meanwhile.

### 6. LOD and streaming

- **Subsystem:** geology topology output, RuntimeFormats mesh visibility,
  VirtualizedWorld cells.
- **Why:** large procedural environments need consistent semantic LOD and page
  residency; current geology produces only the dense master/base LOD.
- **Required input/data:** classified master surface, deterministic simplification
  policy, page bounds/dependencies and hashes.
- **Expected interface:** topology levels preserve/reduce classification
  deterministically; world-cell dependencies publish mesh/material/semantic pages.
- **Owning subsystem:** ProceduralAuthoring + VirtualizedWorld + Render visibility.
- **Dependency:** versioned classified mesh stream.
- **Implementation milestone:** later dedicated M8 geology LOD/streaming slice.

### 7. Runtime modification, destruction and save/load

- **Subsystem:** runtime sparse field/remesh, destruction, collision, scene save.
- **Why:** freshly exposed surfaces require edit provenance and coordinated product
  replacement.
- **Required input/data:** mutable field/page identity, operation generation,
  previous/new classification, dirty bounds, material/physical palette.
- **Expected interface:** one transaction republishes classified render surface,
  collision, world-cell dependencies and invalidation events; save/load retains
  deterministic page state or the operations required to regenerate it.
- **Owning subsystem:** future runtime geology/destruction owner.
- **Dependency:** items 1-3 plus an admitted runtime modification architecture.
- **Implementation milestone:** later explicitly authorized destruction/runtime
  geology slice; not the current FoldDomain slice.

### 8. Navigation, gameplay, Luau, events, particles and audio

- **Subsystem:** Navigation, gameplay query service, Luau, event bus, VFX/audio.
- **Why:** downstream systems cannot currently ask what rock/classification a hit
  belongs to or react when a geological surface changes.
- **Required input/data:** bounded world query, geology class/properties, fresh
  surface/change event, dirty bounds and stable generation.
- **Expected interface:** read-only bounded query plus result events after
  authoritative modification; Luau bindings expose values, not authoring recipes.
- **Owning subsystem:** respective runtime owners consuming one geology query seam.
- **Dependency:** query/page sidecar and runtime modification generation.
- **Implementation milestone:** later consumer-specific slices after the surface
  contract is stable.

## System completeness matrix

| Category | Status | Repository-specific result |
|---|---|---|
| Source/input data | CURRENT | BSP document and canonical typed geology recipe. |
| Asset pipeline | PARTIAL | Static mesh/collision cook exists; productive GPU geology adapter and semantic cook do not. |
| Editor/authoring workflow | PARTIAL | BSP/SDF transactions exist; no admitted GPU geology preview/apply/cook consumer. |
| ECS/components | PARTIAL | `MeshComponent` references a cooked mesh only; no mutable/query geology identity. |
| Runtime representation | PARTIAL | `.rmesh`/`.rcoll` work for static geometry; geology semantics absent. |
| CPU processing | CURRENT/PARTIAL | Mathematical references exist; unified surface classification evaluator does not. |
| GPU processing | CURRENT/PARTIAL | Scalar/structural/weathering/erosion stages and Surface Nets exist; semantic surface output does not. |
| Renderer | CURRENT/PARTIAL | Standard PBR consumes the cooked mesh; no geology input layout/permutation. |
| Shaders | PARTIAL | World position/normal available; only UV PBR sampling, no triplanar geology evaluation. |
| Materials | PARTIAL | PBR and four-layer terrain contracts exist; no geological weighting/mapping contract. |
| Physics/collision | PARTIAL | Static triangle geometry is matched; physical geology class missing. |
| Animation | NOT APPLICABLE to static geology material; reevaluate for deforming geological actors. |
| Scripting/Luau | MISSING | No geological query binding. |
| Events | MISSING | No geology product-change/result events. |
| Sequencer | NOT APPLICABLE to static material; required only if geological parameters become animated. |
| Serialization | PARTIAL | Recipes and base mesh/collision serialize; surface semantics/runtime descriptor do not. |
| Save/load | PARTIAL | Static cooked products load; mutable geology state does not exist. |
| Streaming | PARTIAL | Generic world-cell machinery exists; geology output is not registered. |
| LOD | PARTIAL | Generic visibility exists; geology emits no topology levels. |
| Destruction/runtime modification | MISSING | No runtime remesh/classification/product-replacement path. |
| Navigation/gameplay queries | PARTIAL/MISSING | Manual nav cook proof exists; no geology query or automatic rebake. |
| Particles | MISSING consumer | Needs change/impact classification events when authorized. |
| Audio | MISSING consumer | Needs stable physical/geology class at contact/hit queries. |
| Debugging | PARTIAL | Bounded field inspection exists; no persistent semantic debug views. |
| Visualization | MISSING | No editor mask/class visualization for productive geology surfaces. |
| Profiling | PARTIAL | GPU timing/readback/dirty statistics exist; material/triplanar budget does not. |
| Performance budgets | PARTIAL | Compute/resource admission exists; semantic-stream bytes and PBR sample budgets are undefined. |
| Tooling/UI | PARTIAL | Generic editor seams exist; geology material authoring is absent. |
| Testing | PARTIAL | Strong compute/cook unit gates; no end-to-end geology-material Beauty/runtime proof. |
| Documentation | PARTIAL | Research/blueprint anticipates semantics; actual collapse and missing consumers were not recorded before this audit. |
| Failure handling | PARTIAL | Compute/cook validation is strong; no semantic codec/shader/runtime errors exist because the integration is absent. |

## ARCHITECTURAL RISKS

1. **Semantic collapse:** retaining only final SDF and one material GUID makes
   rendering, physics, audio, destruction and gameplay disagree with the fields
   that generated the geometry.
2. **Parallel-system temptation:** adding a bespoke geology renderer/material
   database would duplicate `RuntimeFormats`, `StaticMeshRenderAsset`, material
   cook and PBR shading. The correct seam is an optional classified mesh stream
   consumed by the existing pipeline.
3. **Cook-boundary quality loss:** recomputed flat normals/tangents already throw
   away higher-quality Surface Nets output before any material work begins.
4. **False reconstruction:** erosion, deposition and fresh exposure cannot be
   recreated from seed/world position. Pretending otherwise would be
   nondeterministic after edits.
5. **Unbounded shader cost:** naively evaluating the complete geology layer stack
   per pixel would multiply ALU/noise cost and diverge numerically from the compute
   field. Bake history/stack-dependent masks and reconstruct only cheap invariants.
6. **Global vertex inflation:** widening `MeshVertex` for every mesh wastes memory
   and bandwidth. Geological attributes must remain an optional stream.
7. **Submesh explosion:** mapping every stratum/fracture class to material GUID
   submeshes can multiply draw calls. Use a bounded local palette and shader
   weighting; reserve submeshes for genuinely distinct render state.
8. **LOD inconsistency:** independently recomputed masks at each simplified LOD
   can shimmer or change class. Semantic reduction must be part of LOD generation.
9. **Shipping boundary regression:** authoring recipes and forbidden dependency
   markers must not leak into runtime descriptors. Cook only the minimal admitted
   appearance/query data.
10. **Invalidation without an owner:** current dirty Navigation/Scatter/EPLS flags
    do not rebuild anything. Runtime modification must publish a coordinated
    generation, not several eventually inconsistent products.

## Smallest coherent implementation plan

This plan closes the first usable vertical without creating a parallel system.
It is a proposal only; no implementation was performed by this audit.

1. **Freeze RED semantic lineage contracts.** Add CPU tests proving that the same
   strata/fracture/cavity/weathering/erosion witnesses which affect SDF also
   produce expected packed surface classifications, including exact disabled
   identity and deterministic quantization.
2. **Deepen `GeologySurfaceMesh`.** Preserve extracted normals/tangents and add the
   optional packed classification stream plus compact descriptor/palette. Surface
   Nets samples existing resident fields at emitted surface vertices; no second
   noise evaluation stack.
3. **Use the existing cook seam.** Add a geology-aware adapter into
   `BspRenderMeshRequest`/`MeshWriteRequest` that preserves normals/tangents and
   optional classification. Extend the existing `.rmesh` codec with versioned,
   bounded optional data and deterministic reread/hash/negative tests; do not add
   a `.rgeo` mesh format.
4. **Deepen the existing material/PBR path.** Add a world/triplanar mapping mode
   and classified-layer input to the canonical surface PBR shader permutations
   and material bindings. Reuse existing PBR textures, material GUIDs, lighting,
   shadows and IBL. Geological masks choose/blend appearance; they are never
   replaced with decorative shader noise.
5. **Wire one editor vertical.** Existing preview/diff/cancel/undo/redo/apply/cook
   machinery displays the classified GPU surface and mask debug modes, then cooks
   the same accepted identity to `.rmesh`/`.rcoll`/`.rmat`.
6. **Prove the vertical.** Tests cover canonical round-trip, CPU/GPU surface
   classification parity, mesh semantic codec, UV-independent/triplanar shader
   use, normal preservation, collision identity, source-free package replay, and
   real PBR Beauty/runtime evidence on a cave/cliff with strata/fracture/deposition
   continuity.
7. **Repeat this audit.** Reclassify every matrix entry and leave all deferred
   collision-properties, LOD/streaming, runtime destruction, gameplay/Luau,
   particles and audio items explicitly `INTEGRATION REQUIRED` until their owning
   slices implement and prove them.

## Execution boundary

The repository's only current `ACTIVE_SLICE` is
`M8-GEOLOGY-BLUEPRINT-P1-FOLD-DOMAIN`. The Geological Surface/Material plan is a
different vertical capability and cannot be implemented silently inside that
slice. Per `ONE_SLICE_ONE_TASK`, FoldDomain must first be closed and published,
then exactly one new top-level task must explicitly accept the new slice and the
persistent Goal. Completing the material vertical would not complete all Geology,
the blueprint, or JericoEngine 1.0.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`

---

## Imported-Mesh Structural Operators re-audit — 2026-08-29

`M8-GEOLOGY-BLUEPRINT-P1-IMPORTED-MESH-STRUCTURAL-OPERATORS` closes the ordered
P1 operator list with the smallest coherent direct-mesh/offline-static boundary.
The complete contract, RED chronology, hashes and runtime evidence are recorded
in `M8_GEOLOGY_BLUEPRINT_P1_IMPORTED_MESH_STRUCTURAL_CLOSURE.md`.

The added lineage is:

```text
immutable imported source
-> existing canonical glTF import and flattened static mesh
-> selected-submesh planar displacement
-> preserved topology/material/UV/optional GEOL
-> reconstructed directions/bounds and preserved visibility participation
-> JRMS + exact JRCL static triangle collision
-> existing package/static Vulkan/Jolt consumers
```

No source asset is modified. Reimport preserves stable root/material/collision
GUID identity while source/content hashes change. Collision positions and
indices equal the final render mesh exactly. A seven-entry source-free package
renders the 3575-vertex/18108-index BoomBox through Vulkan with one draw, four
textures and zero fallbacks; the inspected 1280x720 frame is non-empty.

### Repeated applicability decisions

| Category | Imported-mesh structural result |
|---|---|
| Source/import/reimport | `PASS`: immutable source, stable accepted identity and bounded reimport mutation. |
| Asset/cook/package | `PASS`: existing canonical cooker, manifest, atomic publish and source-free package. |
| CPU/direct mesh | `PASS`: deterministic bounded planar displacement, unchanged topology and stable failures. |
| GPU operator | `NOT APPLICABLE`: deliberately offline CPU; GPU is an existing runtime consumer only. |
| Renderer/materials | `PASS`: existing static PBR Vulkan path, four textures, zero fallback. |
| Collision/physics | Exact static triangles `PASS`; geology physical properties remain deferred. |
| Classification | Optional existing `GEOL` is preserved; synthesis/field conversion remains `INTEGRATION REQUIRED`. |
| Visibility/LOD/streaming | Existing participation is preserved; classified LOD/world cells remain deferred. |
| Runtime mutation/save | `INTEGRATION REQUIRED`; no remesh/destruction/mutable collision/save framework. |
| Editor authoring | Existing CLI/data boundary proved; semantic preview/diff/undo UI remains deferred. |
| Gameplay/Luau/events/navigation/particles/audio | `INTEGRATION REQUIRED` only behind later bounded query/result seams. |
| Testing/debug/evidence | Unit/negative/reimport/collision/package and inspected Vulkan evidence `PASS`. |

### Remaining integration boundary

The P1 operator list is closed, but the complete Geology blueprint is not. The
eight P2/research features remain ordered work, starting with the SpeedRock-like
block grammar source. Field conversion/classification synthesis, robust
Boolean/remesh/retopology, runtime destruction/mutable collision/save identity,
Jolt physical properties, classified LOD/world-cell streaming, semantic editor
views and downstream navigation/gameplay/Luau/events/particles/audio remain
`INTEGRATION REQUIRED` with their existing owners.

The joint Geology/PWAS/EPLS/animation/destruction production expansion remains
forbidden until the complete Geology blueprint is actually finished, published
and closed.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`

---

## Rockfall re-audit — 2026-08-28

`M8-GEOLOGY-BLUEPRINT-P1-ROCKFALL` selects the smallest coherent
`OFFLINE_STATIC_FIELD_ROCKFALL` boundary. Research, independent design, RED
chronology, measurements and product evidence are retained in
`M8_GEOLOGY_BLUEPRINT_P1_ROCKFALL_CLOSURE.md`.

The complete locally proven lineage is:

```text
BSP + schema-19 typed recipe/RBrush mask
-> resident weathered scalar field
-> packed structural fracture + fresh classification
-> bounded GPU readback
-> deterministic CPU six-face support/components/removal
-> scalar/fresh upload + canonical accumulated field hash
-> existing dirty tracking and classified Surface Nets
-> byte-repeatable JRMS 1.1 GEOL + exact static triangle collision
-> existing classified Vulkan static-mesh visual consumer
```

Rockfall removes unsupported occupied voxels during authoring/cook. It does not
create falling rigid bodies, debris, deposition, runtime remesh, mutable
save-state or a physical mass contract.

### Slice applicability matrix

| Category | Rockfall result |
|---|---|
| Source/input data | `PASS`: existing BSP, weathered signed distance, packed fracture, fresh state, gravity/support controls and RBrush masks. |
| Asset pipeline | `PASS`: existing classified Surface Nets, JRMS 1.1 and collision cook; two fresh-process products were byte-identical. |
| Editor/authoring workflow | Generic typed recipe/mask/preview/transaction seams apply; support/component visualization is `INTEGRATION REQUIRED`. |
| ECS/components | Existing static-mesh component remains sufficient for the cooked result; no geology-specific runtime component was added. |
| Runtime representation | `PASS` for the static JRMS/collision result; Rockfall authoring controls/components are deliberately absent. |
| CPU processing | `PASS`: independent support/component/removal reference, stable component identities, cancellation and hard field ceiling. |
| GPU processing | `PASS`: bounded resident field/classification/hash readback and upload around the global CPU topology step; existing Vulkan extraction remains the owner. |
| Renderer/shaders/materials | `PASS`: existing classified Vulkan path rendered 960x720 weathered/fresh/IBL+shadow captures with zero fallback and no new shader/material system. |
| Physics/collision | Exact static triangle collision `PASS`; density, mass, break dynamics and moving bodies are `INTEGRATION REQUIRED`. |
| Animation/Sequencer | `NOT APPLICABLE` to the selected offline/static operator. |
| Scripting/Luau/events | `NOT APPLICABLE` locally; bounded runtime queries/events remain `INTEGRATION REQUIRED` if mutable geology is later authorized. |
| Serialization | `PASS`: schema 19/kind 23 round-trip/hash/negative admission and source-free static runtime products. |
| Save/load | Static cooked load `PASS`; mutable Rockfall state does not exist and remains `INTEGRATION REQUIRED`. |
| Streaming/LOD | Existing static runtime compatibility passes; classified LOD/world-cell propagation is `INTEGRATION REQUIRED`. |
| Destruction/runtime modification | `NOT IMPLEMENTED` by design; a later authorized runtime geology/destruction owner is `INTEGRATION REQUIRED`. |
| Navigation/gameplay queries | `INTEGRATION REQUIRED` only for later runtime geological changes. |
| Particles/audio | `INTEGRATION REQUIRED` only for later bounded result-event consumers. |
| Debugging | `PASS`: detached counts/components/hash/volume/work/readback/CPU timing plus stable failures. |
| Visualization | `PASS`: three inspected 960x720 Vulkan captures show the non-empty removed-volume static product and classified material responses. |
| Profiling/performance budgets | `PASS`: 16,777,216 field-voxel ceiling; proof read 1,105,928 bytes, performed 75,768 checked graph work items and measured 0.079872/0.082944 ms single-frame StaticPbr GPU time in two fresh runtime processes. |
| Tooling/UI | Generic recipe/RBrush path `PASS`; no bespoke generator, preset or restored Geology UI. |
| Testing | `PASS`: RED contracts, Debug/Release CPU matrices, real RTX 3090 repeat/dirty identity, two fresh-process cooks and runtime Vulkan gate. |
| Documentation | Research, design, plan, blueprint status, closure and this re-audit match the implementation. |
| Failure handling | `PASS`: invalid domain, schema/order, duplicates, non-finite data, cancellation, work ceiling, allocation/map and overflow paths. |

### Data-lineage conclusions

Structural fracture is not collapsed into a material name: its packed byte is
decoded before the support graph and directly controls face-bond cuts. RBrush
mask and layer strength determine active topology while inactive occupied
voxels remain support, preventing the operator from destroying protected
regions. Existing Durability Graph has an explicit, deliberately indirect
effect: ordered weathering changes the signed-distance occupancy presented to
Rockfall; normalized durability is never reinterpreted as force or mass.

The CPU pass changes only signed distance and the existing `fresh` bit on
retained neighbours. Strata, strength, fracture, erosion, deposition, cavity,
curvature and exposure bytes remain intact and reach the classified surface
and material consumer. Detached volume remains cubic meters; no downstream
system may infer kilograms without a later versioned density contract.

### Remaining `INTEGRATION REQUIRED`

1. Colluvium/deposition: transport, settling, material lineage and budgets;
   owner is the separate next P1 operator slice.
2. Imported-mesh structural operators: canonical topology/source policy;
   owner is its later P1 slice.
3. Runtime destruction/remesh/save-load: bounded event identity, replacement,
   collision publication and persistence; owner is a later explicitly
   authorized runtime geology/destruction slice.
4. Jolt physical geology: density/mass/resistance/break properties; owner is a
   future Physics/RuntimeFormats contract slice.
5. Classified LOD/world-cell streaming and editor support visualization;
   owners are their respective future VirtualizedWorld/editor slices.
6. Navigation/gameplay/Luau/events/particles/audio: bounded result/query/event
   seams only if runtime geological change is later admitted.

Completing Rockfall does not complete P1, the Geology blueprint, the later
PWAS/EPLS production expansion or JericoEngine 1.0.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`

---

## Colluvium re-audit — 2026-08-29

`M8-GEOLOGY-BLUEPRINT-P1-COLLUVIUM` selects the smallest coherent
`OFFLINE_STATIC_FIELD_COLLUVIUM` boundary. Research, independent design, RED
chronology, exact measurements, cooked identities and runtime captures are
recorded in `M8_GEOLOGY_BLUEPRINT_P1_COLLUVIUM_CLOSURE.md`.

The additional semantic lineage is:

```text
productive Rockfall detached voxel/component/volume result
-> retained post-Rockfall field, classification and canonical identity
-> zero-additional-readback bounded supported Colluvium settling
-> deposited placements + explicit unsettled remainder + conserved volume
-> copied classification with deposition=255 and fresh cleared
-> existing classified Surface Nets / JRMS / static collision / Vulkan runtime
```

### Slice applicability matrix

| Category | Colluvium result |
|---|---|
| Source/input data | `PASS`: unchanged productive Rockfall lineage plus typed schema-20 controls and optional admitted RBrush mask. |
| Asset pipeline | `PASS`: existing classified JRMS 1.1 and exact static collision cook; no new format or cooker. |
| Editor/authoring workflow | Generic recipe/RBrush preview, cancellation and transaction seams apply; a source/deposited/remainder semantic view is `INTEGRATION REQUIRED`. |
| ECS/components | Existing static mesh component path remains sufficient; no geology-specific component was added. |
| Runtime representation | `PASS` for the static cooked result; Colluvium controls and transient source/remainder state are absent from runtime products. |
| CPU processing | `PASS`: deterministic supported traversal, conserved volume, canonical identities, checked arithmetic and bounded cancellation. |
| GPU processing | `PASS`: one Rockfall readback is reused, Colluvium adds zero readback bytes, and publication uses the existing resident field/classification/hash buffers. |
| Renderer/shaders/materials | `PASS`: existing classified Diligent/Vulkan static PBR path consumes positive interpolated deposition with no parallel shader or material schema. |
| Physics/collision | Exact static triangle collision `PASS`; density, mass, break and granular dynamics are `INTEGRATION REQUIRED`. |
| Animation/Sequencer | `NOT APPLICABLE` to the selected static authoring/cook boundary. |
| Scripting/Luau/events | `INTEGRATION REQUIRED` only for later authorized runtime change/query consumers. |
| Serialization | `PASS`: canonical schema 20 and unchanged JRMS/RCOLL products; authoring-only Colluvium state is not leaked. |
| Save/load | Static cooked load `PASS`; mutable sediment/geology state remains `INTEGRATION REQUIRED`. |
| Streaming/LOD | Existing static product compatibility passes; classified deposition across LOD/world cells is `INTEGRATION REQUIRED`. |
| Destruction/runtime modification | `NOT IMPLEMENTED` by design; dirty-range upload/remesh, collision replacement and persistence remain `INTEGRATION REQUIRED`. |
| Navigation/gameplay queries | `INTEGRATION REQUIRED` only for later bounded runtime geology services. |
| Particles/audio | `INTEGRATION REQUIRED` only for later bounded result-event consumers. |
| Debugging | Typed source/deposited/unsettled, volume, work, readback and CPU timing diagnostics plus field-stage inspection `PASS`. |
| Visualization | Three non-empty 960x720 runtime captures, visually inspected and byte-identical across fresh processes: `PASS`; no bespoke Beauty scene claimed. |
| Profiling | `PASS`: 85.3503/90.3135 ms Release CPU settling, zero Colluvium readback bytes and 0.059392 ms focused static PBR GPU frame recorded. |
| Performance budgets | `PASS` for this boundary: 16,777,216 topology voxels, 64 transport cells and 67,108,864 work items; full upload/remesh remains an explicit optimization ceiling. |
| Tooling/UI | Existing typed recipe/mask consumer `PASS`; no bespoke generator, preset or restored Geology UI. |
| Testing | CPU semantics, schema, topology, productive Vulkan, cancellation, determinism, cook, collision and runtime replay `PASS`. |
| Documentation | Research, design, plan, blueprint, closure and this re-audit match the implementation. |
| Failure handling | Stable invalid-lineage/mask/gravity/work/cancellation failures and publish-after-success semantics `PASS`. |

### Data-lineage conclusions

Rockfall component and detached-set identities plus exact voxel volume survive
into Colluvium diagnostics. Every detached source is represented exactly once as
deposited or unsettled, preventing silent material loss. Deposited cells retain
upstream strata, fracture, erosion, cavity, curvature and exposure information,
set the existing deposition byte and clear only the Rockfall-owned fresh bit.
The existing Surface Nets interpolation carries that classification into the
cooked mesh without introducing another semantic field.

The runtime product intentionally excludes authoring controls and transient
transport state. A later runtime geology system must not reconstruct source,
remainder or event identity from the static mesh; it requires separately
versioned cooked contracts.

### Remaining integration boundary

Imported-mesh structural operators are the only remaining P1 operator.
Runtime remesh/destruction/save-load, Jolt physical properties, classified
LOD/world-cell streaming, navigation/gameplay/Luau/events/particles/audio,
semantic editor visualization and arbitrary-gravity/fluid transport remain
`INTEGRATION REQUIRED` with the seams and owners recorded above.

Completing Colluvium does not complete P1, the Geology blueprint, the later
PWAS/EPLS production expansion or JericoEngine 1.0.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`

---

## Durability Graph re-audit — 2026-08-28

The slice `M8-GEOLOGY-BLUEPRINT-P1-DURABILITY-GRAPH` adds one typed semantic
field to the existing static classified-geology vertical. Its provenance,
contract, measurements, negative boundaries and product evidence are recorded in
`M8_GEOLOGY_BLUEPRINT_P1_DURABILITY_GRAPH_CLOSURE.md`.

The complete proven lineage is:

```text
typed source hardness + structural classification + fracture weakening
-> schema-18 DurabilityField graph, mask and strength
-> deterministic CPU reference / resident Diligent-Vulkan durability field
-> immediately downstream existing Bubble or Curvature weathering
-> resident scalar result + unchanged classification
-> classified Surface Nets
-> JRMS 1.1 GEOL + exact triangle collision
-> source-free package/static-mesh runtime
-> existing world/triplanar four-material PBR
```

When no weathering consumes it, durability remains authoring/intermediate
semantic data and the static scalar, topology, classification and collision are
exactly identical to the disabled control.

### Slice applicability matrix

| Category | Durability Graph result |
|---|---|
| Source/input data | `PASS`: existing hardness, structure and fracture weakening feed the typed normalized contract. |
| Asset pipeline | `PASS`: consumed weathering products use the existing classified JRMS/collision/package path; durability-only static identity is exact. |
| Editor/authoring workflow | Existing generic recipe/mask/preview/transaction seams apply; durability visualization/painting is `INTEGRATION REQUIRED`. |
| ECS/components | Existing static mesh component consumption remains sufficient; no geology-specific component was added. |
| Runtime representation | `PASS` for the final static product; the authoring durability field itself is deliberately not shipped. |
| CPU processing | `PASS`: independent deterministic reference, typed domain and declared interpolation/composition. |
| GPU processing | `PASS`: dedicated resident Vulkan evaluation, explicit fracture SRV, parity, dirty tracking and cancellation. |
| Renderer | `PASS`: existing Diligent static-mesh owner, one draw and zero fallback. |
| Shaders | `PASS`: dedicated `durabilityMain` compute entrypoint plus the existing classified PBR consumer. |
| Materials | `PASS`: existing bounded four-material descriptor and three cooked textures. |
| Physics/collision | Exact triangle geometry `PASS`; versioned Jolt physical durability is `INTEGRATION REQUIRED`. |
| Animation/Sequencer | `NOT APPLICABLE` to this static scalar operator. |
| Scripting/Luau/events | `INTEGRATION REQUIRED` only for later authorized runtime query/change consumers. |
| Serialization | `PASS`: schema-18 recipe round-trip and stable identity; runtime product remains source-free. |
| Save/load | Static cooked load `PASS`; mutable runtime durability/geology state is `INTEGRATION REQUIRED`. |
| Streaming/LOD | Package dependency closure `PASS`; classified LOD/world-cell representation is `INTEGRATION REQUIRED`. |
| Destruction/runtime modification | `INTEGRATION REQUIRED`; no runtime remesh or destruction behavior was added. |
| Navigation/gameplay queries | `INTEGRATION REQUIRED`. |
| Particles/audio | `INTEGRATION REQUIRED` for later geological change/event consumers. |
| Debugging | `PASS`: stable validation, inspection, hashes, diagnostics and mutation evidence. |
| Visualization | Package-only non-empty classified runtime result `PASS`; no new bespoke Beauty was required. |
| Profiling/performance budgets | `PASS`: 2-8 keys, checked 276,480-item proof and hard 268,435,456-item admission ceiling. |
| Tooling/UI | Generic typed recipe path `PASS`; bespoke geology UI intentionally absent. |
| Testing | `PASS`: RED chronology, CPU/GPU parity, order/fallback, identities, cook/collision/package/replay. |
| Documentation | Research, design, plan, closure and this re-audit match the implementation. |
| Failure handling | `PASS`: stable invalid-domain, graph, ordering, cancellation and overflow rejection. |

### Data-lineage conclusions

Useful upstream hardness, structural classification and fracture weakening are
not collapsed into a material label. They produce a typed normalized durability
field with its own stable identity and inspection path. An existing weathering
stage consumes only the most recent durability field ordered before it; a field
ordered later cannot affect earlier output. This makes the dependency explicit
and prevents accidental retroactive or stale consumption.

Structural classification remains unchanged and continues to the existing
renderer/material/collision vertical. The durability field is not serialized
into the source-free static runtime package because no runtime consumer is
authorized in this slice. That omission is deliberate, verified by binary marker
scans and recorded below rather than being mistaken for completed runtime
durability.

### Remaining `INTEGRATION REQUIRED`

1. **Jolt physical durability.** Required for physical break/resistance queries.
   Expected input is a versioned cooked physical property or classified-region
   lookup; owner is a future Physics/RuntimeFormats integration slice after its
   contract is authorized.
2. **Classified LOD and streaming.** Required if durability or derived change
   semantics must survive world-cell/LOD transitions. Expected seam is the
   existing classified static product plus a versioned optional durability/change
   representation; owner is the future VirtualizedWorld/geology integration.
3. **Runtime remesh, destruction and save/load.** Required before mutable geology
   can persist or expose new surfaces. Expected inputs are stable region identity,
   durability/change events and classified remesh output; owner is a later
   explicitly authorized runtime geology/destruction slice.
4. **Navigation, gameplay/Luau, events, particles and audio.** Required only when
   runtime geological queries/changes are admitted. Expected seam is a bounded
   query and result-event service, not direct access to authoring fields; owners
   remain their respective future integration slices.
5. **Editor semantic durability view/painting.** Required for direct designer
   authoring. It must use the admitted procedural preview/diff/transaction/
   undo-redo bridge; owner is a future explicitly authorized editor slice.
6. **Later structural operators.** Rockfall, colluvium and imported-mesh
   structural operators remain separate P1 slices with their own research,
   contracts and evidence.

Completing Durability Graph does not complete P1, the Geology blueprint, the
later PWAS/EPLS production expansion or JericoEngine 1.0.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`

---

## Directed Crack Swept Volume re-audit — 2026-08-28

The slice `M8-GEOLOGY-BLUEPRINT-P1-DIRECTED-CRACK-SWEPT-VOLUME` adds one
artist-directed volumetric subtraction to the existing static classified-geology
vertical. Its exact research boundary, contracts, measurements, package and
paired Beauty evidence are recorded in
`M8_GEOLOGY_BLUEPRINT_P1_DIRECTED_CRACK_SWEPT_VOLUME_CLOSURE.md`.

The additional lineage is:

```text
canonical recipe splines / branch and profile data
-> bounded deterministic prepared segment graph
-> implicit-surface planar projection
-> swept negative-inside carving volume
-> max(source, -volume)
-> existing classified Surface Nets / JRMS / collision / PBR lineage
```

### Slice applicability matrix

| Category | DirectedCrack result |
|---|---|
| Source/input data | `PASS`: typed schema-17 layer reuses canonical recipe splines and stable GUIDs. |
| Asset pipeline | `PASS`: classified JRMS 1.1, exact collision and two eight-entry comparison packages. |
| Editor/authoring workflow | Generic spline/recipe/preview transactions remain applicable; crack-specific UI and semantic view are `INTEGRATION REQUIRED`. |
| ECS/components | Existing static mesh consumption is sufficient; no geology-specific component added. |
| Runtime representation | `PASS`: existing field result and optional `GEOL`; no new runtime format. |
| CPU processing | `PASS`: independent deterministic graph preparation and reference evaluator. |
| GPU processing | `PASS`: bounded dedicated compute entry point inside the existing resident `GeologyGpuSession`. |
| Renderer/shaders/materials | `PASS`: existing world/triplanar PBR, four materials, three textures and zero fallback. Fresh crack-surface semantics are `INTEGRATION REQUIRED`. |
| Physics/collision | Exact triangle collision `PASS`; physical geology properties remain `INTEGRATION REQUIRED`. |
| Animation/Sequencer | `NOT APPLICABLE` to this static authoring operator. |
| Scripting/Luau/events | `INTEGRATION REQUIRED` only for future runtime query/change consumers. |
| Serialization/save-load | Recipe round-trip and static runtime product load `PASS`; mutable runtime geology state remains deferred. |
| Streaming/LOD | Source-free package closure `PASS`; classified LOD/world-cell propagation remains `INTEGRATION REQUIRED`. |
| Destruction/runtime modification | `INTEGRATION REQUIRED`; this slice adds no durability, runtime remesh or destruction. |
| Navigation/gameplay/particles/audio | `INTEGRATION REQUIRED` under the existing bounded geology-query/event plan. |
| Debugging/visualization | Field inspection, width/depth/topology gates and paired Shipping Beauty `PASS`; editor semantic view remains deferred. |
| Profiling/performance budgets | `PASS`: checked work ceiling, bounded dirty voxels and productive RTX 3090 proof. |
| Tooling/UI | Canonical data seams `PASS`; no bespoke generator, preset or restored Geology UI. |
| Testing/failure handling | `PASS`: RED chronology, stable rejection, CPU/GPU parity, identities, cancellation, flood-fill/manifold, cook/package/replay. |
| Documentation | Research, design, plan, blueprint status, closure and this re-audit match implementation. |

### Data-lineage conclusions

The operator changes signed distance while passing the existing structural
classification field through exactly. Existing strata, fracture, erosion,
deposition, cavity, curvature, exposure, fresh and structural-cell information
therefore remains available to the admitted Surface Nets/material path.

The operator does not yet publish a distinct newly exposed crack-surface semantic.
That information is useful upstream but changing the shared classification schema
and its rendering/physics/destruction/gameplay/audio/particle consumers belongs to
a separately authorized integration slice and remains `INTEGRATION REQUIRED`.

### Remaining integration boundary

Physical properties, classified LOD/streaming, mutable runtime geology,
destruction/save-load, navigation/gameplay/Luau/events/particles/audio and editor
semantic visualization retain their documented owners and expected seams. The
durability graph, rockfall, colluvium, imported-mesh structural operators and all
P2/research items remain open.

Completing Directed Crack Swept Volume does not complete P1, the Geology
blueprint, the later PWAS/EPLS production expansion or JericoEngine 1.0.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`

---

## Post-integration re-audit — 2026-08-28

The dedicated
`M8-GEOLOGY-SYSTEM-GEOLOGICAL-MATERIAL-INTEGRATION` vertical implemented and
proved items 1-3 of the original `INTEGRATION REQUIRED` list, plus the generic
cooked editor/runtime consumption part of item 4. Full measurements, negative
contracts, package provenance and original-resolution evidence are recorded in
`M8_GEOLOGY_SYSTEM_GEOLOGICAL_MATERIAL_INTEGRATION_CLOSURE.md`.

The actual lineage is now:

```text
BSP + typed geology recipe
-> Vulkan scalar field + packed classification field
-> classified Surface Nets GeologySurfaceMesh
-> JRMS 1.1 optional GEOL + exact triangle collision
-> existing AssetRegistry/package/static-mesh upload
-> canonical world-space/triplanar PBR geology permutation
-> visible classified material appearance
```

The representation is the measured minimum selected by the slice: ordinary
meshes remain 48-byte vertices and JRMS 1.0; geological meshes add an optional
eight-byte record per unique vertex and a bounded four-material descriptor. The
representative mesh carries 56,608 classification bytes, changes file size from
506,398 to 563,210 bytes (11.22%), changes vertex fetch from 48 to 56 bytes
(16.67%), uses 562,432 total geometry device bytes and records a focused
single-frame PBR GPU metric of 0.523264 ms on the RTX 3090.

The package-only Shipping proof contains one mesh, four ordinary PBR materials
and three cooked Poly Haven `rock_surface` textures. AssetDump validates all eight
entries; two visually inspected replays produced the same SHA-256
`223CAE3C8308E7D5FC7E609F9F0EF8EFF843109DEC469EBE849C12E14D004EFA`
with four materials, three textures, zero fallbacks and `source_free=true`.

### Reclassified matrix

| Category | Post-integration result |
|---|---|
| Source/input data | `PASS` for static classified geology. |
| Asset pipeline | `PASS` for direct classified mesh/collision/palette/package cook. |
| Editor/authoring workflow | Generic cooked viewport consumption `PASS`; semantic mask UI remains `INTEGRATION REQUIRED`. |
| ECS/components | Existing static mesh component path `PASS`; no geology-specific component. |
| Runtime representation | `PASS`: bounded optional JRMS 1.1 `GEOL`; ordinary 1.0 unchanged. |
| CPU processing | `PASS`: deterministic codec/reference/cook. |
| GPU processing | `PASS`: same-dispatch semantics and classified Surface Nets. |
| Renderer | `PASS`: existing Diligent static-mesh owner. |
| Shaders | `PASS`: canonical triplanar PBR permutation, no UV dependency. |
| Materials | `PASS`: bounded palette of existing PBR materials, zero fallback. |
| Physics/collision | Geometry identity `PASS`; physical geology properties remain `INTEGRATION REQUIRED`. |
| Animation | `NOT APPLICABLE` to this static vertical. |
| Scripting/Luau | `INTEGRATION REQUIRED`. |
| Events | `INTEGRATION REQUIRED`. |
| Sequencer | `NOT APPLICABLE` until geological parameters become authorized animated state. |
| Serialization | `PASS` for static mesh/classification/descriptor products. |
| Save/load | Static cooked load `PASS`; mutable geology save state remains `INTEGRATION REQUIRED`. |
| Streaming | Package dependency closure `PASS`; geology-aware world cells remain `INTEGRATION REQUIRED`. |
| LOD | `INTEGRATION REQUIRED`. |
| Destruction/runtime modification | Bounded consumer-side `fresh` replacement `PASS`; producer/remesh/destruction remain `INTEGRATION REQUIRED`. |
| Navigation/gameplay queries | `INTEGRATION REQUIRED`. |
| Particles | `INTEGRATION REQUIRED`. |
| Audio | `INTEGRATION REQUIRED`. |
| Debugging | Stable errors/traces `PASS`; editor class views remain deferred. |
| Visualization | Direct and package-only Beauty `PASS`. |
| Profiling | Exact stream/device/sample/GPU measurements recorded: `PASS`. |
| Performance budgets | Bounded stream/palette and fixed nine-sample shader budget: `PASS` for this vertical. |
| Tooling/UI | Generic consumer `PASS`; bespoke geology UI intentionally absent. |
| Testing | Codec, cook, renderer, collision, package and source-free gates `PASS`. |
| Documentation | Design, closure, audit and handoff match implementation: `PASS`. |
| Failure handling | Stable validation/fallback/source boundaries `PASS`; proof-harness first-frame pixel validation is a recorded limitation. |

### Items still `INTEGRATION REQUIRED`

The original items 5-8 remain intentionally open: Jolt physical properties;
classified LOD/streaming; runtime remesh/destruction/save; and navigation,
gameplay/Luau, events, particles and audio consumers. Stable structural fracture
ownership and editor semantic mask views are also still required before their
respective broader systems can be called complete. Their inputs, expected seams,
owners and dependencies remain those documented in the original audit and the
closure record; this slice did not silently absorb them.

Completing Geological Material Integration does not complete P1, the Geology
blueprint, the later production expansion or JericoEngine 1.0.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`

---

## Gradient-Based Implicit Detail Warp re-audit — 2026-08-28

The slice
`M8-GEOLOGY-BLUEPRINT-P1-GRADIENT-BASED-IMPLICIT-DETAIL-WARP` deepens the
existing static classified-geology vertical without changing the post-material
integration ownership described above. Its exact contracts, measurements,
negative results, package and paired Beauty evidence are recorded in
`M8_GEOLOGY_BLUEPRINT_P1_GRADIENT_DETAIL_WARP_CLOSURE.md`.

The additional lineage is:

```text
existing IrregularPolyhedral ImplicitBlock field
-> one-voxel central gradient + local triplanar existing fBm
-> inward candidate sample
-> occupancy-preserving negative-inside min composition
-> existing classified Surface Nets / cook / PBR lineage
```

### Slice applicability matrix

| Category | GradientDetailWarp result |
|---|---|
| Source/input data | `PASS`: canonical typed schema-16 layer over an existing ImplicitBlock composition. |
| Asset pipeline | `PASS`: classified JRMS 1.1 plus exact collision and eight-entry package. |
| Editor/authoring workflow | Existing generic recipe/mask/preview transaction seams remain applicable; semantic mask UI is `INTEGRATION REQUIRED`. |
| ECS/components | Existing static mesh component consumption remains sufficient; no geology-specific component added. |
| Runtime representation | `PASS`: existing field evaluation and optional `GEOL`; no new runtime format. |
| CPU processing | `PASS`: independent deterministic reference with declared finite-difference and sign semantics. |
| GPU processing | `PASS`: resident Diligent/Vulkan ping-pong pass, bounded topology expansion and cancellation. |
| Renderer/shaders/materials | `PASS`: existing classified world/triplanar PBR path, four materials, three textures, zero fallback. |
| Physics/collision | Exact triangle geometry `PASS`; physical geology properties remain `INTEGRATION REQUIRED`. |
| Animation/Sequencer | `NOT APPLICABLE` to this static scalar operator. |
| Scripting/Luau/events | `INTEGRATION REQUIRED` only for later runtime query/change consumers. |
| Serialization/save-load | Recipe round-trip and static product load `PASS`; mutable runtime geology save state remains deferred. |
| Streaming/LOD | Package closure `PASS`; classified LOD/world-cell integration remains `INTEGRATION REQUIRED`. |
| Destruction/runtime modification | `INTEGRATION REQUIRED`; no DirectedCrack or runtime remesh was added. |
| Navigation/gameplay/particles/audio | `INTEGRATION REQUIRED` under the existing bounded geology-query/event plan. |
| Debugging/visualization | Field-stage inspection and paired input/detail Shipping Beauty `PASS`; editor semantic view remains deferred. |
| Profiling/performance budgets | `PASS`: bounded validation/admission, dirty metrics and `0.084544 ms` focused Release GPU duration. |
| Tooling/UI | Generic layer/mask contract `PASS`; no bespoke generator, preset or restored Geology UI. |
| Testing/failure handling | `PASS`: RED chronology, CPU/GPU parity, identities, manifold, cancellation, cook/package/replay and stable errors. |
| Documentation | Research, design, plan, closure and this re-audit match the implementation. |

### Data-lineage conclusions

All upstream semantic classification remains byte-identical through the detail
pass; the operator changes only signed distance. It therefore does not destroy
strata, fracture, erosion, deposition, cavity, curvature, exposure, fresh or
structural-cell information needed by downstream rendering and future physical,
destruction or gameplay consumers. The final Surface Nets extraction samples the
same resident classification field already admitted by Geological Material
Integration.

The layer is deliberately not exact SDF redistance. Consumers that require exact
distance magnitude must use a separately authorized re-distance stage. Surface
extraction, collision and static rendering require coherent sign/topology here
and are proved; no downstream consumer may silently reinterpret this field as an
exact metric SDF.

### Remaining integration boundary

The previously listed physical properties, classified LOD/streaming, runtime
remesh/destruction/save-load and navigation/gameplay/Luau/events/particles/audio
items remain `INTEGRATION REQUIRED` with unchanged expected seams and owners.
The five later P1 operators and all eight P2/research features also remain open.

Completing Gradient-Based Implicit Detail Warp does not complete P1, the Geology
blueprint, the later PWAS/EPLS production expansion or JericoEngine 1.0.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`

---

## SpeedRock-like block grammar source re-audit — 2026-08-29

`M8-GEOLOGY-BLUEPRINT-P2-SPEEDROCK-LIKE-BLOCK-GRAMMAR-SOURCE` selects the
smallest coherent `BOUNDED_TYPED_BLOCK_MASS_SOURCE` boundary. Its primary-source
research, independent design, RED chronology, implementation and exact evidence
are retained in
`M8_GEOLOGY_BLUEPRINT_P2_BLOCK_GRAMMAR_SOURCE_CLOSURE.md`.

The proven lineage is:

```text
schema-21 typed grammar + deterministic seed
-> bounded synchronous CPU reference
-> coalesced additive source boxes
-> existing Vulkan scalar/classification field
-> existing Surface Nets / JRMS / JRCL cook
-> six-entry source-free package
-> existing static PBR Vulkan consumer
```

### Slice applicability matrix

| Category | Block-grammar result |
|---|---|
| Source/input data | `PASS`: fixed alphabet, coordinates, rewrite/seed/termination and work ceilings. |
| Asset/editor authoring | Canonical recipe/schema/round-trip `PASS`; dedicated UI, final presets and semantic views remain deferred. |
| CPU/GPU processing | Bounded CPU reference and existing resident Vulkan field evaluation with parity `PASS`. |
| Extraction/cook/runtime | Classified Surface Nets, exact collision, six-entry package and package-only replay `PASS`. |
| Renderer/materials | Existing PBR Vulkan path, one material, three textures and zero fallback `PASS`. |
| Physics | Exact static triangles `PASS`; geology physical properties remain `INTEGRATION REQUIRED`. |
| Serialization/save-load | Static recipe/products `PASS`; mutable grammar/runtime save identity remains deferred. |
| Streaming/LOD/destruction | Classified LOD/world cells and runtime rewrite/remesh/collision/save remain `INTEGRATION REQUIRED`. |
| Gameplay/Luau/events/navigation/particles/audio | `INTEGRATION REQUIRED` behind later bounded query/result seams. |
| Debug/profiling/failure handling | Stable errors, identity/timing, fixed budgets, cancellation and inspected Beauty `PASS`. |
| Testing/documentation | RED chronology, determinism/seed/negative/schema, Vulkan/cook/package/replay and durable evidence `PASS`. |

### Data-lineage conclusions

Grammar symbols are deliberately generation-time occupancy identity, not
geological material classification. The source maps empty/non-empty cells into
Jerico's declared negative-inside scalar convention, after which the existing
classification field remains authoritative through extraction and cook. No
upstream geological field is discarded or silently synthesized from rule color
or rule ID.

The final resident field and extracted surface publish the same stable field
hash, `18320962864667727086`. The source hash
`7564926986005090053` and recipe hash `10528138175544731394` retain upstream
identity separately. Runtime consumes only cooked Jerico assets and reports
`source_free=true`.

### Remaining integration boundary

At this slice checkpoint, the seven later ordered P2/research features remained
open: evolution/search, aperiodic tiling, contact-preserving erosion, Dorsey
narrow-band mineral weathering, moisture transport, mineral dissolution and
multi-scale microstructure synthesis. Subsequent re-audits below record their
later closure state. Neighborhood erosion, physical properties,
classified LOD/streaming, mutable runtime geology/destruction/save-load,
semantic editor tools and navigation/gameplay/Luau/events/particles/audio also
remain `INTEGRATION REQUIRED` with their existing owners and dependencies.

Completing this source does not complete P2, the Geology blueprint, the later
production expansion or JericoEngine 1.0.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`

---

## Bounded block-grammar evolution/search re-audit — 2026-08-29

`M8-GEOLOGY-BLUEPRINT-P2-EVOLUTION-SEARCH` adds the smallest coherent
`BOUNDED_DETERMINISTIC_RECIPE_MUTATION_SEARCH` above the already admitted typed
block-grammar source. Its primary-source research, independent design, RED
chronology, implementation and exact evidence are retained in
`M8_GEOLOGY_BLUEPRINT_P2_EVOLUTION_SEARCH_CLOSURE.md`.

The additional lineage is:

```text
existing schema-21 block-grammar recipe
-> deterministic bounded one-gene search request
-> immutable objective/diff/identity preview batch
-> explicit designer-selected candidate
-> existing schema-21 recipe with one source revision increment
-> existing Vulkan field / Surface Nets / cook / package / Shipping replay
```

### Slice applicability matrix

| Category | Bounded evolution/search result |
|---|---|
| Source/input data | `PASS`: only a valid existing schema-21 block-grammar recipe; frozen non-genome fields are verified. |
| Asset pipeline | `PASS`: selected output remains the existing recipe and six-entry cooked package; no new schema or format. |
| Editor/authoring workflow | Pure immutable preview/diff/select materialization `PASS`; dedicated UI, transaction wrapper and bespoke presets remain `INTEGRATION REQUIRED`. |
| ECS/components | Existing static mesh consumption remains sufficient; no optimizer or geology component added. |
| Runtime representation | Existing cooked mesh/collision/material/texture assets only; runtime search state is deliberately absent. |
| CPU processing | `PASS`: integer objective, deterministic mutation/ranking, bounded stopping, cancellation and work/memory admission. |
| GPU processing | Search is CPU authoring work; selected output passes the existing Vulkan scalar/classification parity gate. |
| Renderer/shaders/materials | `PASS`: existing static PBR path, one material, three textures and zero fallback. |
| Physics/collision | Exact selected static triangles `PASS`; physical geology properties remain `INTEGRATION REQUIRED`. |
| Animation/Sequencer | `NOT APPLICABLE` to this static authoring search. |
| Scripting/Luau/events | `INTEGRATION REQUIRED` only for later bounded geology query/change consumers. |
| Serialization/save-load | Canonical recipe/result identities and static cooked load `PASS`; mutable runtime search/save state is out of scope. |
| Streaming/LOD | Source-free package closure `PASS`; classified LOD/world cells remain `INTEGRATION REQUIRED`. |
| Destruction/runtime modification | `INTEGRATION REQUIRED`; no runtime evolution, rewrite, remesh, retopology or mutable collision. |
| Navigation/gameplay/particles/audio | `INTEGRATION REQUIRED` behind later geology query/event seams. |
| Debugging/visualization | Stable errors, exact preview diffs/metrics and inspected original Vulkan Beauty `PASS`; semantic editor views remain deferred. |
| Profiling/performance budgets | `PASS`: at most 57 evaluations, fixed 67,108,864 work and 65,536 retained-byte ceilings; accepted fixture uses 258 work items. |
| Testing/failure handling | `PASS`: RED, determinism/seed, objective/diff/rank/stops, invalid/cancel/exhaustion/no-candidate, stale/tamper and full runtime lineage. |
| Documentation | Research, design, plan, closure and this re-audit match implementation. |

### Data-lineage conclusions

The search never invents a parallel asset or field representation. Base recipe
identity, objective, limits and search seed form the request identity; every
candidate retains exact rule/seed diff, source-result hash and canonical recipe
hash. The selected fixture improves integer objective error from `1000` to `0`
with one changed rule bit, then publishes source hash `6420089136575985511`,
recipe hash `17711715928159991393` and final field/surface hash
`4123199065062627126` through the existing vertical.

Grammar rule IDs remain authoring/generation identity rather than geological
material semantics. Existing classification remains authoritative downstream;
no stratum, fracture, erosion, deposition, cavity, durability or wetness data is
discarded or falsely synthesized by the search.

### Remaining integration boundary

At this slice checkpoint, the six later ordered P2/research features remained
open: aperiodic tiling, contact-preserving erosion, Dorsey narrow-band mineral
weathering, moisture transport, mineral dissolution and multi-scale
microstructure synthesis. Subsequent re-audits below record their later state.
Neighborhood erosion, dedicated editor transactions/UI, geology physical
properties, classified LOD/world cells, mutable runtime geology/destruction/
save-load, and navigation/gameplay/Luau/events/particles/audio remain
`INTEGRATION REQUIRED` with their existing owners and dependencies.

Completing bounded evolution/search does not complete P2, the Geology blueprint,
the later production expansion or JericoEngine 1.0.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`

---

## Bounded deterministic corner-lattice source re-audit — 2026-08-29

`M8-GEOLOGY-BLUEPRINT-P2-APERIODIC-TILING` adds the smallest coherent
`BOUNDED_DETERMINISTIC_CORNER_LATTICE_SOURCE` boundary. Its primary-source
research, independent design, RED chronology, implementation and exact evidence
are retained in
`M8_GEOLOGY_BLUEPRINT_P2_APERIODIC_TILING_CLOSURE.md`.

The proven lineage is:

```text
schema-22 finite extent/counts + existing source seed
-> canonical definition/source/recipe identity
-> shared lattice colors + ordered corner signatures
-> bounded cell-confined additive boxes
-> existing Vulkan scalar/classification field
-> existing classified Surface Nets / JRMS / JRCL cook
-> six-entry source-free package
-> existing static PBR Vulkan consumer
```

### Slice applicability matrix

| Category | Bounded corner-lattice result |
|---|---|
| Source/input data | `PASS`: exact finite definition, 1..16 axis counts, seed, termination and overflow-safe construction. |
| Asset pipeline | `PASS`: cumulative schema-22 canonical round-trip/hash and existing six-entry cook/package path. |
| Editor/authoring workflow | Immutable preview/diff and explicit accepted recipe `PASS`; UI, transaction wrapper and bespoke presets remain `INTEGRATION REQUIRED`. |
| ECS/components | Existing cooked static-mesh consumption is sufficient; no runtime tiling component was added. |
| Runtime representation | Existing cooked mesh/collision/material/textures only; source colors/signatures remain authoring identity. |
| CPU processing | `PASS`: shared-coordinate hash, canonical signatures, confined boxes, exact budgets and no-partial cancellation. |
| GPU processing | `PASS`: existing guarded Vulkan field consumes materialized boxes with CPU parity at every box interior. |
| Renderer/shaders/materials | `PASS`: unchanged static PBR Vulkan path, one material, three textures and zero fallback. |
| Physics/collision | Exact cooked static triangles `PASS`; geology physical properties remain `INTEGRATION REQUIRED`. |
| Animation/Sequencer | `NOT APPLICABLE` to this static bounded source. |
| Scripting/Luau/events | `INTEGRATION REQUIRED` only for later bounded geology query/change consumers. |
| Serialization/save-load | Canonical static recipe/products `PASS`; mutable runtime tiling/save state is deliberately absent. |
| Streaming/LOD | Package closure `PASS`; classified LOD/world cells remain `INTEGRATION REQUIRED`. |
| Destruction/runtime modification | `INTEGRATION REQUIRED`; no runtime construction, remesh, retopology or mutable collision. |
| Navigation/gameplay/particles/audio | `INTEGRATION REQUIRED` behind later geology query/event seams. |
| Debugging/visualization | Stable preview/diff/errors plus inspected deterministic original Vulkan Beauty `PASS`; semantic editor views remain deferred. |
| Profiling/performance budgets | `PASS`: hard 9,009 work/1 MiB retention ceilings; fixture uses 84 work items, 1,620 bytes and 2.338432 ms focused Release GPU time. |
| Testing/failure handling | `PASS`: RED, schema/hash/seed/diff, adjacency/confinement, invalid/overflow/budget/cancel, topology/GPU/cook/package/replay. |
| Documentation | Research, design, plan, closure, metrics and this re-audit match implementation. |

### Data-lineage conclusions

Shared corner colors and tile signatures remain explicit immutable authoring
identity through the source result and source hash. They deliberately do not
become geological material classes or runtime state. Stable box GUIDs include
source content identity, coordinate and signature; the existing classification
field remains authoritative downstream through extraction and cook.

The source does not discard or synthesize strata, fractures, erosion,
deposition, cavities, curvature, exposure, fresh-surface, durability, wetness
or physical-property information. Runtime consumes only the resulting cooked
Jerico mesh, collision, material and textures and reports `source_free=true`.

### Remaining integration boundary

At this slice checkpoint, the five later ordered P2/research features remained
open: contact-preserving erosion, Dorsey narrow-band mineral weathering,
moisture transport, mineral dissolution and multi-scale microstructure
synthesis. Subsequent re-audits below record their later state. Neighborhood erosion,
dedicated editor transactions/UI, geology physical properties, classified
LOD/world cells, mutable runtime geology/destruction/save-load and navigation/
gameplay/Luau/events/particles/audio remain `INTEGRATION REQUIRED` with their
existing owners and dependencies.

Completing bounded corner-lattice tiling does not complete P2, the Geology
blueprint, the later production expansion or JericoEngine 1.0.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`

---

## Bounded explicit-contact damped erosion re-audit — 2026-08-29

`M8-GEOLOGY-BLUEPRINT-P2-CONTACT-PRESERVING-EROSION` adds the smallest coherent
`BOUNDED_EXPLICIT_CONTACT_DAMPED_EROSION` boundary. Its primary-source research,
independent design, observable RED chronology, implementation and exact Release
evidence are retained in
`M8_GEOLOGY_BLUEPRINT_P2_CONTACT_PRESERVING_EROSION_CLOSURE.md`.

The proven lineage is:

```text
schema-23 explicit contacts + scalar controls + seed/strength/mask
-> canonical serialization and recipe identity
-> finite contact/domain/work/byte admission
-> 128-byte contact block in existing per-layer Vulkan constants
-> one resident SDF/classification pass
-> classified Surface Nets
-> JRMS 1.1 GEOL + JRCL
-> source-free six-entry package
-> static Vulkan PBR and collision consumers
```

One to eight contacts are finite, exactly float-preserved, ordered and unique.
The pass returns the input field exactly inside any declared preservation
radius, erodes only admitted inside-surface points, terminates after one finite
active-voxel pass and fails before publication on invalid input, cancellation,
domain mismatch, arithmetic overflow or exhausted work/memory budgets. The
maximums are eight contacts, 67,108,864 contact-distance evaluations and 128
retained contact bytes.

The Release fixture preserves `-0.08 -> -0.08` at the protected witness while
producing `0.121478` useful outside wear. It emits 3,744 vertices and 22,440
indices with stable recipe/final/surface identity. A six-entry source-free
package replays twice on RTX 3090 Vulkan with one material, three textures, zero
fallback and byte-identical inspected 1280x720 Beauty frames.

Source/input, recipe serialization, CPU reference/diff/admission, GPU field,
classification, mesh/collision, renderer/material, cook/package, cancellation,
failure, debugging, budgets, tests and documentation apply and are proven.
Animation, Sequencer and mutable runtime simulation do not apply to this static
authoring slice.

### Remaining integration boundary

Dedicated editor transactions/UI, physical contact production/Jolt properties,
classified LOD/world cells, mutable runtime geology/destruction/save-load and
navigation/gameplay/Luau/events/particles/audio remain `INTEGRATION REQUIRED`
with the interfaces, owners and milestones enumerated in the slice closure.

At this slice checkpoint, the four later ordered P2/research features remained
open: Dorsey narrow-band mineral weathering, moisture transport, mineral
dissolution and multi-scale microstructure synthesis. Subsequent re-audits below
record their later state. Completing contact-preserving erosion does not
complete P2, the Geology blueprint, the later production expansion or
JericoEngine 1.0.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`

---

## Bounded interior mineral-response band re-audit — 2026-08-29

`M8-GEOLOGY-BLUEPRINT-P2-DORSEY-NARROW-BAND-MINERAL-WEATHERING` adds the
smallest coherent `BOUNDED_INTERIOR_MINERAL_RESPONSE_BAND` boundary. Its
primary-source research, independent design, four observable RED gates,
implementation and exact Release evidence are retained in
`M8_GEOLOGY_BLUEPRINT_P2_DORSEY_NARROW_BAND_MINERAL_WEATHERING_CLOSURE.md`.

The proven lineage is:

```text
schema-24 susceptible mineral class + finite band/output controls + seed/strength/mask
-> canonical recipe serialization and identity
-> checked one-pass active-voxel work admission
-> existing resident Vulkan SDF + packed classification field
-> bit-exact scalar + monotonic weathered-mineral response in deposition byte
-> classified Surface Nets
-> JRMS 1.1 GEOL + JRCL + two-material response palette
-> source-free seven-entry package
-> static Vulkan PBR and collision consumers
```

The operation admits at most one identity-transform layer as the final active
operation after Source, active Strata and active Rebuild. It reads the existing packed Strata mineral class,
operates only for `-interiorBandMeters <= d <= 0`, preserves `d` and every
unrelated classification byte exactly, and raises only the packed deposition
response with `max(old, candidate)`. It terminates after one finite pass, uses
zero additional variable GPU buffers/bytes and rejects more than 67,108,864
pointwise evaluations before dispatch.

### Slice applicability matrix

| Category | Bounded mineral-weathering result |
|---|---|
| Source/input data | `PASS`: existing BSP source, active Strata mineral class and five finite schema-24 controls. |
| Asset pipeline | `PASS`: canonical round-trip/hash and existing JRMS/JRCL seven-entry cook/package path. |
| Editor/authoring workflow | Pure payload/seed diff, immutable preview/cancel and explicit recipe choice `PASS`; dedicated UI/transaction wrapper remains `INTEGRATION REQUIRED`. |
| ECS/components | Existing cooked static-mesh consumption is sufficient; no weathering component or runtime simulation state added. |
| Runtime representation | Existing cooked mesh, collision, two materials and three textures only; authoring payload remains source-side. |
| CPU processing | `PASS`: pointwise reference, exact invariants, pure diff and checked work admission. |
| GPU processing | `PASS`: one finite classification pass in the existing 368-byte layer constants and resident buffers; three inspected witnesses match CPU exactly. |
| Renderer/shaders/materials | `PASS`: response slot two selects the second existing-PBR-pipeline material; two package-only replays report two materials, three textures and zero fallback. |
| Physics/collision | Exact unchanged geometry and cooked static triangles `PASS`; mineral-dependent physical properties remain `INTEGRATION REQUIRED`. |
| Animation/Sequencer | `NOT APPLICABLE` to this static authoring classification. |
| Scripting/Luau/events | `INTEGRATION REQUIRED` only for later read-only mineral/weathering query consumers. |
| Serialization/save-load | Canonical static recipe/products `PASS`; mutable weathering time/state is deliberately absent. |
| Streaming/LOD | Package closure `PASS`; classified LOD/world cells remain `INTEGRATION REQUIRED`. |
| Destruction/runtime modification | `INTEGRATION REQUIRED`; no scalar erosion, remesh, retopology, mutable collision or save state. |
| Navigation/gameplay/particles/audio | `INTEGRATION REQUIRED` behind later cooked geology query/event seams. |
| Debugging/visualization | Stable errors, exact stage classification inspection and original-resolution Beauty `PASS`; semantic editor views remain deferred. |
| Profiling/performance budgets | `PASS`: 67,108,864-work ceiling, 147,456 accepted work items and zero additional variable bytes. |
| Testing/failure handling | `PASS`: RED, schema/hash/seed/diff, band/mineral/invariant/monotonicity, invalid/budget/cancel, CPU/GPU parity, cook/package/replay. |
| Documentation | Research, design, closure, metrics, blueprint status and this re-audit match implementation. |

### Data-lineage conclusions

The existing packed Strata byte remains the source mineral/material class and
is preserved. The weathered-mineral result is a bounded authoring response in
the existing deposition byte; it is not conserved transported mass and must not
be interpreted downstream as physical dissolution or recrystallization. The
classified Surface Nets stream carries both into JRMS GEOL, whose response slot
two selects the weathered palette material. Source, recipe, scalar-field,
classification-stream, cooked product, package and frame identities remain
separately observable.

### Remaining integration boundary

Dedicated editor transactions/UI, a production mineral taxonomy and material
palette workflow, mineral-dependent Jolt properties, classified LOD/world
cells, mutable runtime geology/destruction/save-load and navigation/gameplay/
Luau/events/particles/audio remain `INTEGRATION REQUIRED` with their existing
owners and dependencies.

The later moisture-transport and mineral-dissolution entries are now closed under
their subsequent evidence. Multi-scale microstructure synthesis remains open.
Completing this bounded Dorsey-inspired response does not complete P2, the
Geology blueprint, the later production expansion or JericoEngine 1.0.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`

---

## Bounded normalized moisture-pressure re-audit — 2026-08-29

`M8-GEOLOGY-BLUEPRINT-P2-MOISTURE-TRANSPORT` adds the smallest coherent
`BOUNDED_NORMALIZED_MOISTURE_PRESSURE_PROFILE` boundary. Primary-source
research, independent design, observable CPU/Vulkan RED gates, implementation
and exact Release evidence are retained in
`M8_GEOLOGY_BLUEPRINT_P2_MOISTURE_TRANSPORT_CLOSURE.md`.

The proven lineage is:

```text
schema-25 profile controls + strength/mask/full-width seed
-> canonical recipe serialization and identity
-> checked one-pass work/memory admission
-> existing resident Vulkan SDF/classification/structural/durability inputs
-> separately owned bounded normalized pressure-potential float field
-> bounded pressure inspection + dedicated field hash
-> unchanged classified Surface Nets geometry/classification
-> deterministic JRMS/JRCL cook with schema-25 recipe provenance
-> unchanged static source-free package/runtime boundary, without moisture state
```

The final active layer follows Source, active Strata, active Rebuild and active
Dorsey mineral response. It admits only one finite interior band and one
pointwise dispatch. Surface pressure is a seeded authored source; pressure is
zero at the fixed inner boundary and outside the band. There is no lateral
exchange, state history, moving front or numerical iteration. The result is a
dimensionless normalized pressure potential rather than physical Pa, water mass,
volume, saturation or conserved quantity.

The operation rejects zero or more than 67,108,864 work items before allocation
or dispatch. It owns exactly four additional bytes per admitted voxel, for a
maximum of 268,435,456 bytes, and does not raise any global device limit. Signed
distance, classification, Strata, durability and Dorsey response remain exact.
Recipe validation also rejects an inner boundary beyond the active rebuilt SDF
extent. The CPU uploads already-validated binary32 `sqrt(D * t)`, so the shader
cannot underflow or overflow a binary32 `D * t` product.

### Slice applicability matrix

| Category | Bounded moisture-pressure result |
|---|---|
| Source/input data | `PASS`: existing BSP/field lineage plus five finite schema-25 controls, strength, regional mask and full-width seed. |
| Asset pipeline | `PASS`: canonical round-trip/hash and deterministic existing JRMS/JRCL cook carrying schema-25 recipe provenance. The authoring field is deliberately not packaged. |
| Editor/authoring workflow | Pure payload/seed diff, immutable preview/cancel and explicit recipe choice `PASS`; dedicated pressure visualization/UI/transaction wrapper remains `INTEGRATION REQUIRED`. |
| ECS/components | Existing cooked static-mesh consumption remains sufficient; no runtime moisture component or mutable simulation state was added. |
| Runtime representation | Existing cooked mesh, collision and materials only; the pressure-potential field remains authoring-side. |
| CPU processing | `PASS`: exact pointwise reference, boundary/invariant checks, pure diff and checked work/memory admission. |
| GPU processing | `PASS`: one finite dispatch in the existing geology session, one optional resident float field and three inspected CPU/GPU witnesses. |
| Renderer/shaders/materials | Static geometry/material output remains exact. No moisture renderer/material semantics exist, so Beauty is `NOT APPLICABLE`; future wet response is `INTEGRATION REQUIRED`. |
| Physics/collision | Exact unchanged Surface Nets/cooked static triangles `PASS`; moisture-dependent Jolt properties remain `INTEGRATION REQUIRED`. |
| Animation/Sequencer | `NOT APPLICABLE` to this static authoring pressure profile. |
| Scripting/Luau/events | `INTEGRATION REQUIRED` only for a later read-only cooked moisture query/event seam demanded by a product. |
| Serialization/save-load | Canonical recipe and deterministic static products `PASS`; the transient authoring field is not serialized and mutable wet state/save is absent. |
| Streaming/LOD | Existing static product lineage remains valid; pressure preservation/discard across LOD/world cells remains `INTEGRATION REQUIRED`. |
| Destruction/runtime modification | `INTEGRATION REQUIRED`; no erosion, remesh, retopology, mutable collision or save state. |
| Navigation/gameplay/particles/audio | `INTEGRATION REQUIRED` behind later cooked query/event seams only if demanded. |
| Debugging/visualization | Stable errors, exact stage pressure samples and a dedicated field hash `PASS`; semantic editor visualization remains deferred. |
| Profiling/performance budgets | `PASS`: 67,108,864-work ceiling, 268,435,456-byte ceiling, 147,456 accepted work items and 589,824 measured bytes. |
| Testing/failure handling | `PASS`: RED, schema/hash/seed/diff, boundaries/units/invariants, invalid/order/budget/cancel, CPU/GPU parity, deterministic cook and four backend regressors. |
| Documentation | Research, design, closure, machine-readable metrics, blueprint status and this re-audit match implementation. |

### Data-lineage conclusions

The pressure potential is stored only in its own float field and never aliases
heightfield `waterMeters`, packed classification, Dorsey deposition response,
Strata or durability. Its bounded samples and dedicated hash remain observable
during authoring; the accepted schema-25 recipe remains observable in cooked mesh
provenance after the field lifetime ends. No downstream consumer may silently
reinterpret the field as conserved water, saturation, mineral quantity or
dissolution progress.

Source, recipe, scalar-field, surface/classification, moisture-field and cooked
product identities remain separately observable. The unchanged scalar/surface
hashes and static triangles prove that this layer does not alter geometry or
collision.

### Remaining integration boundary

Dedicated pressure visualization/editor transactions, mineral dissolution,
multi-scale microstructure, a runtime wetness/material contract,
moisture-dependent Jolt properties, classified LOD/world cells, mutable runtime
weather/destruction/save-load and navigation/gameplay/Luau/events/particles/
audio remain `INTEGRATION REQUIRED` with the interfaces, owners and milestones
enumerated in the slice closure.

Mineral dissolution is now closed under its subsequent evidence. Multi-scale
microstructure synthesis remains open. Completing bounded normalized moisture
pressure does not complete P2, the Geology blueprint, the later production
expansion or JericoEngine 1.0.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`

---

## Bounded local mineral-dissolution re-audit — 2026-08-29

`M8-GEOLOGY-BLUEPRINT-P2-MINERAL-DISSOLUTION` adds the smallest coherent
`BOUNDED_LOCAL_MINERAL_DISSOLUTION_FRACTION` boundary. Primary-source research,
independent design, observable CPU/Vulkan RED gates, implementation and exact
Release evidence are retained in
`M8_GEOLOGY_BLUEPRINT_P2_MINERAL_DISSOLUTION_CLOSURE.md`.

The proven lineage is:

```text
schema-24 susceptibility + schema-25 normalized moisture exposure
+ explicit schema-26 local inventory/rate/duration/threshold/frequency
+ strength/mask/full-width seed
-> canonical recipe serialization and identity
-> checked one-pass work admission
-> existing packed classification plus inspected/hashed auxiliary moisture field
-> same auxiliary float buffer overwritten by bounded dissolved fraction
-> dedicated dissolution hash + three CPU/Vulkan inspection witnesses
-> unchanged scalar/classification/Strata/durability/Surface Nets/collision
-> deterministic JRMS/JRCL cook with schema-26 recipe provenance
-> unchanged static source-free package/runtime boundary, without dissolution state
```

For each represented stone voxel, the operation converts the schema-24 response
byte to susceptibility `w`, applies the schema-25 moisture threshold to obtain
activation `a`, and evaluates one seeded bounded pattern. The available local
amount is `M = inventory * w`; the dissolved fraction is
`D = M * (1 - exp(-dose * a * strength * mask * pattern))`; remaining local
amount is `R = M - D`. The invariant `0 <= D <= M <= 1` and the local partition
`D + R = M` hold within the shared CPU/GPU tolerance.

This is a dimensionless local authoring partition, not mineral concentration,
transported mass or a global conservation claim. The schema-24 byte remains
susceptibility and is never decremented. The schema-25 value remains normalized
exposure and is captured before the shared buffer changes semantic. There is no
cross-voxel transport, recrystallization, erosion, geometry removal, history or
numerical iteration.

The operation rejects zero or more than 67,108,864 work items before dispatch.
It reuses the already admitted four-byte-per-voxel auxiliary allocation, whose
maximum remains 268,435,456 bytes, and adds exactly zero dissolution bytes. It
does not add a descriptor, buffer or global device limit. Cancellation or any
failure publishes no candidate result.

### Slice applicability matrix

| Category | Bounded local dissolution result |
|---|---|
| Source/input data | `PASS`: schema-24 susceptibility, schema-25 exposure, explicit five-scalar schema-26 payload, strength, mask and seed. |
| Asset pipeline | `PASS`: canonical schema-26 round-trip/hash and deterministic existing JRMS/JRCL cook carrying recipe provenance. The authoring field is deliberately not packaged. |
| Editor/authoring workflow | Pure payload/seed diff, immutable preview/cancel and explicit recipe choice `PASS`; dedicated dissolution visualization/UI/transaction wrapper remains `INTEGRATION REQUIRED`. |
| ECS/components | Existing cooked static-mesh consumption remains sufficient; no runtime dissolution component or mutable simulation state was added. |
| Runtime representation | Existing cooked mesh, collision and materials only; the dissolved-fraction field remains authoring-side. |
| CPU processing | `PASS`: exact pointwise reference, local partition/invariant checks, pure diff and checked work admission. |
| GPU processing | `PASS`: one finite dispatch in the existing geology session, shared auxiliary-field reuse and three matching CPU/GPU witnesses. |
| Renderer/shaders/materials | Compute shader evaluation is `PASS`; static geometry/material output remains exact. No dissolution renderer/material semantic exists, so Beauty is `NOT APPLICABLE`; a future response is `INTEGRATION REQUIRED`. |
| Physics/collision | Exact unchanged Surface Nets and cooked static triangles `PASS`; mineral-dependent Jolt properties remain `INTEGRATION REQUIRED`. |
| Animation/Sequencer | `NOT APPLICABLE` to this static authoring fraction. |
| Scripting/Luau/events | `INTEGRATION REQUIRED` only for a later read-only cooked mineral query/event seam demanded by a product. |
| Serialization/save-load | Canonical recipe and deterministic static products `PASS`; the transient authoring field is not serialized and mutable dissolution/save state is absent. |
| Streaming/LOD | Existing static product lineage remains valid; dissolution preservation or deliberate discard across LOD/world cells remains `INTEGRATION REQUIRED`. |
| Destruction/runtime modification | `INTEGRATION REQUIRED`; no erosion, scalar removal, remesh, retopology, mutable collision or save state. |
| Navigation/gameplay/particles/audio | `INTEGRATION REQUIRED` behind later cooked query/event seams only if demanded. |
| Debugging/visualization | Stable errors, exact stage samples and a dedicated dissolution-field hash `PASS`; semantic editor visualization remains deferred. |
| Profiling/performance budgets | `PASS`: 67,108,864-work ceiling, unchanged 268,435,456-byte shared-field ceiling, 147,456 accepted work items and zero incremental dissolution bytes. |
| Testing/failure handling | `PASS`: CPU/Vulkan RED, schema/hash/order/diff, zero/monotonic/partition/seed, invalid/budget/cancel, CPU/GPU parity, prior-field preservation and deterministic cook. |
| Documentation | Research, design, closure, metrics, blueprint status and this re-audit match implementation. |

### Data-lineage conclusions

Strata/classification remains classification; schema-24 remains susceptibility;
schema-25 remains exposure; schema-26 inventory is explicit local capacity; and
schema-26 output is a separately identified dissolved fraction. Moisture samples
and identity are captured before the shared auxiliary storage changes semantic.
The final field's bounded samples and dedicated hash remain observable during
authoring, while schema-26 recipe provenance survives in the cooked mesh.

Source, recipe, scalar-field, surface/classification, moisture-field,
dissolution-field and cooked-product identities remain separately observable.
The unchanged scalar/surface identities and static triangles prove that the
operation does not alter geometry or collision.

### Remaining integration boundary

| Subsystem | Required data/interface | Owner/dependency | Milestone |
|---|---|---|---|
| Dedicated editor transaction/UI | Typed payload, dissolved-fraction preview visualization, pure diff and existing preview/cancel/apply transaction | JericoEditor geology authoring | Later Geology editor integration |
| Multi-scale microstructure | Explicit scale hierarchy and bounded synthesis inputs; no implicit dissolution-field reuse | Next ordered Geology P2 owner | `M8-GEOLOGY-BLUEPRINT-P2-MULTI-SCALE-MICROSTRUCTURE-SYNTHESIS` |
| Transport/recrystallization | Explicit species/units, conservation domain, boundaries, finite solver and output ownership | A separately authorized future Geology owner | Feature-driven future research slice |
| Runtime mineral/material response | Cooked bounded field schema and renderer/material semantics if a real product requires them | Geology plus runtime/material owners | Feature-driven production integration |
| Mineral-dependent Jolt properties | Cooked mineral-to-physical-property contract | Geology plus Jolt integration | Production Geology/physics |
| LOD/world-cell streaming | Defined dissolution-field preservation or deliberate discard across simplification/cell cook | Geometry LOD and World Cell owners | Production streaming |
| Mutable destruction/save | Stable modification identity, time/state schema, remesh/collision replacement and persistence | Future runtime modification owner | Post-blueprint production expansion |
| Navigation/gameplay/Luau/events/particles/audio | Read-only cooked mineral query/event seam if demanded by a product | Respective downstream owners | Feature-driven production slices |

Multi-scale microstructure synthesis remains the only open ordered P2/research
entry in the current Geology blueprint. It was not implemented or silently
claimed complete. The complete Geology blueprint must close before the later
PWAS/EPLS production expansion begins.

Completing this slice does not complete P2, the Geology blueprint, the later
production expansion or JericoEngine 1.0.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`

---

## Bounded two-scale microstructure re-audit — 2026-08-29

`M8-GEOLOGY-BLUEPRINT-P2-MULTI-SCALE-MICROSTRUCTURE-SYNTHESIS` adds only the
`BOUNDED_TWO_SCALE_PORE_SYNTHESIS` boundary demonstrated in
`M8_GEOLOGY_BLUEPRINT_P2_MULTI_SCALE_MICROSTRUCTURE_CLOSURE.md`.

The proven lineage is:

```text
six explicit fine/meso controls + strength/mask/full-width seed
-> canonical schema-27 recipe and identity
-> checked 16-candidate-per-voxel work admission
-> existing scalar ping-pong + shared pore auxiliary + packed cavity response
-> Surface Nets + static-triangle collision + GEOL material
-> deterministic JRMS/JRCL cook
-> source-free StaticPbr replay and original-resolution Beauty
```

Exactly 147,456 admitted voxels consumed 2,359,296 candidate evaluations and
589,824 shared auxiliary bytes, with zero incremental bytes, one dispatch and
zero numerical iterations. The RTX 3090 proof published recipe hash
`2590024913169852556`, final scalar/surface hash `2282109602832013339` and
microstructure-field hash `889530289141831535`. Nine CPU/GPU witnesses matched.

The cooked products contain 533,954 mesh bytes and 244,398 collision bytes. The
existing StaticPbr replay consumed 6,569 vertices and 41,328 indices with zero
fallback texture/error/fatal, and three distinct 960x720 images demonstrate a
connected host, fine plus mesoscopic cavities and classified IBL/shadow
response.

Applicable source/input, authoring diff/cancel, recipe serialization, CPU/GPU
processing, scalar/classification lineage, extraction, collision, cook,
materials, source-free runtime, debugging, budgets, failure handling, tests and
documentation are implemented. Animation and Sequencer are not applicable.
Dedicated editor UI/transaction, semantic LOD/world-cell preservation,
scale-aware Jolt properties, mutable runtime modification/save state and any
product-driven gameplay/Luau/navigation/event/VFX/audio query remain explicitly
`INTEGRATION REQUIRED` under their later subsystem owners.

No general porous-media, implicit-material, multiphase, geochemistry,
weathering, erosion, destruction, remesh, retopology, unlimited scale hierarchy,
distributed simulation, machine learning, VDB/UI, bespoke preset or parallel
runtime system was added.

All eight ordered P2/research entries are individually closed. A dedicated
complete-blueprint exit audit is still required before starting the later
PWAS/EPLS production expansion. JericoEngine 1.0 remains open.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`

---

## Classified LOD1 source-free vertical re-audit — 2026-08-29

`M8-GEOLOGY-CLASSIFIED-LOD1-SOURCE-FREE-VERTICAL` closes only the first bounded
post-blueprint production dependency: one classified `LOD1` derived from the
same accepted resident scalar/classification fields as the dense `LOD0` master.

```text
one checked GeologyGpuTopologyProfile
-> existing final resident scalar + packed classification
-> classified Surface Nets LOD0 + one coarser classified LOD1
-> one JRMS GEOL stream + one VISI lodSetId with levels 0/1
-> one complete source-free .epak
-> CPU conventional or Diligent/Vulkan Two-Pass Hi-Z selection
-> exactly one classified StaticPbr draw near/far/transition
```

The RTX 3090 proof retained 8,120 dense triangles and produced 2,184 LOD1
triangles. Measured maximum point-to-source-triangle deviation is `0.0858255 m`.
LOD1 has complete classifications, normals and tangents; repeat evaluation and
cook are deterministic. Dense collision remains the only collision product and
is byte-identical to the dense-only baseline.

The existing JRMS version and `VISI` chunk remain compatible. Each set must be
contiguous from level zero, unique per level and strictly decreasing in
projected-diameter threshold. Legacy ordinary and single-level assets retain
their bytes. The conventional renderer now consumes the same CPU LOD oracle as
the GPU path when `VISI` exists; missing-`VISI` fallback behavior remains intact.

### Slice applicability matrix

| Category | Classified LOD1 result |
|---|---|
| Source/input data | `PASS`: one finite, positive, coarser bounded profile; more than one profile is rejected. |
| Asset pipeline | `PASS`: deterministic JRMS `GEOL + VISI` LOD0/LOD1 and one complete `.epak`; source assets are absent at replay. |
| Editor/authoring workflow | `NOT APPLICABLE` to this evaluator/cook/runtime vertical; dedicated LOD UI/preview remains `INTEGRATION REQUIRED`. |
| ECS/runtime representation | `PASS`: existing cooked static-mesh representation and visibility records; no new component or format. |
| CPU/GPU processing | `PASS`: same resident geology fields feed both levels; checked extraction, deviation, feature, triangle and cardinality limits. |
| Renderer/shaders/materials | `PASS`: CPU and Vulkan select exactly one level near/far/transition, preserve classified material lineage and use zero texture fallbacks. Beauty redesign is excluded. |
| Physics/collision | `PASS` for unchanged dense-master static collision only. Jolt class-to-physical-property mapping and collision LOD remain `INTEGRATION REQUIRED`. |
| Animation/Sequencer | `NOT APPLICABLE` to static geology topology. |
| Scripting/Luau/events | `INTEGRATION REQUIRED` only if a later product needs read-only LOD/class queries or events. |
| Serialization/save-load | `PASS` for immutable cooked LOD products; mutable runtime topology/save state remains `INTEGRATION REQUIRED`. |
| Streaming/LOD | Exactly one local classified LOD1 is `PASS`; `LOD2+`, HLOD, chunks, world cells, geometry pages and streaming/residency policy remain `INTEGRATION REQUIRED`. |
| Destruction/runtime modification | `INTEGRATION REQUIRED`; no runtime remesh, topology replacement, destruction or persistence was added. |
| Navigation/gameplay/particles/audio | `INTEGRATION REQUIRED` behind later product-owned consumers; none were added here. |
| Debugging/visualization | `PASS`: stable validation errors, real extraction metrics, CPU/GPU draw diagnostics and inspected non-empty captures. Dedicated editor visualization remains deferred. |
| Profiling/performance budgets | `PASS`: 2,184 LOD1 triangles, `0.0858255 m` measured deviation and bounded extraction; no global quality tier was invented. |
| Testing/failure handling | `PASS`: focal RED/GREEN, invalid profiles, JRMS malformed sets, dense collision identity, CPU/Vulkan exclusivity/hysteresis, package-only replay and determinism. |
| Documentation | `PASS`: ADR-037, JRMS, closure evidence and this re-audit match the implemented boundary. |

### Remaining integration boundary

| Subsystem | Required data/interface | Owner/dependency | Milestone |
|---|---|---|---|
| Classified world cells | Stable cell/chunk identity, per-cell LOD cook and package ranges | Geometry/World Cell owner after this LOD contract | Future production-expansion selection |
| Jolt physical properties | Cooked geology-class-to-Jolt material mapping on dense collision | Geology plus physics owner | Future production-expansion selection |
| Editor LOD authoring | Profile preview/diff/cancel/apply through existing transaction seams | JericoEditor geology owner | Later editor integration |
| Runtime modification/save | Stable modification identity, remesh/collision replacement and persistence | Future destruction/runtime owner | Post-blueprint production expansion |
| Navigation/gameplay/Luau/events/VFX/audio | Product-driven read/query/event contracts | Respective downstream owners | Feature-driven later slices |

No P0/P1/P2 recipe/operator, material/Beauty design, VDB/UI, preset, dependency,
`LOD2+`, HLOD, world-cell/streaming policy, Jolt mapping, destruction, navigation,
PWAS/EPLS, gameplay, Luau, event, VFX or audio capability was reopened or added.
Completing this vertical does not complete the production expansion or
JericoEngine 1.0.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`

---

## Classified dense-collision Jolt surface-property re-audit — 2026-08-29

`M8-GEOLOGY-CLASSIFIED-DENSE-COLLISION-JOLT-SURFACE-PROPERTIES` closes one
immutable dense-collision consumer: the accepted geology classification now
selects bounded friction/restitution records per canonical collision triangle.

```text
accepted dense GeologySurfaceMesh classifications
+ existing MeshGeologyDescriptor palette/response routing
+ explicit ordered Jerico physical-surface palette
-> GeologyCook canonical dense triangle order
-> one deterministic u8 material index per dense triangle
-> JRCL 1.4 SHAP + PHYS
-> package Collision entries / CookedCollisionAsset
-> private Jolt IndexedTriangle material + PhysicsMaterial adapter
-> exact Jerico-owned ray surface identity
-> geometric-mean friction / maximum restitution contacts
-> repeatable fixed-step motion from one source-free Shipping-valid .epak
```

JRCL 1.4 is opt-in. The physical palette is bounded to four unique, non-zero
identities with finite `[0,1]` coefficients; every dense triangle owns exactly
one in-range index. Unclassified boxes, capsules and static meshes retain their
body-wide physics path and exact JRCL 1.0-1.3 bytes. Classified LOD1 remains
render-only and does not enter collision.

The package witness contains exactly three Collision entries in one 1,735-byte
`.epak`: one low/high-restitution floor, one low/high-friction slope and one
dynamic box. The directory contains no loose inputs. Two identical replays
produced exact query/material/transform equality. At the fixed checkpoints the
low/high rebound Y positions were `0.48963` / `2.38315`, and low/high friction
travel Z positions were `6.27544` / `-2.81753`. AssetDump reported
`shipping_valid`; package SHA-256 is
`e0c961ddfc67f55476841964d9e4b14f7ca04b5209462a70ae1a2c2ddf760957`.

### Slice applicability matrix

| Category | Classified dense-collision result |
|---|---|
| Source/input data | `PASS`: the explicit ordered physical palette reuses the existing geology material identities and response routing; no texture/world-position inference or second geology database exists. |
| Asset pipeline | `PASS`: GeologyCook writes deterministic dense JRCL 1.4; legacy assets remain exact; one package-only replay passed Shipping validation. Generic non-geology material/model propagation remains `INTEGRATION REQUIRED`. |
| Editor/authoring workflow | `NOT APPLICABLE` to this cook/runtime slice. A general material physical-property editor, validation, preview/diff and batch/import workflow is `INTEGRATION REQUIRED`. |
| ECS/components | Existing Collider/RigidBody bytes remain sufficient. Surface identity is returned through a bounded Physics query result; no Jolt ID or pointer leaks. |
| Runtime representation | `PASS`: CookedCollisionAsset owns the bounded palette/index stream and the private adapter creates per-triangle Jolt materials. |
| CPU/GPU processing | CPU classification resolution and private CPU physics consumption are `PASS`; no GPU processing is applicable or changed. |
| Renderer/shaders/materials | Visual materials, shaders and Beauty are deliberately unchanged. Aligning generic visual materials with authored physical records is `INTEGRATION REQUIRED`. |
| Physics/collision | `PASS`: exact ray identities/coefficients, real low/high friction travel, real low/high restitution rebound, repeatability and legacy body-wide fallback. |
| Animation/Sequencer | `NOT APPLICABLE` to immutable static collision. |
| Scripting/Luau/events | `INTEGRATION REQUIRED` only for a later product-owned read-only surface query/event seam. |
| Serialization/save-load | `PASS` for immutable JRCL/package round-trip. Mutable material changes and save state are excluded and `INTEGRATION REQUIRED` with runtime modification. |
| Streaming/LOD | Dense collision only is `PASS`; collision LOD, LOD2+, world cells, pages and streaming/residency are `INTEGRATION REQUIRED`. |
| Destruction/runtime modification | `INTEGRATION REQUIRED`; no breakability, damage, remesh, collision replacement or persistence was added. |
| Navigation/gameplay/particles/audio | `INTEGRATION REQUIRED` behind later explicit query/event contracts; none was silently added. |
| Debugging/visualization | `PASS`: exact Physics ray surface records, JRCL dump counts and stable validation failures. Editor visualization remains deferred. |
| Profiling/performance budgets | `PASS` for bounded storage: at most four physical records, one byte per dense triangle and no unbounded allocation before validation. Production-scale contact profiling remains owned by later hardening. |
| Testing/failure handling | `PASS`: three retained RED stages; round-trip/corruption, dense cook, LOD1 exclusion, Jolt query/contact determinism, package-only replay and legacy physics gates. |
| Documentation | `PASS`: JRCL schema, focused closure evidence and this append-only re-audit match implementation. |

### Required general material expansion

The project requirement is broader than this geology proof:

> Every Jerico material must be able to declare explicit physical properties
> and make cooked collision/runtime contacts behave according to the selected
> surface, through one authoring -> cook -> package -> physics route.

This is `INTEGRATION REQUIRED`, not claimed implemented here. The expected
future boundary needs a Jerico-owned stable physical identity and authored
coefficients at the generic material/import layer, deterministic propagation by
the normal collision cooker for supported model/collider types, editor
validation/preview/batch/reimport, package preservation, query exposure and
measured runtime behavior. “Intelligent simulation” means deterministic
selection and physically coherent response from the authored surface contract;
it is not an AI label or a visual-only tag.

### Remaining integration boundary

| Subsystem | Required data/interface | Owner/dependency | Milestone |
|---|---|---|---|
| Generic materials and imported models | Stable Jerico physical record on material/import settings; deterministic propagation to every supported collision representation | Material, Import and Physics Cook owners | Dedicated production material-physics vertical |
| Editor physical-material authoring | Property validation, presets, preview/diff, selection/batch, reimport and undo/redo | JericoEditor Material/Import owners | Later editor integration |
| Density/mass/breakability/damage | Explicit units, aggregation policy, body/region ownership and product-independent contracts | Physics plus future destruction owners | Separate production slices |
| Classified world cells/streaming | Stable cell/chunk identity, per-cell collision cook and package ranges | Geometry/World Cell owners | Future production expansion |
| Collision LOD | Selection policy, contact/query identity stability and cooked LOD products | Geometry/Physics owners | Dedicated collision-LOD slice |
| Destruction/save | Stable modification identity, remesh/collision replacement and persistence | Runtime modification owner | Post-streaming production expansion |
| Navigation/gameplay/Luau/events/VFX/audio | Read-only surface query and event contracts only when demanded by a product | Respective downstream owners | Feature-driven later slices |

No density, mass inference, breakability, damage, destruction, collision LOD,
world cell, streaming, editor UI, navigation, gameplay, Luau, event, VFX or
audio behavior was added. Completing this slice does not complete the production
expansion or JericoEngine 1.0.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`

## General Physical Material Phase A re-audit — 2026-08-30

`M8-GENERAL-PHYSICAL-MATERIAL-VERTICAL` reclassifies the preceding generic
material integration from wholly `INTEGRATION REQUIRED` to `PASS` for exactly
one supported product boundary: an explicitly selected reusable contact
definition on a single-material imported structural static mesh. It does not
claim every collider/material/editor consumer.

```text
schema-1 reusable Physical Material { identity, friction, restitution }
+ schema-2 imported-model material-slot selection
-> canonical import validation and exact contained resolution
-> structural JericoGltfCook
-> collision-owned JRCL 1.4 PHYS record + u8 triangle index
-> 15-entry source-free Shipping-valid package
-> CookedCollisionAsset / private Jolt triangle adapter
-> exact query identity and repeatable friction/restitution behavior
```

The low/high physical records are `11...` with `0.04/0.10` and `22...` with
`0.81/0.90`. Two package builds are byte/SHA-identical and two package-only
replays preserve exact query/transform equality;
measured restitution Y is `0.0333462/0.489699` and friction travel Z is
`0.372709/0.00998178`. The 9,301-byte package SHA-256 is
`fe974ba8d050ff42605dd45a3d33e92e8ccb69e91e6ae777c8f954765d996eb0`.
The strict recipe serializer derives its schema from selection presence, and
the sole unnamed glTF material uses the existing canonical `Material` slot.

### Reclassified applicability matrix

| Category | Phase A result |
|---|---|
| Source/input and authoring | `PASS` for the reusable strict definition and explicit imported-model selection. Full editor authoring remains `INTEGRATION REQUIRED`. |
| Asset pipeline | `PASS` for deterministic canonical single-material structural resolution/cook; no texture/name/geology inference. |
| Runtime representation and Physics | `PASS`: the existing JRCL 1.4 collision-owned copy and private Jolt adapter expose exact Jerico records and real contact separation. |
| Renderer/shaders/Visual Material | `PASS` unchanged: `.rmesh`, `.rmat` and `.rtext` are byte-equal between otherwise equivalent selections; Physical Material never becomes appearance truth. |
| Serialization/package | `PASS`: repeated cook, one Shipping-valid source-free `.epak`, repeatable package-only replay and exact JRCL 1.0-1.3 compatibility. |
| ECS/animation/Sequencer/GPU | `NOT APPLICABLE`: no schema or processing change is required for this static contact slice. |
| Debugging/failure/testing | `PASS`: exact definition/selection/JRCL dumps, strict negative mappings, three required REDs plus two review-driven regression REDs, package rebuild equality and legacy/runtime regressions. |

### Remaining integration boundary

| Subsystem | Required data/interface | Owner/dependency | Milestone |
|---|---|---|---|
| Full editor workflow | Validation, presets, preview/diff, batch, reimport and Undo/Redo consuming the Phase A contract | Editor Material/Import owners | Later editor slice |
| Remaining collider/material coverage | Explicit multi-slot mapping, box/capsule overrides, Physical Prefab propagation | Import/Physics Cook owners | Later Phase A coverage |
| Density/mass | Explicit units, aggregation, ownership and preference rules | Physics/material authority | Separate authorized slice |
| World cells/streaming and collision LOD | Stable cell/chunk/LOD identity and package/runtime selection | Geometry/World Cell/Physics owners | Separate verticals |
| Fracture/destruction/save | Fracture Profile, structural graph, runtime replacement and persistence | Published Phases B-G | Later ordered phases |
| Navigation/gameplay/Luau/events/VFX/audio | Product-demanded read-only query/event consumers | Downstream owners | Feature-driven slices |
| Concurrent authoring replacement hardening | Authenticated immutable recipe/definition snapshot or equivalent secured read boundary | Asset authoring transaction owner | Production hardening |

The focused closure is recorded in
`docs/evidence/M8_GENERAL_PHYSICAL_MATERIAL_VERTICAL_CLOSURE.md`. No density,
mass, fracture, destruction, streaming, collision LOD, full editor or product
consumer was added.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`
