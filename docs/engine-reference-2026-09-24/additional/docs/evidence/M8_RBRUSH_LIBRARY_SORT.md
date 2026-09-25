# M8 RBrush Library Sort Evidence

Date: 2026-08-12

## Result

RBrush Slice 16 Pack Management `Sort`: `PASS`.

`Sort` is a deterministic Brush Library view operation. It does not rewrite a
`.rbrush`, change canonical IR, start cook, publish a transaction or add a
runtime/Shipping dependency. Slice 16 remains `PARTIAL`; `Tag` is the next
unproven Pack Management capability. M8, C04 Jobs/Async and the JericoEngine 1.0
master Goal remain active.

## Observable contract

- `GroupThenName`, `NameThenGroup` and `SourceThenName` are admitted sort keys.
- `Ascending` and `Descending` apply to canonical pack-path order and to preset
  order independently inside each pack; pack ownership is preserved.
- The default remains `GroupThenName + Ascending`, byte-for-byte compatible with
  the previous observable library order.
- Filtering completes before sorting, so sort never changes result membership.
- Every preset comparison ends in stable `PresetGUID`; every pack comparison
  ends in the exact project-relative path. Shuffled catalog input therefore
  produces the same view.
- Unknown sort-key or direction enum values fail closed with
  `editor.rbrush_library.query_invalid`.
- The admitted catalog snapshot and every AssetGUID, PresetGUID, metadata field,
  source state and source asset remain unchanged.

## RED

The focal Debug build failed because `RBrushLibrarySortKey`,
`RBrushLibrarySortDirection`, `RBrushLibraryQuery::sortKey` and
`RBrushLibraryQuery::sortDirection` did not exist. No production fallback could
express the requested order.

## GREEN and regression

- Focal Debug `editor.rbrush_project_catalog`: `1/1 PASS` in `0.32 s` real.
- Fresh Debug catalog/import/transaction matrix: `3/3 PASS` in `20.41 s`.
- Fresh Release catalog/import/transaction matrix: `3/3 PASS` in `15.45 s`.
- Matrix tests:
  - `editor.project_transaction_coordinator`
  - `editor.rbrush_import_transaction`
  - `editor.rbrush_project_catalog`
- Fresh gates after reconciling `origin/main`:
  - scheduling contract: `PASS` in `0.02 s`;
  - runtime-only target discovery: `PASS` in `29.95 s`;
  - Shipping source exclusion: `PASS` in `1.91 s`;
  - editor boundary audit: `PASS` in `300.41 s`;
  - aggregate: `4/4 PASS` in `332.34 s`.
- `git diff --check`: `PASS` before product publication.

The product tests additionally prove both directions, all sort modes, preserved
default behavior, shuffled input determinism, stable source-state ordering,
unchanged catalog input and invalid-enum rejection.

## Published product

- Design: `d68126a docs(rbrush): design deterministic library sort`.
- Product: `c78dfcd feat(editor): sort rbrush library views`.

The checkout reconciled the new Virtualized World Runtime and JSVGI authorities
through `664cd10` without discarding or staging user work. Those authorities
expand the M8 exit gate but do not change this view-only capability or authorize
skipping the legal RBrush sequence.

## Preserved local work

All pre-existing user-modified tracked paths and all untracked attachments, the
JericoBSP image and 14 conceptual editor images remain unstaged and untouched.

## Next legal capability

Continue RBrush Slice 16 with the observable contract and RED for `Tag`. Do not
advance Painter, runtime, Terrain/EPLS 2.0, JericoParticles, Vegetation or the
renderer exit gates out of roadmap order.
