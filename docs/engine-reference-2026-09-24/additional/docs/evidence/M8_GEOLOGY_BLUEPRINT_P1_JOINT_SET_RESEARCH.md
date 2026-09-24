# M8 P1 JointSet research mapping

## Scope and provenance

This note supports only `M8-GEOLOGY-BLUEPRINT-P1-JOINT-SET`, the second P1
item in blueprint section 88.

The binding research synthesis is already recorded in blueprint section 19 as
`[JERICO SYNTHESIS strongly motivated by R8]`, Paris et al., *Modeling Rocky
Scenery using Implicit Blocks* (2020), DOI
`10.1007/s00371-020-01905-6`. This slice used that local authority together
with the published Jerico `PlaneCut`, `Strata`, `FaultDisplace`,
`AnisotropicVoronoiCells`, `FractureCells` and `ImplicitBlock` contracts. It did
not import or inspect an external implementation as a code donor.

No research or reference-engine source, shader, data structure, API, constant,
distribution or preset is copied, translated or ported. The implementation is
an independent extension of Jerico's canonical recipe, resident
Diligent/Vulkan scalar field, bounded inspection, dirty tracking and Surface
Nets paths.

## Problem understanding retained

A structural joint family is a finite set of roughly parallel planes. Each
plane is represented by a dominant unit normal and a signed metre offset. A
small authored angular or offset perturbation may break perfect regularity,
but the dominant family orientation must remain legible. Combining two or
three such families can divide existing implicit matter into tabular,
columnar or polyhedral regions without replacing the underlying source.

This is a structural field problem, not a mesh-generation problem. Plane
families identify thin joint slabs to subtract from the negative-inside source;
the existing resident field and Surface Nets remain responsible for the final
surface.

## Independent Jerico mapping

Jerico admits one data-driven `JointSet` recipe layer:

- one to three families;
- two to sixteen strictly ordered explicit plane offsets per family, with at
  most thirty-two planes in the complete layer;
- one canonical local unit dominant normal per family;
- bounded authored angular jitter in `[0, 15]` degrees and non-negative bounded
  offset jitter;
- a positive world-metre joint half-width, conservatively bounded with jitter
  against the minimum family separation;
- the existing full translation/quaternion/non-uniform-scale layer transform;
- one canonical seed, non-zero exactly when any family uses irregularity.

Explicit offsets keep topology and recipe identity authored and inspectable;
the layer does not hide a spacing distribution, generator or named geological
preset. The CPU reference deterministically resolves each local plane to a
normalized world-space plane using the inverse-transpose scale/rotation rule.
The productive GPU path uploads those resolved planes once, so CPU and Vulkan
do not duplicate random or transform policy.

For a world point, the layer finds the nearest absolute resolved-plane
distance, constructs `jointScalar = halfWidth - nearestDistance`, and subtracts
the union of those thin slabs with `max(source, jointScalar)`. Existing strength
and regional-mask composition interpolate the result. Repeated canonical
inputs retain identity; every offset, normal, jitter bound, seed and transform
participates in canonical serialization and hashing.

## Explicit boundary

This mapping does not admit implicit block generation, `FoldDomain`,
gradient-based detail warp, `DirectedCrack`, durability, rockfall, colluvium,
imported mesh structural operators, P2 research, named cliff/rock presets,
OpenVDB or editor UI. Completing `JointSet` does not complete P1, the geology
blueprint or JericoEngine 1.0.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
