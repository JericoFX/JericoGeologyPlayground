# M8 Geology Blueprint P2 — Bounded Evolution/Search Closure

**Slice:** `M8-GEOLOGY-BLUEPRINT-P2-EVOLUTION-SEARCH`

**Status:** `PASS`

**Date:** 2026-08-29

**Repository:** `F:\JericoEngine`, `main`

## Result

Jerico now has one deterministic, bounded authoring search over the admitted
schema-21 typed block-grammar source. The system publishes immutable candidate
previews with exact objective values, diffs and identities; the designer may
choose or reject every suggestion; only an explicitly selected candidate is
materialized as the existing `GeologyStackRecipe` consumed by Vulkan, Surface
Nets, cook and Shipping.

The frozen boundary is `BOUNDED_DETERMINISTIC_RECIPE_MUTATION_SEARCH`:

- mutable genome: the existing 32 grammar rule slots plus the source-layer
  seed; axiom, extents, rewrite depth, implosion, transform, quality and every
  other layer remain frozen;
- objective: canonical occupied-lattice span ratio in integer permille, with
  three positive components and at least one component equal to `1000`;
- score: integer sum of absolute component errors; no arbitrary fitness code;
- mutation: exactly one rule slot or the layer seed per offspring, derived from
  a domain-separated deterministic stream;
- selection: bounded `(1 + lambda)` generations, ordered by
  `(objectiveErrorPermille, recipeHash)` with the baseline used as parent but
  never returned as a suggestion;
- stopping: generation ceiling or consecutive non-improving generation ceiling;
- hard bounds: population `2..8`, generations `1..8`, stagnation `1..4` and no
  greater than generations, returned candidates `1..8`, at most 57 evaluations,
  `67,108,864` work items and `65,536` retained bytes;
- designer choice: pure stale-checked materialization increments the existing
  source revision once and revalidates source/result/recipe identities without
  mutating an editor session or asset.

This is an independent Jerico design informed by the dimensional-ratio problem
in Dart, De Rossi and Togelius, *SpeedRock: Procedural Rocks through Grammars
and Evolution* (2011), official PDF
`https://pcgworkshop.com/archive/dart2011speedrock.pdf`, DOI
`10.1145/2000919.2000927`. No reference implementation or source code was
inspected, copied or ported. Research and the frozen design precede code in:

- `docs/evidence/M8_GEOLOGY_BLUEPRINT_P2_EVOLUTION_SEARCH_RESEARCH.md`;
- `docs/superpowers/specs/2026-08-29-m8-bounded-evolution-search-design.md`;
- `docs/superpowers/plans/2026-08-29-m8-bounded-evolution-search.md`.

## RED chronology

1. The first consumed Vulkan test failed to compile with MSVC `C1083` because
   `GeologyBlockGrammarSearch.hpp` did not exist.
2. Focused CPU tests then failed observably for the unimplemented retained-byte
   ceiling and source-result tamper rejection.
3. The stopping fixture exposed and corrected a test/contract mismatch without
   weakening the published `stagnation <= generations` rule; a one-cell grammar
   plus one adjacent-cell mutation proves real `GenerationLimit`, while a
   zero-error baseline proves `Stagnation`.
4. The Release cook branch failed to compile because the selected-recipe helper
   and recipe-override consumer did not exist.
5. The first alternative evidence fixture was rejected by exact CPU/GPU parity;
   the accepted adjacent mutation reaches objective error zero and passes the
   unchanged `1e-4` parity contract.

## Proven lineage

```text
valid schema-21 BlockGrammar GeologyStackRecipe
-> canonical base recipe hash + objective + limits + searchSeed
-> deterministic bounded one-gene mutation stream
-> existing bounded block-grammar evaluator
-> immutable ranked previews + exact diff/objective/source/recipe identities
-> explicit designer selection
-> stale/tamper-checked existing schema-21 recipe, revision +1
-> existing Vulkan scalar/classification field
-> existing classified Surface Nets and JRMS/JRCL cook
-> existing material plus three texture cook
-> six-entry source-free .epak
-> package-only Vulkan static PBR replay
```

The accepted Release fixture begins with one occupied cell and target ratio
`{1000, 500, 500}`. Search seed `0` evaluates seven candidates in 258 work items.
The selected rank-zero preview changes only rule bit `2`, improves the baseline
error from `1000` to `0`, and reports:

```text
request_hash=9385297875040815579
result_hash=720849066376213352
selected_recipe_hash=17711715928159991393
selected_source_hash=6420089136575985511
changed_rule_mask=2
seed_changed=0
vertices=1823
indices=10980
mesh_bytes=146786
collision_bytes=66054
material_bytes=1864563
textures=3
final_field_hash=4123199065062627126
surface_hash=4123199065062627126
source_gpu_ns=98912
```

The package contains exactly six cooked entries, is `2,079,932` bytes and has
SHA-256 `0FBAAADD6559BC39C64B0D27698BF67388A591DC6A258C79ACD8C8F8C3943706`.
Package-only Vulkan on the NVIDIA GeForce RTX 3090 reported one draw, one
material, three textures, zero fallbacks, `source_free=true`, and render graph
`StaticPbr,ExposureToneMap,Present`.

The inspected original 1280x720 evidence is
`docs/evidence/images/m8-geology-blueprint-p2-evolution-search/evolution-search-vulkan.bmp`.
It visibly shows the selected two-cell elongated mass and has SHA-256
`4941B8EA069365BCD5D13E76EA7B5D00FA2DAD43318215442FA6AC381E121116`.

## Artifact hashes

| Artifact | SHA-256 |
|---|---|
| `block-grammar.rmesh` | `515AD89C80837ECC07C98B3AB6BE2FE1A08DED0887A56F0906A17161EDA5AB32` |
| `block-grammar.rcoll` | `05051D93BB27CAADCEBD92C1D86779EB073F49CD1AAD536598B311BE82B359DA` |
| `block-grammar.rmat` | `DC47C3AB09B456A6939B0699CFA3134990028D6FD7ED03E4AF4330F847C2985E` |
| `block-grammar-base-color.rtext` | `34040454C658E63C33E8E4AE52980497E3C925DD1615B84C0EC2C5232A12D9CD` |
| `block-grammar-normal.rtext` | `0528860BD90D4B0FE94AD2558EA03F7CE17DD4313CDE99B109ABD8EC515945A8` |
| `block-grammar-orm.rtext` | `857BE2F3D194B1492B3D8D37FFFBC87D81024F7EAE517ABA5F9E82E7436075DF` |
| `evolution-search.epak` | `0FBAAADD6559BC39C64B0D27698BF67388A591DC6A258C79ACD8C8F8C3943706` |
| Vulkan Beauty BMP | `4941B8EA069365BCD5D13E76EA7B5D00FA2DAD43318215442FA6AC381E121116` |

## End-to-end system completeness audit

| Category | Bounded evolution/search result |
|---|---|
| Source/input | `PASS`: only an already-valid schema-21 block-grammar recipe is admitted. |
| Asset/recipe pipeline | `PASS`: selected output is the existing canonical recipe; no new schema or runtime format. |
| Editor/authoring | Pure preview/diff/choose seam `PASS`; dedicated UI, transaction wrapper and final presets remain `INTEGRATION REQUIRED`. |
| CPU processing | `PASS`: deterministic integer objective, mutation, ranking, stopping, cancellation and hard work/memory ceilings. |
| GPU processing | Search is authoring-side CPU work; the selected recipe passes unchanged existing Vulkan field parity. |
| Extraction/cook/runtime | `PASS`: existing classified Surface Nets, exact collision, material/textures, package and Shipping replay. |
| Renderer/shaders/materials | `PASS`: existing static PBR path, one material, three textures, zero fallback. |
| Physics | Exact selected static triangles `PASS`; geology-derived physical properties remain `INTEGRATION REQUIRED`. |
| Serialization/save-load | Canonical recipe identity and static cooked load `PASS`; mutable runtime search/save state is deliberately absent. |
| Streaming/LOD | Package closure `PASS`; classified LOD/world-cell integration remains `INTEGRATION REQUIRED`. |
| Destruction/runtime modification | `INTEGRATION REQUIRED`; no runtime search, rewrite, remesh, retopology, mutable collision or save framework. |
| Gameplay/Luau/events/navigation/particles/audio | `INTEGRATION REQUIRED` behind later bounded geology query/result seams. |
| Debug/visualization/profiling | Stable errors, exact identities/work counters, GPU timing and inspected Vulkan Beauty `PASS`. |
| Testing/failure handling | `PASS`: determinism, seed sensitivity, objective/diff/ranking, both stops, invalid/cancel/work/memory/no-candidate, stale/tamper, Vulkan/cook/package/replay. |
| Documentation | Research, independent design, plan, closure and system re-audit match implementation. |

## Explicitly deferred integrations

The following remain `INTEGRATION REQUIRED` or separate ordered slices:

- aperiodic tiling, contact-preserving erosion, Dorsey narrow-band mineral
  weathering, moisture transport, mineral dissolution and multi-scale
  microstructure synthesis;
- neighborhood erosion, which is neither this objective nor an authorized P2
  implementation here;
- dedicated editor UI, undo/redo transaction wrapper, bespoke final presets
  and semantic candidate visualization;
- geology physical properties, classified LOD/world-cell streaming, runtime
  destruction/remesh/retopology/save state;
- navigation, gameplay/Luau, events, particles and audio consumers;
- the post-Geology PWAS/EPLS/animation/destruction production expansion.

This slice did not reopen P0/P1 or the closed block-grammar source, introduce a
general optimizer/grammar/destruction framework, add arbitrary fitness code,
run distributed or unbounded search, restore VDB/UI, copy a reference
implementation or start another P2.

Completing this bounded authoring search does not complete P2, the Geology
blueprint, the later production expansion or JericoEngine 1.0.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`
