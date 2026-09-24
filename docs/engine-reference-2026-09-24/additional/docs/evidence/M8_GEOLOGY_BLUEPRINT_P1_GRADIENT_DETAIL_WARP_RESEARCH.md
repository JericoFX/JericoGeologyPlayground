# M8 P1 gradient-based implicit detail warp research mapping

## Scope and provenance

This note supports only
`M8-GEOLOGY-BLUEPRINT-P1-GRADIENT-BASED-IMPLICIT-DETAIL-WARP`, the P1 item
immediately after the published `FoldDomain` and geological-material verticals.
It records the research used to design an independent Jerico implementation
before production code is changed.

Primary sources inspected on 2026-08-28:

- Axel Paris's official project page for *Modeling Rocky Scenery using Implicit
  Blocks*, including the author-linked preprint and code links:
  <https://aparis69.github.io/public_html/projects/paris2020_Blocks.html>.
- the version of record, DOI `10.1007/s00371-020-01905-6`, published in *The
  Visual Computer* 36 (2020), pages 2251-2261:
  <https://link.springer.com/article/10.1007/s00371-020-01905-6>.
- the official companion repository at exact inspected commit
  `b91965b3011ed269cbc3a051b00c9b284aaa2e36`:
  <https://github.com/aparis69/Rock-fracturing>.
- the repository's MIT license, copyright 2019 Axel Paris:
  <https://github.com/aparis69/Rock-fracturing/blob/b91965b3011ed269cbc3a051b00c9b284aaa2e36/LICENSE>.
- Axel Paris's later technical note *Adding Details to Implicit Surfaces*,
  published 2024-03-02 and updated 2025-01-30:
  <https://aparis69.github.io/public_html/posts/2024_implicit_details.html>.

The repository README explicitly says its companion program was recoded and is
not the implementation that produced the paper scenes; results and timings may
differ, and several paper capabilities are absent. It is therefore evidence of
the problem and one possible experiment, not a normative implementation.

The paper/preprint and repository were studied read-only. No source, shader,
data structure, constant, API, asset, preset or scene was copied, translated or
ported into Jerico.

## Published idea, separated from implementation

The paper first keeps the block field differentiable through smooth implicit
composition. It derives a normalized local surface gradient, uses that gradient
to weight the three orthogonal projections of a scalar detail function, and
displaces the sampling domain along the gradient. The paper demonstrates this
as a way to add small implicit rock detail without the obvious cavities produced
by uncontrolled deformation.

The later author note makes the risk more explicit: a domain deformation along
the normal can become discontinuous where normals are discontinuous, can create
holes, and generally destroys exact Euclidean-distance/Lipschitz properties.
Those observations are requirements for Jerico's admission and topology tests;
they are not permission to import the reference implementation.

The inspected companion program illustrates additional non-portable choices:
fixed finite-difference spacing, fixed displacement strength, a global texture,
fixed smooth-operation widths and its own coordinate/sign conventions. Those
values and structures are discarded. Jerico derives scale from its admitted
voxel contract, uses explicit world-unit recipe parameters, preserves its
negative-inside sign convention and remains inside its resident Vulkan field
session.

## Existing Jerico ownership audit

Jerico already owns all adjacent responsibilities:

- `GeologyRecipe` owns typed layer identity, schema migration, canonical text,
  masks, strength, seed, transforms and deterministic hashing.
- `GeologyGpuSession` owns the resident scalar ping-pong field, packed geological
  classification, workload/device admission, cancellation and dirty tracking.
- `GeologyValueNoiseReference` and the matching shader routines own the admitted
  seeded fractal-noise semantics used by current CPU/GPU parity tests.
- `GeologyGpuTopologyPlan` owns conservative layer expansion.
- classified Surface Nets owns the only geological mesh-extraction path.
- `GeologyCook`, JRMS 1.1 `GEOL`, exact collision, Asset Registry loading and the
  canonical Diligent triplanar PBR path own downstream productive integration.

`DirectionalWarp` offsets the source domain with a directional stochastic
vector. `Noise` performs scalar field displacement. Neither computes the source
gradient, builds local triplanar weights and performs a normal-directed domain
sample with a topology-preserving contract. Extending either with unrelated
semantics would make its interface shallower. A new typed recipe operator inside
the same evaluator is the smallest coherent extension; no parallel field,
mesher, renderer, material system or cooker is needed.

## Independent Jerico conclusions

The implementation must freeze the following contracts rather than inherit
reference-code accidents:

- field sign is negative inside;
- gradient spacing is exactly one admitted voxel in world metres;
- detail displacement and the surface band are world metres;
- detail frequency is cycles per locally transformed authored metre;
- the existing layer transform defines the local triplanar frame;
- finite-difference zero gradients produce exact identity;
- disabled, zero-strength, zero-mask and zero-displacement paths produce exact
  identity;
- the highest transformed octave remains below a conservative voxel Nyquist
  limit;
- displacement remains a bounded fraction of the surface band;
- the field result can add bounded exterior relief but can never turn existing
  solid into empty space;
- source classification passes through unchanged because this operator creates
  no new geological class;
- canonical recipe identity includes every control, seed, transform and mask;
- CPU and productive Vulkan paths use the same equations and exact constants.

The monotonic occupancy rule is a deliberate Jerico safety design, not a claim
made by the paper. For source field `s` and normal-warped candidate `w`, the
negative-inside output is `min(s, w)`. Consequently every point that was solid in
`s` remains solid in the result. A bounded surface window and inward sample
direction then restrict added detail to a connected neighbourhood of the source
surface.

## Risks that require observable gates

- A finite difference over a non-distance or discontinuous field can produce an
  unstable or zero normal. Zero-gradient identity and bounded amplitude are
  mandatory.
- A frequency valid in local coordinates can alias after non-uniform scale. The
  admission test must use the smallest physical transform scale and the highest
  octave.
- Triplanar relief multiplies noise and field reads. Octaves, field extent and
  one fixed seven-sample field stencil must remain bounded by the existing
  admitted voxel workload.
- Domain warping invalidates exact SDF-distance assumptions. Surface Nets may use
  the zero set and local gradients, but no consumer may silently treat the output
  value as an exact travel distance.
- Occupancy preservation alone does not prove a closed, recognizable extracted
  mesh. The RED/green contract must test monotonic samples, absence of enclosed
  empty pockets, closed Surface Nets edge incidence, non-zero small-scale detail
  and retained block proportions.
- CPU-only or shader-source proof is insufficient. Productive Vulkan parity,
  classified cook/collision/material compatibility and runtime Beauty evidence
  are required.

## Explicit boundary

This mapping does not admit `DirectedCrack`, later P1/P2/M9/Task23 work,
durability, destruction, rockfall, colluvium, imported structural meshes,
OpenVDB, geology UI, named generators/presets or bespoke proof geometry. It does
not start
`docs/JERICO_1_0_PRODUCTION_EXPANSION_GEOLOGY_PWAS_EPLS_ANIMATION_DESTRUCTION.md`.

Completing gradient-based implicit detail warp does not complete P1, the geology
blueprint or JericoEngine 1.0.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
