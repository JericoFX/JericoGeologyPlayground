# M8 RBrush Preset Tags Evidence

Date: 2026-08-12

## Result

RBrush Slice 16 Pack Management `Tag`: `PASS`.

All actions enumerated for Slice 16 now preserve a valid asset graph, so RBrush
Slice 16 Pack Management is `PASS`. RBrush, M8, C04 Jobs/Async and the
JericoEngine 1.0 master Goal remain active and incomplete.

## Observable contract

- One request replaces the complete desired tag set of one admitted PresetGUID.
- The pure AssetPipeline operation sorts and removes exact duplicates, allows an
  empty set to clear tags and validates the result through canonical RBrush IR.
- Invalid/missing PresetGUID, invalid UTF-8, empty or oversized tag text, `/`,
  `\`, more than 64 canonical tags and no-op edits fail closed.
- Tag spelling/case is preserved; Brush Library query remains case-insensitive.
- Only `RBrushPresetIR::tags` changes. AssetGUID, PresetGUID, name, group,
  TipGUIDs, settings, source manifest, `sourceInfo`, other presets and TIPD remain
  exact. World references therefore require no fanout.
- JericoEditor reuses the existing single-pack ProjectAsset `Replace` path.
  Preview/Cancel write nothing; Apply/Undo/Redo alternate exact bytes; stale
  target/catalog state rejects without partial publication.
- Reopening the project and querying the canonical Brush Library resolves the
  newly tagged preset from the persisted `.rbrush`.
- No tag sidecar/index, schema fork, UI, Painter, cook, runtime loader or
  Shipping dependency was added.

## RED

The focal Debug build failed with MSVC `C1083` because
`Jerico/AssetPipeline/RBrushPresetTags.hpp` did not exist. The tests also
referenced the absent `RBrushTagPresetRequest` and `prepareTagPreset` product
contract.

## GREEN and regression

- Focal Debug build: `PASS` after implementing the minimum product path.
- Focal Debug `editor.rbrush_pack_management_transaction`: `1/1 PASS` in
  `10.12 s` real (`9.21 s` test).
- Fresh Debug build of the four affected targets: `PASS`.
- Debug transaction/catalog/import matrix: `4/4 PASS` in `29.21 s`.
- Fresh Release build of the same four targets: `PASS`.
- Release transaction/catalog/import matrix: `4/4 PASS` in `22.01 s`.
- Matrix tests:
  - `editor.project_transaction_coordinator`
  - `editor.rbrush_project_catalog`
  - `editor.rbrush_import_transaction`
  - `editor.rbrush_pack_management_transaction`
- Fresh repository/Shipping gates:
  - scheduling contract: `PASS` in `0.02 s`;
  - runtime-only target discovery: `PASS` in `29.90 s`;
  - Shipping source exclusion: `PASS` in `1.88 s`;
  - editor boundary audit: `PASS` in `286.36 s`;
  - aggregate: `4/4 PASS` in `318.20 s`.
- `git diff --check`: `PASS` before product publication.

## Published product

- Design: `8893636 docs(rbrush): design transactional preset tags`.
- Product: `767758e feat(editor): tag rbrush presets transactionally`.

## Preserved local work

All pre-existing user-modified tracked paths and all untracked attachments, the
JericoBSP image and 14 conceptual editor images remain unstaged and untouched.

## Next legal capability

Continue RBrush Slice 17 Editor. Before adding property UI or a private RBrush
metadata fork, audit and freeze the shared canonical metadata/dirty-region seam
required by `docs/JERICO_VIRTUALIZED_WORLD_RUNTIME_1_0.md` and
`docs/JERICO_JSVGI_GLOBAL_ILLUMINATION.md`. The first product RED must exercise
one real editable preset property set through persistence and the existing
transaction path; it must not claim the Editor slice from a form or empty panel.
