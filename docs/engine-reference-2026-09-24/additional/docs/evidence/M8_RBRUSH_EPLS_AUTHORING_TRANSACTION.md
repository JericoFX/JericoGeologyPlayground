# M8 RBrush Slice 25 Task 4 — EPLS authoring transaction

**Date:** 2026-08-14
**State:** Task 4 `PASS`; RBrush Slice 25, C26 EPLS, M8 and the master Goal remain `PARTIAL / ACTIVE`.

## Observable result

JericoEditor can now persist one canonical EPLS authoring document as a bounded
`JELS` schema-1 container and transact exactly the owning World descriptor plus
that `.epls` file through the existing ProjectAsset coordinator.

The container uses the canonical chunk set `MANF/ELAY/EMSK/EPNT/EVOL`, fixed
little-endian records, checked ranges and independently authenticated chunks.
Canonical bytes reopen exactly and the catalog admits the `.epls` only when its
World owner, GUID, revision, path and content hash agree.

`Preview` and `Cancel` write nothing. `Apply`, `Undo` and `Redo` restore exact
World and `.epls` bytes through the shared transaction/history authority. A
change to the World, current `.epls`, referenced RegionMask or referenced
RBrush pack after Preview is rejected before publication.

## Authentic RED progression

The first build failed because the EPLS container and transaction headers did
not exist. Subsequent causal REDs exposed, in order:

1. colliding test-helper names in the integrated target;
2. duplicate catalog GUID fixtures;
3. an incorrectly resolved project-relative RBrush dependency;
4. an invalid third directory operation in the required two-file transaction;
5. a stale Automation Bridge scope during replacement Preview.

Each failure was corrected at its owning boundary. No alternate EPLS journal,
catalog, transaction system or proof-only path was introduced.

## Verification retained

- Prescribed Debug matrix:
  `editor.project_asset_publication_transaction`,
  `editor.project_transaction_coordinator`,
  `editor.epls_paint_transaction`, `epls.container` — `4/4 PASS`,
  `150.99 s`.
- Debug transaction neighbours including Surface Paint and Terrain —
  `6/6 PASS`, `25.06 s`.
- Release focal EPLS container + transaction — `2/2 PASS`, `2.26 s`.
- Full Debug EPLS matrix — `5/5 PASS`, `4.48 s`.
- Architecture, runtime-only discovery, Shipping source exclusion and editor
  boundary audit — `23/23 PASS`; the boundary audit rebuilt and exercised the
  Debug/Release runtime proof (`277.74 s`).
- Full integrated Debug build — `PASS`, approximately `98 s`.
- `git diff --check` and staged diff check — no whitespace errors.

## Scope and remaining gaps

This checkpoint does not claim the Task 5 Qt workbench, viewport evidence,
C04 job ownership, incremental cook, package, runtime consumer, MeshBlend,
source-free Shipping replay, Slice 25, M8 or Goal completion. `.epls`, RBrush
packs and strokes remain editor authoring data and never become Shipping input.

The next legal work is Task 5: a cancelable C04 owner and brush-first EPLS
surface inside the existing shared World/RBrush workspace.
