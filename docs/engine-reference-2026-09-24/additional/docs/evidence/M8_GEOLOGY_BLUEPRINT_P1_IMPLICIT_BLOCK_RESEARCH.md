# M8 P1 ImplicitBlock research mapping

## Scope and provenance

This note supports only
`M8-GEOLOGY-BLUEPRINT-P1-IMPLICIT-BLOCK-ARCHETYPES`, the first P1 item in
blueprint section 88.

Primary references inspected on 2026-08-27:

- Axel Paris, Adrien Peytavie, Eric Guerin and Eric Galin, *A framework for
  rock modeling using a vector-based representation*, The Visual Computer
  (2020), DOI `10.1007/s00371-020-01905-6`;
- the authors' project page,
  <https://aparis69.github.io/public_html/projects/paris2020_Blocks.html>;
- the official companion repository,
  <https://github.com/aparis69/Rock-fracturing>, inspected at commit
  `b91965b3011ed269cbc3a051b00c9b284aaa2e36`;
- the companion repository's MIT license, copyright Axel Paris 2019.

The upstream is a research reference, not a donor. No upstream source,
shader, data structure, API, fixed constant or preset is copied, translated or
ported into Jerico. The Jerico implementation uses its existing canonical
recipe, resident Diligent/Vulkan scalar field, bounded inspection, dirty
tracking and Surface Nets contracts.

## Problem understanding retained

The paper organizes rocks as bounded implicit blocks whose negative-inside
scalar is the intersection of oriented half-spaces. It distinguishes broad
structural families: tabular blocks dominated by one short dimension,
equant/equidimensional blocks, oblique rhombohedral blocks and irregular
polyhedral blocks. Its broader pipeline constructs fracture graphs and can
replicate blocks over terrain; those are separate problems.

The companion implementation exposes Equidimensional, Tabular, Rhombohedral
and Polyhedral cases. It does not implement a columnar/prismatic archetype; the
paper identifies that family as future work. Its random state, constants and
geometry construction are research-program choices and are not Jerico
contracts.

## Independent Jerico mapping

Jerico admits one canonical `ImplicitBlock` recipe layer with five archetypes:

- `Tabular`: a bounded box-like scalar with one canonically short local axis;
- `Columnar`: an independently designed regular convex prism with a bounded
  authored side count and one canonically long local axis;
- `Equidimensional`: a bounded box-like scalar with constrained aspect ratio;
- `Rhombohedral`: a bounded convex scalar formed by three paired, mutually
  oblique local planes;
- `IrregularPolyhedral`: a bounded convex scalar formed from paired axial and
  fixed oblique planes, with deterministic plane offsets derived from the
  canonical layer seed.

All definitions use metres, negative-inside sign, the existing layer strength
and regional-mask composition, and the existing full translation/quaternion/
non-uniform-scale transform. The scalar intersects the upstream resident field;
it does not generate bespoke geometry. Surface Nets remains the sole mesh
extraction path and preserves the canonical document material GUID.

Only `IrregularPolyhedral` consumes a non-zero seed. Inactive parameters must
remain at their canonical neutral values, so serialization and identity cannot
encode ambiguous state. Validation bounds aspect ratios, smoothing, prism side
count, obliqueness and irregularity before the GPU path sees them.

## Explicit boundary

This mapping does not admit `JointSet`, fracture-family distributions,
`FoldDomain`, gradient-based detail warp, `DirectedCrack`, durability, rockfall,
colluvium, terrain replication, named cliff/rock presets, OpenVDB or editor UI.
Completing these five implicit block archetypes does not complete P1, the
geology blueprint or JericoEngine 1.0.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
