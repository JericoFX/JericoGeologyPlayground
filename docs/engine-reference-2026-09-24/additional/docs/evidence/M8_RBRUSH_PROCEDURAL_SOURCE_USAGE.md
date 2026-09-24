# M8 Slice33 — RBrush Procedural Source Usage Evidence

**Date:** 2026-08-14

**State:** PASS for Slice33 Procedural Source Usage

**Implementation checkpoint:**

- `b569599 feat(pwas): use rbrush tips as procedural masks`

## Observable product result

PWAS schema 6 now admits one explicit `RBrushMaskFilter` between a procedural
scatter and its downstream filters. Grid, Poisson and spline placement can use
the immutable R8 coverage of a catalog-pinned RBrush tip as a projected World
mask without synthesizing or replaying a mouse stroke.

The graph pins brush asset GUID, preset GUID, brush-pack content hash, tip GUID,
projection frame, extents, threshold, gamma and invert. The editor adapter
reuses `RBrushLibraryEditablePreset`; no second scanner or brush catalog was
introduced. Evaluation resolves the real support position, samples bilinearly,
rejects outside the projection, and produces deterministic per-node statistics.
An existing Poisson preset binding survives graph construction unchanged.

## RED

The product tests were committed before implementation:

- `bf0fd50 test(rbrush): require procedural mask source`
- `f224a05 test(pwas): require schema six rbrush mask source`

The first focal builds failed for the absent product contracts:

```text
fatal error C1083: cannot open include file:
'Jerico/Editor/RBrushProceduralMaskSource.hpp': No such file or directory

missing PwasRBrushMaskSource.hpp
missing PwasNodeKind::RBrushMaskFilter
missing PwasRBrushMaskFilter
```

## Canonical and deterministic evidence

Schema 6 round-trips the complete RBrush mask node while schema 1-5 golden
outputs remain byte-exact. Missing/stale tip content fails with
`procedural.pwas.rbrush_mask_dependency_stale`; cancelled graph construction
fails with `procedural.pwas.rbrush_mask_cancelled`; invalid projections fail
with `procedural.pwas.rbrush_mask_invalid`.

The retained focal output is:

```text
procedural.pwas.rbrush_mask_source.evidence
accepted=2
rejected=2
input_hash=9121500286876942151
output_hash=1482684853974904927
workers_one_four_equal=1
stroke_samples=0
```

The same test also covers grid, Poisson and spline construction, invert,
dependency staleness, cancellation, invalid projection and preservation of the
existing Poisson preset binding.

## Verification

### Debug focal matrix

```text
editor.rbrush_project_catalog PASS
procedural.pwas_graph PASS
procedural.pwas_recipe PASS
procedural.pwas_evaluator PASS
4/4 PASS, 1.96 s
```

### Release focal matrix

```text
Release build of the four focal targets PASS, 81.1 s
editor.rbrush_project_catalog PASS
procedural.pwas_graph PASS
procedural.pwas_recipe PASS
procedural.pwas_evaluator PASS
4/4 PASS, 0.52 s
```

### Integrated Debug build

```text
cmake --build --preset windows-vulkan-editor -j 4
PASS, 38.5 s
```

### Architecture and source-free boundaries

```text
repository.test_scheduling_contract PASS, 0.02 s
repository.runtime_only_target_discovery PASS, 35.79 s
repository.shipping_source_exclusion PASS, 2.83 s
editor.boundary_audit PASS, 264.92 s
4/4 PASS, 303.60 s
```

The earlier broad PWAS regression matrix passed 10/10, including weighted
Poisson. The RBrush editor matrix passed five tests under parallel load; the
workbench timing assertion observed one `153 ms` gap against its `<100 ms`
threshold, then passed alone in `34.68 s`. This is retained as a parallel-load
timing risk, not hidden and not attributed to the Qt-free mask evaluator.

The generic `dev` configure path remains independently broken by Terrain test
references, and `windows-vulkan-dev` with the editor disabled exposes a test
link-order/configuration defect. The authoritative `windows-vulkan-editor`
Debug and Release gates above are green; no product workaround was made for
those unrelated presets.

## Boundaries and remaining work

- This is editor authoring data. Runtime and Shipping do not load `.rbrush`.
- No UI, new workspace, Procedural 3D Layout implementation, static cook,
  package or visual PASS is claimed.
- Slice34 owns baking static Surface Paint output with all RBrush dependencies
  removed. Slice35 owns the corresponding static Sparse SDF cook.
- Slice33 does not complete RBrush as a whole, M8 or the JericoEngine 1.0 Goal.
