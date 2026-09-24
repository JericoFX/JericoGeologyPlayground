# M8 Geology Blueprint P2 — Aperiodic Tiling Research

**Date:** 2026-08-29
**Slice:** `M8-GEOLOGY-BLUEPRINT-P2-APERIODIC-TILING`
**Selected boundary:** `BOUNDED_DETERMINISTIC_CORNER_LATTICE_SOURCE`

## Question

What is the smallest independent Jerico source that reduces visible periodic
repetition, has an exact finite extent and deterministic identity, fails closed
under cancellation/budgets, and reaches the existing Vulkan geology field,
Surface Nets and cook/runtime consumers without importing later P2 work?

## Primary sources

The blueprint reference is Peytavie, Galin, Grosjean and Mérillou,
[*Procedural Generation of Rock Piles using Aperiodic Tiling*](https://perso.liris.cnrs.fr/eric.galin/Articles/2009-aperiodic.pdf),
*Computer Graphics Forum* 28(7), 2009, DOI
[`10.1111/j.1467-8659.2009.01557.x`](https://doi.org/10.1111/j.1467-8659.2009.01557.x).
The [Eurographics Digital Library record](https://diglib.eg.org/items/6d0c83da-b4b7-48e5-94be-fd464b76d333)
is the official proceedings record.

The colored-corner construction is described independently by Lagae and Dutre
in [*An Alternative for Wang Tiles: Colored Edges versus Colored Corners*](https://graphics.cs.kuleuven.be/publications/LD06AWTCECC/),
DOI [`10.1145/1183287.1183296`](https://doi.org/10.1145/1183287.1183296),
and the 3D point-distribution construction appears in their official
[*Poisson Sphere Distributions* project record](https://graphics.cs.kuleuven.be/publications/LD06PSD/index.html).

For terminology boundaries, Culik II and Kari's
[*An aperiodic set of Wang cubes*](https://doi.org/10.1007/3-540-60922-9_12)
proves a different 21-cube face-colored set, while Tyburec and Zeman's
[*Bounded Wang tilings with integer programming and graph-based heuristics*](https://www.nature.com/articles/s41598-023-31786-3)
formalizes finite bounded solving and reports a periodic counterexample for a
previously claimed corner-tile set. These are terminology/limit evidence, not
implementation donors.

No reference implementation, shader, tool, tile library or source code was
inspected, copied or ported.

## Confirmed construction facts

Peytavie et al. use two colors at the eight corners of an axis-aligned cube, so
the complete fixed-orientation set contains `2^8 = 256` corner signatures.
Cubes on an integer lattice are compatible because adjacent cubes read the same
four lattice-vertex colors on their common face. Assigning colors to lattice
vertices then selects one complete-set tile for each cube.

The paper's production construction is substantially larger than that matching
rule. It subdivides tiles into cells, precomputes corner/edge/face/interior seed
regions, evaluates anisotropic Voronoi volumes including cross-boundary
neighbors, erodes while retaining contacts, voxelizes and polygonizes. A support
membership test and anchor/inside thresholds decide which rocks instantiate.

Those later operations are not required to establish the colored-corner tiling
identity and they overlap explicitly forbidden later P2 slices.

## What the sources do not establish

- The 2009 use of “aperiodic” describes a stochastic non-repeating construction;
  it is not a proof that the complete two-color 256-tile set admits no periodic
  infinite tiling.
- A deterministic coordinate hash can itself have a finite period. Determinism
  is an identity property, not a proof of mathematical aperiodicity.
- No finite patch proves that every infinite extension is non-periodic.
- The paper does not define canonical recipe bytes, hash/seed versioning,
  cancellation, finite work/memory budgets, overflow handling or no-partial
  publication.
- Its reported preprocessing timings and rock counts are observations, not
  Jerico production budgets.
- A formal face-colored Wang-cube theorem does not provide the paper's rock
  geometry, finite query contract or Jerico runtime lineage.

Jerico must therefore describe this capability as a bounded deterministic
stochastic/corner-lattice source and reserve `MathematicallyAperiodic` for a
separately verified theorem-backed set.

## Existing Jerico seams

Jerico already owns the required vertical:

```text
typed GeologyStackRecipe Source
-> canonical source/recipe identity + stable layer seed
-> temporary additive BSP geometry
-> guarded Vulkan scalar/classification field
-> classified Surface Nets
-> JRMS/JRCL + material/textures cook
-> source-free package/runtime
```

The closed block-grammar source proves that a bounded CPU source may publish
deterministic additive boxes into that vertical. It is not a dependency or tile
prototype for this slice and its source/search contracts remain unchanged.

## Independent Jerico conclusion

Add one cumulative schema-22 `AperiodicTiling` source whose precise diagnostic
name is `BoundedDeterministicCornerLattice`. Its only new authored values are a
positive local root half-extent and three finite tile counts; the existing layer
seed drives a domain-separated full-coordinate color hash.

Evaluate the shared vertex colors in lexicographic order. Each cell's canonical
eight-bit signature derives exactly one additive box confined to that cell:
opposing-face color balance controls bounded center offset and per-axis corner
transitions control bounded half extent. This is an independent Jerico mapping,
not the paper's Voronoi, erosion or seed-region implementation.

Return one immutable preview containing vertex colors, tile signatures, boxes,
exact work/retention counters and content identity. A source-specific diff makes
extent/count/seed and changed-tile consequences inspectable. The designer
chooses by accepting the canonical recipe/revision; the existing explicit GPU
evaluation request consumes that chosen recipe. No UI or second Apply/session
system is introduced.

The construction uses fixed finite loops, preflights work and retained memory,
checks cancellation throughout and exposes no partial result. The boxes then
reuse the existing BSP/Vulkan/Surface Nets/cook/runtime path unchanged.

This boundary reduces visible repetition in a finite authored result. It does
not claim a mathematical aperiodicity theorem, reproduce the reference rock
generator or implement any subsequent P2.
