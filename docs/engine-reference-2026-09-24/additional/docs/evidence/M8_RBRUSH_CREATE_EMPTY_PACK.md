# M8 RBrush Create Empty Pack Evidence

Date: 2026-08-12

## Result

`Create Empty RBrush Pack`: `PASS`.

RBrush Slice 16 Pack Management remains `PARTIAL`. M8, C04 Jobs/Async and the
JericoEngine 1.0 master Goal remain active.

## Observable contract

- Preview validates a strict lowercase `.rbrush` leaf, an Automation Bridge
  command, canonical empty IR, exact round-trip bytes/hash, the complete
  conditional Create closure and the candidate asset catalog without writes.
- Cancel writes nothing.
- Apply creates only absent `EditorAssets`, `EditorAssets/Brushes` and the exact
  pack, then reopens it through the anchored filesystem and verifies the exact
  catalog row and canonical IR.
- Undo removes the pack and only roots created by the transaction; pre-existing
  roots survive. Redo restores the exact bytes and identity.
- Invalid names, paths, roots, scope/kind, duplicate GUID/path, stale baselines
  and canceled requests are rejected.
- Empty pack creation has no external source manifest and introduces no new
  dependency or runtime/Shipping authoring path.

## RED

The focal test was admitted before the API existed. Its first Debug target build
failed with MSVC `C1083` because
`Jerico/Editor/RBrushPackManagementTransaction.hpp` did not exist.

## GREEN and regression

- Debug focal, three consecutive runs: `PASS` in `1.32 s`, `1.35 s`, `1.30 s`.
- Debug transaction matrix: `4/4 PASS` in `19.92 s`.
- Release transaction matrix: `4/4 PASS` in `15.24 s`.
- Matrix tests:
  - `editor.project_transaction_coordinator`
  - `editor.rbrush_project_catalog`
  - `editor.rbrush_import_transaction`
  - `editor.rbrush_pack_management_transaction`
- Test scheduling contract: `PASS`; 255 total tests, 54 scheduled tests and 14
  `RUN_SERIAL` tests. Scheduled hash
  `65e13150fa7018d7c742ecbec410d622284059ec7773d6f8c2d295d49af0e71a`.
- Runtime-only target discovery: `PASS` in `30.15 s`.
- Shipping source exclusion: `PASS` in `1.57 s`.
- Editor boundary audit: `PASS` in `282.27 s`.
- Complete `windows-vulkan-runtime` Release build: `PASS`, exit `0`, `49.6 s`.
- Runtime binary scan: 49 non-dependency `.exe/.dll/.lib`, zero Pack
  Management/RBrush/EditorAssets/Import authoring matches.
- Runtime generated-configuration scan: 214 files, zero matches.
- `git diff --check 499971d..75550e3`: `PASS`.

## Published commits

- `499971d docs(rbrush): define empty pack creation`
- `6b0e1ef feat(editor): create empty rbrush packs transactionally`
- `75550e3 test(repository): admit rbrush pack management`

## Preserved local work

All pre-existing user-modified tracked paths and all untracked attachments,
JericoBSP image and 14 conceptual editor images remain unstaged and untouched.

## Next legal capability

Continue RBrush Slice 16 with transactional `Add Sources To Pack`. Do not
advance UI, Painter, runtime, Terrain/EPLS 2.0, JericoParticles or Vegetation.
