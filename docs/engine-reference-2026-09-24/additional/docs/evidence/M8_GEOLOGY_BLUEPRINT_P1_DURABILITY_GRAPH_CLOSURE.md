# M8 Geology Blueprint P1 Durability Graph Closure

**Slice:** `M8-GEOLOGY-BLUEPRINT-P1-DURABILITY-GRAPH`

**Date:** 2026-08-28

**State:** `PASS`

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`

## Result

Jerico now has one typed, deterministic and bounded durability graph inside the
existing geology recipe and resident CPU/Vulkan field system. The graph derives a
normalized material-resistance field from admitted geological structure, exposes
that field to the already existing Bubble and Curvature weathering stages when it
is ordered before them, and preserves exact legacy behavior when no usable
durability field precedes a weathering stage.

The implementation is not a second geology architecture. It reuses the canonical
recipe, mask/strength rules, resident ping-pong fields, inspection, dirty tracking,
cancellation, classified Surface Nets, JRMS 1.1 `GEOL`, exact collision, package
and existing world/triplanar PBR runtime path.

Completing this operator does not complete P1, the Geology blueprint, the later
production expansion or JericoEngine 1.0.

## Research and independent design

The research record is
`M8_GEOLOGY_BLUEPRINT_P1_DURABILITY_GRAPH_RESEARCH.md`. It traces the directable
weathering publication and its author-hosted paper, distinguishes concepts from
implementation, and records the independent Jerico adaptation and prohibited
donor boundary.

The accepted design and executable plan are:

- `docs/superpowers/specs/2026-08-28-m8-durability-graph-design.md`;
- `docs/superpowers/plans/2026-08-28-m8-durability-graph.md`.

The design freezes a scalar `[0, 1]` durability contract where larger values
mean greater resistance, linear clamped key interpolation, existing mask and
strength composition, typed structural inputs and no new weathering iteration,
rockfall, colluvium or runtime destruction behavior.

## Published implementation lineage

```text
d6691e66 docs(geology): design durability graph
0f9a7303 docs(geology): plan durability graph
498cfe38 feat(geology): define durability graph contract
f8ddb567 feat(geology): admit durability recipe schema
41f6e218 feat(geology): connect typed durability to weathering
64e30201 feat(geology): evaluate durability graphs on Vulkan
ca77e9f5 test(geology): prove durability product lineage
```

Schema 18 adds kind 22 `DurabilityField`. The CPU reference and dedicated
`durabilityMain` HLSL entrypoint share the same declared domain and graph
semantics. The Vulkan path reads fracture weakening through an explicit SRV,
publishes a resident durability field/hash, reports inspection and diagnostics,
and admits only 2-8 graph keys with checked total work no greater than
268,435,456 items.

## RED chronology

The slice began with observable failing contracts before implementation:

1. the domain type, graph ordering/interpolation, normalized output, existing
   mask/strength composition, disabled identity and validation were absent;
2. schema-18 serialization, stable identity and rejection contracts were absent;
3. Bubble/Curvature could not consume typed durability or distinguish a graph
   ordered before versus after weathering;
4. the resident Vulkan evaluator, bounded admission, parity, cancellation, dirty
   tracking and inspection were absent;
5. end-to-end typed structural lineage, cook/reread/collision and source-free
   package proof were absent.

Each RED was observed, then closed by the minimum corresponding implementation.

## Productive CPU/Vulkan proof

Fresh Debug and Release execution on Diligent/Vulkan with an NVIDIA GeForce RTX
3090 produced:

```text
field_hash=3717182473233754313
durability_hash=1608270033279173559
mesh_hash=3717182473233754313
graph_keys=3
checked_work_items=276480
graph_mutation_dirty_voxels=92160
unchanged_repeat_dirty_voxels=0
vertices=5376
triangles=10744
cpu_gpu_parity_max=2.38419e-08
durability_samples=0.638534,0.703371,0.842868
weathering_wear_samples=0.00500491,0,0
```

The ordered lower-durability sample wears more. Moving the durability stage after
weathering produces the exact legacy fallback, so consumers cannot accidentally
use future graph output retroactively. Graph, hardness and fracture-weakening
mutations produce distinct dirty/product identities; unchanged repeat evaluates
to zero dirty voxels. Cancellation and invalid/overflow admission are covered.

Existing weathering identities remain:

```text
BubbleWeathering=3373581106375854912
CurvatureWeathering.convex=17123893163529741849
CurvatureWeathering.concave=1930275177814404830
```

## Static product and semantic lineage

The representative typed recipe is:

```text
Source
-> Strata
-> FractureCells
-> FractureWeakening
-> DurabilityField
```

Durability alone is semantic authoring/intermediate data: it does not mutate the
static scalar surface. Against the same recipe with `DurabilityField` disabled,
the final scalar field, canonical positions, canonical triangles, packed
eight-byte `GEOL` records after JRMS 1.1 reread, and exact triangle collision
vertices/indices are all byte-identical. This proves the field is carried without
silently deforming or discarding existing structural classification.

When an existing weathering operator consumes the field, its result continues
through the admitted chain:

```text
typed source/structure
-> resident durability
-> existing weathering scalar result
-> classified Surface Nets
-> JRMS 1.1 GEOL + exact triangle collision
-> package/static-mesh runtime
-> world/triplanar four-material PBR
```

## Cook, package and runtime proof

Representative cooked product:

```text
format=JRMS 1.1
bytes=430762
vertices=5376
indices=32232
sha256=12D66831B85D95D6293167B00C36DCB44430D8723BE1BD2F9F11684E093DB400
mesh_guid=7172737475767778797A7B7C7D7E7F80
source_hash=0xFB0C1E520D1B89F3
recipe_hash=0xB56E7E07FEFA9F69
content_hash=0xDB48E9A6C786ED2E
```

Eight-entry Shipping package:

```text
bytes=12087212
sha256=1F65EBD9644B9C4000FAF92D514E7C44C1B5D6F99BF52F6F234DE7C5758CDC8A
content_hash=0xD215494143A5FCC5
recipe_hash=0xD6F7B3B98BF14779
```

AssetDump reports `shipping_valid`, eight entries and no ABR, brush source
images, editor RBrush or EditorAssets. Binary marker scans find no schema-18,
`DurabilityField`, `fractureWeakening`, `hardness`, geology recipe or authoring
payload.

Two isolated package-only Vulkan replays started with only
`Packages/durability-graph.epak`. Each rendered one non-empty 960x720 classified
weathered static product with 5,376 vertices, 32,232 indices, one draw, four
materials, three textures, zero fallbacks, `source_free=true` and
`gpu_available=true`. Both captures were byte-identical:

```text
B0FB72F749DC74B3DB972AFB9CD42B58EE68F2E60086062A705D106924B8A83A
```

The package proof verifies compatibility and semantic lineage. This slice did
not require or claim a new bespoke Beauty composition.

Stable raw measurements are in
`docs/evidence/m8-geology-blueprint-p1-durability-graph/product-evidence.txt`.

## Focused verification

- Debug and Release recipe/schema tests: `PASS`;
- Debug and Release weathering tests: `PASS`;
- Debug and Release GPU topology-plan tests: `PASS`;
- Debug and Release SDF/classified cook tests: `PASS`;
- fresh Debug and Release `--durability-graph` Vulkan gates: `PASS`;
- Release CTest `procedural.geology_gpu_durability_graph`: `PASS` in 55.95 s;
- classified JRMS reread and exact collision: `PASS`;
- AssetDump Shipping validation and marker-negative contracts: `PASS`;
- two isolated package-only Vulkan replays: `PASS`;
- `git diff --check`: `PASS`.

## End-to-end completeness boundary

The repeated system and data-lineage audit is recorded in
`M8_GEOLOGY_SYSTEM_COMPLETENESS_AUDIT.md`. The following remain explicitly
`INTEGRATION REQUIRED` and were not silently absorbed:

1. versioned Jolt physical durability properties/queries;
2. classified LOD and world streaming representation;
3. runtime remesh/destruction plus save/load and newly exposed surfaces;
4. navigation, gameplay/Luau, events, particles and audio consumers;
5. editor durability visualization/painting through the admitted bridge;
6. rockfall, colluvium and imported-mesh structural operators in their own
   authorized slices.

Weathering after this field remains the existing implementation only. No new
erosion behavior, rockfall, colluvium, imported-mesh structural operator,
runtime destruction/remeshing, VDB/UI system, bespoke generator/preset, P2/M9 or
Task23 work was added. The post-Geology production expansion was not started.

## Local closure

`M8-GEOLOGY-BLUEPRINT-P1-DURABILITY-GRAPH` is `PASS` against its bounded
acceptance. The next blueprint item is rockfall, which requires its own single
top-level task and research-first contract. It is not implemented by this task.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`
