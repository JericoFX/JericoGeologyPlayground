# M8 RBrush Surface Painter Roughness / Slice 20

**Date:** 2026-08-13
**State:** `PASS` for RBrush Slice 20 only. M8, C04 and the JericoEngine 1.0
master Goal remain `ACTIVE / PARTIAL`.

## Accepted product result

JericoEditor now exposes `Surface Paint` beside `Brush Preset` in the RBrush
workspace. The first vertical is deliberately narrow: one admitted RBrush
preset paints deterministic `Roughness + Add` strokes onto the active World's
PBR surface tile. The workspace shows a large Before/Preview split, the actual
brush tip, canonical dab footprints, layers, scalar controls, changed texels,
`SurfacePages`, cook state and measured canvas render time.

Preview evaluates an immutable World/RBrush/base-tile snapshot on a `QThread`.
It performs zero project writes. Cancel is cooperative. Apply performs only the
brief existing multi-root project transaction that publishes the canonical
Surface Paint recipe and updates its owning World facet. Undo, Redo and reopen
restore/re-evaluate the exact canonical bytes. Destroying the owner detaches the
completion callback and lets the cancelled worker retire without waiting on the
Qt thread.

This is the first functional painter layout, not the final JericoEditor UX/UI
polish pass. The captured `canvas FPS` is the measured Qt canvas paint cost; it
is not claimed as renderer or viewport FPS.

## C04 and persistence proof

The fresh job probes produced:

```text
Debug  preview 768: heartbeats=21, max_qt_gap_ms=5, changed_texels=69348
Debug  cancel 1024: max_qt_gap_ms=0
Debug  owner destruction 1024: destroy_ms=65, late_callback=false
Release preview 768: heartbeats=8, max_qt_gap_ms=5, changed_texels=69348
Release cancel 1024: max_qt_gap_ms=0
Release owner destruction 1024: destroy_ms=28, late_callback=false
```

The real World product test additionally proves:

- immediate Cancel: zero World/SurfacePaint writes and stale completion discard;
- Preview: Qt heartbeat gap below `100 ms` and zero writes;
- Apply: World bytes change and the canonical Surface Paint facet appears;
- Undo: exact original World bytes and no Surface Paint file;
- Redo: exact applied World and Surface Paint bytes;
- reopen: existing facet loads through the anchored worker path and re-evaluates
  without modifying either file.

C04 remains `PARTIAL`: this checkpoint proves the Surface Paint owner only and
does not close the remaining cooker/import/bake/package owner matrix.

## Cook, package and source-free Shipping

The Release Shipping proof uses the real Khronos BoomBox ORM texture
(`2048 x 2048`, 12 mips), cooks the painted result through the existing
`TextureCooker`, publishes one `.epak`, removes authoring/source directories and
loads the result through the runtime-only path.

```text
surface_paint.shipping.succeeded
guid=07AF40354A6BB757A3502F59341B5572
old=0xEAE2C786A364F851
new=0xF93A755BD748697C
package_sha256=931d04ef0fe4f33aa3129633236b699c8b586379ba5a806a4f9e193299346afb
capture_sha256=ea71eeeb5d88c0c4bdaddbd9a2978f3acbcdbf0c3ab3f3127d5505401def7971
```

The complete Release build/proof took `92.2 s`; it is an explicit full
Build/Package validation, not the interactive paint path. An accidental Debug
invocation established that unoptimized BC7 recompression is unsuitable as a
latency measurement; no alternate cooker or reduced-quality Shipping path was
introduced.

## Verification matrix

```text
Debug build: JericoRBrushEditorWorkbenchTests, JericoSurfacePaintJobTests,
             JericoEditor PASS
Debug editor.rbrush_editor_workbench: PASS 9.30 s
Debug JericoSurfacePaintJobTests: exit 0

Release build: JericoRBrushEditorWorkbenchTests, JericoSurfacePaintJobTests,
               JericoEditor PASS
Release editor.rbrush_editor_workbench: PASS 2.21 s
Release JericoSurfacePaintJobTests: exit 0

repository.qt_admission_contract: PASS
repository.test_scheduling_contract: PASS
repository.runtime_only_target_discovery: PASS
repository.shipping_source_exclusion: PASS
editor.qt_deployment: PASS

JericoSurfacePaintShippingProof Release: PASS 92.2 s
```

The product test checks that Roughness changes while all other PBR fields remain
byte-exact, emits one bounded dirty region with exactly `SurfacePages`, and uses
the shared Brush Engine rather than another dab evaluator.

## Retained visual evidence

- [Surface Painter Roughness](images/m8-rbrush-surface-painter-roughness.png)
- Dimensions: `1600 x 900`
- Bytes: `244365`
- SHA-256:
  `97C2BC30EBC7601020BD09EC66E82F79F66CE376D13C28296481F8F791426DB5`
- Capture type: real Qt Windows product test, not concept art.

## Boundary and next action

No Surface Paint evaluator, RBrush/ABR source, recipe, Qt UI or authoring tip is
added to runtime targets or the Shipping package. No Surface Paint result is
presented as the M8 renderer foundation gate, Virtualized World completion,
JSVGI completion, or JericoEngine completion.

The next legal RBrush capability is Slice 21 `Material Height`. It must begin
with its own design/RED and may not broaden this Roughness slice or advance M9.
