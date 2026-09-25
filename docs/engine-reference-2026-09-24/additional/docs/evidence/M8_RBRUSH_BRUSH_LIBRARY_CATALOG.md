# M8 RBrush Brush Library Catalog Evidence

Date: `2026-08-12`

Capability state: `PASS`

Slice state: RBrush Slice 15 Brush Library `PARTIAL`

Higher state: M8 `ACTIVE / PARTIAL`; C04 Jobs/Async `PARTIAL`;
JericoEngine 1.0 `ACTIVE / NOT COMPLETE`.

## Scope demonstrated

This checkpoint proves the first canonical catalog capability of RBrush Slice
15. Admitted project `.rbrush` packs project bounded browsing metadata during
the existing strict anchored catalog decode. A Qt-free service then searches,
filters and navigates those immutable rows without reopening `.rbrush` files or
consulting the original `.abr`/image source.

The product exposes the stable selection pair
`RBrushAssetGUID + PresetGUID`, deterministic pack navigation, group-subtree
filters, required/optional/excluded tags, source-type filters and truthful
source states. A consumed external source remains healthy
`ConsumedExternal`; a canonical preset retained by Slice 14 after disappearing
from an updated source is `MissingFromSource`.

This checkpoint does not implement preview pixels, engine-global library
discovery, Qt widgets, Painter, World paint records, pack management, RBrush
runtime loading, Terrain, SDF, PWAS/EPLS 2.0, particles, vegetation or
Animation/Rigging.

## Architecture and reused authority

- `RBrushProjectCatalog` remains the only admitted filesystem/catalog
  authority. It already reads each canonical container once through the
  anchored reader and validates `RBrushPackIR`.
- `ProjectRBrushAsset` now carries small `ProjectRBrushPresetCatalogEntry`
  rows: identity, name/group/tags, primary-tip identity/format/dimensions,
  spacing and optional source provenance. It never retains tip pixels or full
  dynamics.
- `summarizeProjectRBrushCatalog()` projects source and preset summaries in one
  validated pass. Normal discovery and authenticated Create/Replace candidate
  construction consume the same projection.
- `RBrushLibrary` owns no path and performs no filesystem mutation or I/O. It
  consumes immutable catalog snapshots and returns value views.
- Selection resolution ignores query/order through the exact pair
  `RBrushAssetGUID + PresetGUID`.
- No dependency, database, sidecar index, registry, transaction authority,
  schema or cooked/runtime format was added.

## Product commits

- `76f108f docs(rbrush): define brush library catalog`
- `36f028f docs(rbrush): plan brush library catalog`
- `daf9838 feat(editor): project rbrush preset catalog metadata`
- `1b211d9 feat(editor): query canonical rbrush library`
- `9a3e4ab test(editor): verify source-gone rbrush browsing`

## Authentic RED evidence

1. Catalog projection RED: the Debug focal build failed in
   `RBrushProjectCatalogTests.cpp` because `ProjectRBrushAsset::presets` and
   `summarizeProjectRBrushPresets` did not exist.
2. Library RED: the Debug focal build failed with `C1083` because
   `Jerico/Editor/RBrushLibrary.hpp` did not exist.
3. Provenance integrity RED: a fabricated preset summary whose logical source
   contradicted the source manifest was accepted. The focal test failed at the
   explicit `!queryProjectRBrushLibrary(...).hasValue()` assertion. The GREEN
   validates unique source hashes/names, exact preset-to-source ownership and
   full manifest/preset coverage before querying.

## Observable behavior

| Behavior | Evidence |
|---|---|
| Catalog projection | Real `.rbrush` reopen returns preset GUID, name, group, sorted tags, primary-tip GUID/format/width/height, spacing and source provenance. |
| Transaction candidate parity | Apply's `ProjectRBrushAsset` contains the same summary before a process restart; the subsequent project reopen compares equal. |
| Original source gone | `EmbedAndConsume` removes the inbox `.abr`; a fresh catalog query still finds and resolves the preset. |
| Deterministic navigation | Reversing the input asset vector produces an exactly equal `RBrushLibraryView`; output sorts pack/group/name/GUID. |
| Search | Every whitespace token can match pack, preset, group, tag, logical source, source preset or source internal ID. |
| Group/tags | Exact-or-descendant group matching; required tags are AND, optional tags OR, excluded tags reject. |
| Source filters | Imported ABR/image/generated type and five truthful source states are filterable. |
| Stable reference | A surviving preset resolves before and after query changes, Apply, Update and catalog reopen through the same GUID pair. |
| Missing source preset | Update retains the removed preset GUID and the library emits exactly one `MissingFromSource` row. |
| Fail closed | Zero/duplicate identities, case-folded path collisions, duplicate preset GUIDs, conflicting tags, contradictory provenance and query/catalog limits reject with typed errors. |

ASCII search/filter comparison is locale-independent and case-insensitive;
non-ASCII UTF-8 bytes remain exact. This limitation is explicit and no Unicode
dependency was introduced.

## Fresh verification

### Focal RED/GREEN

- Projection GREEN: `editor.rbrush_project_catalog` plus
  `editor.rbrush_import_transaction`: `2/2 PASS`, `4.29 s`.
- Library focal ran three consecutive times after GREEN: `1/1 PASS` each
  (`0.98 s`, `0.13 s`, `0.13 s`).
- Provenance-integrity RED failed in `0.16 s`; corrected GREEN ran three
  consecutive times: `1/1 PASS` (`0.17 s`, `0.13 s`, `0.13 s`).
- Real source-gone/update transaction: `1/1 PASS`, `4.74 s`.

### Debug and Release integration

Exact matrix:

```text
editor.project_transaction_coordinator
editor.rbrush_import_transaction
editor.rbrush_project_catalog
```

- Debug: `3/3 PASS`, `18.66 s`.
- Release: `3/3 PASS`, `14.09 s`.

### Repository, Shipping and runtime-only

Release gates: `4/4 PASS`, `362.94 s`.

- scheduling contract: `0.06 s`;
- Shipping source exclusion: `2.04 s`;
- runtime-only target discovery: `38.47 s`;
- editor boundary audit: `323.45 s`.

`windows-vulkan-runtime` configured successfully in `28.3 s` and its complete
Release build passed in `64.9 s` with `/m:2 /nr:false`.

Two explicit non-dependency scans produced:

```text
runtime_generated_config_matches=0
runtime_binary_matches=0
```

The patterns were:

```text
RBrushLibrary
queryProjectRBrushLibrary
ProjectRBrushPresetCatalogEntry
.abr
.rbrush
EditorAssets/Brushes
Import/Brushes
```

The binary scan covered all non-dependency runtime `.exe/.dll/.lib`; the
configuration scan covered generated project/filter/props/targets/CMake files.

## Isolation and C04

`git diff --check` passed. All pre-existing user changes remain unstaged and
outside product/checkpoint commits: twelve tracked paths, the attachment
directory, the JericoBSP JPG and fourteen conceptual images.

The existing catalog decode may be heavy for large packs and remains subject to
the future C04 Interactive owner. The query is immutable and Qt-free, but this
checkpoint does not claim editor responsiveness, cancellation or FPS.

## Acceptance

The canonical project Brush Library catalog capability is `PASS`: admitted
packs remain searchable and navigable after original sources are gone, with
stable references and source-free Shipping proof.

RBrush Slice 15 remains `PARTIAL`. Its next legal capability is admitted,
on-demand preset preview derived from the canonical `.rbrush`, with identity and
content-hash revalidation and no per-query pack decode. M8, C04 and the master
JericoEngine 1.0 Goal remain active.
