# F4 interactive GPU layer preview

2026-09-06. Parent: `1e8bf0bed1b770eac613e527a87601298ee56e22`.
Status: **PASS for the tested interactive workflow; M8/Beauty remains PARTIAL/RED**.
Persistent Goal: ACTIVE until full JericoEngine 1.0 closure.

The owner explicitly requested adding layers and dragging values in the existing
native ImGui preview, as close to real time as possible, using the GPU route.
This supersedes the older control freeze for this diagnostic host only. Final Qt
editor UI stays M10.5. No geological algorithm, LOD threshold, recipe schema or
shader-cache admission is part of this checkpoint.

## Implemented behavior

- One resident GPU session; automatic initial preparation and 250 ms edit
  coalescing. Controls/camera remain usable while generating. New edits cancel
  obsolete work at safe boundaries; a completed older recipe cannot replace the
  latest draft. Failed/invalid/cancelled candidates preserve the displayed mesh.
- 25 layer templates covering the document-free admitted operations, including
  convex/concave/bubble weathering. Typed numeric controls, source extents/grammar,
  enable/disable, insertion, unique-identity duplication, deletion and ordering.
  The source cannot be removed, moved or duplicated. No automatic resolution loss.
- Bounded gesture-coalesced Undo/Redo, separate draft saving and native captures
  with the exact displayed recipe/mesh/collision. Captures exclude the panel.
- Setup, evaluation wall time, measured GPU pass sum and CPU cook are separate.
  Evaluation includes CPU validation/topology/readbacks; this is **not all-GPU**.
  No fallback CPU generator or alternate meshing path was introduced.
- Existing GPU admission constraints remain visible. Directed cracks require the
  existing spline workflow. MacroWarp, legacy Fracture, CaveSweep and standalone
  HeightfieldErosion are unavailable here. Catalog availability does not establish
  that arbitrary combinations/orderings are valid. Mask/spline authoring is not
  implemented again in this host.

## Native RED/GREEN and performance

All logs below are retained in the proof archive. Commands use the matched
`f4-strata-warp-material-01/member-1` recipe/mesh/collision plus `authored.rmat` and
its cooked textures; exact input paths are in each `inputs.txt`.

The prior program rejects `--verify-live` (`f4-live-preview-red.log`). The first
actual extended workflow then reproduced an Undo failure. Two defects were
corrected: button press/release was incorrectly treated as an already-recorded
drag gesture, and conditional status rows moved button hit targets between input
frames. Status rows now remain stable and history records the first actual edit.
The failing native runs `f4-live-preview-01/02.log` are retained.

`--verify-controls`: **exit 0, 2.81 s**, native ImGui input verifies insertion,
unique duplication, removal and exact Undo/Redo. `--verify-live`: **exit 0** in
both Debug (`f4-live-preview-03`) and Release (`f4-live-preview-release-01`). It
also checks:

1. Actual phase widget changes the draft by 0.05 cycles; generation changes
   vertices/indices, not only metadata.
2. Capture retains exact displayed recipe, mesh and collision.
3. Explicit cancellation preserves output; restoration is byte/pixel exact.
4. A second phase edit while automatic generation is running cancels the old
   candidate and publishes only the final +0.10-cycle recipe.
5. The warm request reuses its GPU session and saves matching draft/capture data.

Same recipe, resolution and material, validation enabled:

| Measurement | Debug | Release |
|---|---:|---:|
| First setup wall | 60,930.5 ms | 222,805 ms |
| First full generation wall | 66,381.8 ms | 223,381 ms |
| Warm generation wall | 5,326.35 ms | **472.149 ms** |
| Warm evaluation wall | 5,027.13 ms | 449.949 ms |
| Warm CPU cook | 299.217 ms | 22.1993 ms |
| Warm measured GPU passes | 79.7402 ms | 22.0746 ms |

These are individual witnessed samples, not a percentile or worst-case guarantee.
The 250 ms debounce, upload and presentation are outside generation wall time.
Release improves warm generation about 11.3x, but initial shader preparation is
still unacceptable for a seamless start. Keeping the session open avoids repeating
setup between edits. No claim of persistent-cache correctness or quick restart.
Busy-frame counts only prove loop responsiveness; they are not an FPS benchmark.

The subsequent interactive Release launch was left open for the designer (native
title `JericoEngine - Geology Preview`, responding): setup **4095.58 ms**, first
generation **4601.13 ms**, 3512 triangles matching the initial member. This is a
separate observed warm-start sample, recorded in `live-startup.json`; it does not
establish that all restarts will be fast or diagnose which existing cache helped.

The two generated captures in Debug and Release have **byte-identical recipe,
mesh and collision** (six file pairs). The +0.05-cycle result also matches the
earlier native preview proof `f4-native-preview-green-04` exactly. The final
result is 3,290 triangles, recipe hash `14135304695165896690`:

- mesh SHA256 `9d6f410c74a71e1d72e87fc01a5a780b725e65f814e4da8901a408041b3466f4`;
- collision SHA256 `03d2f41833b1ae88c2e643cde533dbcfb1981a418ecc4d09d19f54bfb4b11e5c`.

## Evidence and completeness boundary

`data/m8-f4-live-preview-01/metrics.json` contains the parsed timings and exact
cross-configuration hashes. `proof.zip`: **38 verified file entries**, 6,057,686
bytes, SHA256 `f7a6e23f20fe8d900a285f192bb1ca6f7752fbf18f23b2356fb5ff96c58608bf`.
Every archived entry was rehashed after compression. Original 1280x720 images:
`images/m8-f4-live-preview-01/panel.bmp` and `live-result.bmp`.

Data lineage: canonical recipe/draft -> existing resident GPU evaluation ->
validated dense surface/classification -> existing mesh/collision cook -> native
PBR preview -> recipe-matched saved/captured output. Source inputs are immutable;
outputs use new directories. Material and triangle physical palettes are retained.
There is no world/scene Apply or Shipping dependency on this diagnostic tool.

Source/input, serialization, authoring, CPU/GPU, renderer/materials, collision,
cook, cancellation, diagnostics, timing, captures and tests apply and were checked
at the boundaries above. LOD is not regenerated by this dense preview; its M8
baseline/gates remain separate and unchanged. Runtime ECS, gameplay events,
scripting, animation, audio, navigation, Sequencer, streaming, destruction and
world save/load gain no behavior from this UI change and require no new adapters.

INTEGRATION REQUIRED: final Qt authoring UI (owner JericoEditor, M10.5), using the
existing recipe/session/cook contracts and transactions. Full mask/spline editing
stays with the existing authoring owners. Further near-real-time optimization
must distinguish CPU costs/readback from GPU execution and preserve acceptance.

The worktree also contains earlier experimental stepped-source diagonal/region
changes and unrelated cache WIP, excluded from this UI checkpoint. Tests use the
current worktree engine; the tested native workflow output remains byte-identical
to the prior published workflow. No PASS is inferred for those other changes.
Native rock appearance remains artificial: this workflow does not close Beauty,
the four rocks/cliff, package-only proof or M8. PlaneCut remains closed.
