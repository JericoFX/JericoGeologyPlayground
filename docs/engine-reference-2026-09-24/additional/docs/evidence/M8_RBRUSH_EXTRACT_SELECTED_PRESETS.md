# M8 RBrush Extract Selected Presets Evidence

Date: 2026-08-12

## Result

`Extract Selected Presets To New Pack`: `PASS` for copying one non-empty,
unique selection from one admitted `.rbrush` into one new canonical pack through
one reversible Create transaction.

RBrush Slice 16 Pack Management remains `PARTIAL`. M8, C04 Jobs/Async and the
JericoEngine 1.0 master Goal remain active.

## Observable contract

- Extraction is a copy: the source pack remains byte-exact through Preview,
  Cancel, Apply, Undo and Redo.
- Selection is independent of request order and closes primary plus dual-tip
  references without copying unselected presets or tips.
- Output pack, presets and tips receive new deterministic, domain-separated
  identities. Exact TIPD payloads deduplicate; a hash naming different bytes
  fails closed.
- Only source-manifest records belonging to selected presets survive, with
  remapped PresetGUIDs. Literal authoring settings remain intact.
- Empty, duplicate or missing PresetGUID selections, output/source identity
  collision and stale catalog identity are rejected.
- Preview and Cancel write nothing. Apply rechecks the anchored input identity
  and exact bytes; source drift after Preview blocks publication.
- Apply publishes one Create, Undo removes only the output and Redo restores its
  exact bytes/catalog.
- No UI, importer, dependency or runtime path was added.

## RED

The pure focal Debug build failed before implementation with MSVC `C1083`
because `Jerico/AssetPipeline/RBrushPackExtract.hpp` did not exist.

The transaction RED then failed with missing
`RBrushExtractPresetsRequest`, `RBrushExtractPresetSource` and
`prepareExtractPresets`. An initial GREEN compile exposed one direct missing
`<algorithm>` include; adding that required include was the only correction.

## GREEN and regression

- Final Debug focal repeated three consecutive times: `3/3 PASS` in `2.71 s`,
  `2.68 s` and `2.65 s`.
- Debug transaction matrix: `4/4 PASS` in `22.09 s`.
- Release transaction matrix: `4/4 PASS` in `16.96 s`.
- Matrix tests:
  - `editor.project_transaction_coordinator`
  - `editor.rbrush_project_catalog`
  - `editor.rbrush_import_transaction`
  - `editor.rbrush_pack_management_transaction`
- Scheduling contract: `PASS` in `0.02 s`.
- Runtime-only target discovery: `PASS` in `29.91 s`.
- Shipping source exclusion: `PASS` in `1.54 s`.
- Editor boundary audit: `PASS` in `283.22 s`, exact product revision
  `73ff52c`.
- Runtime-only Release configure/build workflow: `PASS`, exit `0`, `65.8 s`.
- Runtime binary scan: 49 non-dependency `.exe/.dll/.lib`, zero extraction or
  RBrush authoring matches.
- Runtime project/configuration scan: 647 non-dependency files, zero extraction
  or RBrush authoring matches.

## Published commits

- `36e95eb docs(rbrush): define preset extraction transaction`
- `73ff52c feat(editor): extract rbrush presets transactionally`

## Preserved local work

All 12 pre-existing user-modified tracked paths and all untracked attachments,
the JericoBSP image and 14 conceptual editor images remain unstaged and
untouched.

## Next legal capability

Continue RBrush Slice 16 with transactional `Duplicate Preset`. Do not advance
UI, Painter, runtime, Terrain/EPLS 2.0, JericoParticles or Vegetation.
