# M8 Geology Blueprint P1 Colluvium Closure

Date: 2026-08-29
Slice: `M8-GEOLOGY-BLUEPRINT-P1-COLLUVIUM`
State: `PASS`
Selected boundary: `OFFLINE_STATIC_FIELD_COLLUVIUM`

Colluvium is complete only as one bounded terminal authoring/cook operation over
the unchanged productive Rockfall result. It is not runtime sediment, debris,
destruction, mutable save-state, imported-mesh structural processing or a
general remeshing/transport framework. Completing it does not complete P1, the
Geology blueprint, the production expansion or JericoEngine 1.0.

## Provenance and independent design

Research preceded implementation and remains in:

- `docs/evidence/M8_GEOLOGY_BLUEPRINT_P1_COLLUVIUM_RESEARCH.md`;
- `docs/superpowers/specs/2026-08-28-m8-colluvium-design.md`;
- `docs/superpowers/plans/2026-08-28-m8-colluvium.md`.

Jones et al. establish the primary motivation for directable weathering with
rockfall and colluvium. Separate geomorphology sources support treating
colluvium as locally transported slope material rather than fluid sediment.
Those sources do not define Jerico's grid traversal, units, masks, work ceiling,
identity, cook format or runtime boundary. Jerico copied no implementation,
format, API or recipe and selected a smaller independent contract compatible
with the existing classified scalar field.

## Frozen contract

Schema 20 / kind 24 appends exactly one enabled terminal `Colluvium` layer
immediately after productive `Rockfall` with:

- `maximumTransportDistanceMeters` in meters, quantized down to whole voxel
  cells and capped at 64 cells;
- normalized `activationCutoff`;
- inherited cardinal Rockfall gravity;
- existing layer strength and one optional admitted RBrush local-field mask.

For the bounded existing field:

1. the unchanged Rockfall detached voxel set and component identities are the
   only Colluvium source;
2. occupied support and empty destinations are read from the post-Rockfall
   field snapshot;
3. deterministic breadth-first traversal considers only the already allocated
   topology and never expands it;
4. candidate rank is greatest gravity progress, then least lateral distance,
   shortest path and canonical voxel-key order;
5. a source may settle only where the effective strength/mask weight reaches
   `activationCutoff` and an occupied gravity-side support exists;
6. sources are processed in canonical gravity order so their destinations may
   support later sources without duplication;
7. every source becomes exactly one deposited placement or one explicit
   unsettled remainder;
8. source volume equals deposited plus unsettled volume in cubic meters;
9. a deposited cell copies source geological classification, sets its exact
   voxel deposition byte to 255 and clears its `fresh` bit;
10. Surface Nets may interpolate that byte at generated vertices, so the
    observable surface contract is positive deposition rather than byte 255 at
    every boundary vertex.

The operator inherits Rockfall's 16,777,216-voxel topology ceiling, caps work at
67,108,864 visited nodes, uses checked coordinate arithmetic and supports
cooperative cancellation. Arbitrary gravity, runtime movement, mass, density
and fluid behavior are deliberately absent.

## RED chronology

Implementation began from observable failures:

1. the conservation/settling test failed because the typed Colluvium reference
   did not exist;
2. canonical recipe tests failed because schema 20 and kind 24 were absent;
3. the productive RTX 3090 test reached Colluvium and failed with
   `procedural.geology.gpu_layer_unsupported`;
4. cancellation and productive lineage tests failed until evaluation used the
   retained Rockfall snapshot and published only after full success.

The topology test separately froze the required zero-expansion boundary before
productive integration. Each behavioral RED was retained before its minimum
implementation made it green.

## Productive data lineage

```text
BSP source + admitted geology recipe/RBrush mask
-> resident Diligent/Vulkan weathered classified scalar field
-> productive Rockfall one-readback detached voxels/components/volume
-> retained post-Rockfall CPU field + classification + canonical identity
-> zero-additional-readback bounded deterministic Colluvium settling
-> deposited placements + explicit unsettled remainder + conserved volume
-> copied classification with deposition=255 and fresh cleared
-> resident full field/classification/hash upload
-> existing dirty tracking
-> existing classified Dense Surface Nets
-> existing JRMS 1.1 GEOL + exact static triangle collision
-> existing Diligent/Vulkan static runtime consumer
```

There is no second SDF, recipe, mesher, collision cooker, renderer, physics
owner or runtime format. Colluvium consumes the one CPU snapshot that Rockfall
already had to read; `colluviumReadbackBytes` is therefore exactly zero.

## Implementation lineage

- `41f2e2c2` — provenance, design and execution plan;
- `4803248a` — conserved deterministic reference operation;
- `83d8476a` — schema 20/kind 24 canonical serialization and terminal ordering;
- `1e002047` — topology non-expansion boundary;
- `6cee77e8` — productive Rockfall-lineage integration, diagnostics, cook,
  cancellation and repeat contracts;
- `dec1661b` — publication boundary after preserving unrelated remote docs.

## Verification

Fresh Release builds and focused executions passed for:

- `JericoGeologyRecipeTests`;
- `JericoGeologyGpuTopologyPlanTests`;
- `JericoGeologyGpuSessionTests --colluvium-cook` twice in fresh processes;
- `JericoGeologyGpuSessionTests --rockfall-cook` as the predecessor regression;
- `JericoStaticMeshRenderAssetTests --geology-generated` twice in fresh
  processes;
- `repository.cancelled_vdb_geology_absent`;
- `repository.gpu_geology_boundary`.

The CPU matrix proves:

- deterministic supported settling and exact cubic-meter conservation;
- permutation invariance and canonical component/source identity;
- layer strength plus protected and admitted mask destinations;
- cardinal inherited gravity and explicit rejection of arbitrary gravity;
- invalid, duplicate and non-finite field rejection;
- cancellation, topology ceiling, 64-cell distance ceiling and checked work;
- canonical schema round-trip/hash and terminal ordering.

The productive RTX 3090 path used a 3x3 RBrush with only the center texel active
and observed:

```text
backend=Diligent/Vulkan
source_components=5872
source_component_hash=5411876830298058130
source_voxels=5872
source_detached_set_hash=2581702913471543033
deposited_voxels=303
unsettled_voxels=5569
deposited_set_hash=9135724728737452032
unsettled_set_hash=886343683847025803
colluvium_result_hash=15321078296911193843
source_volume_m3=10.1468
deposited_volume_m3=0.523584
unsettled_volume_m3=9.62323
checked_work_items=457051
colluvium_readback_bytes=0
release_cpu_ms=85.3503,90.3135
maximum_surface_deposition_byte=192
field_hash=1280197861758030778
surface_hash=1280197861758030778
vertices=3408
indices=20436
unchanged_repeat_dirty_leaves=0
unchanged_repeat_dirty_voxels=0
```

The unequal-looking decimal volumes conserve at the exact stored double values;
the printed text is rounded. A protected inspection point outside the RBrush
retained identical Rockfall and Colluvium distance/sign. A forced cancellation
returned `procedural.geology.cancelled`, published no partial result and the
next evaluation reproduced all identities with zero dirty work.

Two Release cooks launched in fresh processes were byte-identical:

```text
colluvium.jrms bytes=273370
colluvium.jrms sha256=2FA9141C649CF01D57C948B2D2E958185447D1DC6CDF4E2BC43ED7A3F16D5D17
colluvium.rcoll bytes=122898
colluvium.rcoll sha256=10EC99C0A254E83A1CF7BF848E5FD50746322FADCCA9C52683A9D1559BC9E1DD
fresh_process_mesh_equal=true
fresh_process_collision_equal=true
```

Both products reread through the existing runtime parsers. The JRMS carries the
existing geological classification and the collision reproduces its exact
vertex/index cardinality as a static triangle mesh. Binary scans found no
`Colluvium`, `Rockfall`, transport distance, cutoff or RBrush authoring markers.

The existing Release static-mesh runtime gate loaded the cooked Colluvium JRMS
on Vulkan, created the canonical classified asset, rendered weathered, fresh and
IBL+shadow responses at 960x720 and reported:

```text
single_frame_static_pbr_gpu_ms=0.059392
generated-geology-weathered.bmp sha256=A4AFBC0A626C7D3E72E8E74209436A268189FB6101D9BF047A30B48704211C7B
generated-geology-fresh.bmp sha256=886951A62541145FBF3FEAD4BCE688D18ADF4CA5075B7B5DFCA097787754289A
generated-geology-fresh-ibl-shadow.bmp sha256=31EF773C87C02DA5CF382BEA7B0C160FD01329CFE7C16A99E1C410B574DCA12C
fresh_process_capture_repeat=exact_3_of_3
```

The three original-resolution captures were visually inspected as non-empty
static geometry. They show the central deposited pile, the inherited Rockfall
cavity and distinct material/IBL responses. This proves compatibility with the
existing runtime visual consumer; it does not claim a new bespoke Beauty scene.

## Honest optimization boundary

This first static boundary includes:

- dense O(1)-average CPU voxel lookup instead of per-source ordered-tree search;
- generation-stamped visited storage instead of clearing a visited set for every
  detached source;
- zero additional GPU readback after Rockfall;
- bounded transport, topology, work and cancellation checkpoints;
- exact-repeat dirty-leaf/dirty-voxel zero behavior.

It deliberately retains one full field/classification upload and one full final
Surface Nets extraction after successful Colluvium publication. Dirty-range
upload and partial remeshing are possible later optimizations only if measured
Release evidence justifies their added synchronization and seam complexity.

## `INTEGRATION REQUIRED`

1. **Imported-mesh structural operators.** A separate P1 slice must define
   canonical topology, source ownership and authoring policy.
2. **Runtime modification/destruction/save-load.** A later authorized owner must
   define stable event identity, bounded dirty-range upload/remesh, collision
   replacement and persistence. This slice ships only the static result.
3. **Jolt physical/material properties.** Exact static triangle collision
   passes; density, mass, resistance, granular dynamics and break behavior need
   versioned cooked contracts. Voxel volume is not mass.
4. **Classified LOD/world-cell streaming.** Deposition classification must retain
   identity across LOD and streaming boundaries before mutable geology exists.
5. **Navigation/gameplay/Luau/events.** Required only if later runtime geological
   changes or queries are admitted, through bounded cooked result/event services.
6. **Particles/audio.** Required only for later bounded result events; no authoring
   field access or hidden runtime simulation is authorized here.
7. **Editor semantic visualization.** Generic recipe/RBrush preview, transaction,
   cancellation and undo/redo seams apply. A dedicated source/deposited/remainder
   view is a separate editor integration.
8. **Arbitrary-gravity or fluid transport.** Requires a separately researched
   solver, budgets and validation. The current contract intentionally accepts
   only cardinal gravity and supported static settling.

No item above is silently claimed complete by this static operator.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
