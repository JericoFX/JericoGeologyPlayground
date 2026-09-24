# M8 SpeedRock-Like Block-Grammar Source Design

**Date:** 2026-08-29
**Selected boundary:** `TYPED_CPU_GRAMMAR_TO_EXISTING_VULKAN_SOURCE_FIELD`
**Research:** `docs/evidence/M8_GEOLOGY_BLUEPRINT_P2_BLOCK_GRAMMAR_RESEARCH.md`

## Observable goal

Add one deterministic, bounded coarse block-mass source to the existing typed
geology recipe. It must generate complete terminal occupancy on CPU, feed the
existing guarded Vulkan scalar field, produce a useful classified Surface Nets
mesh and cook exact static render/collision assets without creating another
recipe, graph, field, mesh, cooker or runtime system.

## Typed source contract

Recipe schema 21 adds `GeologySourceKind::BlockGrammar` to the existing first and
only `GeologyLayerKind::Source`.

The fixed alphabet is:

```text
Empty = 0
Rule1 = 1
Rule2 = 2
Rule3 = 3
Rule4 = 4
```

`GeologyBlockGrammarSourceDefinition` contains:

- positive finite local `rootHalfExtentMeters`;
- one non-empty `axiom` in `Rule1..Rule4`;
- exactly four rules, each containing exactly eight symbols;
- `rewriteDepth` in `1..6`;
- `implode` as one explicit Boolean.

The definition is embedded only when `sourceKind = "BlockGrammar"`. Its stable
source GUID and positive revision remain in `GeologySourcePayload`.
`documentGuid` must be zero, selected BSP GUIDs must be empty and
`sourceContentHash` must equal the canonical block-grammar definition hash. BSP
source kinds retain their existing schema and validation unchanged.

## Canonical rewrite and seed

Each rule slot maps to child coordinates by:

```text
childIndex = x + 2*y + 4*z
x,y,z in {0,1}
```

Parents are visited with Z outermost, then Y, then X. Children are emitted in
indices `0..7`. Each generation reads one immutable input buffer and writes one
new output buffer; there is no in-place rewrite or convergence test.

For each parent, a fixed 64-bit integer mixer consumes:

```text
layer seed, generation, parent linear index, parent rule
```

The low three mixed bits form an axis-reflection mask. The output child index is
looked up at `childIndex XOR reflectionMask` in the authored rule. This is the
only seeded variation. It has no mutable PRNG state, so traversal cannot change
the result. Same seed and input must produce the same terminal occupancy and
identity; an asymmetric rule fixture must prove different-seed sensitivity.

The terminal grid has side `2^rewriteDepth` in the source-local domain
`[-rootHalfExtentMeters, +rootHalfExtentMeters]`. Rule 0 is exterior and every
non-zero rule is occupied. The grammar itself publishes occupancy, not density,
material or an SDF sign.

## Optional center compaction

`implode = true` applies one bounded stable compaction pass on each axis in the
fixed order X, Y, Z after the final rewrite. Grid side is always even. The two
central indices are `side/2 - 1` and `side/2`, with the center plane between
them.

For each line, occupied cells in the negative half compact into the highest
indices next to the plane; occupied cells in the positive half compact into the
lowest indices next to the plane. Relative near-to-far order is stable and cells
never cross the center plane. Each axis processes lines in canonical Z/Y/X order.

This is Jerico's explicit bounded interpretation of the paper's under-specified
implosion, not a claim of reference traversal equivalence. It is not erosion.

## Occupancy to existing Vulkan field

After rewrite/compaction, a deterministic greedy scan starts at the lowest
unconsumed occupied `(z,y,x)` cell, extends first through X, then whole matching
rows through Y, then whole matching slabs through Z, marks the chosen volume and
emits one disjoint local cuboid. The cuboids exactly cover occupied cells without
overlap or approximation.

The source layer transform is applied to those local cuboids. They materialize as
the existing additive convex source geometry. Existing topology bounds, source
geometry buffers, Vulkan negative-inside source shader, workload guard, dirty
tracking, field hash, Surface Nets, surface classification and diagnostics remain
the only productive path.

There is no grammar shader. CPU performs bounded authoring expansion; Vulkan is
still justified because it consumes the complete cuboid source through the
already admitted scalar-field path rather than a CPU-only debug list.

## Budgets, cancellation and failures

Hard grammar ceilings are:

```text
rewriteDepth <= 6
terminal cells <= 262144
cumulative child writes <= 299592
coalesced cuboids <= 65536
checked grammar work items <= 8388608
```

Every cell/rule read or write performed by rewrite, compaction and coalescing is
checked against the work ceiling. Cancellation is checked before evaluation,
between generations, at bounded chunks during cell work, between compaction axes,
during coalescing and before result publication.

Unknown symbols, empty final occupancy, non-finite/non-positive domain,
non-canonical source identity, arithmetic overflow, excess work, excess cuboids
or cancellation return stable actionable errors. The evaluator returns by value
and never exposes a partial grid or cuboid list. The existing GPU guard remains
authoritative for voxel-to-primitive work, VRAM, allocation and dispatch limits
and may reject a grammar that passes the CPU ceiling.

## Identity, classification, cook and runtime

Canonical recipe bytes include every grammar symbol, dimension, depth, implosion
flag, source identity, transform and seed. The recipe hash therefore changes for
every semantic grammar change. A separate source-result hash covers final
occupancy and emitted cuboids for preflight/materialization agreement.

The source invents no geological material class. Existing downstream operators
may enrich fields; Surface Nets must still publish classification cardinality
matching vertices. `GeologyCook` receives source, recipe, final-field and
final-surface identities and remains the only `.rmesh`/`.rcoll` writer. Collision
triangles must equal the reread render mesh. Runtime receives only cooked Jerico
assets and no grammar or source data.

## RED and verification

RED precedes production code and must demonstrate:

1. schema-21 round-trip/hash and rejection of pre-schema, unknown, empty,
   non-canonical and excessive definitions;
2. exact child order, synchronous rewrite, same-seed determinism,
   different-seed sensitivity and optional compaction semantics;
3. bounded-work and cancellation failures without a returned product;
4. Vulkan source-stage parity against CPU cuboid signed distance at inspected
   points, non-empty Surface Nets, deterministic field/surface hashes and valid
   classification cardinality;
5. productive cook with canonical `.rmesh` and exact `.rcoll`, followed by the
   existing source-free package/runtime Vulkan consumer.

The first RED must fail because schema 21 and the block-grammar source do not
exist, not because of an unrelated build failure.

## Completeness audit before implementation

- Source/input: embedded typed grammar with explicit source GUID/revision/hash.
- Asset/recipe pipeline: existing canonical recipe parser/writer/hash, extended
  only for schema 21.
- Editor/authoring: data/CLI boundary and cancellation apply; dedicated UI,
  preview controls and bespoke preset remain deferred.
- CPU: deterministic bounded rewrite, compaction and cuboid coalescing.
- GPU: existing Vulkan source field only; no new field representation or shader.
- Renderer/materials: existing classified Surface Nets and static mesh consumer;
  no new material classification is invented.
- Physics/collision: existing exact static triangle `.rcoll` and Jolt consumer.
- Serialization/package: recipe authoring schema 21; existing JRMS/JRCL/EPAK
  runtime formats remain unchanged and source-free.
- Streaming/LOD: existing static participation only; classified LOD/world-cell
  policy remains `INTEGRATION REQUIRED`.
- Runtime modification/destruction/save: `INTEGRATION REQUIRED`; no mutable
  grammar, remesh, collision replacement or save state.
- Gameplay/Luau/events/navigation/particles/audio: `INTEGRATION REQUIRED` behind
  later existing owners; this source exposes no new runtime API.
- Debugging/profiling: diagnostics retain rewrite work, occupied cells, cuboids,
  hashes and existing GPU timings/budgets.
- Testing/failure handling/documentation: CPU RED, Vulkan/cook/runtime proof,
  stable errors, primary provenance and repeated system/data-lineage audit.

## Non-goals

- evolution, search, fitness, crossover or mutation;
- neighborhood erosion or any other later P2 feature;
- spherical skinning, OBJ export, bespoke mesh generator or final named preset;
- general grammar/destruction/remesh/retopology/save framework;
- VDB restoration, dedicated UI, M9, Task23 or the post-Geology production
  expansion.
