# M8 Geology Blueprint P2 Block-Grammar Source Research

**Date:** 2026-08-29
**Slice:** `M8-GEOLOGY-BLUEPRINT-P2-SPEEDROCK-LIKE-BLOCK-GRAMMAR-SOURCE`
**Selected boundary:** `TYPED_CPU_GRAMMAR_TO_EXISTING_VULKAN_SOURCE_FIELD`

## Question

What is the smallest independent Jerico source contract that retains the useful
coarse-mass idea from SpeedRock while preserving deterministic authoring,
bounded work, cancellation and the existing field/Surface Nets/cook/runtime
pipeline?

## Primary source and provenance

The primary source is Isaac M. Dart, Gabriele De Rossi and Julian Togelius,
[*SpeedRock: Procedural Rocks through Grammars and Evolution*](https://pcgworkshop.com/archive/dart2011speedrock.pdf),
PCGames/PCG Workshop 2011, DOI
[`10.1145/2000919.2000927`](https://doi.org/10.1145/2000919.2000927).

The official four-page PDF retrieved on 2026-08-29 has SHA-256:

```text
7B05B8B2C61D836061E67FCAF644F8C1FB515032D80791B5AC3A6B9B0B1A6592
```

No reference implementation or downloadable tool was inspected.

## Confirmed source facts

Sections 2 and 2.1 on PDF page 2 define one axiom and four rules. Each rule is
a `2x2x2` byte matrix. A cell names another rule, the current rule, or Rule 0.
Rule 0 is empty; every non-zero rule is solid during rendering. Every rewrite
subdivides each current cuboid along X, Y and Z into eight children. The paper
uses six rewrites.

Section 2.2 on page 2 applies implosion after rewrite. It moves occupied blocks
toward one center plane per axis when empty space lies between the block and the
plane, and calls that process deterministic.

Section 2.5 on page 3 wraps the block skeleton by projecting vertices of an
enclosing sphere toward its center, then adds small displacement and recomputes
normals. This is the paper's export mesh route, not a Jerico runtime contract.

The paper does not specify:

- numeric values beyond Rule 0, axiom dimensions or the initial rule;
- child byte order, axis handedness, origin or cell-to-world mapping;
- a reproducible seed, PRNG, stream split or traversal-independent randomness;
- synchronous versus in-place storage, although a parallel generation follows
  naturally from the described L-system rewrite;
- implosion axis/scan order, even-grid center convention, conflicts or work cap;
- a signed-distance sign, material/classification payload, collision, LOD,
  streaming or package contract;
- skinning resolution, ray-miss policy, noise bounds, topology or UV guarantees.

Those omissions are not filled by guessing reference behavior.

## Derived limits

For one root cell and six eight-way rewrites:

```text
terminal cells = 8^6 = 262144
cumulative child writes = 8 + 8^2 + ... + 8^6 = 299592
```

These are arithmetic consequences, not budgets stated by the paper. The four
rules contain 32 child slots total. Empty pruning may reduce occupied output but
cannot be relied on for admission.

The paper itself reports that early shapes leaned cubic or triangular, suggests
possible crystalline/man-made uses and reports no user study. It is evidence
for a coarse source, not a general realistic-rock guarantee.

## Current Jerico seams

Jerico already has one typed `Source` layer, canonical recipe hashing, stable
layer seed and transform, BSP source materialization, guarded Vulkan source-field
evaluation, classified Surface Nets, deterministic `.rmesh`/`.rcoll` cook and
source-free runtime/package consumers.

The source shader already computes the union of admitted convex brushes into a
negative-inside scalar field. A bounded grammar can therefore materialize its
terminal cuboids as the existing source geometry. No second field, mesh,
classification, cooker or runtime system is needed.

`ImplicitBlock` remains a P1 field operator over an existing source. Repeating it
as a synthetic block list would not create an alternate source and would consume
the recipe layer budget, so it is not the selected seam.

## Independent Jerico recommendation

Extend the existing `GeologySourceKind` with one fixed `BlockGrammar` source.
The source owns exactly one axiom, four rules of eight typed symbols, a bounded
rewrite depth, a local root domain and optional deterministic center compaction.
The layer header seed controls a traversal-independent child reflection; it does
not reproduce the paper's unspecified random population.

CPU authoring expands and validates the grammar, compacts occupancy when enabled,
coalesces occupied cells into deterministic disjoint cuboids and publishes only a
complete result. Those cuboids enter the existing guarded Vulkan source field,
then the existing Surface Nets, classification, cook, collision, package and
runtime consumers.

The paper's evolution/search, stochastic neighborhood erosion, spherical
skinning, Unity tool, GUI and OBJ export are discarded. Later P2 features,
runtime destruction/remesh/save-state and a general grammar framework remain
outside this slice.
