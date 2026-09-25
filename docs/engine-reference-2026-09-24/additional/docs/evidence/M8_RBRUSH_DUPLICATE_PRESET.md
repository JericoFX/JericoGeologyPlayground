# M8 RBrush Duplicate Preset Evidence

Date: 2026-08-12

## Result

`Duplicate Preset`: `PASS` for duplicating one preset inside one admitted
`.rbrush` as an independent Jerico-native variation through one reversible
Replace transaction.

RBrush Slice 16 Pack Management remains `PARTIAL`. M8, C04 Jobs/Async and the
JericoEngine 1.0 master Goal remain active.

## Observable contract

- The pack AssetGUID, all existing PresetGUIDs, all TipGUIDs, TIPD bytes and the
  complete source manifest remain exact.
- The duplicate receives one caller-owned non-zero PresetGUID and requested
  name. Group, tags, tip shape, dynamics, dual brush, texture, color dynamics,
  pose and miscellaneous flags are copied exactly.
- Immutable primary and dual TIPD are shared by GUID rather than duplicated.
- Only the duplicate's `sourceInfo` is cleared and no source manifest record is
  added. It is a Jerico-native designer variation and is not owned by a future
  source Update/Reimport.
- The pure operation is deterministic and rejects zero/missing source,
  zero/existing duplicate identity and invalid names.
- Preview and Cancel write nothing. Apply publishes one exact `Replace`; target
  mutation after Preview blocks publication.
- Undo restores the exact original bytes and Redo restores the exact duplicated
  bytes and catalog.
- No UI, move/rename/delete, dependency or runtime path was added.

## RED

The pure focal Debug build failed before implementation with MSVC `C1083`
because `Jerico/AssetPipeline/RBrushPresetDuplicate.hpp` did not exist.

The transaction RED then failed because `RBrushDuplicatePresetRequest` and
`prepareDuplicatePreset` did not exist. Remaining compiler errors were direct
cascade from those absent contracts.

The first pure GREEN compile exposed a direct missing `<algorithm>` include for
`std::ranges::sort`. Neighboring working RBrush merge/extract modules confirmed
the root cause; adding only that required include resolved it.

## GREEN and regression

- Pure Debug focal: `1/1 PASS` in `2.70 s`.
- Final Debug focal repeated three consecutive times: `3/3 PASS` in `3.40 s`,
  `3.31 s` and `3.40 s`.
- Debug transaction matrix: `4/4 PASS` in `22.41 s`.
- Release transaction matrix: `4/4 PASS` in `16.70 s`.
- Matrix tests:
  - `editor.project_transaction_coordinator`
  - `editor.rbrush_project_catalog`
  - `editor.rbrush_import_transaction`
  - `editor.rbrush_pack_management_transaction`
- Scheduling contract: `PASS` in `0.02 s`.
- Runtime-only target discovery: `PASS` in `30.06 s`.
- Shipping source exclusion: `PASS` in `1.54 s`.
- Editor boundary audit: `PASS` in `279.73 s`, exact transactional product
  revision `1292bf2`.
- Runtime-only Release configure/build workflow: `PASS`, exit `0`, `65.7 s`.
- Runtime binary scan: 49 non-dependency `.exe/.dll/.lib`, zero Duplicate or
  RBrush authoring matches.
- Runtime project/configuration scan: 647 non-dependency files, zero Duplicate
  or RBrush authoring matches.

## Published commits

- `49135d0 docs(rbrush): define duplicate preset transaction`
- `130ef34 docs(rbrush): plan duplicate preset transaction`
- `6791de6 feat(asset-pipeline): duplicate rbrush presets natively`
- `1292bf2 feat(editor): duplicate rbrush presets transactionally`

## Preserved local work

All 12 pre-existing user-modified tracked paths and all untracked attachments,
the JericoBSP image and 14 conceptual editor images remain unstaged and
untouched.

## Next legal capability

Continue RBrush Slice 16 with transactional `Move Preset To Group`. Do not
advance UI, Painter, runtime, Terrain/EPLS 2.0, JericoParticles or Vegetation.
