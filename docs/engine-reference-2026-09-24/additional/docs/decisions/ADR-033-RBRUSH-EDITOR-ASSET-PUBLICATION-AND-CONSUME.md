# ADR-033: RBrush editor-asset publication and source consume

## Status

Accepted on 2026-08-11 as the narrow reconciliation required between the
owner-approved RBrush specification, ADR-031 and ADR-032.

## Context

The RBrush specification requires two logical roots:

- `Import/Brushes/Inbox` for external inputs that may be consumed; and
- `EditorAssets/Brushes` for canonical `.rbrush` authoring assets.

It also requires a source to be deleted only after the published RBrush has
been reopened through the normal reader and Asset Registry path.

ADR-031 currently closes project transaction roots and catalog inventory to
`Assets`, `Procedural` and `Presets`. ADR-032 keeps `SourceAssets` immutable and
requires imports to publish through the one project catalog and canonical
transaction coordinator. Mapping RBrush into `Assets` would mix editor-only
authoring data with cooked runtime assets. Mapping it into `Presets/PWAS` would
misclassify the asset and weaken that root's closed codec. A standalone brush
registry or atomic writer would duplicate the admitted catalog and transaction
system.

## Decision

1. Add the fixed project-contained `EditorAssets` transaction root. Its first
   admitted family is `EditorAssets/Brushes/**/*.rbrush`. It is editor-only,
   non-reparse, catalogued by the existing `EditorProjectAssetCatalog` and
   excluded from runtime mounts, cook inputs unless explicitly resolved by an
   authoring cooker, package and Shipping.
2. Add `RBrush` to the existing `ProjectAsset` command kind and add an
   `EditorAsset` transaction target kind. RBrush publication uses the existing
   `ProjectTransactionCoordinator`; it does not create a second journal,
   registry, writer lock or recovery path.
3. A new RBrush pack owns its final `AssetGUID` before Preview. The absent
   baseline is represented by expected revision/content hash `0/0`; the
   admitted pack has catalog revision `1` and a non-zero canonical content
   hash. RBrush updates may keep catalog revision `1` and are distinguished by
   exact content hash until an authority-approved RBrush revision field exists.
   Slice 12 admits creation only. Slice 14 owns replacement/update semantics.
4. The coordinator's physical candidate under `.JericoTransactions` fulfills
   the RBrush specification's logical `<name>.rbrush.tmp` role. No adjacent
   arbitrary `.tmp` file is created. Candidate bytes are flushed, reread with
   `readRBrushContainer`, and validated before publication. The final path is a
   guarded no-replace create in Slice 12.
5. `EditorProjectAssetCatalog` discovers `.rbrush` files only below the exact
   `EditorAssets/Brushes` root. Every file is bounded, structurally decoded by
   the normal RBrush reader, validated, and admitted by unique case-folded path
   and `AssetGUID`. The one catalog inventory hash includes this root. Unknown
   files, links, invalid packs, GUID collisions and path collisions fail closed.
6. `Import/Brushes/Inbox` is a dedicated external intake boundary, not project
   authoring authority and not a `SourceAssets` mount. `SourceAssets` remains
   immutable. `EmbedAndConsume` is legal only for a regular file reached through
   the exact project-contained inbox with no traversal, link, alias or alternate
   stream. ABR always embeds; existing project assets are never consumed.
7. Heavy parse/decode/map/compression prepares an owned, bounded immutable
   source snapshot and canonical RBrush candidate outside the Qt thread. The
   candidate manifest must bind the exact source hash and retention policy.
   Publication operates on those owned bytes; it does not read mutable source
   data during the coordinator exchange.
8. Apply proceeds in two ordered phases:

   1. validate source identity/hash and candidate;
   2. prepare and apply the canonical project transaction;
   3. receive the admitted replacement catalog;
   4. find the exact RBrush by GUID/path/content hash in that catalog;
   5. reopen the final file through the normal reader and compare canonical IR;
   6. only for `EmbedAndConsume`, reacquire the inbox leaf, require the original
      persistent identity, length and SHA-256, remove that exact regular file,
      and flush the parent directory.

9. Any failure before catalog admission leaves both source and prior project
   authority unchanged. A post-admission source-consume failure keeps the
   already valid RBrush and reports `PublishedSourceRetained`; it never rolls
   back or corrupts the sole canonical result. Cancellation before admission
   publishes nothing. Cancellation after admission skips consume and retains the
   source.
10. Successful `EmbedAndConsume` is an explicit ingestion commit point. Generic
    Undo is not exposed after the source has been intentionally consumed because
    deleting the sole canonical RBrush would lose the imported content. Before
    consume, Cancel remains exact. A later dependency-aware RBrush Delete command
    is separate from import/update and may provide its own transaction history.
11. Slice 12 does not add UI, update/reimport, duplicate detection, runtime brush
    loading or painter behavior. It exposes a Qt-free service suitable for the
    existing C04 job owner. UI responsiveness, progress, callback fencing and
    shutdown remain mandatory when the modal/workspace owner is introduced.
12. Shipping validation rejects `Import/Brushes`, `EditorAssets/Brushes`,
    `.abr` and `.rbrush`. Static consumers must bake their admitted derivatives;
    runtime-dynamic brush data uses its future cooked boundary, never the editor
    pack.

## Acceptance

- an owned valid pack publishes through the canonical coordinator, is present in
  the one replacement catalog, and reopens byte/IR-exactly before the inbox
  source disappears;
- injected failures at candidate validation, prepare, publication, catalog
  admission and final reopen leave the source untouched and never expose a
  partial `.rbrush`;
- changed identity, bytes, size, link/alias or path after snapshot prevents
  consume and retains the source;
- cancellation before admission publishes nothing; cancellation after admission
  reports a valid published pack with retained source;
- an invalid or colliding `.rbrush` makes catalog admission fail closed;
- transaction recovery and existing ProjectAsset tests remain green;
- runtime-only/Shipping builds contain no RBrush importer, editor catalog parser,
  `.abr`, `.rbrush`, `Import/Brushes` or `EditorAssets/Brushes` dependency.

## Consequences

- Jerico gains the first editor-only asset root without weakening cooked
  `Assets`, procedural recipes or PWAS presets.
- RBrush reuses the expensive transaction, recovery and catalog work already
  proven by M7G.
- Source consumption is safe and explicit, but intentionally not a reversible
  delete after acceptance.
- Future editor-only asset families require their own ADR and closed codec; this
  decision does not make `EditorAssets` a generic loose-file root.
