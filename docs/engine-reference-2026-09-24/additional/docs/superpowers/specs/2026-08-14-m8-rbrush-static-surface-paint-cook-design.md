# M8 RBrush Slice 34 — Static Surface Paint Cook Design

**Status:** accepted implementation design

**Date:** 2026-08-14

**Milestone:** M8

**Slice:** RBrush 34 — Cook static Surface Paint

**PASS:** `World renders correct baked output with RBrush dependency removed`

## 1. Decision

Extend the existing `Jerico::SurfacePaint::SurfacePaintCooker`; do not create a
second cooker, package path, runtime brush asset or Surface Paint format.

The existing evaluator remains the only owner of brush/stroke evaluation and
the existing `TextureCooker` remains the only owner of `.rtext` production.
Slice 34 adds a bounded World/page adapter that:

1. accepts immutable page snapshots and canonical dirty regions;
2. selects only intersecting pages for `Cook Changed`;
3. derives deterministic cook keys and exact reason records;
4. emits Shipping-readable `.rtext` bytes under stable page/output GUIDs;
5. exposes only runtime texture dependencies after the bake; and
6. proves a cooked World renders after every RBrush/recipe/source input is
   removed.

RBrush AssetGUID, PresetGUID, tip data and strokes remain authoring inputs. They
participate in the cook key through the accepted recipe/RBrush-set revision but
never become runtime dependency records or package entries.

## 2. Why this option

Three approaches were considered:

1. Treat the existing BoomBox Shipping fixture as Slice 34 PASS. Rejected: it
   manually assembles cooked outputs and never proves a World-owned Surface
   Paint facet or page-aware invalidation.
2. Add a new World Surface Paint cooker beside the existing cooker. Rejected:
   it would duplicate evaluation, texture cook and dependency ownership.
3. Add a page-aware orchestration layer to the existing cooker. Selected: it
   reuses the proven evaluator/TextureCooker, matches the EPLS/Terrain
   incremental contract and preserves the Virtualized World page identity
   required by M8.

## 3. Existing components reused

- `SurfacePaintRecipe` remains the canonical editor authoring value.
- `evaluateSurfacePaint` / `evaluateMaterialPaint` remain the only evaluators.
- `SurfacePaintCooker::cook` / `cookMaterial` remain the per-output cook path.
- `TextureCooker` remains the `.rtext` writer, mip/BC owner and readback gate.
- `DirtySpatialRegion` remains the shared bounded invalidation contract.
- `WorldDocumentRecipe` remains the World/facet owner.
- `WorldRenderDependencyClosure` remains the cooked runtime dependency
  resolver.
- `PackageTool`, VFS and Asset Registry remain the only package/runtime route.

## 4. Static page cook contract

The Surface Paint tool layer adds Jerico-owned values equivalent to:

```text
SurfacePaintStaticPageInput
  pageGuid
  surfaceOwnerGuid
  world bounds
  per-page SurfacePaintCookRequest

SurfacePaintStaticCookRequest
  worldDocumentGuid
  surfacePaintRecipeGuid
  pages
  dirtyRegions
  fullCook
  toolchainContentHash
  targetProfileHash
  runtimeFormatVersion

SurfacePaintStaticCookDependencyRecord
  sourceGuid
  outputGuid
  cookKey
  why

SurfacePaintStaticCookedPage
  pageGuid
  surfaceOwnerGuid
  textureGuid
  cookKey
  rtextBytes

SurfacePaintStaticCookResult
  changedPages
  pages
  dependencies
  runtimeTextureGuids
```

Exact C++ names may be shortened if the test makes the same ownership clear.

For incremental cook, a page is selected only when its validated bounds
intersect a validated dirty region carrying `SurfacePages`. Full cook selects
every page. Output order is canonical by raw `pageGuid` bytes.

The exact normal reason is:

```text
Dirty Surface Paint page intersects authored change
```

`Force Recook Selected` and processor/profile invalidation may add their own
explicit reasons later through the same record type. Slice 34 does not invent a
general project dependency graph.

## 5. Cook key

The per-page cook key includes every material input that can change the output:

```text
domain/version
worldDocumentGuid
surfacePaintRecipeGuid
pageGuid
surfaceOwnerGuid
page bounds
base tile hash
World content hash
Surface Paint recipe hash
RBrush set hash
resolved payload identities/hashes
toolchain content hash
target profile hash
runtime format version
quality
```

The output `.rtext` recipe hash may retain the opaque RBrush-set hash because it
is a deterministic build input. It must not retain a brush path, AssetGUID,
PresetGUID, tip bytes or authoring dependency record.

Same inputs with one or four workers must produce identical page order, cook
keys and `.rtext` bytes.

## 6. World/runtime boundary

The authoring World continues to own:

```text
WorldDocumentRecipe
-> SurfacePaint facet
-> RBrush preset references
```

The accepted static bake projects that authoring state into cooked runtime
dependencies:

```text
World runtime presentation
-> stable Material GUID
-> stable baked Texture GUID(s)
```

No Surface Paint recipe or RBrush reference is serialized into `.rlevel` or
`.epak`. A World/material can keep stable Texture GUIDs while only the dirty
`.rtext` page generation changes.

The first implementation may use the admitted fully resident `.rtext` payload
as the hardware fallback. Its page GUID, output GUID, cook key, dirty selection
and dependency records are already page-aware; VT later changes storage and
residency without creating a second authoring/cook path.

## 7. C04 ownership

Evaluation and TextureCook remain worker work. The owner supplies immutable,
Qt-free snapshots, bounded worker count/memory and a cancellation token.

Cancellation before complete closure publishes nothing. Result application,
catalog refresh and package mounting occur only on their current owner threads
after generation/world/recipe revalidation. A late page result cannot overwrite
a newer generation.

Slice 34 proves worker-count determinism and cancellation at the tool boundary.
It does not move publication into Qt or add a second scheduler.

## 8. TDD and acceptance

### RED 1 — page-aware incremental cook

The existing `surface_paint.cooker` test requests two bounded pages and one
dirty region. It must initially fail because no World/page cook API exists.

Required assertions:

- exactly one page changes;
- the selected page identity/output GUID are stable;
- the dependency record contains the exact reason;
- the other page remains absent from the changed set;
- runtime dependency GUIDs contain only cooked texture identities;
- RBrush AssetGUID/PresetGUID are absent from cooked bytes and runtime closure;
- one/four workers are byte-equal;
- cancellation returns no partial result.

### RED 2 — source-free World replay

Extend the existing Surface Paint Shipping proof so the package contains a
cooked World/Level whose render dependency closure resolves the baked textures.
The proof must fail first with:

```text
surface_paint.shipping.world_baked_surface_missing
```

Then it must:

1. cook/package the World and baked textures through canonical tools;
2. remove `SourceAssets`, `Procedural`, `EditorAssets`, cache, recipes, ABR and
   `.rbrush`;
3. mount only `.epak`;
4. render twice through Vulkan with identical captures;
5. show the painted result, not the pre-paint baseline; and
6. verify no authoring identifier/path/string exists in package/runtime
   dependencies.

## 9. Non-goals

- Static Sparse SDF cook (Slice 35).
- Runtime dynamic brush derivative (Slice 36).
- The complete generic Shipping validator matrix (Slice 37).
- VT sparse residency implementation itself.
- Material Studio UX changes.
- RBrush format/schema changes.
- A new `.rsurfacepaint` runtime asset.

## 10. PASS gate

Slice 34 is PASS only when the focal unit/integration tests, Debug and Release
builds, Shipping source-exclusion boundaries and retained Vulkan World replay
all pass. Compilation, a `.rtext` readback or the existing mesh-only BoomBox
fixture alone is not PASS.
