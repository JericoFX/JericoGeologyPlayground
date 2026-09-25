# M8 RBrush Material Height — Technical checkpoint

Date: 2026-08-13
Milestone: M8
Slice: 21 — Material Height
State: **PASS**

The Material Height vertical is implemented and verified from canonical RBrush
dabs through cooked runtime bytes. Mouse/tablet projection, localized Vulkan
preview, transaction lifetime, source-free Shipping and the focused RBrush UX
are now demonstrated on the shared World. Slice 21 is PASS; M8, C04 and the
JericoEngine master Goal remain ACTIVE / PARTIAL.

## Scope proven

- `SurfacePaintChannel::MaterialHeight` is canonical schema 2; schema 1
  Roughness-only bytes remain exact.
- Height evaluation writes only little-endian `PbrSurfaceTile::heightR16`.
- Mixed Roughness/Height layers are deterministic and report the union of
  changed texels.
- The intrinsic dirty dependency is exactly `SurfacePages`. Geometry pages,
  BSP, Sparse SDF, collision and navigation are not dirtied by Material Height.
- The Qt workbench selects Material Height, previews it asynchronously, applies
  through the existing multi-root transaction and preserves exact Undo/Redo and
  reopen behavior.
- SurfacePaint cook converts R16 deterministically to the existing ScalarMask
  Linear `.rtext` route. The proof fixture no longer manufactures Height through
  a parallel direct TextureCook path.
- The resulting BC4 Linear `.rtext` is consumed by the existing `.rmat` 1.3
  Height slot and its Off, Parallax and MicroDisplacement paths.
- Package and runtime-only replay remain source-free.

## RED evidence

1. Recipe/evaluator RED: MSVC failed on the absent
   `SurfacePaintChannel::MaterialHeight` enumerator in
   `SurfacePaintRecipeTests.cpp:118`.
2. Editor RED: `editor.rbrush_editor_workbench` failed because the channel
   selector exposed only Roughness, selected data was not `2`, diagnostics did
   not report Material Height and persisted recipe schema was not 2.
3. Integrated fixture RED:

```text
editor.material_height_displacement.fixture_evidence_missing
editor.material_height_fixture.succeeded|...|semantic=ScalarMask|vertices=4225|indices=24576
```

   The prior M6F fixture could not report
   `producer=SurfacePaint|channel=MaterialHeight|dirty=SurfacePages`.

## GREEN evidence

Product focal executables, Debug:

```text
JericoSurfacePaintRecipeTests.exe       exit 0
JericoSurfacePaintEvaluatorTests.exe    exit 0
JericoSurfacePaintCookerTests.exe       exit 0
JericoSurfacePaintTransactionTests.exe  exit 0
JericoSurfacePaintJobTests.exe          exit 0
surface_paint.job.preview|dimension=768|heartbeats=23|max_qt_gap_ms=5|changed_texels=69348
surface_paint.job.owner_destroyed|dimension=1024|destroy_ms=73|late_callback=false
```

Integrated proof:

```text
Debug   editor.material_height_displacement PASS 56.27 s
Release editor.material_height_displacement PASS 43.14 s
Debug integration matrix                7/7 PASS 95.19 s
Release RBrush + Height matrix           2/2 PASS 45.44 s
CTest inventory                          258 tests
windows-vulkan-runtime configure         exit 0
windows-vulkan-runtime Release build     exit 0
```

The Debug integration matrix included Qt admission, scheduling, runtime-only
target discovery, Shipping source exclusion, RBrush workbench, Material Height
Vulkan proof and Qt deployment.

## Runtime and package evidence

From `MaterialHeightDisplacement/evidence.properties`:

```text
mesh_guid=E1E01D769B0AB5680C508C70E3FFDBA2
published_material_guid=6485912ED6FE0854F26DCA4C84186076
package_inventory=material-000.rmat,model.rmesh,texture-000-height.rtext
package_bytes=397101
package_sha256=8eb749d17373bf382fa2e5088983eb0f89e1dd90e48f0c3c4fba2ac2d16530cd
package_height=3383|1c28b5afd276a374ea2d10ffc28d2b9acf2a9daa6ae4680a262a543cc13035a1
runtime_debug_sha256=8d80ff6db2d188bf5d6206a88f4e4993468c5db9fef0fbf8394211359393710f
runtime_repeat_sha256=8d80ff6db2d188bf5d6206a88f4e4993468c5db9fef0fbf8394211359393710f
runtime_release_sha256=8d80ff6db2d188bf5d6206a88f4e4993468c5db9fef0fbf8394211359393710f
```

The Off, Parallax and MicroDisplacement captures are non-blank and distinct.
Debug repeat and runtime-only Release replay are byte-identical.

## Performance boundary

The representative Qt preview stayed under the 100 ms responsiveness gate with
a measured maximum owner-thread heartbeat gap of 5 ms. Cancellation and owner
destruction produced no late callback.

A separate real 2048 x 2048 BC4 Height cook completed in 10.967 s. That is not
accepted as an interactive full-texture workflow. It is safely outside Qt, but
M8 still needs page/delta `Cook Changed` through the canonical virtualized-world
and incremental-cook contracts; no hidden full recook may be presented as
instantaneous.

## World-first UX evidence and next observable

The prior Brush Engine and standalone Surface Paint captures remain rejected.
The accepted retained evidence is:

```text
docs/evidence/images/m8-rbrush-world-first-material-height.png
1920 x 1032 | 174890 bytes
SHA-256 9549E7ABC78A8F2BD27B19B6D8165D5B06BF4922643C8EA8E5C70187D4DF5CD2
```

The image was captured from the Windows desktop after Vulkan reported
`editor.viewport.ready|backend=Vulkan|embedded=true`; the fixture consumes the
staged canonical PBR material instead of its four-byte placeholder. It shows the
loaded World as the dominant 1180 px viewport, a 480 px RBrush inspector,
explicit Paint and World Surface state, Material Height, the admitted brush,
layer/settings/actions, and an uncluttered final radius/falloff cursor. Fonts are
real Windows glyphs. The concept image 10 remains reference only, never evidence.

The ordinary functional tests remain offscreen. The evidence path instead uses
Windows QPA, stages the Diligent Vulkan runtime in `PATH`, captures the desktop
region so the native `QWindow` is included, and waits for window composition
before capture. A mutation with Paint unchecked failed at
`paintMode->isChecked()`. A second RED required the inspector to be at most 540
px; it failed before the RBrush page stopped imposing its old 520 px canvas
minimum and passed after the dock reached 480 px.

Final verification after the UX changes:

```text
Debug RBrush focal                         PASS 10.57 s
Release RBrush focal                       PASS  2.40 s
Debug runtime/Shipping/RBrush/Height/Qt    5/5 PASS 102.18 s
Release RBrush + Material Height           2/2 PASS 45.76 s
Surface Paint job                          exit 0
preview 768 | 69348 texels | max Qt gap 5 ms
owner destroyed 1024 | 74 ms | late_callback=false
Windows/Vulkan retained capture runner     exit 0
```

This proves the world-first RBrush shell, not interactive in-world painting.
The next missing observable is real pointer/tablet hit projection into the
shared World, with the same canonical dab/job/transaction path and visibly
painted Material Height relief on the selected surface. Screen-space fakery or
a second viewport is not acceptable.

## Commits in this checkpoint

- `87a860f` — design Material Height painting.
- `cad1992` — TDD implementation plan.
- `8665057` — canonical recipe and evaluator.
- `a998853` — Material Height SurfacePaint cook target.
- `b0d2365` — editor/job/transaction integration.
- `400083b` — Vulkan/editor/Shipping fixture integration.
- `a75e158`, `6d14889` — world-first UX design and plan.
- `78f3143` — world-first RED.
- `2e9d074`, `f08cf7c` — compact RBrush product and accepted Vulkan evidence.

M8 and the JericoEngine master Goal remain **ACTIVE / PARTIAL**.

## Shared World interactive Height checkpoint

Commit `581ee03` replaces the synthetic panel stroke with the real shared
viewport path. The editor now:

- ray-picks the admitted cooked World and preserves entity, mesh, material,
  Height texture and interpolated UV identity;
- captures mouse and tablet strokes with a radius/falloff bitmap cursor;
- evaluates the existing cancelable Surface Paint job off the Qt owner thread;
- updates the already resident Vulkan Height resource without creating a
  second viewport or performing a cook; and
- restores the byte-identical baseline frame on Cancel.

Fresh focal verification:

```text
editor.rbrush_editor_workbench       PASS 16.05 s
repository.test_scheduling_contract  PASS  0.02 s
render.shader_cook_proof             PASS  0.73 s
render.static_mesh_asset             PASS  8.93 s
Surface Paint preview 768 | 69348 texels | max Qt gap 5 ms
Surface Paint owner destroyed 1024 | 76 ms | late_callback=false
Windows/Vulkan interactive capture runner exit 0
```

The functional runner requires the preview frame to differ from the baseline
and the post-Cancel frame to equal the baseline byte-for-byte. The current
fixture uses the canonical Parallax Height mode; topology deformation remains a
separate MicroDisplacement proof and is not claimed here.

The new interactive capture was inspected and **rejected as UX/showcase
evidence**. It proves the product path, but the cyan authoring overlay dominates
the material, the on-surface brush state is not legible in the retained frame,
and the inspector still reads as a diagnostic form rather than a mature sculpt
and paint tool. It remains under `build/` only and is not copied into
`docs/evidence/images/`. Slice 21 therefore remains **PARTIAL**. The next legal
observable is a focused RBrush UX/UI pass with a legible surface cursor,
localized Height feedback, clearer sculpt/paint hierarchy and a newly captured
Windows/Vulkan frame that passes visual inspection.

## Accepted shared-World RBrush UX checkpoint

The rejected frame above was not retained. The focused UX pass keeps the same
backend and makes the tool state explicit without introducing a second viewport
or unsupported sculpt modes:

- the in-render cursor uses the real World hit and shows outer radius, dashed
  falloff, crosshair and active Height state;
- the preview texture encodes evaluated Height plus localized delta/direction,
  so unchanged texels no longer receive an unconditional cyan tint;
- the compact inspector keeps `PAINT · MATERIAL HEIGHT`, World target, active
  layer, admitted brush, radius, strength, spacing and preset falloff visible;
- layer tree and 2D diff remain available as secondary collapsible sections;
  `Edit preset` returns to the canonical Brush Preset tab; and
- Cancel, channel/World replacement, focus loss and viewport destruction clear
  transient cursor/preview state.

Accepted Windows/Vulkan evidence:

```text
docs/evidence/images/m8-rbrush-world-surface-material-height.png
1920 x 1032 | 154148 bytes
SHA-256 B1E5324DFE87A67807ECC0FA53DD1DC4C426B82B75F16D565097D2EDF07F442D
```

Original-resolution review passed: the World owns more than 60% of content
width; the cursor is visibly supported by the selected surface; radius and
falloff rings are distinct; Material Height feedback is localized; active
brush, operation, layer, World target and primary controls are legible; and no
placeholder or missing glyph is present.

Fresh verification for this checkpoint:

```text
Debug integration matrix   4/4 PASS 25.86 s
Release integration matrix 4/4 PASS  6.19 s
Debug Surface Paint job    preview 768 | 69348 texels | max gap 5 ms
                           owner destroy 1024 | 76 ms | late_callback=false
Release Surface Paint job  preview 768 | 69348 texels | max gap 5 ms
                           owner destroy 1024 | 30 ms | late_callback=false
Debug source-free proof    PASS 51.04 s
Release source-free proof  PASS 42.69 s
Windows/Vulkan capture     exit 0
```

An initial attempt to execute the two nested source-free proofs concurrently
was invalid because both target the same `build/vulkan-runtime` object tree;
one MSBuild process received `Permission denied` for `Main.obj`. Sequential
reruns passed in both configurations without a code change. This is retained as
orchestration evidence, not classified as a product failure.

This closes RBrush Slice 21 only. It does not claim topology deformation
authoring, Sparse SDF stamping, Texture Paint, Material Paint, Terrain,
incremental virtual-texture cook, M8 or the master Goal.
