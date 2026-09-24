# M8 Geology Blueprint P2 Multi-Scale Microstructure Research

**Slice:** `M8-GEOLOGY-BLUEPRINT-P2-MULTI-SCALE-MICROSTRUCTURE-SYNTHESIS`

**Research state:** primary-source review complete before RED

**Selected independent boundary:** `BOUNDED_TWO_SCALE_PORE_SYNTHESIS`

## Primary source

Bojja Venu, Adam Bosak and Juan Raúl Padrón-Griffe, “Procedural Multiscale
Geometry Modeling using Implicit Surfaces,” *Computer Graphics Forum* 44(7),
e70233, 2025.

- DOI: <https://doi.org/10.1111/cgf.70233>
- publisher full text: <https://onlinelibrary.wiley.com/doi/10.1111/cgf.70233>
- authors' project, paper, supplement, slides and project code:
  <https://graphics.unizar.es/projects/MultiscaleGeometry_2025/>

The paper and project are the only external sources needed for this boundary.
They were studied as primary research, not as donor implementation.

## What the source actually demonstrates

The paper fills a triangle-mesh host with generalized implicit geometry and
evaluates it on demand. Its material structures include particulate, fibrous,
porous and laminar examples. A regular cubic grid plus a cell-coordinate seed
locates procedural particles without precomputing the entire population. For
suspended particles whose bounding sphere radius is at most half the cell width,
the paper's dual-grid argument bounds one point query to the eight neighboring
cells in 3D (sections 3.1.1-3.1.3, equations 1-3).

The published scale terms are geometric/visibility terms rather than universal
physical thresholds. The host is the macro form; larger particles, agglomerates
and grains provide mesoscopic structure; finer particles, fibers, pores and
layers provide microscopic detail. Cell width, particle size, density,
distribution, orientation and spatial functions control transitions between
them. The source does not supply a universal boundary in meters between macro,
meso and micro structure.

The primary output is a queryable implicit distance bound rendered by multiscale
sphere tracing. It is not a finite Jerico voxel field, extracted triangle mesh,
collision product or cooked runtime asset. The paper reports on-demand procedural
evaluation and `O(1)` procedural-state memory, but it does not publish a maximum
finite domain, work count, wall time, stable serialization, cross-platform
bitwise determinism, LOD/streaming contract or no-partial-publication rule.

## Limits and forbidden inferences

The source explicitly describes appearance modeling and material-design
potential. It does not validate geological, mechanical, thermal or chemical
properties. Particle piling is constructed without explicit physical simulation.
Reconstruction from images/distance values is a proof of concept; real SEM, TEM
or micro-CT reconstruction remains future work. Triangle-mesh extraction and
local manipulation are listed limitations.

Therefore Jerico must not claim that this source proves:

- physical porosity, permeability, strength, heat transfer or chemistry;
- sedimentation, collision, transport, dissolution or recrystallization;
- finite synthesis termination or reproducible cross-platform identity;
- Surface Nets, collision, material classification, cook, package or runtime;
- sub-voxel topology, unlimited scale hierarchies or arbitrary implicit media.

## Jerico observations before design

Jerico already owns one cumulative geology recipe through schema 26, one finite
planned voxel topology, scalar-field ping-pong, packed surface classification,
an authoring-only shared auxiliary float field, bounded inspection/hashes,
Surface Nets, exact static collision, `GEOL` vertex classification, deterministic
JRMS/JRCL cook and the existing Vulkan PBR geology material path.

The packed `cavity` response is already preserved from voxel classification to
surface vertices, cooked `GEOL`, runtime upload and material selection. A new
runtime format or parallel field/mesher/cooker is not required to prove a
resolved porous structure. Conversely, a transient authoring-only signal would
not satisfy the required mesh/material/cook consumer boundary.

Schema 26's dissolution value is semantic mineral state, not a structural input.
It may be inspected and hashed before later storage reuse, but its value must not
be interpreted as pore geometry, density or a multiscale distribution.

## Independent Jerico conclusion

Jerico adopts only the bounded problem insight:

```text
finite host SDF
+ exactly two represented pore-scale descriptors
+ stable layer seed / mask / strength
-> fixed 8-neighbor cell queries per scale
-> one finite pointwise Vulkan evaluation
-> resolved scalar carve + cavity classification
-> existing Surface Nets / collision / GEOL / PBR / cook consumers
```

The two Jerico bands are explicitly relative to the admitted voxel size. They are
named `fine` and `meso`; neither name claims a physical micrometre scale. Both
must be large enough to be represented by the current voxel topology. Anything
below that boundary remains material/texture research and is not silently
promoted to topology.

The exact inputs, scale bounds, distribution, output, budgets, termination,
identity and consumer contract are frozen in
`docs/superpowers/specs/2026-08-29-m8-bounded-two-scale-pore-synthesis-design.md`.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
