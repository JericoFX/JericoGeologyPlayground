# M8 RBrush Terrain authoring container and transaction

**Date:** 2026-08-13  
**State:** Task 5 `PASS`; RBrush Slice 24, M8, C04 global and the master Goal
remain `ACTIVE / PARTIAL`.

## Observable proved

One admitted World owns at most one canonical binary `.terrain` facet. A
Terrain paint Preview evaluates the accepted RBrush layer without persistent
writes, and one ProjectAsset transaction publishes exactly the World descriptor
plus the Terrain container. Cancel writes nothing; Apply, Undo and Redo switch
both authorities atomically and byte-exactly. Changes to the World, Terrain or
RBrush after Preview reject Apply without partial publication.

## RED retained

- The first transaction build failed because
  `Jerico/Editor/TerrainPaintPlan.hpp` and
  `Jerico/Editor/TerrainPaintTransaction.hpp` did not exist.
- After freezing the binary-container decision, the container test linked
  unsuccessfully because `writeTerrainContainer`, `readTerrainContainer` and
  `inspectTerrainContainer` were undefined. This was the product RED for the
  canonical `.terrain` boundary.

The REDs were caused by missing product contracts, not a typo, empty UI or a
test-only flag.

## GREEN implementation

- `.terrain` schema 1 starts with `JTRN\r\n\x1A\n`, a fixed 40-byte header and
  fixed 64-byte directory records.
- `MANF`, `PHGT`, `PWGT` and `PLAY` chunks separate the small manifest, patch
  heights, patch/material weights and non-destructive paint layers.
- Directory identity is stable by `(kind, owner GUID, item GUID)`; the writer
  sorts patches/materials deterministically and stores a hash for every chunk
  plus the complete container.
- The reader fails closed on invalid magic/schema/counts, unknown or duplicate
  chunk identities, non-contiguous/overlapping/truncated ranges, invalid hashes,
  orphan chunks, unsafe paths, invalid UTF-8 display names and invalid Terrain
  IR.
- The World facet persists path, recipe GUID, revision and container hash. The
  catalog reopens and validates the embedded World ownership and every RBrush
  reference used by Terrain.
- The existing `ProjectTransactionCoordinator` remains the only publication
  and History authority; no Terrain-specific file writer bypasses it.

Changing one patch height sample changes that `PHGT` hash while the untouched
patch hash remains identical. This proves logical chunk invalidation, not yet a
physical patch-local disk update.

## Fresh verification

```text
cmake --build --preset windows-vulkan-editor --target
  JericoTerrainContainerTests JericoTerrainPaintTransactionTests
PASS, 14.7 s

cmake --build --preset windows-vulkan-editor
complete Debug build PASS, 461.7 s

publication rebuild after the legacy-preview regression fix
complete Debug build PASS, 66.2 s

ctest --test-dir build/windows-vulkan-editor -C Debug
  -R ^(terrain.container|terrain.recipe|terrain.evaluator|
       editor.terrain_paint_transaction)$ --output-on-failure
4/4 PASS, 4.12 s

ctest --test-dir build/windows-vulkan-editor -C Debug
  -R ^(terrain.container|editor.terrain_paint_transaction)$
  --repeat until-fail:3 --output-on-failure
2/2 tests, three repetitions each, PASS, 11.94 s

JericoSurfacePaintTransactionTests.exe
exit 0, 2.6 s

repository.test_scheduling_contract
1/1 PASS; inventory 263, scheduled 56, RUN_SERIAL 15

architecture.*
20/20 PASS, 1.35 s

ctest --preset windows-vulkan-editor -R
  ^(editor.world_render_dependency_closure|
    editor.project_asset_publication_transaction|
    editor.project_transaction_coordinator|
    editor.rbrush_project_catalog|
    editor.rbrush_import_transaction|
    editor.rbrush_pack_management_transaction|
    editor.terrain_paint_transaction|
    editor.procedural_migration_transaction|
    editor.world_level_bake_transaction|
    editor.world_level_bake_plan|
    editor.world_dependency_rebase|
    editor.material_asset_transaction|
    editor.material_project_catalog|
    editor.physical_prefab_asset_transaction|
    editor.source_asset_catalog|
    editor.smart_import_asset_transaction.*)$
  --parallel 1 --output-on-failure
19/19 PASS, 208.46 s
```

The complete 263-test matrix was also executed before publication. Its first
run passed 244 tests and failed 19. Serial diagnosis found one real shared
viewport regression: legacy RGBA8 `.rtext` assets carry the required
`Unspecified` format semantic and must derive their editable preview semantic
from the strict material binding. The regression was reproduced by a new
product-boundary test, fixed without weakening modern BC, Normal or Height
admission, and published separately as `157643d`. Fresh serial proof passed
`editor.rbrush_editor_workbench` and `editor.physical_prefab_door` (`2/2`,
121.17 s), and the original Material Paint rejection disappeared from the
remaining proof matrix.

The remaining broad failures are not reported as PASS: a serial rerun of the
17 original editor/procedural failures passed five and retained twelve failures
in proof/evidence flows (navigation status expectations, retained fixture
hashes, evidence capture sizing/PNG capture, Smart Import handshake timeout and
integrated fixture reopening). Several relevant proof/controller files have
preserved user changes, so this checkpoint does not assign those failures a
Terrain root cause or overwrite them. The 19 direct catalog, World, transaction
and publication regressions above pass serially and provide the scoped Terrain
regression evidence. The user-owned files remain preserved and were not edited
to force a green matrix.

Published checkpoints:

- `157643d` — strict material-binding fallback for legacy RGBA8 preview;
- `282c088` — binary Terrain container, World ownership and exact transaction.

## Authority reconciliation

ADR-035 supersedes the temporary `.terrain.toml` World facet. The text codec is
migration-only. `.terrain` is authoring; `.rterrain` is the required cooked
runtime manifest/container and does not yet exist. Vegetation, water, materials,
collision, navigation and environment remain independent GUID-linked assets or
facets rather than being physically duplicated inside Terrain.

## Explicit limitations

- The canonical writer currently rebuilds a complete `.terrain` byte vector and
  the catalog currently admits a bounded complete file (64 MiB). Range-read
  MANF/directory admission and patch-local physical replacement are pending.
- No C04 Terrain preview worker, Qt heartbeat/cancellation proof, viewport
  picking, editor Terrain panel or visual evidence exists yet.
- No `.rterrain`, dirty-tile cook, `.rmesh`, weight `.rtext`, terrain `.rmat`,
  `.rcoll`, `.rnav`, delta `.epak`, runtime render, collision/navigation proof
  or source-free Shipping replay is claimed.
- This checkpoint does not close RBrush Slice 24, M8, Virtualized World, JSVGI,
  C04 global or JericoEngine 1.0.

## Next action

Task 6 starts with an authentic RED for a generation-fenced, cancelable Terrain
paint job outside the Qt thread, including a measured heartbeat under 100 ms,
late-callback rejection and safe owner destruction. It reuses the active World
viewport and does not create a second Terrain painter.
