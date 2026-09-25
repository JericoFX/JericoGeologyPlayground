# M8 RBrush Add Source To Pack Evidence

Date: 2026-08-12

## Result

`Add Sources To Pack`: `PASS` for one exact prepared Inbox source per
transaction, including an admitted empty pack.

RBrush Slice 16 Pack Management remains `PARTIAL`. M8, C04 Jobs/Async and the
JericoEngine 1.0 master Goal remain active.

## Observable contract

- Existing target plus no prior source hash selects `AddSource`; Create and
  Update retain their prior meanings.
- Preview captures and binds the exact Inbox source, rejects a globally admitted
  duplicate hash, anchored-reopens the exact target and produces one `Replace`
  without writes.
- The pure combine step preserves the target `AssetGUID` and all existing
  sources/presets/tips, appends exactly one non-empty source and canonical-sorts
  and revalidates the complete graph.
- PresetGUID/TipGUID collision, duplicate source, wrong AssetGUID/cardinality,
  stale source/target and `ReferenceAndKeep` from Inbox fail closed.
- Cancel writes nothing. Apply requires admitted catalog plus exact final reopen
  before optional consume. Undo/Redo alternate exact prior/combined bytes.
- No UI, importer, dependency or runtime path was added.

## RED

The focal Debug build failed before implementation with:

- MSVC `C3861`: `addRBrushSourceToPack` not found;
- MSVC `C2838/C2065`: `RBrushImportMode::AddSource` did not exist.

## GREEN and regression

- Final Debug focal, three consecutive runs: `PASS` in `5.58 s`, `4.94 s`,
  `4.88 s`.
- Debug transaction matrix: `4/4 PASS` in `20.89 s`.
- Final Release transaction matrix: `4/4 PASS` in `15.78 s`.
- Matrix tests:
  - `editor.project_transaction_coordinator`
  - `editor.rbrush_project_catalog`
  - `editor.rbrush_import_transaction`
  - `editor.rbrush_pack_management_transaction`
- Scheduling contract: `PASS` in `0.02 s`; 255 total tests, 54 scheduled and 14
  `RUN_SERIAL`.
- Runtime-only target discovery: `PASS` in `30.26 s`, 43 active projects and
  zero authoring targets.
- Shipping source exclusion: `PASS` in `1.58 s`.
- Editor boundary audit: `PASS` in `275.81 s`, exact revision `8ab0517`.
- Runtime-only configure: `PASS` in `30.6 s`.
- Complete runtime-only Release build: `PASS`, exit `0`, `50.2 s`.
- Runtime binary scan: 49 non-dependency `.exe/.dll/.lib`, zero AddSource,
  RBrush, Inbox or authoring-path matches.
- Runtime generated-configuration scan: 214 files, zero matches.

## Published commits

- `3a00616 docs(rbrush): design add source transaction`
- `8ab0517 feat(editor): add sources to rbrush packs`

## Preserved local work

All 12 pre-existing user-modified tracked paths and all untracked attachments,
the JericoBSP image and 14 conceptual editor images remain unstaged and
untouched.

## Next legal capability

Continue RBrush Slice 16 with transactional `Merge Packs`. Do not advance UI,
Painter, runtime, Terrain/EPLS 2.0, JericoParticles or Vegetation.
