# M8 Geology Blueprint P2 — Contact-Preserving Erosion Research

**Date:** 2026-08-29
**Slice:** `M8-GEOLOGY-BLUEPRINT-P2-CONTACT-PRESERVING-EROSION`
**Selected boundary:** `BOUNDED_EXPLICIT_CONTACT_DAMPED_EROSION`

## Question

What is the smallest independent Jerico erosion operator that consumes an
explicit finite contact set, preserves every declared contact, produces useful
seed-sensitive erosion through the existing Vulkan geology vertical, terminates
under hard work/memory ceilings and leaves the designer in control without
reopening earlier P2 sources or introducing a general erosion framework?

## Primary source

The binding blueprint reference is Adrien Peytavie, Eric Galin, Jerome
Grosjean and Stephane Merillou,
[*Procedural Generation of Rock Piles using Aperiodic Tiling*](https://perso.liris.cnrs.fr/eric.galin/Articles/2009-aperiodic.pdf),
*Computer Graphics Forum* 28(7), 2009, DOI
[`10.1111/j.1467-8659.2009.01557.x`](https://doi.org/10.1111/j.1467-8659.2009.01557.x).
The [Eurographics Digital Library record](https://diglib.eg.org/items/6d0c83da-b4b7-48e5-94be-fd464b76d333)
is the official proceedings record.

No reference implementation, shader, tool, source asset or library was
inspected, copied or ported.

## Confirmed problem and mathematical idea

Peytavie et al. first construct coarse voxelized anisotropic Voronoi rock
volumes. For every pair of volumes that is actually in contact, they retain a
contact point on the segment between the two sites. They keep the complete
contact set because discarding contacts can create floating rocks.

Their erosion attenuation around one contact is compactly supported. With
distance `r` from contact point `c`, protected radius `a` and outer blend radius
`b`:

```text
omega_c(r) = 0                 when r < a
             (r-a)/(b-a)       when a <= r < b
             1                 otherwise
```

The combined field is the minimum over all contacts, so any protected contact
can suppress erosion locally. Their iterative voxel state update multiplies the
erosion amount by that attenuation. This separates two concerns cleanly:

```text
where contact must survive
!=
how much erosion is requested elsewhere
```

That separation is the reusable idea. The paper's Voronoi construction,
bisection contact discovery, spheroidal neighborhood erosion, iterative voxel
state and meshing implementation are not Jerico implementation donors.

## What the source does not define for Jerico

- canonical recipe syntax, schema migration or identity;
- an authoring source for contacts outside its own Voronoi pipeline;
- finite contact/work/memory ceilings;
- overflow, invalid-input or cancellation behavior;
- no-partial publication;
- immutable preview/diff and explicit designer acceptance;
- CPU/GPU numerical parity or Jerico field/classification lineage;
- cook, package or source-free runtime boundaries.

The paper also does not require every contact-preserving implementation to
repeat its contact discovery or iterative neighborhood erosion. Jerico can
admit explicit contact witnesses and a bounded pointwise erosion amount while
retaining the published preservation invariant.

## Existing Jerico facts

Jerico already owns:

```text
typed GeologyStackRecipe + stable layer seed
-> guarded finite Vulkan scalar/classification field
-> classified Surface Nets
-> GeologySurfaceMesh
-> JRMS/JRCL/material/texture cook
-> source-free package-only static PBR runtime
```

The closed schema-22 corner-lattice source publishes strictly cell-confined
boxes and deliberately does not construct Voronoi rocks or contact points.
Changing those boxes to infer or create contacts would reopen a closed P2.

`AnisotropicVoronoiCells` publishes nearest/second-nearest structural metrics,
but deriving a complete adjacency/contact graph would couple this P2 to a P0
producer and would not serve BSP, imported or other existing scalar sources.

The recipe layer stack already supplies immutable typed inputs, masks,
full-width seeds, canonical hashing and explicit accepted-recipe choice.
`GeologyGpuSession` already supplies bounded topology/device admission,
cancellation before publication, field inspection, deterministic final-field
identity and classified Surface Nets. `GeologySurfaceClassification` already
has a dedicated normalized erosion channel that survives the admitted mesh/cook
vertical. None of these seams should be reimplemented.

## Independent Jerico conclusion

Select `BOUNDED_EXPLICIT_CONTACT_DAMPED_EROSION`:

- one cumulative schema-23 layer, not a new source or field system;
- 1..8 explicit, unique, canonically ordered world-space contact points;
- one protected radius and one positive transition width shared by the set;
- one finite inside-surface band, maximum wear and isotropic wear frequency;
- existing layer strength, mask and full-width seed;
- one pointwise finite pass over the already admitted active topology;
- exact zero wear inside every protected sphere and linear damping through the
  transition band;
- seed-sensitive Jerico-native value-noise modulation only for the erosion
  amount away from contacts;
- the same applied wear updates the final SDF and existing erosion
  classification channel;
- existing GPU result as immutable preview, a pure payload/seed diff and the
  accepted recipe as the designer decision.

Hard ceilings are eight contacts, `67,108,864` contact-distance work items and
`128` retained GPU bytes for the packed contact constants. Contact/work arithmetic
is checked before allocation or dispatch. Invalid data, contacts outside the
finite evaluated domain, cancellation or exhausted budgets produce no result.

This boundary implements no generic neighborhood erosion, no contact discovery,
no iterative simulation, no moisture/mineral/microstructure process and no
runtime destruction/remesh/save state.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`
