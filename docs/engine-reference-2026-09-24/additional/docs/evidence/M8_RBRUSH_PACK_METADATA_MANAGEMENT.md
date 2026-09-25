# M8 RBrush Pack Metadata Management Evidence

Date: 2026-08-12

## Result

`Move Preset To Group`, `Rename Preset` and `Rename Pack`: `PASS`.

`Delete Preset`: `PASS` for an unreferenced preset, the default safe `Cancel`,
referenced `Replace References` and `Force Delete` with a durable broken-
reference report when admitted Worlds consume the preset by canonical
`RBrushAssetGUID + PresetGUID`.

RBrush Slice 16 Pack Management remains `PARTIAL`. M8, C04 Jobs/Async and the
JericoEngine 1.0 master Goal remain active.

## Observable contract

- Move changes only normalized `groupPath`; Rename changes only the visible
  preset name. Both preserve every asset, preset and tip identity, settings,
  manifest record and source provenance.
- Delete removes one PresetGUID and its matching source-manifest record. It
  retains primary or dual TIPD while any surviving preset references the TipGUID
  and removes only tips that become unreferenced.
- Deleting the final preset produces a valid empty pack while retaining the
  source provenance record with an empty preset list.
- World authoring now admits a bounded, sorted, unique list of canonical RBrush
  references. Old World documents remain compatible because the field is
  optional; canonical write/read preserves it exactly.
- An admitted World consumer makes Delete fail closed with
  `editor.rbrush_pack_management_transaction.delete_referenced`. The default is
  Cancel and no file is changed.
- Explicit `Replace References` requires one different reference resolved by the
  admitted RBrush catalog. Preview lists every affected World; Apply replaces
  the pack and all exact World descriptors in one ProjectAsset transaction.
  Every World revision advances once and Undo/Redo restore all bytes exactly.
- An identical or missing replacement, cancellation, catalog drift or a World
  changed after Preview aborts the whole transaction before the pack is
  published. No partial fanout is accepted.
- Explicit `Force Delete` retains the unresolved reference in every affected
  World and appends one sorted, unique `rbrushBrokenReferenceReports` record
  with the deleting command GUID, exact source-pack content hash and fixed
  reason `PresetForceDeleted`.
- The pack and every World report publish in one ProjectAsset transaction.
  Preview identifies the references that remain broken; Apply advances each
  World revision once; Undo removes the reports and restores the pack; Redo
  restores the exact forced bytes. Force never invents a replacement or marks
  the World healthy.
- World read/write rejects reports with zero identity/hash, another reason,
  duplicate/unsorted references or a report whose unresolved reference was
  silently removed. Old descriptors without reports remain compatible.
- Preview and Cancel write nothing. Apply publishes one exact pack `Replace`;
  stale target bytes block Apply. Undo and Redo restore exact bytes.
- Rename Pack publishes one path-sorted ProjectAsset relocation: Delete of the
  admitted immediate `.rbrush` leaf plus Create of the new immediate leaf. Both
  snapshots carry identical container bytes, SHA-256 and canonical content
  hash; only `ProjectRBrushAsset::relativePath` changes.
- Rename Pack preserves AssetGUID, revision, content/file hashes, PresetGUIDs,
  TipGUIDs, source manifest, preset summaries and every World descriptor byte.
  Preview/Cancel write nothing; Apply/Undo/Redo alternate exact old/new paths.
- Same-path or case-only names, uppercase/wrong extensions, traversal,
  subfolders, occupied or case-folded destinations, cancellation and stale
  source/destination state fail closed without partial relocation.
- No Painter, RBrush runtime loader, cook path or Shipping dependency was added.

## RED

- Pure Delete first failed with MSVC `C1083` because
  `Jerico/AssetPipeline/RBrushPresetDelete.hpp` did not exist.
- Transaction Delete then failed because `RBrushDeletePresetRequest` and
  `prepareDeletePreset` did not exist.
- The first real World-consumer test failed admission because the canonical
  `rbrushPresetReferences` field did not exist. After adding the strict World
  authoring contract, the fixture initially failed the existing authored Level
  claim gate until it supplied a real cooked `.rlevel` with the matching GUID
  and content hash.

## GREEN and regression

- Final focal Debug test: `1/1 PASS` in `5.40 s`.
- Debug transaction matrix: `4/4 PASS` in `24.84 s`.
- Release transaction matrix: `4/4 PASS` in `19.04 s`.
- Matrix tests:
  - `editor.project_transaction_coordinator`
  - `editor.rbrush_project_catalog`
  - `editor.rbrush_import_transaction`
  - `editor.rbrush_pack_management_transaction`
- Scheduling contract: `PASS` in `0.02 s`.
- Runtime-only target discovery: `PASS` in `30.37 s`.
- Shipping source exclusion: `PASS` in `1.56 s`.
- Editor boundary audit: `PASS` in `295.73 s`.
- Runtime-only Release configure/build workflow: `PASS`, exit `0`, `66 s`.
- Runtime binary scan: 49 non-dependency `.exe/.dll/.lib`, zero Move, Rename,
  Delete, RBrush-reference or `.rbrush` authoring matches.
- Runtime project/configuration scan: 514 non-dependency files, zero matches.
- `git diff --check`: `PASS` before product publication.

### Referenced Delete / Replace References

- RED: the focal build failed because
  `RBrushDeletePresetReferencePolicy::ReplaceReferences`, the replacement field
  and Preview impact inventory did not exist.
- The first GREEN reached Preview but Apply failed closed with
  `editor.project_transaction.scope_invalid`; the coordinator still admitted
  only a single-file RBrush Replace. The next run failed the schema-1 target
  order gate, proving that the multi-root fanout also needed canonical root/path
  ordering.
- Final focal Debug: `1/1 PASS` in `6.73 s`.
- Debug transaction matrix: `4/4 PASS` in `26.58 s`.
- Release transaction matrix: `4/4 PASS` in `20.34 s`.
- The vertical uses two real admitted World descriptors plus cooked Level
  claims, proves Preview without writes, atomic Apply, exact Undo/Redo,
  invalid/missing replacement rejection, cooperative cancellation and
  late-World mutation rejection with the pack unchanged.

### Referenced Delete / Force Delete

- RED: the focal build failed because `RBrushBrokenReferenceReport` and
  `WorldDocumentRecipe::rbrushBrokenReferenceReports` did not exist.
- GREEN focal Debug: `1/1 PASS` in `7.77 s`.
- Fresh Debug build plus transaction/fanout matrix: `7/7 PASS` in `30.65 s`.
- Fresh Release build plus transaction/fanout matrix: `7/7 PASS` in `22.44 s`.
- Integrated World smoke Debug/Vulkan: `1/1 PASS` in `11.09 s`.
- Repository scheduling, runtime-only discovery, Shipping source exclusion and
  editor boundary audit: `4/4 PASS` in `319.00 s`; boundary audit `286.03 s`.
- The vertical uses two admitted Worlds and their real cooked Level claims,
  proves Preview without writes, retained canonical references, exact report
  provenance, strict invalid-report rejection and byte-exact Apply/Undo/Redo.
- Force with a replacement is rejected; Force with no broken consumer directs
  the caller to ordinary Delete. Cancel remains the default safe policy.

### Rename Pack

- RED: the focal Debug build failed with MSVC errors because
  `RBrushRenamePackRequest` and `prepareRenamePack` did not exist.
- The first integration run reached the coordinator and failed closed at its
  existing RBrush guards for scope, same-hash identity, initial Delete,
  EditorAsset target count and schema-1 ordering. The final implementation
  classifies exactly one canonical RBrush relocation and retains the journal's
  root/path execution order rather than weakening its generic validation.
- GREEN focal Debug: `1/1 PASS` in `8.53 s`.
- Fresh Debug transaction/fanout matrix: `7/7 PASS` in `32.18 s`.
- Fresh Release transaction/fanout matrix: `7/7 PASS` in `23.61 s`.
- Scheduling, runtime-only discovery, Shipping source exclusion and boundary
  audit: `4/4 PASS` in `318.18 s`; boundary audit `285.36 s`.
- `git diff --check`: `PASS` before product publication.
- This editor-authoring path does not claim renderer/runtime visual evidence;
  the Shipping gate proves that the new authoring symbols and source path do not
  enter the source-free target boundary.

## Published product

- `247636c feat(editor): delete unreferenced rbrush presets`
- `31e6493 feat(editor): replace deleted rbrush references`
- `dbbbc5b feat(editor): report forced rbrush reference breaks`
- `2d2d6a3 feat(editor): rename rbrush packs transactionally`
- `d195272` reconciled the concurrent incremental-cook authority without
  discarding local work.
- Verification completed after fast-forwarding to `9ec3b44`, which also adds
  the Hot Reload authority.

## New versioned authorities retained

The following documents were read completely and do not change the active
milestone:

- `docs/JERICO_INCREMENTAL_COOK_DELTA_EPAK.md`
- `docs/JERICO_HOT_RELOAD_LIVE_ASSET_REFRESH.md`
- `docs/JERICO_M8_RENDER_FOUNDATION_GATE.md`
- `docs/JERICO_M8_BSP_SDF_VISUAL_SHOWCASE_GATE.md`

They require one canonical explainable dependency graph, minimal deterministic
cook, delta `.epak` over cooked assets and generation-safe Hot Reload. They do
not authorize implementing those later owners inside this RBrush checkpoint.
The two M8 exit gates additionally block M9 until production Sponza rendering,
frame pacing and BSP/SDF/Terrain visual showcases have real measured,
source-free evidence; they preserve the legal order of current M8 slices.

## Preserved local work

All 12 pre-existing user-modified tracked paths and all untracked attachments,
the JericoBSP image and 14 conceptual editor images remain unstaged and
untouched.

## Next legal capability

Continue RBrush Slice 16 with `Sort`, the first remaining Pack Management item
in the authoritative list. Define its observable canonical order before RED;
`Tag` remains the following unproven requirement.
Do not advance Painter, runtime, Terrain/EPLS 2.0, JericoParticles, Vegetation,
incremental packaging or Hot Reload out of roadmap order.
