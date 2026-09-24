# M8 Rock-Family Final-Shaded Visual Quality Repair Design

**Status:** APPROVED explicitly by the project owner on 2026-09-01.

**Active slice:**
`M8-GEOMETRY-CRITERION-13-ROCK-FAMILY-BEAUTY-REPAIR`.

This document defines the second ordered product repair for M8 Geometry
Technology Showcase criterion 13. It follows the accepted JericoBSP repair and
does not authorize cliff, cave, interoperability or M9 work.

## Authorities

- `docs/IMPLEMENTATION_ROADMAP.md`, M8;
- `docs/evidence/M8_GEOMETRY_TECHNOLOGY_SHOWCASE_EXIT_AUDIT.md`;
- `docs/evidence/M8_GEOMETRY_FINAL_SHADED_VISUAL_REVIEW_CLOSURE.md`;
- `docs/evidence/M8_JERICOBSP_FINAL_SHADED_VISUAL_QUALITY_REPAIR.md`;
- `docs/JERICO_PROCEDURAL_GEOLOGY_RESEARCH_IMPLEMENTATION_BLUEPRINT.md`;
- `docs/evidence/M8_GEOLOGY_BLUEPRINT_EXIT_AUDIT.md`.

The final-shaded audit is binding: the retained rock-family product reads as
coarse truncated prisms, its uniform dark response hides useful surface form,
and the Shipping composition occludes one member. Identity, determinism,
collision, debug images and package proof cannot compensate for that Beauty
failure.

## Decision

Replace the rejected rock-family product through the current Geology blueprint
authority and recertify the complete product vertical. Do not rehabilitate the
legacy `SparseSdfRockFamily` generator, preserve its old hashes as a target, or
hide its defects through crop, fog, darkness, camera placement or debug color.

The new product uses existing general Jerico seams:

```text
four explicit GeologyStackRecipe instances
-> existing GeologyGpuSession Vulkan evaluation
-> existing Surface Nets extraction and classified GeologySurfaceMesh
-> existing GeologyCook .rmesh + exact .rcoll
-> existing geological-material StaticPbr permutation
-> existing editor transaction and project catalog
-> one accepted source-free .epak
-> family and landmark Beauty views
```

Recipe values and product composition belong to the showcase fixture. No
rock-family policy, product name or acceptance threshold enters `Engine/`.

## Product contract

The accepted family contains exactly four independently authored members in one
coherent scene. Each member must:

- be a closed volumetric surface with finite positive volume;
- retain a stable support region at the authored ground plane;
- expose non-trivial mass above that contact rather than a scaled prism;
- obtain macro silhouette from structural Geology operators, not Detail noise;
- remain recognizable with Detail disabled;
- use the cooked geological classification/material path with no fallback;
- own exact cooked collision derived from the accepted final surface.

The family as a whole must:

- keep all four members visible in the family Beauty view;
- contain four materially and structurally readable forms, not transformed
  clones;
- retain at least three independently useful silhouette directions per member;
- avoid sheet, membrane, hanging-fabric, hollow-border and clipped-mass reads;
- use one honest ground/context composition and the real M8 lighting baseline.

Quantitative structure gates support visual review but never replace it. The
legacy `0.0411372` structural-capture regression remains in the RED corpus and
must not be weakened or relabelled as acceptance.

## Recipe ownership

The showcase owns four bounded data recipes assembled from already admitted
Geology sources/operators. The initial implementation may select among
`ImplicitBlock`, structural warp/fold, joint/fracture, erosion/weathering and
bounded detail stages only when their published contracts fit the intended
form. Every member has explicit source identity, seed, quality profile, bounds,
voxel budget and ordered layer identity.

The product builder is an Editor/test-side composition over the current public
Geology APIs. It does not add a second evaluator, mesher, material database,
cache root, transaction system or runtime format.

## Authoring and transaction

The product must be reproducible through the ordinary current Geology authoring
flow. Preview generates a candidate without mutating accepted project bytes.
Cancel leaves the project exact. Apply publishes all four members and their
scene/catalog references atomically. Undo and Redo restore exact before/after
bytes and selection.

A failed member, stale source revision, cancellation, device error, invalid
classification, cook failure or catalog mismatch rejects the family as one
candidate. Partial families never become accepted project state.

## Identity replacement and recertification

Changing the failed geometry changes accepted product identity. Therefore this
slice may not preserve criteria 6-12 by assertion. It must recertify the
rock-family row against all affected aggregate contracts:

- materials/lighting and zero fallback;
- exact collision and Jolt package consumption;
- navigation as explicit `NotApplicable` for bounded static props;
- transactional authoring;
- deterministic regeneration from frozen recipes, seeds and tool identities;
- truthful same-run performance metrics;
- cook, Shipping validation and one-package source-free replay.

Aggregate manifests must replace the rock-family row only after the new product
passes. The other four product rows and their identities remain exact.

## Beauty evidence

The retained Beauty set contains:

1. one family hero view with all four members fully visible;
2. four member landmark views that expose mass, contact, silhouette and surface
   response;
3. one package-only Shipping family replay;
4. matched structural/debug views used only for diagnosis.

Final Beauty images are native renderer output at no less than 1280x720. They
use real StaticPbr geological materials, physical lighting, shadows and the
accepted M8 post chain. The direct and package-only family replays must be
deterministic. Original-resolution inspection remains mandatory and judges
silhouette, mass, thickness, contact, material response, framing and claim
readability.

## Performance and budgets

Measurements report actual kinds independently:

- recipe/evaluation CPU time;
- Geology GPU evaluation and Surface Nets GPU time where timestamped;
- active voxels, generated vertices/triangles and classified bytes;
- cook CPU time and package bytes;
- renderer CPU submission and GPU frame/pass time;
- collision bytes and Jolt body count;
- peak owned CPU/GPU memory when available.

The slice does not invent FPS or budget PASS values. It compares the four
members and reports single-sample, p50 or p95 only when the sampling method
actually produced that metric.

## Failure contracts

Stable negative tests cover:

- transformed or uniformly scaled clones;
- coarse prism/truncated-pyramid silhouette;
- insufficient support contact or below-ground clipping;
- Detail owning the silhouette;
- missing/occluded family member in the hero composition;
- non-finite or over-budget recipe/evaluation;
- stale source, recipe, mesh, material, collision or package identity;
- ordinary/fallback PBR used for a geological product;
- partial Apply, Undo or Redo;
- source/editor payload leakage into Shipping;
- empty, debug-only, cropped or resolution-reduced Beauty evidence.

## Pre-implementation end-to-end completeness audit

| Area | Decision |
|---|---|
| Source/input data | Four explicit current Geology recipes and admitted material identities. No legacy product hashes are preservation gates. |
| Asset pipeline | Existing Geology evaluator, Surface Nets, GeologyCook, `.rmesh`, `.rcoll` and `.epak` paths. |
| Editor/authoring | Existing Geology preview/diff/cancel and project transaction; four-member publication is atomic. |
| ECS/scene | One bounded showcase scene references four cooked static geological meshes. No new component type. |
| Runtime representation | Existing classified cooked mesh and material palette only. |
| CPU/GPU processing | Existing bounded recipe planning and `GeologyGpuSession`; no second evaluator. |
| Renderer/shaders | Existing geological StaticPbr permutation, main/depth/shadow and M8 post chain. |
| Materials | Existing cooked geological classification and palette; no uniform fallback material. |
| Physics/collision | Exact triangle collision per accepted member and package-only Jolt admission. |
| Animation | Not applicable to static rock props. |
| Luau/events/Sequencer | Not applicable; no gameplay or authored animation contract is introduced. |
| Serialization/save | Recipes remain authoring data; Shipping receives only cooked products. Project transactions are byte-exact. |
| Streaming/LOD | The accepted family is bounded and fully resident. Lack of a family LOD is recorded truthfully and is not hidden. |
| Destruction/runtime modification | Not part of this Beauty repair; existing material/destruction semantics remain intact. |
| Navigation | `NotApplicable` with retained bounded-static-prop rationale. |
| Particles/audio | Not applicable to the static showcase claim. |
| Debug/visualization | Structural and classification views diagnose the same accepted surfaces but grant no Beauty credit. |
| Profiling | Same-run evaluator/cook/package/render metrics with explicit metric kinds. |
| Tooling/UI | Existing Geology authoring and ordinary project/World flow; no new workspace. |
| Testing | Authentic visual-structure RED first, then focused recipe, authoring, cook, renderer, package and aggregate gates. |
| Documentation | Product evidence, criterion-13 ledger, aggregate closures, roadmap, `ACTIVE_SLICE` and `HANDOFF`. |
| Failure handling | Invalid/stale/cancelled/partial candidates fail before accepted publication. |

## Semantic data lineage

```text
SOURCE PARAMETERS
    four current Geology recipes, seeds, bounds, quality and materials
-> GENERATION
    GeologyGpuSession ordered field evaluation
-> INTERMEDIATE REPRESENTATION
    classified resident field + GeologySurfaceMesh
-> FINAL DATA
    four canonical .rmesh + exact .rcoll + scene/material dependencies
-> RUNTIME CONSUMERS
    AssetRegistry, StaticPbr, depth/shadow, Jolt and package replay
-> RESULT
    inspectable four-member Beauty family with truthful structural evidence
```

Geological classification, material identity, field/surface identity and
collision lineage survive the full chain. No debug-only representation becomes
the product.

## First authentic RED

The first executable contract replays the currently rejected accepted product
and fails it for the documented reasons before any new implementation exists.
It checks literal retained inputs and requires:

- four simultaneously visible member identities;
- closed positive-volume surfaces and stable contact witnesses;
- macro-silhouette signatures that survive Detail disable;
- non-clone pairwise structure;
- non-fallback geological material bindings;
- native-size final Beauty inventory.

The current coarse-prism family must produce the expected RED. Production
helpers may report measurements but cannot compute the test oracle.

## Acceptance and ordered continuation

The slice passes only when the new rock-family product and every affected
criterion 6-12 row pass, the original-resolution Beauty review accepts all five
required rock-family views plus Shipping, and the criterion-13 ledger advances
from `1/5` to `2/5` without weakening any rejection.

The next ordered criterion-13 product is cliff. M9 remains blocked until every
mandatory M8 exit condition is `PASS`.

Persistent Goal: ACTIVE until full JericoEngine 1.0 closure.
