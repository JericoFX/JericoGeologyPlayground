# M8 RBrush Terrain C04 preview and shared viewport

**Date:** 2026-08-13
**State:** Task 6 `PASS`; RBrush Slice 24, M8, C04 global and the master Goal
remain `ACTIVE / PARTIAL`.

## Observable proved

One immutable Terrain recipe plus resolved RBrush tips evaluates outside the Qt
owner thread under the shared interactive authoring budget. The owner receives
bounded progress and only the active generation. Cancel returns no publishable
candidate, generation N cannot call back after N+1 starts, and owner destruction
joins the worker inside the admitted `100 ms` deadline with no late callback.

The active World viewport now derives the Terrain ray from its existing camera
and `pickWorldSurface` result. That same ray intersects evaluated heightfield
triangles and returns the stable Terrain recipe/patch GUID, interpolated world
position, normal and distance. No second viewport, camera convention, painter or
Terrain render path was created.

## RED retained

The first job build failed with:

```text
TerrainPaintJobTests.cpp(1,10): fatal error C1083:
No se puede abrir el archivo incluir: 'TerrainPaintJob.hpp'
```

After the job GREEN, the picking target failed independently with:

```text
TerrainViewportInteractionTests.cpp(1,10): fatal error C1083:
No se puede abrir el archivo incluir: 'TerrainViewportInteraction.hpp'
```

Both failures represented absent product contracts. The initial job fixture
then exposed that a one-material Terrain is invalid; the fixture was corrected
to the canonical two-layer sum `45875 + 19660 = 65535` without weakening product
validation.

## GREEN implementation

- `TerrainPaintJob` moves one immutable recipe/RBrush snapshot to
  `QThread::create` and uses the existing `AuthoringWorkCoordinator` with the
  `Interactive` profile and a pre-admission working-set estimate.
- Only one CPU-heavy Terrain generation runs physically at once. A replacement
  generation requests cooperative cancellation; the queued worker waits off Qt
  until the retiring producer releases the coordinator slot.
- Worker progress and completion marshal back to the QObject owner. Generation
  changes discard queued progress and completion from prior work.
- The evaluator's existing cancellation checks cover layers, strokes, dabs and
  raster rows. Failure/cancellation never returns a candidate.
- Height samples are interpreted as UNorm16 across each patch's declared
  vertical bounds. A patch AABB plus two-dimensional DDA visits only the cells
  crossed by the ray, tests their actual grid triangles and chooses the nearest
  positive hit across all stable patch identities.
- `EditorViewportWindow::pickWorldTerrain` consumes the active cooked/BSP World
  camera and existing surface ray. Physical Prefab, Material Preview and empty
  viewport states fail closed.

## Fresh measurements

Direct Debug executable output:

```text
terrain_paint.job.preview|dabs=768|changed_height_samples=9984|max_qt_gap_ms=10|heartbeat_count=6|progress_events=4
terrain_paint.job.cancel|candidate=false|max_qt_gap_ms=0
terrain_paint.job.generation_fence|old_callback=false|current_callback=true
terrain_paint.job.owner_destroyed|dabs=1024|destroy_ms=2|late_callback=false
terrain.viewport.pick|patch=second|world_y=2.5|distance=47.5|tested_cells=1
```

Direct Release executable output:

```text
terrain_paint.job.preview|dabs=768|changed_height_samples=9984|max_qt_gap_ms=10|heartbeat_count=2|progress_events=4
terrain_paint.job.cancel|candidate=false|max_qt_gap_ms=0
terrain_paint.job.generation_fence|old_callback=false|current_callback=true
terrain_paint.job.owner_destroyed|dabs=1024|destroy_ms=0|late_callback=false
terrain.viewport.pick|patch=second|world_y=2.5|distance=47.5|tested_cells=1
```

## Verification

```text
Debug full build after DDA integration: PASS, 116.5 s

Debug focal repetition:
editor.terrain_paint_job + editor.terrain_viewport_interaction
50 repetitions each: PASS, 4.92 s

Debug integration:
Terrain recipe/container/evaluator/transaction/job/picking,
AuthoringWorkCoordinator, viewport math/surface picking,
repository scheduling and architecture.*
30/30 PASS, 5.59 s

Release focal build:
JericoTerrainPaintJobTests + JericoTerrainViewportInteractionTests
+ JericoEditorShell
PASS, 41.7 s
```

The CTest inventory is now `265`, scheduled inventory `57`, and `RUN_SERIAL`
count `16`; `repository.test_scheduling_contract` passes. The Terrain job is
serial because it measures Qt heartbeat and global interactive ownership. The
Qt-free picker remains parallel-safe.

## Explicit limitations

- This checkpoint does not render a live Terrain candidate and is not visual
  editor/runtime evidence.
- No `.rterrain`, dirty-tile `.rmesh`/weight `.rtext`/terrain `.rmat`,
  `.rcoll`, `.rnav`, `.rlevel`, delta `.epak`, package or source-free Shipping
  Terrain path exists yet.
- The picker now avoids a full triangle scan inside a patch, but still scans
  patch AABBs linearly. Runtime page residency and a world-level patch spatial
  index remain part of Terrain runtime geometry and Virtualized World.
- C04 remains globally `PARTIAL`; every remaining heavy cooker/import/bake/
  validation/package owner still needs the same contract and evidence.
- RBrush Slice 24, renderer M8, M8 and JericoEngine 1.0 are not complete.

## Next action

Task 7 begins with an authentic RED for deterministic dirty-tile Terrain cook:
one bounded Height/Weight change must emit only the intersecting `.rmesh`,
material-weight `.rtext` and `.rterrain` identities with an exact dependency
explanation. It must not hide a full Terrain/world recook.
