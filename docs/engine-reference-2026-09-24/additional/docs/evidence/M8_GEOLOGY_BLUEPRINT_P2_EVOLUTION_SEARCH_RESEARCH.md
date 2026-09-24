# M8 Geology Blueprint P2 — Evolution/Search Research

**Date:** 2026-08-29
**Slice:** `M8-GEOLOGY-BLUEPRINT-P2-EVOLUTION-SEARCH`
**Selected boundary:** `BOUNDED_DETERMINISTIC_RECIPE_MUTATION_SEARCH`

## Question

What is the smallest deterministic and bounded Jerico search adjunct that can
suggest useful variants of the admitted schema-21 block-grammar source while
preserving canonical recipe identity, cancellation, hard budgets, explicit
designer choice and the existing field/mesh/cook/runtime lineage?

## Primary source and provenance

The primary source is Isaac M. Dart, Gabriele De Rossi and Julian Togelius,
[*SpeedRock: Procedural Rocks through Grammars and Evolution*](https://pcgworkshop.com/archive/dart2011speedrock.pdf),
PCGames 2011, DOI
[`10.1145/2000919.2000927`](https://doi.org/10.1145/2000919.2000927).
The publisher record identifies the paper as pages 8:1-8:4 of the second
International Workshop on Procedural Content Generation in Games. The official
workshop program also lists the paper and authors:
[`PCGames 2011 program`](https://pcgames.fdg2011.org/program.html).

The official PDF was already pinned by the closed block-grammar slice with
SHA-256:

```text
7B05B8B2C61D836061E67FCAF644F8C1FB515032D80791B5AC3A6B9B0B1A6592
```

No reference implementation, Unity project, downloadable tool or donor source
was inspected.

## Confirmed paper facts

The representation is an indirect 3D L-system chromosome containing one axiom
and four `2x2x2` byte-matrix rules. Rule zero is empty. Non-zero cells reference
rules and produce the block skeleton after six expansions
([SpeedRock section 2 and 2.1, PDF page 2](https://pcgworkshop.com/archive/dart2011speedrock.pdf)).

The user's formal target is a desired `x:y:z` dimensional ratio. The paper
defines fitness as `1 - d`, where `d` is the normalized mean difference between
desired and produced ratios. An earlier target-mesh comparison was discarded
because it constrained creativity and required a target asset
([SpeedRock section 2.3, PDF page 3](https://pcgworkshop.com/archive/dart2011speedrock.pdf)).

The reported evolutionary configuration uses a population of 200, rank-based
roulette selection, random one-point crossover and a 0.5 percent per-rule
mutation rate. A mutation selects one rule cell: empty becomes a random rule and
non-empty becomes empty. Initial chromosomes are random
([SpeedRock section 2.4, PDF page 3](https://pcgworkshop.com/archive/dart2011speedrock.pdf)).

The experiments graph best fitness over 25 generations. Operational search is
described as stopping after no progress over the last few generations, commonly
around ten. The paper does not specify a hard evaluation, time, memory or work
budget for production use
([SpeedRock section 3, PDF page 4](https://pcgworkshop.com/archive/dart2011speedrock.pdf)).

Implosion is described as deterministic, but initialization, crossover,
mutation, optional erosion and final displacement are stochastic. The paper
does not publish a seed, PRNG, domain separation, stream split, replay contract
or cross-run reproducibility guarantee.

The Unity tool lets an artist provide parameters and inspect the fittest
results. The authors explicitly report no user tests. The paper therefore
demonstrates a prototype search over an indirect representation, not artist
acceptance or a production asset/runtime pipeline.

## What the paper does not establish

- Ratio fitness does not establish geological plausibility, useful topology,
  material semantics, collision or physical behavior.
- The paper does not prove deterministic candidate identity or reproducible
  search order.
- It does not define cancellation, overflow handling, bounded allocation,
  package lineage, LOD, streaming, source-free runtime or save compatibility.
- It does not show that the evaluated chromosome identity remains equivalent to
  the post-erosion, skinned and displaced mesh identity.
- It does not compare against a reproducible baseline with confidence intervals
  or provide a user study.

These omissions cannot be filled by guessing reference behavior.

## Current Jerico facts

Jerico already owns the complete candidate representation needed by this slice:

```text
schema-21 GeologyStackRecipe
-> first and only typed BlockGrammar Source
-> four fixed rules x eight typed symbols + layer seed
-> canonical sourceContentHash + recipeHash
-> bounded block evaluator
-> existing Vulkan field/classification
-> classified Surface Nets
-> JRMS/JRCL cook and source-free package/runtime
```

The admitted block evaluator already has fixed depth/cell/box/work ceilings,
deterministic seed reflection, cancellation and no partial publication. Generic
Jerico authoring sessions establish the applicable preview pattern: immutable
proposal, before/after identities, explicit diff, stale validation and separate
designer Apply. No dedicated geology search UI or second recipe/session system
is required for this slice.

## Independent Jerico conclusion

Do not reproduce the paper's population 200, roulette, crossover or unseeded
randomness. Add one synchronous CPU authoring function implementing a small
deterministic `(1+lambda)` search over the existing typed grammar recipe.

Freeze every recipe field except the 32 rule symbols and source layer seed.
Evaluate a fixed canonical occupied-span ratio objective, rank candidates by an
integer error and canonical recipe hash, stop by explicit generation or
stagnation limits, and enforce hard evaluation/work/retention ceilings. Return
only bounded candidate previews with diffs and identities. The designer chooses
one preview; a pure stale-checked materializer then produces the existing
schema-21 recipe consumed by the admitted geology vertical.

This is an independent Jerico design. It is not a general optimizer, grammar
framework, runtime evolutionary system or port of SpeedRock.
