# M8 RBrush Rock Generator Brush Sets — Design

**Date:** 2026-08-14

**Status:** accepted under the standing autonomous-decision directive

**Milestone:** M8

**Slice:** 32 — Rock Generator Brush Sets

## 1. Observable result

A Rock Generator recipe can resolve RBrush presets by canonical tags, combine
those results with explicit weighted overrides, and emit a bounded sequence of
deterministic crack or chip brush choices. Given the same admitted catalog,
brush-set data, seed and count, the exact selected preset, scale, depth,
rotation and SDF profile are identical regardless of catalog input order.

The result is backend-ready authoring data: every emitted choice explicitly
names `CrackCutter` or `ChipCutter`, but this slice does not generate a rock,
choose world positions, add UI, cook the result or introduce runtime RBrush
loading.

## 2. Reused authority

No second library, scanner or tag implementation is introduced:

- `ProjectRBrushAsset` remains the admitted immutable project snapshot.
- `queryProjectRBrushLibrary` remains the only tag-query implementation and
  already owns case-folding, `require`/`any`/`exclude` semantics, catalog
  validation and bounded results.
- `RBrushPresetIR::sdfDefaults` remains the canonical compatibility and SDF
  profile authority.
- `RBrushPresetReference` remains the stable `(AssetGUID, PresetGUID)` identity.
- Slice 28 `SdfBrushOperation` remains the downstream crack/chip operation
  vocabulary.

The in-memory project catalog projection gains the existing `sdfDefaults`
metadata. This is a projection of canonical IR, not a new format or catalog.
It lets query resolution avoid reopening every `.rbrush` pack merely to learn
whether a candidate is SDF-compatible.

## 3. Ownership decision

Add one Qt-free `RBrushRockGeneratorBrushSet` resolver to
`JericoEditorProjectAuthority`. The editor authority is the correct seam because
RBrush is editor-only authoring input and the resolver consumes the existing
project catalog snapshot. The returned recipe may later be consumed by PWAS and
the Rock Generator without moving `.rbrush` into runtime or Shipping.

Rejected alternatives:

1. A Rock Generator directory scan would duplicate admission, identity and
   query policy and make results depend on filesystem order.
2. Embedding the resolver in a future UI panel would hide reusable deterministic
   policy behind widgets and prevent headless tests.
3. Adding weighted-set data to `.rbrush` would confuse reusable brush assets
   with a generator recipe; sets belong to the authoring recipe that uses them.

## 4. Public data contract

The resolver exposes the authority vocabulary directly:

```cpp
struct RBrushTagQuery
{
    std::vector<std::string> require;
    std::vector<std::string> any;
    std::vector<std::string> exclude;
};

struct RBrushFloatRange
{
    float minimum{};
    float maximum{};
};

struct WeightedRBrushReference
{
    RBrushPresetReference brush;
    float weight{1.0F};
    RBrushFloatRange scale{1.0F, 1.0F};
    RBrushFloatRange depth;
    RBrushFloatRange rotationRadians;
    float threshold{0.5F};
    float gamma{1.0F};
    bool invert{};
};

struct RBrushSet
{
    std::vector<WeightedRBrushReference> entries;
    RBrushTagQuery query;
    std::uint64_t seed{};
};
```

One resolve request also supplies `Crack` or `Chip` and a bounded count. Each
resolved choice contains the stable brush reference, pack content hash,
canonical tip hash, sampled parameters, an operation-specific seed and the
matching `SdfBrushOperation`.

## 5. Query and override policy

The resolver maps:

- `require` to `RBrushLibraryQuery::requiredTags` (all must match);
- `any` to `optionalTags` (at least one must match when non-empty);
- `exclude` to `excludedTags` (none may match).

Query results are flattened and sorted by `AssetGUID`, then `PresetGUID` before
any weighted selection. Library group/name/path ordering therefore cannot alter
the recipe.

Every SDF-compatible query result becomes an implicit entry with:

- weight `1`;
- scale `1..1`;
- depth equal to the preset's canonical default depth;
- rotation `0..2*pi`;
- threshold, gamma and invert from canonical SDF defaults.

Explicit entries are manual authority. They may name a compatible preset even
when it does not match the dynamic query, and replace the implicit entry with
the same identity. Duplicate explicit identities are rejected rather than
silently resolved. A query that matches no candidate is valid only when at
least one explicit entry remains.

## 6. Validation and limits

Resolution validates the complete request before producing output:

- no more than 4,096 explicit entries or emitted choices;
- non-zero seed and non-zero count;
- canonical, conflict-free bounded tag filters;
- unique explicit `(AssetGUID, PresetGUID)` identities;
- every explicit reference resolves through the admitted catalog;
- every candidate is SDF-compatible and has a non-zero canonical tip hash;
- finite positive weights and a finite positive aggregate weight;
- finite ordered ranges; scale and depth are strictly positive;
- threshold is in `[0,1]` and gamma is positive;
- no ambiguous or empty final candidate set.

Invalid input returns one explicit editor error and no partial recipe.

## 7. Deterministic sampling

The resolver never uses `std::uniform_*`, global state or library iteration
order. It hashes a fixed domain, set seed, choice index and candidate identity
through a Jerico-owned integer mixer. Weighted selection uses an integer sample
against a fixed-point cumulative weight table. Scale, depth and rotation use
separate domains and a 24-bit fraction, then quantize the result to `1/4096`.

The recipe hash includes normalized query data, explicit configuration, sorted
candidate identities, pack content hashes, canonical tip hashes, SDF defaults,
feature kind, count, seed and every emitted choice. Changing a referenced pack
therefore changes the recipe identity even if GUIDs remain stable.

## 8. RED/GREEN proof

The focal product test constructs two admitted packs in deliberately different
orders and proves:

1. `require={sdf}`, `any={crack,chip}`, `exclude={large}` uses the canonical
   library semantics;
2. explicit weight/range/profile data overrides one dynamic candidate;
3. a fixed crack seed emits the exact same choices and recipe hash for both
   catalog orders and every choice carries `CrackCutter`;
4. a fixed chip seed emits deterministic but different choices and every choice
   carries `ChipCutter`;
5. changing seed or admitted pack content changes the appropriate recipe;
6. invalid weights, ranges, duplicate refs, incompatible presets, conflicts,
   missing refs and oversize counts fail without output;
7. the resolved choices can feed the existing SDF operation vocabulary without
   translation or a parallel rock-only brush type.

The focal CTest is `editor.rbrush_rock_generator_brush_sets`. Compilation or a
UI list alone is not PASS.

## 9. Completion boundary

Slice 32 is PASS only after the authentic RED becomes GREEN, tag/library and SDF
regressions remain GREEN, deterministic exact metrics are retained, and all
slice files are committed and published.

This slice does not claim Rock Generator geometry, world distribution, UI,
Procedural 3D Layout, cook/package, source-free Shipping, M8 or the master Goal.
