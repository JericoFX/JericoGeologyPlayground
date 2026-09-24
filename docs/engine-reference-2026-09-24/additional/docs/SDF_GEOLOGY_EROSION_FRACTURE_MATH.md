# SDF Geology, Erosion and Fracture Mathematics

**Project:** JericoEngine  
**Status:** companion technical directive  
**Domain:** volumetric SDF / implicit geometry, not heightfield terrain

This document is a mathematical companion to the authoritative geology blueprint:

- `docs/JERICO_PROCEDURAL_GEOLOGY_RESEARCH_IMPLEMENTATION_BLUEPRINT.md`

Read that document first. This companion locks in volumetric erosion/fracture
interpretation without admitting the cancelled BSP-to-VDB architecture.

## Core rule

The authoritative procedural object is a scalar field `phi(x,y,z)` whose zero-isosurface is the visible solid boundary.

JericoEngine must not reduce geological generation to one height per `(x,z)` coordinate. The same field operations must work on isolated rocks, cliffs, overhangs, caves, tunnels, ceilings, BSP-derived masses, strata, fractures, deposits and eroded underground geometry.

Recommended sign convention:

```text
phi < 0 : inside solid
phi = 0 : surface
phi > 0 : outside solid
```

A tracked signed-distance field should satisfy approximately:

```text
|grad(phi)| = 1
```

near the interface. Arbitrary noise, warping and smooth CSG may preserve the correct implicit surface while no longer being an exact Euclidean SDF. Track that distinction explicitly and re-distance when required.

## Volumetric erosion

For a true SDF, uniform erosion of the solid by world-space distance `r` is conceptually:

```text
phi_new = phi + r
```

and dilation is:

```text
phi_new = phi - r
```

This operates in every direction and therefore works on cave ceilings, vertical walls, pillars, overhangs and tunnel interiors as well as upward-facing surfaces.

For spatially varying wear `E(x)`:

```text
phi_new(x) = phi(x) + E(x)
```

where `E` may depend on material hardness, erodibility, fracture proximity, curvature, water exposure, wind exposure, gravity orientation, sediment cover, noise and author masks.

For more general interface evolution use the level-set form:

```text
d(phi)/dt + F |grad(phi)| = 0
```

With positive wear interpreted as inward motion, `F = -E`. Explicit solvers must use conservative substeps relative to voxel size and maximum interface speed.

## Re-distancing

After arbitrary deformation, do not assume the field still satisfies `|grad(phi)| = 1`.

Support a repair/reinitialization path based on an accepted method such as Fast Sweeping, Fast Marching, PDE reinitialization or narrow-band distance rebuild. Do not run it blindly after every operator; track field quality and invoke it when physical-distance correctness, curvature stability or meshing quality requires it.

## Geometry and geology are separate fields

The geometry field answers:

```text
inside / outside / distance
```

The geology field answers:

```text
what material is present here and how does it behave?
```

At minimum geology may expose:

```cpp
struct GeologySample
{
    float hardness;
    float erodibility;
    float porosity;
    float permeability;
    float fractureResistance;
    float sedimentAffinity;
    uint16_t materialId;
};
```

Strata must be a 3D field, not hardcoded horizontal stripes. A layer coordinate should be derived from an arbitrary normalized strata direction, physical layer spacing and optional low-frequency coordinate warp. Different layers may have different erodibility so the visible ledges and recesses emerge from differential erosion rather than texture alone.

## Fracture topology

Use 3D Voronoi/Worley fields and directional plane joint sets as volumetric fracture sources.

For Voronoi feature distances `F1` and `F2`, a useful boundary signal is:

```text
B = F2 - F1
```

Near cell boundaries, `B` approaches zero. Retain stable nearest-cell IDs so the same internal fracture topology can later drive chunk separation, exposed interior materials and gameplay destruction.

Fractures require finite, variable thickness. They should be geometric volumes that can be subtracted from the host SDF, not black lines painted onto the final mesh. Fracture proximity may also increase local erodibility, allowing repeated weathering to open cracks naturally.

Support multiple fracture families with independent direction, spacing, thickness, persistence, mask and seed. Some rocks are better described by anisotropic Voronoi cells; others by semi-regular plane joint sets; many require both.

## Sediment and deposits

Do not model sediment as height added to terrain. Sediment may be represented as a separate deposit implicit field and combined with the host solid when baked.

This allows deposits on cave floors, against walls, inside cavities and cracks, beneath overhangs and at cliff bases. A later transport implementation may simulate loose particles and rasterize settled particles into a density/SDF volume.

## BSP to SDF

A convex BSP brush represented by outward-facing planes can use the implicit half-space intersection:

```text
f_brush(x) = max_i(dot(n_i, x) + d_i)
```

This gives the correct sign for a convex polyhedron, but it is not automatically the exact Euclidean signed distance near edges and corners. Preserve this distinction. A BSP mass may be converted to an implicit field, sampled into the native sparse representation and re-distanced before geological modifiers that require true distance behavior.

Naturalization must preserve gameplay constraints through protected-space and structural-anchor masks. Procedural geology must not close required corridors, door clearances, ladders, navigation space or scripted interaction areas.

## Meshing

The final evaluated SDF must already contain the intended geological form before surface extraction. Meshing does not decide what a rock looks like.

Marching Cubes remains useful, but sharp fractures, sedimentary ledges and BSP-derived features justify a Dual-Contouring-family path. Hermite samples provide crossing positions and normals; the representative cell vertex may be solved with a quadratic error function:

```text
min_x ||A x - b||^2
```

Adaptive subdivision should be driven by sign changes, interval bounds, curvature, QEF error, material boundaries, fracture importance and LOD. Do not subdivide merely because high-frequency noise exists deep inside a solid.

## Hybrid representation

Do not force the whole system to be analytical or voxel-only.

Preferred architecture:

```text
analytic / functional field graph
        -> adaptive sampling or bake
        -> native sparse narrow-band SDF
        -> simulation / deformation / local edits
        -> adaptive surface extraction
        -> game mesh / collision / remesh
```

Use stable world coordinates and deterministic seeds across chunks. Physical parameters such as erosion depth, fracture width and strata spacing must use world-space units rather than voxel counts.

## Graph categories

The procedural graph should evolve toward typed nodes for:

```text
Sources:
  primitives, BSP import, mesh-to-SDF, spline tube, volume import

CSG:
  union, intersection, difference, controlled smooth variants

Geology:
  strata, folded strata, material layers, hardness, porosity, veins

Fracture:
  Voronoi cells 3D, boundary field, plane joint sets, fracture carve, crack widening

Weathering:
  uniform offset, masked offset, curvature wear, differential erosion, exposure wear

Sediment:
  source, settling/transport, deposit field, debris field

Repair:
  re-distance, fast sweep, narrow-band rebuild, SDF validation

Output:
  mesh, collision, SDF volume, material/fracture volumes, debug slices
```

Pins should distinguish scalar fields, vector fields, SDF fields, material fields, masks, point sets, particle sets, meshes and BSP solids rather than treating every connection as a float.

## Debugging requirements

The editor should eventually expose:

```text
arbitrary 2D/3D SDF slices
zero-isosurface
phi values
gradient direction
|grad(phi)| error
curvature
hardness / erodibility
material ID
strata coordinate
Voronoi cell ID
fracture proximity
wear / deposition
active narrow band
meshing cells / QEF error
```

A surface probe should be able to report the same data at a selected point.

## Implementation order

Do not implement erosion, fractures, sediment, graph UI and final meshing as one undifferentiated change.

A useful first vertical slice is:

```text
Convex BSP / basic primitive
  -> SDF evaluation
  -> sampled sparse narrow band
  -> 3D strata/material field
  -> 3D Voronoi fracture field
  -> masked volumetric offset
  -> re-distance
  -> existing/admitted adaptive mesh extraction
```

Then prove the architecture with a boulder, a layered fractured boulder, a BSP cave/room naturalized into rock and a tunnel containing overhangs. Debug slices must demonstrate that the fields are genuinely volumetric.

## Licensing / reference rule

Published sparse level-set literature may inform morphology, tracking and
fast-sweeping mathematics, but OpenVDB/NanoVDB/PNanoVDB code and formats are not
part of Jerico geology. libfive and Fidget remain conceptual references for
functional implicit modeling, expression evaluation, interval methods and
adaptive implicit meshing.

Do not copy GPL implementation code into JericoEngine unless the project deliberately accepts the resulting licensing requirements. Prefer original mathematics, papers and clean-room implementations, and follow the exact license obligations of any source actually incorporated.

## Final check for Codex

For every proposed geological algorithm ask:

> Does this operate on an arbitrary 3D solid, or did I accidentally turn the problem back into a heightmap?

A core JericoEngine SDF solution must work without a special case on:

```text
top surface
vertical wall
underside
cave ceiling
tunnel wall
isolated rock
BSP room
overhang
```

The volumetric field is the source of truth.
