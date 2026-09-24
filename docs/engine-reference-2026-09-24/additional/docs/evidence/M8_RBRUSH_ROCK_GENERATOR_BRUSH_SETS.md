# M8 Slice32 — RBrush Rock Generator Brush Sets Evidence

**Date:** 2026-08-14

**State:** PASS for Slice32 Rock Generator Brush Sets

**Implementation checkpoints:**

- `ff8b13b feat(rbrush): project sdf defaults into catalog`
- `d61faba feat(rbrush): resolve deterministic rock brush sets`

## Observable product result

One Qt-free editor-authority resolver now turns the existing canonical RBrush
project catalog into deterministic Rock Generator crack/chip choices.

It reuses `queryProjectRBrushLibrary` for tag semantics, combines query results
with explicit weighted overrides, sorts by `(AssetGUID, PresetGUID)`, and emits
backend-ready `CrackCutter` or `ChipCutter` choices. Each choice retains the pack
content hash, canonical tip hash, sampled scale/depth/rotation, SDF profile and
operation seed.

The existing project catalog now projects `RBrushPresetIR::sdfDefaults`. No
filesystem scanner, duplicate brush catalog, UI-only selector or runtime
`.rbrush` path was added.

## RED

The new focal registration first changed the admitted scheduling inventory:

```text
tests: 290 -> 291
test inventory SHA-256:
f542a560e24e2c580f436e354bad32870ceca9f57a57642aadecee33c6fe1f26
-> 21ef2baa7df18e104eb53eb93dc0be0474e0458aa56e5c0956bef3cb10ab3151

scheduled: 68 -> 69
scheduled inventory SHA-256:
19ddbbd9f2b7af36be9850a9a1ff75c2cedc763de0475fca5875a556ceb25b2f
-> 5a0407991d8ed898843db6eeea98748c6b78fd8e3f89a0f5cd356cbe4afe7fe5
```

After admitting only that mechanical inventory change, the focal build failed
for the missing product contract:

```text
fatal error C1083: cannot open include file:
'Jerico/Editor/RBrushRockGeneratorBrushSet.hpp': No such file or directory
```

The catalog projection had an independent RED:

```text
error C2039: 'sdfDefaults' is not a member of
'Jerico::Editor::ProjectRBrushPresetCatalogEntry'
```

These were the expected failures before either production change existed.

## Deterministic metrics

Two independent executions returned exit code zero and byte-identical metric
output:

```text
query_candidates=2
crack_hash=15220791635596693644
reverse_hash=15220791635596693644
chip_hash=4018117597811150281
changed_seed_hash=7308239171001440359
changed_content_hash=1778542606885272603
```

`crack_hash == reverse_hash` proves the admitted catalog and preset input order
does not influence the recipe. Feature kind, seed and pack content each change
the recipe identity.

The first four retained crack choices were:

```text
choice=0 preset=3132333435363738393A3B3C3D3E3F40
scale=1.000000 depth=0.040000 rotation=4.822510
threshold=0.450000 gamma=1.250000 seed=9941533135769752487

choice=1 preset=3132333435363738393A3B3C3D3E3F40
scale=1.000000 depth=0.040000 rotation=0.898682
threshold=0.450000 gamma=1.250000 seed=6370354461332044375

choice=2 preset=3132333435363738393A3B3C3D3E3F40
scale=1.000000 depth=0.040000 rotation=5.759033
threshold=0.450000 gamma=1.250000 seed=11771440722288167507

choice=3 preset=3132333435363738393A3B3C3D3E3F40
scale=1.000000 depth=0.040000 rotation=2.591797
threshold=0.450000 gamma=1.250000 seed=18177081731083067324
```

The focal proof also demonstrated:

- `require={sdf}`, `any={crack,chip}`, `exclude={large}`;
- explicit query-candidate replacement without duplicate identity;
- an `8:1` explicit weight selecting the heavy candidate more than four times
  as often over 256 deterministic choices, independent of entry order;
- bounded scale/depth/rotation sampling and exact threshold/gamma/invert;
- rejection of zero seed/count, count above 4,096, conflicting filters,
  duplicate/missing/incompatible references, invalid weights/ranges/profiles
  and empty results.

## Verification

### Focal product test

```text
ctest --preset windows-vulkan-editor
  -R ^editor\.rbrush_rock_generator_brush_sets$
  --output-on-failure

1/1 PASS
0.04 s test time
```

### Catalog + SDF integration chain

```text
editor.rbrush_project_catalog
editor.rbrush_rock_generator_brush_sets
editor.rbrush_sdf_material_stamp_session
procedural.sdf_brush_stamp
procedural.sdf_brush_projection
procedural.sdf_brush_crack_geometry

6/6 PASS
4.38 s real time
```

### Scheduling and runtime/editor boundary

```text
repository.test_scheduling_contract PASS, 0.03 s
editor.boundary_audit PASS, 270.28 s
2/2 PASS
```

The boundary duration was nested runtime proof compilation/execution, not a
cooker and not Qt/UI work.

### Complete editor build

```text
cmake --build --preset windows-vulkan-editor
PASS
62.4 s incremental verification
```

The preceding full build process ran longer than the wrapper's 304 s limit but
remained responsive and completed. The subsequent unchanged build above
returned exit code zero and is the retained result; no product workaround was
made for the wrapper timeout.

## Procedural Layout authority reconciliation

After `git fetch origin`, local was five commits ahead and zero behind
`origin/main`; no remote content required merging. The following were read in
full before further planning:

- `docs/JERICO_PROCEDURAL_3D_LAYOUT_GENERATOR.md`;
- `docs/ui/procedural-world-design/README.md`;
- the complete vector content of
  `docs/ui/procedural-world-design/procedural-world-design.svg`.

The reference binds future Layout work to one full-size World/PWAS mode with
Plan/Perspective/Front/Side views of the same `Procedural3DLayout`, reusable
shapes, Focus Layout docks, traceable Sketch-to-Generated output, and explicit
`Respect/Touch/Wrap/Carve/Blend/Exclude` SDF policies in the existing Inspector.
It is design authority, not implementation evidence, and Slice32 did not
implement or claim it.

## Explicit boundary and remaining work

- No Rock Generator mesh, world placement or distribution was added.
- No UI, top-level tab/workspace, Cave/Dungeon editor or Procedural 3D Layout
  implementation was added.
- No static Surface Paint/SDF cook, package or source-free Shipping proof was
  added.
- RBrush remains editor-only authoring input. Shipping will consume only later
  cooked static results.
- No visual image is claimed for this data-selection slice. User UI references
  and concept art are not product evidence.
- Slice32 does not complete M8 or the JericoEngine 1.0 master Goal.

The next legal RBrush slice is Slice33 Procedural Source Usage: prove that an
RBrush functions as a non-mouse mask source for procedural placement.
