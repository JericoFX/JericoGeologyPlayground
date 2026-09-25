# M8 RBrush Terrain adaptive page cook

**Date:** 2026-08-13
**State:** Task 7 `PASS`; RBrush Slice 24, M8, C04 global and the master Goal
remain `ACTIVE / PARTIAL`.

## Observable proved

A Terrain without an imported heightmap starts as one coarse runtime page per
patch. A bounded Height edit builds a deterministic 2:1-balanced quadtree and
subdivides only the affected locality. A material-weight-only edit leaves that
geometry tree coarse and emits no mesh, collision or navigation payload.

The complete lightweight `.rterrain` manifest records stable patch/Morton/depth
page identity, sample bounds, transition mask, cook key and runtime GUIDs. Dirty
page families emit validated `.rmesh`, lossless linear material-weight `.rtext`,
layered Terrain `.rmat`, `.rcoll` and `.rnav` payloads. Preparation uses 1-8
workers in bounded batches and serialization gathers canonically.

## RED retained

The adaptive contract first failed independently from the earlier uniform
cooker prototype:

```text
TerrainCookerTests.cpp(2,10): fatal error C1083:
No se puede abrir el archivo incluir:
'Jerico/Terrain/TerrainRefinement.hpp': No such file or directory
```

This failure demonstrated that the local refinement/page geometry API did not
exist. The prior patch-level cooker was not published as the Terrain design.

## GREEN implementation

- `TerrainRefinementPolicy` validates an even fixed page grid, maximum depth and
  a one-level neighbor delta. The authoring lattice is exactly
  `(pageQuads << maxDepth) + 1` samples per axis.
- Height dirtiness splits intersecting leaves to the admitted maximum depth,
  then balances edge neighbors to 2:1 across patch boundaries. Surface-only
  dirtiness does not alter the geometry tree.
- Page identity is deterministic from patch GUID, Morton code, depth, payload
  family and output family. Child order is fixed `SW, SE, NW, NE`.
- Fine transition vertices snap to the exact coarse boundary segment. The test
  injects a nonlinear midpoint and proves the transition no longer follows the
  unmatched fine height. Shared patch-border vertex sets are byte-exact.
- Runtime mesh, material weights, layered material, collision and navigation
  payloads are read back with the existing canonical readers.
- Every material-weight mip pixel is normalized to sum exactly `255`.
- Dependency records answer `Why is this asset cooking?` with
  `Dirty Terrain page intersects authored change`.
- Worker counts `1` and `4` execute different bounded preparation schedules but
  produce byte-identical `TerrainCookResult` values.

## Fresh measurements

Direct Debug output:

```text
terrain.cooker.interior|changed_pages=7|manifest_pages=8|dependencies=36|rterrain_bytes=1156|rmesh_bytes=990|rtext_bytes=477|rmat_bytes=609|rcoll_bytes=455|rnav_bytes=752
terrain.cooker.border|changed_pages=20|seam_vertices_equal=true
terrain.cooker.weights_only|changed_pages=1|mesh=false|weights=true|material=true|collision=false|navigation=false
```

The interior fixture uses two adjacent `9x9` patches with `pageQuads=2` and
`maxDepth=2`. The edited patch contains seven leaves at depths 1 and 2 while the
untouched neighboring patch remains one depth-0 page. The manifest therefore
contains eight pages and only seven changed page payload groups.

## Verification

```text
Debug complete build: PASS

Debug focal integration:
terrain.cooker + runtime_formats.rmat/rmesh/rtext/rcoll/rnav
+ repository.test_scheduling_contract
7/7 PASS

terrain.cooker direct repeat:
20/20 PASS

Release focal build: PASS
Release focal integration: 7/7 PASS

CTest inventory: 266
scheduled inventory: 58
RUN_SERIAL: 17
git diff --check: PASS
```

## Explicit limitations

- This checkpoint proves runtime bytes and adaptive topology, not a rendered
  Terrain inside Vulkan/JericoEditor.
- Live generation replacement, Terrain shader binding, virtual residency,
  World Cells, VT, JSVGI/shadows, delta `.epak`, package and source-free Shipping
  replay remain unproved.
- `.terrain` authoring range IO and patch-local physical updates remain future
  work; the cooker consumes an immutable evaluated snapshot off the Qt thread.
- No imported heightmap workflow is required for local refinement, but imported
  heightmaps will enter through the same patch lattice rather than a second
  Terrain system.
- C04 remains globally `PARTIAL`; other heavy cookers/bakes keep their own
  pending audits.

## Next action

Task 8 begins with the Vulkan Terrain material RED: load the cooked page mesh,
weight texture and 2-4 referenced PBR materials, render an observable layered
Terrain, and prove safe scoped generation replacement. This does not authorize
M9 or claim Terrain/Slice 24 complete.
