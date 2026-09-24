# M8 Geology Blueprint P2 — Block Grammar Source Closure

**Slice:** `M8-GEOLOGY-BLUEPRINT-P2-SPEEDROCK-LIKE-BLOCK-GRAMMAR-SOURCE`

**Status:** `PASS`

**Date:** 2026-08-29

**Repository:** `F:\JericoEngine`, `main`

## Result

Jerico now has one bounded, typed block-mass source inside the existing geology
recipe, field, classification, Surface Nets, cook and runtime vertical. It is an
independent Jerico design informed by the published SpeedRock problem; it is not
a port, a general grammar framework, an evolutionary system, a second field or
mesh path, or the paper's skinning/OBJ implementation.

The frozen source contract is:

- alphabet `Empty`, `Rule1` ... `Rule4`, with one non-empty axiom;
- one canonical root cuboid, right-handed Jerico coordinates and child index
  `x + 2*y + 4*z`;
- synchronous double-buffered 2x2x2 rewrite, depth `1..6`;
- stateless seed-derived reflection inside each rewrite, with stable identity;
- optional deterministic one-pass center-half compaction in X, Y, Z order;
- empty/non-empty occupancy converted deliberately to Jerico's negative-inside
  scalar convention;
- deterministic greedy cuboid coalescing before materialization through the
  existing additive source geometry;
- checked cancellation, terminal-cell, child-write and work-item ceilings;
- canonical schema `21`, serialization and source/recipe hashing.

The detailed provenance and design decisions are retained in:

- `M8_GEOLOGY_BLUEPRINT_P2_BLOCK_GRAMMAR_RESEARCH.md`;
- `docs/superpowers/specs/2026-08-29-m8-speedrock-like-block-grammar-source-design.md`;
- `docs/superpowers/plans/2026-08-29-m8-speedrock-like-block-grammar-source.md`.

Primary source: Isaac M. Dart, Gabriele De Rossi and Julian Togelius,
*SpeedRock: Procedural Rocks through Grammars and Evolution* (2011), official
PDF `https://pcgworkshop.com/archive/dart2011speedrock.pdf`, DOI
`10.1145/2000919.2000927`. The inspected PDF SHA-256 is
`7B05B8B2C61D836061E67FCAF644F8C1FB515032D80791B5AC3A6B9B0B1A6592`.

## RED chronology

The implementation began with observable consumers, not a debug-only block
list:

1. the CPU contract first failed to compile because
   `GeologyBlockGrammarSource.hpp` did not exist;
2. the topology/Vulkan contract next failed because the existing topology plan
   had no typed block-grammar overload and the request could not consume a
   document-free grammar source;
3. the cook/artifact contract next failed because no `.rmesh`, `.rcoll` or
   `.rmat` was produced;
4. the existing cooker then rejected a sparse disconnected fixture with
   duplicate/non-manifold runtime vertices, demonstrating that topology
   validation was genuinely consumed;
5. the accepted connected fixture, existing canonical `Rebuild` stage and real
   cooked material/texture inputs made the same gates green without weakening
   the cooker or adding another mesher.

## Proven vertical lineage

```text
schema-21 GeologyStackRecipe + seed + typed BlockGrammar definition
-> bounded synchronous CPU reference and canonical source identity
-> existing topology plan materializes deterministic additive source boxes
-> existing Vulkan scalar/classification evaluation
-> existing classified Surface Nets extraction
-> existing JRMS/JRCL cook and exact static collision
-> existing PBR shader cook + three Jerico texture assets
-> six-entry source-free .epak
-> package-only Vulkan static PBR replay
```

The final Release cook on NVIDIA GeForce RTX 3090 reported:

```text
mesh_guid=2B6772616D6D6172736F757263650001
collision_guid=2B6772616D6D6172736F757263650002
material_guid=2B6772616D6D6172736F757263650003
vertices=3882
indices=23424
mesh_bytes=311866
collision_bytes=140538
material_bytes=1864563
textures=3
source_hash=7564926986005090053
recipe_hash=10528138175544731394
final_field_hash=18320962864667727086
surface_hash=18320962864667727086
source_gpu_ns=17770272
```

The field and surface hashes are identical because the existing extraction
consumes the final resident field identity without an alternate conversion
path. The package contains six cooked entries, rather than only the initially
planned mesh/collision/material trio, because real PBR replay requires three
explicit Jerico textures to meet the stronger zero-fallback acceptance rule.

## Artifact hashes

| Artifact | SHA-256 |
|---|---|
| `block-grammar.rmesh` | `B30EC8A6A95031EC497EDB29507AAA07759A16F40D1690293967381C78A07242` |
| `block-grammar.rcoll` | `7EDDF1618CF872EDBFBC79E48BEE07FC4425B0E2D3C4C2E21725372722088B71` |
| `block-grammar.rmat` | `11E9F3A734AAE6B57692C091848BD867CAAD3A7D00091B1DBBE0C193FBC4C361` |
| `block-grammar-base-color.rtext` | `3BC78FD73ABCCC8CDE9D1FE6B9C6C9ECE4464550F49E30E6CAA04B100A8B6D29` |
| `block-grammar-normal.rtext` | `921EB4CE3B7590BC9395FAF616086719CA374F6BB12A3A7D3C9E0098C8E30447` |
| `block-grammar-orm.rtext` | `BA663EC58B4CC44E15D0731A1C316A892EEA1039A81DFE4659BB0FBCA5946144` |
| `block-grammar.epak` | `27624C21F3EAF3A7430ACAC350E7CFEDBA5385630EF27905C960DBC588E28AB3` |
| Vulkan Beauty BMP | `B2CE5F73C68A21FF81585DE128742763494C43023B8ABB05E1AD0430CF5B9B34` |

The inspected original-resolution evidence is
`docs/evidence/images/m8-geology-blueprint-p2-block-grammar-source/block-grammar-vulkan-yaw25.bmp`.
It is a non-empty 1280x720 package-only Vulkan frame of the coarse hierarchical
mass at 25 degrees yaw. Runtime reported one draw, one material, three textures,
zero fallbacks, `source_free=true` and render graph
`StaticPbr,ExposureToneMap,Present`.

## End-to-end system completeness audit

| Category | Block-grammar result |
|---|---|
| Source/input and authoring | `PASS`: canonical typed recipe/schema/seed/transform/mask contract; bespoke UI and final presets are intentionally absent. |
| Asset pipeline/cook/package | `PASS`: existing classified mesh, exact collision, material/texture cook and six-entry source-free package. |
| CPU processing | `PASS`: independent deterministic reference, bounded rewrite/compaction/coalescing and stable failures. |
| GPU processing | `PASS`: the existing Vulkan scalar/classification pipeline consumes the materialized source; CPU/GPU sign parity is sampled at coalesced-box centers. |
| Renderer/shaders/materials | `PASS`: existing static PBR Vulkan path, three textures, zero fallback; no SpeedRock skinning or shader path. |
| Physics/collision | Exact static triangle collision `PASS`; geology-derived physical properties remain `INTEGRATION REQUIRED`. |
| Classification/semantic lineage | Existing classification survives unchanged to JRMS; grammar rule IDs remain source-generation data and are not falsely promoted to geological materials. |
| Serialization/save-load | Recipe and cooked static products `PASS`; mutable runtime grammar/save identity remains `INTEGRATION REQUIRED`. |
| Streaming/LOD | Source-free package closure `PASS`; classified LOD/world-cell streaming remains `INTEGRATION REQUIRED`. |
| Destruction/runtime modification | `INTEGRATION REQUIRED`; no runtime rewrite, remesh, retopology, mutable collision or save-state framework was added. |
| Gameplay/Luau/events/navigation/particles/audio | `INTEGRATION REQUIRED` behind later bounded geology query/result seams. |
| Debugging/visualization/profiling | Stable errors, field identity, nanosecond source timing and inspected Vulkan Beauty `PASS`; semantic editor views remain deferred. |
| Performance budgets/failure handling | `PASS`: fixed depth and cell/write/work ceilings, cancellation and no partial publication. |
| Testing/documentation | CPU determinism/seed/invalid/cancel/budget/schema tests, Vulkan parity/topology/cook/package/replay proof and durable records `PASS`. |

The CMake source registration is committed, but regenerating the existing build
tree is independently blocked by the pre-existing
`Tests/TestSchedulingPolicy.cmake` registry mismatch: several already-existing
geology tests are absent from that policy inventory. This slice did not modify
that unrelated authority. The new Release executable was rebuilt through the
already-generated target and exercised directly; the relevant six registered
regression tests also pass.

## Explicitly deferred integrations

The following remain `INTEGRATION REQUIRED` or separate ordered slices, with
their existing subsystem owners and dependencies unchanged:

- the seven remaining P2/research features: evolution/search, aperiodic tiling,
  contact-preserving erosion, Dorsey narrow-band mineral weathering, moisture
  transport, mineral dissolution and multi-scale microstructure synthesis;
- neighborhood erosion from SpeedRock, which is not part of this source
  grammar contract;
- physical geology properties for Jolt;
- classified LOD and world-cell streaming;
- runtime destruction, rewrite/remesh/retopology, mutable collision and
  save/load identity;
- semantic editor visualization, dedicated UI, preview/diff/undo integration
  and bespoke final presets;
- navigation, gameplay/Luau, events, particles and audio consumers;
- the post-Geology PWAS/EPLS/animation/destruction production expansion.

This slice did not reopen P0/P1, restore VDB/UI, introduce a general grammar or
search framework, create a parallel recipe/field/mesh/cooker system, copy the
reference implementation, or start a later P2 feature.

Completing this bounded source does not complete P2, the full Geology blueprint,
the later production expansion or JericoEngine 1.0.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`
