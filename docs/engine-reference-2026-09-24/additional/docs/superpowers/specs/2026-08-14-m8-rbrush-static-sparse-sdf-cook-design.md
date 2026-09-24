# M8 RBrush Static Sparse SDF Cook Design

**Status:** Accepted for implementation  
**Milestone:** M8  
**Slice:** RBrush 35 — Cook static SDF  
**Observable PASS:** An accepted Sparse SDF rock result is published as canonical
runtime geometry and collision, packaged incrementally, and replayed in
source-free Shipping without any RBrush dependency.

## 1. Scope

This slice closes the boundary between editor-only Sparse SDF authoring and the
runtime formats that already exist. It does not add another SDF implementation,
runtime authoring format, navigation bake, renderer feature, or top-level editor
surface.

The accepted source is one immutable `SdfAuthoringResult` owned by the admitted
`SparseSdf` World facet. The existing `cookSdfCandidate` path remains the only
geometry/collision cooker.

## 2. Decision

Static publication replaces exactly two stable runtime assets:

- one `.rmesh`, preserving its admitted mesh GUID;
- one `.rcoll`, preserving its admitted collision GUID.

The owning World descriptor is revised transactionally so its render dependency
closure reflects the new mesh content and its downstream procedural state is
honest. The existing `.rlevel` is not rewritten because it already references
the stable mesh GUID.

No `.rsdf`, `.rbrush`, ABR, stroke, preset, authoring mask, or source recipe is
mounted by Shipping.

## 3. Planning contract

`planSparseSdfStaticCook` receives pinned, admitted inputs:

- project session and asset catalog;
- automation bridge scope;
- Current World and exact `SparseSdf` facet;
- canonical SDF recipe and accepted `SdfAuthoringResult`;
- stable mesh, collision, and material GUIDs;
- exact dirty spatial region;
- document generation and cook profile identity;
- cancellation token.

The planner must reject stale World, facet, recipe, generation, catalog,
automation scope, asset identity, or dirty-region lineage before preparing any
write.

The dependency explanation retained by the plan is:

`Dirty Sparse SDF region intersects accepted geometry change`

The cook key is deterministic over the accepted source/recipe/content hashes,
stable output identities, material identity, toolchain/profile identity, and
runtime format version. Repeating the same admitted request must produce the
same key and byte-identical `.rmesh`/`.rcoll` outputs.

## 4. Dirty-region and downstream policy

The exact `replacementField.bounds()` is the authored geometry dirty region.
The plan preserves invalidation for geometry, collision, navigation, shadows,
GI, World cells, PWAS scatter, and EPLS consumers where applicable. It does not
claim those later products were cooked.

After publication:

- `SparseSdf` remains the accepted authored facet;
- `PwasRecipe` and `Navigation` are explicitly stale;
- the World cannot advertise those dependent products as current;
- no `.rnav` is emitted by this slice.

This is preferable to silently retaining an obsolete navigation asset or hiding
a full navigation bake inside a small SDF cook.

## 5. Transaction and incremental package

The prepared transaction contains exactly:

1. replace admitted `.rmesh` bytes;
2. replace admitted `.rcoll` bytes;
3. replace the owning World descriptor.

There are zero `.rlevel` operations. Apply, undo, and redo use the existing
project transaction coordinator and must restore all three paths atomically.
The candidate catalog and render dependency closure are validated before Apply.

`Package Changed` stages only the changed `.rmesh` and `.rcoll`. The base package
retains the Level and unchanged dependencies; the delta package overrides the
two stable GUIDs. Compacting base plus delta must be equivalent to a clean full
package of the same cooked state.

## 6. Threading and cancellation

The planner and cooker are Qt-free work. The editor must invoke them from a
worker job using immutable snapshots. The Qt/UI owner may coordinate progress
and perform only the short validated transaction commit.

Cancellation before publication produces no filesystem mutation. Completion is
accepted only if the World/facet/document generation still matches; late worker
callbacks are discarded. This slice does not claim broader cooker scheduler
completion.

## 7. Verification

The RED/GREEN product test must prove:

- one accepted SDF change produces one same-GUID `.rmesh` replacement and one
  same-GUID `.rcoll` replacement;
- the outputs differ from the admitted baseline and are deterministic;
- the transaction contains those two assets plus the World descriptor only;
- `.rlevel` bytes remain identical through Apply/Undo/Redo;
- World render closure changes and Navigation/PWAS become explicitly stale;
- the retained reason and dirty region identify why the assets cooked.

The Shipping proof must prove:

- delta inventory is exactly Mesh + Collision and contains no Level or source;
- base + delta is mounted and replayed twice through Vulkan;
- Jolt consumes the changed collision;
- compact equals clean full packaging;
- authoring roots and RBrush inputs are removed before replay;
- repeated captures and package bytes are deterministic.

## 8. Non-goals

- no runtime Sparse SDF field or RBrush parser;
- no full Level, navigation, PWAS, EPLS, shadow, or GI recook;
- no UI redesign;
- no new package, catalog, dependency graph, or transaction implementation;
- no claim that M8, RBrush, Jobs/Async, navigation, or JericoEngine 1.0 is
  complete when this slice passes.
