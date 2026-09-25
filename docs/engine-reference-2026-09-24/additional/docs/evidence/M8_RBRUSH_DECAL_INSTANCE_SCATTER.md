# M8 RBrush Slice 26 — Decal and instance scatter

**Date:** 2026-08-14
**State:** RBrush Slice 26 `PASS`; C22 PWAS 2.0, projected decal rendering,
static scatter cook, the complete RBrush family, M8 and the master Goal remain
`PARTIAL / ACTIVE / NOT COMPLETE`.

## Observable result

The admitted `Rocks/Cliffs/Cliff Scatter` RBrush opens in the existing World
shell. Its fifth nested page, `Decals / Instances`, converts a captured World
stroke to the canonical hand-painted PWAS source and produces either typed
prefab placements or typed decal projectors. There is no new top-level tab,
World session, picker, transaction system or procedural framework.

Preview evaluates outside the Qt thread, authenticates the pre-evaluated result
on the owner thread and publishes only a transient typed overlay. Cancel writes
nothing. Apply, Undo and Redo use one transaction over the exact World and PWAS
authoring bytes; a late World, PWAS recipe, RBrush pack, prefab or material
change fails closed.

The retained editor evidence contains two real captures from the same editor,
project and admitted RBrush. The first shows `Prefab Instances`; the second
shows `Typed Decals`. Both show the nested page, its parameters, the Vulkan
viewport and the distinct typed overlay:

![RBrush decal and instance scatter](images/m8-rbrush-decal-instance-scatter.bmp)

Retained BMP: `8,242,614` bytes; SHA-256
`F6DA2A370710E4C991F6751941DF42E59751E150B36CF3AD7B5B9FB7A89F7BAD`.

Stable preview identities in the retained run were:

```text
instance_count=1
instance_guid=AFB0E120B6E72A667195575A2CB1BACD
decal_count=1
decal_guid=5A1DFFCD48100BBBA70D750CE6099B7E
```

## Determinism and PWAS authority

The evaluator proof runs the same typed graphs with one and four workers and
requires complete result equality, including order and hashes:

```text
prefab_count=4
prefab_recipe_hash=18281316646447519768
prefab_result_hash=14358668470326481960
prefab_one_four_equal=true
decal_count=4
decal_recipe_hash=13682808170341086981
decal_result_hash=7901046650000219135
decal_one_four_equal=true
changed_seed_result_hash=7647270209259067861
changed_seed_differs=true
```

Repeating the same decal Preview returns the exact same selection GUIDs.
Changing the graph seed produces a different valid result hash. Existing PWAS
manual locks/removals, provenance, material/prefab hashes and dirty-region
contracts remain authoritative; RBrush does not implement a second scatter
evaluator.

## C04 responsiveness and lifecycle

`RBrushDistributionJob` receives immutable brush, stroke, recipe, World query
and catalog snapshots. It uses the Interactive authoring profile with at most
four workers and a bounded memory estimate, then returns through a generation
fence to the Qt owner.

The focused lifecycle proof retained:

```text
cancelled=true
cancel_gap_ms=4
accepted_gap_ms=5
stale_callbacks=0
owner_thread=true
progress_events=1
dabs=2
workers=4
peak_bytes=17302944
shutdown_ms=0
```

The ordinary editor run retained maximum Qt heartbeat gaps of `30 ms` for the
instance Preview and `26 ms` for the decal Preview, both below the `<100 ms`
contract. Publication was `5 ms`. The same run measured exact durable commits
of `528 ms` Apply, `519 ms` Undo and `521 ms` Redo; these are owner-thread
journal transactions, not hidden evaluator work.

## Authentic RED and causal GREEN

The original Task 6 RED failed because `RBrushDistributionWorkbench` did not
exist. The minimum product path added the fifth nested page, catalog-backed
typed choices, the C04 job handoff, typed overlays and the existing World+PWAS
transaction.

The integrated proof then exposed a real layout-dependent test defect: the
Terrain test hard-coded `28` visible pages although normal window geometry
correctly produced `22`. Tagged measurements proved the adaptive Terrain dirty
region changed with viewport geometry; the assertion now compares the exact
runtime page count with its own triangles/draw metrics. No Terrain algorithm was
changed.

The retained evidence RED failed exactly because
`m8-rbrush-decal-instance-scatter.bmp` did not exist. GREEN captures the Vulkan
viewport for each typed Preview, composites it into the real Qt shell, requires
the frames to differ and writes the requested retained BMP.

## Verification

- Complete Debug editor build: `PASS`, `485.4 s`.
- Integrated Debug PWAS/RBrush matrix: `17/17 PASS`, `264.81 s`.
- Final post-evidence Debug focal: first run exposed one non-reproducible
  Terrain Weight heartbeat spike of `154 ms`; the same binary then passed
  isolated and the repeated matrix passed `5/5` in `34.22 s`. The spike remains
  recorded for the M8 renderer/pacing gate; the `<100 ms` assertion was not
  weakened.
- Focused BSP Vulkan executable: `PASS`, `6.0 s`.
- Focused static-mesh Vulkan executable: `PASS`, `3.1 s`.
- Final Release focal build: `PASS`; PWAS/RBrush matrix `5/5 PASS`, `11.95 s`.
- Runtime-only Vulkan Release build: `PASS`, `48.9 s`.
- Runtime-only, source exclusion, Shipping, procedural-authority and scheduling
  gates: `12/12 PASS`, `168.03 s`.
- Evidence GREEN: real editor `--world-surface-pick` `PASS`, `17.4 s`.
- `git diff --check`: no whitespace errors before documentation update.

## Scope and next legal work

Slice 26 proves deterministic authoring and a real editor overlay. Decal
projectors are still diagnostic overlay geometry, not projected decal shading.
This slice does not cook scatter results, add a runtime dynamic brush, package
`.rbrush`, or claim Shipping behavior for the new adapter. Existing Shipping
gates only prove that Qt, `.rbrush`, strokes and schema-5 authoring types do not
leak into runtime targets.

The next legal RBrush capacity is Slice 27 Derived 2D SDF:

```text
R8 crack mask -> valid 2D signed distance representation
```

Slice 28 SDF stamping remains later. Procedural 3D Layout remains `NOT STARTED`
inside PWAS/World and must not become another tab or framework.
