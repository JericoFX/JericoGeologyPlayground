# F4 geology pipeline startup

Status: local optimization verified by canonical generation and the adaptive
editor regression. This is not F4 completion or a Beauty acceptance.

## Problem and implementation

The synchronous canonical startup spent about29.465s inside source shader
creation and20.597s inside source pipeline creation; its additional readiness
wait was0.1ms. Surface Nets pipeline creation returned at80242.8ms cumulative,
before final dirty-pipeline setup. Previous editor runs also exceeded their
unchanged120s per-generation wait.

The standalone Vulkan geology device now requests optional native asynchronous
support. Pipeline creation overlaps the subsequent synchronous shader calls.
All twelve pipelines must report READY before the session can be returned.
Failures identify the pipeline. Shader compilation remains synchronous because
its compiler-output pointer is stack-owned and reused. Shader source, compiler
math flags, evaluation, formats and generated-data contracts are unchanged.

The owner explicitly requested hardware-adaptive defaults for the future
open-source engine. The final implementation uses Diligent's native worker count:
max(logical processor count,2)-1, with at least one worker. The initial fixed2
experiment is not the final policy. Borrowed renderer devices keep their existing
configuration and thread pool; unsupported devices retain Diligent's synchronous
fallback. No GPU model or CPU count is hard-coded.

## Reproduction and observed evidence

From F:/JericoEngine, using the configured Debug build:

```powershell
cmake --build build/windows-vulkan-editor --config Debug --target JericoGeologyGpuSessionTests JericoFractureEditorPanelTests
& ./build/windows-vulkan-editor/Tests/ProceduralAuthoring/Debug/JericoGeologyGpuSessionTests.exe --canonical-source
$env:QT_QPA_PLATFORM='offscreen'
& ./build/windows-vulkan-editor/QtRuntime/JericoFractureEditorPanelTests/Debug/JericoFractureEditorPanelTests.exe --geology-world-workspace F:/JericoEngine/docs/evidence/data/m8-physical-material-destruction-f/coarse-source.rmesh
```

- Final adaptive build99075 exited0: f4-adaptive-startup-build.log.
- Adaptive canonical test70865 exited0: f4-adaptive-startup-tests.log.
  All pipelines ready at68411ms cumulative. Canonical repeatability,
  cancellation, changed-layer evaluation and readback counts passed.
- The fixed2-worker canonical experiment89282 exited0 at68081.9ms for all
  pipelines ready. This machine shows no additional improvement from the
  adaptive worker count in these samples.
- The fixed2-worker full editor regression31945 exited0, with cold creations
  at70274.6/68855.3ms against earlier matched UI checkpoints83650.6/81096.4ms.
  Reprepare retained field hash13834464709005319960 and source mesh
  hash9387757058957299150; publication/restoration and fracture bake passed.
- Adaptive editor regression97235 exited0: f4-adaptive-startup-ui.log.
  Creation took69344.9/69044.6ms at the matched UI checkpoints. Generation,
  stale/cancel rejection, publication/restoration and fracture bake passed.
  Reprepare retained field hash13834464709005319960 and source mesh
  hash9387757058957299150. No build/test remains live from these runs.

Logs are retained under build/windows-vulkan-editor. Times are host wall durations,
not GPU execution times or frame timings. Canonical baseline and final startup
endpoints differ slightly; do not derive an exact percentage from those values.
Here cold means a new geology session; driver and OS caches were not reset.

## Completeness and limits

This change affects device initialization, scheduling, diagnostics, failure
handling and authoring/cooker startup. Evaluation still receives a fully ready
session. No source parameters, semantic fields, mesh/material/physical identities,
serialization or runtime representation are changed. Editor cancellation and
publication remain integration checks; no new runtime simulation or shader math
is introduced. Existing F4 end-to-end and semantic-lineage requirements remain
binding in ACTIVE_SLICE.md and M8_F4_STRUCTURAL_AUTHORING.md.

These measurements are local samples, not low-end-hardware acceptance. Startup
still takes about a minute; latency and cancellation during initialization need
further work. No portable performance guarantee, full repository PASS, natural
rock-family Beauty PASS or JericoEngine1.0 completion is claimed.
