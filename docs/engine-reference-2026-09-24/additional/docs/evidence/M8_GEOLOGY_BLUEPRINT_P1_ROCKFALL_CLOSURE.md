# M8 Geology Blueprint P1 Rockfall Closure

Date: 2026-08-28
Slice: `M8-GEOLOGY-BLUEPRINT-P1-ROCKFALL`
State: `PASS`
Selected boundary: `OFFLINE_STATIC_FIELD_ROCKFALL`

Rockfall is complete only as a bounded terminal authoring/cook operator. It is
not runtime destruction, debris simulation, colluvium/deposition, mutable
save-state or a general remeshing framework. Completing it does not complete
P1, the Geology blueprint, the production expansion or JericoEngine 1.0.

## Provenance and independent design

Research preceded implementation and is retained in:

- `docs/evidence/M8_GEOLOGY_BLUEPRINT_P1_ROCKFALL_RESEARCH.md`;
- `docs/superpowers/specs/2026-08-28-m8-rockfall-design.md`;
- `docs/superpowers/plans/2026-08-28-m8-rockfall.md`.

Jones et al. provide the primary directable-weathering motivation: voxel
weathering, editable durability and later rockfall/colluvium plausibility.
They do not define Jerico's support plane, bond rule, units, topology identity
or runtime boundary. Ito et al. provide separate primary evidence that
face-connected voxel bonds, discontinuities and component searches are useful
ways to study block support. Jerico copied no implementation, format, API or
recipe. It selected an independent, smaller contract compatible with the
existing classified scalar field.

## Frozen contract

Schema 19 / kind 23 appends exactly one enabled terminal `Rockfall` layer with:

- normalized non-zero `gravityDirectionWorld` in world space;
- `supportPlaneOffsetMeters` in meters along `-gravity`;
- normalized `fractureCutoff` and `activationCutoff`;
- existing layer strength and existing RBrush regional-mask semantics.

For the bounded field:

1. occupied means `signedDistance < 0`;
2. inactive/masked occupied voxels and occupied voxels at or below the support
   plane are support roots;
3. support propagates only through six face-neighbours;
4. a bond is cut when either endpoint's packed fracture value is greater than
   or equal to `fractureCutoff`;
5. active occupied voxels not reached by support form canonical detached
   components;
6. detached voxels are removed by making their signed distance positive by at
   least half one voxel;
7. retained occupied neighbours become `fresh` without erasing other packed
   geological classification;
8. detached volume is voxel count times `voxelSizeMeters^3`. It is cubic
   meters, never kilograms.

The operator has a hard `16,777,216`-voxel field ceiling, checked byte
arithmetic and cooperative cancellation. Durability affects rockfall only
through the already ordered weathering result; this slice does not invent a
force or mass interpretation for the normalized durability scalar.

## RED chronology

Implementation began from observable failures:

1. the CPU support/disconnection test failed because
   `GeologyRockfall.hpp` did not exist;
2. schema round-trip tests failed because schema 19 and kind 23 were absent;
3. the large disconnected-mass cancellation test failed until scan, support,
   component and removal checkpoints were admitted;
4. the real RTX 3090 Vulkan test failed with
   `procedural.geology.gpu_layer_unsupported` after reaching the terminal
   Rockfall layer.

Each RED was retained in the test history before the minimum implementation
made it green.

## Productive data lineage

```text
BSP source + admitted geology recipe/mask
-> resident Diligent/Vulkan scalar field
-> weathering-modified SDF
-> packed structural fracture + existing fresh classification
-> one bounded GPU field/classification/hash readback
-> deterministic CPU support graph and detached components
-> updated scalar/fresh classification + compatible accumulated field hash
-> resident GPU upload
-> existing dirty tracking
-> existing classified Dense Surface Nets
-> existing JRMS 1.1 GEOL + exact static triangle collision
-> existing static runtime consumers
```

There is no second SDF, recipe, mesher, collision cooker, renderer, physics
owner or runtime format. The CPU step is global because support connectivity is
global; the surrounding scalar/classification fields, Surface Nets, cook and
runtime paths remain the published owners.

## Implementation lineage

- `4b4742fb` — provenance, design and execution plan;
- `88058c9f` — typed support/component/removal reference contract;
- `3bda7422` — schema 19/kind 23 canonical serialization and admission;
- `8b68a530` — bounded work, cancellation and topology-plan contracts;
- `843bad38` — productive Vulkan readback/evaluate/upload seam, diagnostics,
  classified surface/collision cook and Vulkan CTest registration.

## Verification

Fresh Debug and Release focused builds passed for:

- `JericoGeologyRecipeTests`;
- `JericoGeologyGpuTopologyPlanTests`;
- `JericoGeologyWeatheringTests`;
- `JericoGeologyGpuSessionTests --rockfall`;
- Release `JericoGeologyGpuSessionTests --rockfall-cook`.

The CPU matrix proves:

- supported versus disconnected mass;
- fracture values below and at the cutoff;
- face bonds only, not diagonal contact;
- mask/strength inactivity as preserved support;
- inverted gravity;
- deterministic component witness/hash under input permutation;
- cubic-meter volume;
- invalid/duplicate/non-finite rejection;
- cancellation and explicit work ceiling;
- canonical schema round-trip/hash and terminal ordering.

The productive RTX 3090 path, including an admitted all-positive RBrush mask,
observed:

```text
backend=Diligent/Vulkan
occupied_voxels=12628
supported_voxels=6756
detached_voxels=5872
detached_components=5872
detached_set_hash=2581702913471543033
detached_volume_m3=10.1468
checked_work_items=75768
rockfall_readback_bytes=1105928
field_hash=16597015834408543223
vertices=3230
indices=19368
unchanged_repeat_dirty_leaves=0
unchanged_repeat_dirty_voxels=0
```

The repeated resident evaluation preserved field hash, detached-set hash,
component/voxel counts, mesh counts and zero dirty work. Raw GPU Surface Nets
allocation order is not used as Rockfall identity; canonical field and
component identities are.

Two Release cooks launched in fresh processes were byte-identical:

```text
rockfall.rmesh bytes=259130
rockfall.rmesh sha256=3D6E674E7839E1BBC6DD3EB85F1E4910AF41883D1F55CED5715A38D7E6098DE4
rockfall.rcoll bytes=116490
rockfall.rcoll sha256=BC0665FBB6BFEF26782F214E28A634CE5B5B3466B904F432CEFAFFE27B907453
fresh_process_mesh_equal=true
fresh_process_collision_equal=true
```

Both products reread through the existing runtime parsers. The mesh carries
the existing geological classification and the collision reproduces exactly
the same vertex/index cardinality. Binary scans found no `Rockfall`, gravity,
support-plane or cutoff authoring markers in either runtime product.

The existing Release `JericoStaticMeshRenderAssetTests --geology-generated`
runtime gate then loaded the Rockfall JRMS on Vulkan, created the canonical
classified static-mesh asset with zero texture fallback, rendered the
weathered/fresh material responses and the combined IBL+shadow path at 960x720,
and reported no Diligent error/fatal diagnostics:

```text
runtime_vertices=3230
runtime_indices=19368
classification_bytes=25840
geometry_device_bytes=258352
runtime_final_field_hash=16597015834408543223
runtime_surface_hash=16597015834408543223
single_frame_static_pbr_gpu_ms=0.079872,0.082944
```

The three original-resolution captures were visually inspected as non-empty
static geometry with the expected removed-volume silhouette/cavities and
distinct weathered/fresh/IBL material responses. A second fresh runtime process
produced byte-identical captures for all three paths:

```text
generated-geology-weathered.bmp sha256=71C814BE2D67FE0DD0FBB3630200B2F83B591C2965C51A17B0178C20B1BE4A3A
generated-geology-fresh.bmp sha256=ADCADFA3A2C8370156B2DCB5864EBDA27B4FED5942BC452814D6ABCB0F2A5CEF
generated-geology-fresh-ibl-shadow.bmp sha256=154C6BCB91E8C24A1BD3FAD3CCA6327002ACB8FB14C51D2D4B240CE093012F5F
fresh_process_capture_repeat=exact_3_of_3
```

This proves compatibility with the existing runtime visual consumer. It does
not claim a new bespoke Beauty composition or a new source-free package; those
owners and their already published static-mesh package route are unchanged.

## Honest build boundary

The configured Vulkan project builds and focused executables pass when invoked
directly. A global CMake regeneration is independently blocked by preserved
foreign work: `Tests/AssetPipeline/CMakeLists.txt` references absent `Qt6::Core`
and AssetPipeline/Editor tests reference absent `Jerico::EditorProjectAuthority`
in the non-editor preset. The earlier `build/dev` tree also lacks two Diligent
targets. These are outside this slice, were not edited and do not invalidate the
fresh focused Debug/Release results above.

## `INTEGRATION REQUIRED`

1. **Colluvium/deposition.** A separate P1 slice must define transport,
   settling, material lineage and budgets. Rockfall only removes unsupported
   static volume.
2. **Imported-mesh structural operators.** A separate P1 slice must define
   canonical topology and authoring-source policy.
3. **Runtime modification/destruction/save-load.** A later authorized owner
   must define event identity, bounded remesh, collision replacement and
   persistence. This slice deliberately ships only the static result.
4. **Jolt material/break properties.** Exact static triangle collision passes;
   physical density, mass, resistance and break policy require versioned cooked
   contracts. Detached voxel volume must not be treated as mass.
5. **Classified LOD/world-cell streaming.** Required before fresh/new-surface
   semantics or mutable geology can cross LOD or streaming boundaries.
6. **Navigation/gameplay/Luau/events/particles/audio.** Required only if later
   runtime geological changes are admitted, through bounded result/event
   services rather than authoring-field access.
7. **Editor semantic visualization.** Generic recipe/mask/preview/transaction
   seams apply; a dedicated support/component view remains a separate editor
   slice.

No item above is silently claimed complete by this static operator.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
