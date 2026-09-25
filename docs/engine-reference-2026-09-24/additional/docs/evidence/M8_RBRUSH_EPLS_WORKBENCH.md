# M8 RBrush Slice 25 Task 5 — EPLS C04 World workbench

**Date:** 2026-08-14
**State:** Task 5 `PASS`; RBrush Slice 25, C26 EPLS, M8 and the master Goal remain `PARTIAL / ACTIVE`.

## Observable result

JericoEditor now exposes EPLS as the fourth nested surface of the existing
RBrush World workspace. It is not a new top-level tab, editor or procedural
framework. The surface reuses the admitted World picker/overlay and the opened
RBrush preset to author canonical EPLS semantics with Whole World or canonical
PWAS Region Mask scope.

One World surface stroke creates a real RBrush-backed manual layer. Include and
Exclude are replayed deterministically into separate U16 pages and displayed
alongside Procedural and Final. Exclude wins in the final composition. Preview
writes nothing; Apply, Undo and Redo use the Task 4 World + `.epls`
transaction and restore exact bytes.

## C04 owner

`EplsPaintJob` owns the heavy replay/composition/preview plan outside the Qt
thread through `AuthoringWorkCoordinator` with the `Interactive` profile. Its
request is an immutable snapshot with explicit project/document generations
and an estimated memory admission. Progress and completion return to the Qt
owner, cancellation is cooperative, an older generation cannot publish, and
destruction retires the worker without a late callback.

The focal Debug proof processed `736` deterministic brush dabs and changed
`15,352` texels with a maximum measured Qt heartbeat gap of `10 ms` in the
retained run. Cancellation produced `editor.epls_paint.cancelled`; the stale
generation callback and destruction callback were both discarded.

## Authentic RED and GREEN

The initial product RED failed to compile because `EplsPaintJob.hpp` did not
exist. The minimum GREEN then exposed two causal integration mistakes: the
canonical PWAS mask identity is `guid`, not `maskGuid`, and the M7G fixture
already owns the test helpers `guid`/`writeBytes`. Both were corrected at the
calling boundary. A test assumption of one dab per input sample was replaced
with behavioral assertions for real Include and Exclude work because canonical
brush spacing deterministically resamples strokes.

## Verification retained

- Debug focal EPLS job/workbench: `2/2 PASS`, `3.15 s` in the final run.
- Release focal EPLS job/workbench: `2/2 PASS`, `1.59 s`.
- Integrated components: EPLS focal, `procedural.mask`, scheduling and the
  isolated RBrush workbench all `PASS`.
- Test scheduling contract: `283` tests, `65` governed, `20` serial.
- EPLS, architecture, editor/runtime and Shipping boundaries: `31/31 PASS`.
  The boundary audit rebuilt seven Release runtime-only proof executables.
- `git diff --check` and staged diff check: no whitespace errors.

The first RBrush integration run immediately after rebuilding EditorShell
recorded a one-time Terrain preview Qt gap of `22,565 ms`; an isolated repeat
with the same binary passed at `26 ms` height and `20 ms` weight. This is
classified as cold Vulkan pipeline/shader warm-up, not a reproducible EPLS
regression. It remains a renderer/C04 risk and is not hidden as a Task 5 fix.

No screenshot is retained for this checkpoint: the automated offscreen view
did not provide a legible shared World plus all four masks, so it is not valid
visual evidence.

## Scope and remaining gaps

This checkpoint does not claim EPLS cook, incremental invalidation, material or
decal runtime consumption, MeshBlend, package/delta `.epak`, source-free
Shipping replay, Slice 25, M8 or master Goal completion. `.epls`, `.rbrush`,
ABR sources and editable strokes remain editor-only authoring inputs.

The next legal work is Task 6: cook the dirty EPLS result through the shared
dependency graph, publish the runtime output/package delta, prove runtime
consumption and source-free Shipping, then decide Slice 25 status from all of
its acceptance criteria.
