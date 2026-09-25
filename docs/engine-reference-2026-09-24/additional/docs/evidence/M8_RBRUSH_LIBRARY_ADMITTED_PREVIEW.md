# M8 RBrush Library Admitted Preview Evidence

Date: `2026-08-12`

Capability state: `PASS`

Slice state: RBrush Slice 15 Brush Library `PASS`

Higher state: M8 `ACTIVE / PARTIAL`; C04 Jobs/Async `PARTIAL`;
JericoEngine 1.0 `ACTIVE / NOT COMPLETE`.

## Scope demonstrated

This checkpoint closes RBrush Slice 15. A stable
`RBrushAssetGUID + PresetGUID` selected from the canonical project Brush
Library now produces an admitted 256x256 or 384x384 R8 coverage preview after
the original `.abr` or image source is gone.

The Qt-free primitive reopens only the catalog-owned project `.rbrush` through
the anchored filesystem boundary, compares the exact admitted container
SHA-256, validates and decompresses only the selected `TIPD`, revalidates full
tip GUID/format/dimensions/canonical SHA-256, converts all canonical formats and
fits the result into a square canvas without stretching. It supports
cooperative cancellation and never publishes partial output.

Combined with the preceding canonical catalog checkpoint, Slice 15 now proves
search, groups, tags, truthful source status, preview and deterministic pack
navigation after external sources are absent.

This checkpoint does not add Qt widgets, persistent DDC thumbnails, `THMB`,
the RBrush Editor test canvas, pack management, Painter, World paint records,
runtime RBrush loading, Terrain, SDF, PWAS/EPLS 2.0, particles, vegetation or
Animation/Rigging.

## Architecture and reused authority

- `ProjectRBrushAsset::containerFileHash` stores SHA-256 over the exact
  admitted `.rbrush` bytes. It is intentionally distinct from the existing
  semantic 64-bit `contentHash` used by transactions.
- `ProjectRBrushPresetCatalogEntry::canonicalTipHash` projects the already
  validated `RBrushTipIR` hash. Normal discovery and authenticated
  Create/Replace candidate construction populate both identities from bytes
  already in memory; no extra read or full decode was added.
- `loadProjectRBrushPresetPreview()` accepts only a project session, immutable
  canonical catalog snapshot and stable reference. Callers cannot supply an
  arbitrary path or `TipGUID`.
- The existing `readAnchoredFilePinned()` protects path/file identity and
  bounds the snapshot to `maximumRBrushContainerBytes`.
- The existing `readRBrushContainerTip()` validates header, complete TOC and
  selected chunk CRC, then decompresses only the requested `TIPD` payload.
- The existing half-float conversion was moved without semantic changes to
  `Jerico/AssetPipeline/Detail/RBrushHalf.hpp` and is shared by importer and
  preview. No duplicate decoder or dependency was introduced.
- Output allocation is fixed at 65,536 or 147,456 bytes. Fixed-point bilinear
  fitting preserves aspect ratio with zero-coverage padding.
- The synchronous API is a cancel-aware worker primitive. A later UI consumer
  must submit it through the C04 interactive scheduler; this checkpoint does
  not place I/O or conversion on the Qt owner thread.

## Product commits

- `a81acaa docs(rbrush): define admitted library preview`
- `62d1a67 docs(rbrush): plan admitted library preview`
- `71e58a5 feat(editor): pin rbrush preview identities`
- `27bde76 feat(editor): load admitted rbrush preset previews`
- `769a45b refactor(asset_pipeline): share rbrush half conversion`

## Authentic RED evidence

1. Catalog identity RED: the Debug focal build failed in
   `RBrushProjectCatalogTests.cpp` with MSVC `C2039` because
   `ProjectRBrushAsset::containerFileHash` and
   `ProjectRBrushPresetCatalogEntry::canonicalTipHash` did not exist.
2. Preview API RED: the next Debug focal build failed because
   `RBrushLibraryPreviewRequest` and `loadProjectRBrushPresetPreview` were
   absent. The GREEN was not produced by weakening the tests; the missing
   product API and behavior were implemented.

## Observable behavior

| Behavior | Evidence |
|---|---|
| Source-free authoring preview | Real project `.rbrush` previews after no original source file exists anywhere in the temporary project. |
| Stable selection | Only the exact `RBrushAssetGUID + PresetGUID` resolves; unknown or zero identities reject. |
| Stale replacement detection | Any exact container-byte change rejects with `editor.rbrush_library.preview_stale` until catalog refresh. |
| Selected-tip isolation | Corrupting an unselected compressed `TIPD` still allows the selected preset to preview after repinning the fixture; corrupting the selected `TIPD` rejects. |
| Canonical identity | Full TipGUID, format, dimensions, decoded canonical hash and catalog tip hash must all agree. |
| Format coverage | R8 UNORM, R16 UNORM, finite R16 float, RGB8 and RGBA8 all produce deterministic R8 coverage; non-finite half-float rejects. |
| Bounded canvas | Only 256 and 384 are accepted. Rectangular tips fit without crop/stretch and unused pixels remain zero. |
| Determinism | Repeated calls compare exactly equal, including every preview byte. |
| Cancellation | A pre-cancelled request rejects with the typed cancellation error; conversion and scaling also contain bounded stop checks. |
| Missing asset | A removed admitted `.rbrush` rejects as unavailable rather than consulting or reconstructing from its source. |

## Fresh final verification

### Focal RED/GREEN

- Identity GREEN: `editor.rbrush_project_catalog` plus
  `editor.rbrush_import_transaction`: `2/2 PASS`, `5.54 s`.
- Preview focal ran three consecutive times after GREEN: `1/1 PASS` each
  (`0.32 s`, `0.26 s`, `0.26 s`).
- Shared half conversion and extended missing/non-finite coverage:
  `editor.rbrush_project_catalog` plus
  `asset_pipeline.rbrush_image_importer`: `2/2 PASS`, `1.12 s`.

### Debug and Release integration

Exact final matrix on commit `769a45b`:

```text
editor.project_transaction_coordinator
editor.rbrush_import_transaction
editor.rbrush_project_catalog
```

- Debug: `3/3 PASS`, `18.70 s`.
- Release: `3/3 PASS`, `14.24 s`.

### Repository, Shipping and runtime-only

Final Release gates: `4/4 PASS`, `321.45 s`.

- scheduling contract: `0.02 s`;
- Shipping source exclusion: `1.61 s`;
- editor boundary audit: `291.84 s`;
- runtime-only target discovery: `29.57 s`.

`windows-vulkan-runtime` configured successfully in `28.0 s` and its complete
Release build passed in `50.2 s` with `/m:2 /nr:false`.

Literal scans used `rg -a` for binary artifacts and normal `rg` for generated
configuration. Both returned exit code `1` with no output, the documented
`rg` result for zero matches. The binary scan covered `49` non-dependency
runtime `.exe/.dll/.lib`. Patterns:

```text
loadProjectRBrushPresetPreview
RBrushLibraryPreviewRequest
RBrushLibraryPreview
containerFileHash
EditorAssets/Brushes
Import/Brushes
.rbrush
.abr
```

One earlier attempted PowerShell binary scan was discarded because PowerShell
7 rejected `-Encoding Byte`; it is not counted as evidence.

## Isolation and C04

`git diff --check` passed. All twelve pre-existing tracked user changes, the
attachment directory, the JericoBSP JPG and the fourteen conceptual images
remained unstaged and unmodified by this checkpoint.

The primitive accepts a stop token and performs only selected-tip
decompression, but no UI consumer is wired yet. Therefore this checkpoint
makes no editor FPS, callback-discard, shutdown or end-to-end responsiveness
claim. C04 remains `PARTIAL` for this future owner and the other heavy
cookers/bakes.

## Acceptance and next legal work

RBrush Slice 15 Brush Library is `PASS`: the project can browse, search,
navigate and preview canonical RBrush presets after original sources are gone,
with stable identities and source-free Shipping proof.

RBrush Slice 16 Pack Management is now the next legal slice and is
`NOT STARTED`. Its first observable capability is transactional
`Create Empty RBrush Pack`: a valid canonical empty pack created through the
Automation Bridge, admitted by the project catalog and reversible by
Preview/Cancel/Apply/Undo/Redo without breaking the asset graph. M8, C04 and the
master JericoEngine 1.0 Goal remain active.
