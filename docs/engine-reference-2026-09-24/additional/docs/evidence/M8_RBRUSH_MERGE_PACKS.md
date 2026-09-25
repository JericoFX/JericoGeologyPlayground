# M8 RBrush Merge Packs Evidence

Date: 2026-08-12

## Result

`Merge Packs`: `PASS` for two or more admitted project packs merged into one
new canonical `.rbrush` through one reversible Create transaction.

RBrush Slice 16 Pack Management remains `PARTIAL`. M8, C04 Jobs/Async and the
JericoEngine 1.0 master Goal remain active.

## Observable contract

- The pure merge is independent from selection order and validates every input.
- The output receives a new AssetGUID; copied presets and canonical TIPD receive
  deterministic new GUIDs.
- Exact identical TIPD payloads deduplicate; same hash with different payload
  fails closed.
- Preset names, groups, tags, tip shape, dynamics and source metadata survive.
- Primary/dual tip links and source-manifest preset links are remapped.
- Exact duplicate sources coalesce; ambiguous source name/hash or metadata
  conflicts fail closed.
- Preview and Cancel write nothing. Apply publishes one Create after rechecking
  exact pinned input identity/bytes. Inputs remain byte-exact.
- Undo removes only the merged output; Redo restores its exact bytes/catalog.
- No UI, importer, dependency or runtime path was added.

## RED

The focal Debug build failed before implementation with:

- MSVC `C1083`: missing `Jerico/AssetPipeline/RBrushPackMerge.hpp`;
- MSVC `C2039`: `prepareMergePacks` was not a member;
- MSVC `C2065`: merge request/source contracts were undeclared.

An initial attempt with the generic `dev` preset stopped before this RED because
that preset did not create `Jerico::EditorProjectAuthority`. The authoritative
`windows-vulkan-editor` preset isolated the intended product failures.

## GREEN and regression

- Final Debug focal, three consecutive runs: `PASS` in `2.05 s`, `1.97 s`,
  `2.07 s`.
- Debug transaction matrix: `4/4 PASS` in `21.37 s`.
- Release transaction matrix: `4/4 PASS` in `16.43 s`.
- Matrix tests:
  - `editor.project_transaction_coordinator`
  - `editor.rbrush_project_catalog`
  - `editor.rbrush_import_transaction`
  - `editor.rbrush_pack_management_transaction`
- Scheduling contract: `PASS` in `0.02 s`.
- Runtime-only target discovery: `PASS` in `30.35 s`.
- Shipping source exclusion: `PASS` in `1.55 s`.
- Editor boundary audit: `PASS` in `297.50 s`, exact product revision `166eb79`.
- Runtime-only Release configure/build workflow: `PASS`, exit `0`, `65.8 s`.
- Runtime binary scan: 49 non-dependency `.exe/.dll/.lib`, zero merge/RBrush
  authoring matches.
- Runtime generated-configuration scan: 475 non-dependency project/config files,
  zero merge/RBrush authoring matches.

## Published commits

- `ef41106 docs(rbrush): define merge packs transaction`
- `166eb79 feat(editor): merge rbrush packs transactionally`

## Preserved local work

All 12 pre-existing user-modified tracked paths and all untracked attachments,
the JericoBSP image and 14 conceptual editor images remain unstaged and
untouched.

## Next legal capability

Continue RBrush Slice 16 with transactional `Extract Selected Presets To New
Pack`. Do not advance UI, Painter, runtime, Terrain/EPLS 2.0, JericoParticles or
Vegetation.
