# M8 Geology Blueprint P2 — Bounded Mineral Dissolution Research

**Date:** 2026-08-29
**Slice:** `M8-GEOLOGY-BLUEPRINT-P2-MINERAL-DISSOLUTION`
**Selected boundary:** `BOUNDED_LOCAL_MINERAL_DISSOLUTION_FRACTION`

## Question

What is the smallest independently designed Jerico capability that consumes the
already admitted narrow-band mineral response and normalized moisture exposure,
represents a finite local mineral inventory, publishes a deterministic bounded
dissolved fraction through the existing geology vertical, and does not implement
mineral transport, recrystallization, erosion or a general chemistry simulator?

## Primary source

The binding primary source is Julie Dorsey, Alan Edelman, Henrik Wann Jensen,
Justin Legakis and Hans Koeling Pedersen,
[*Modeling and Rendering of Weathered Stone*](https://groups.csail.mit.edu/graphics/pubs/stone.pdf),
*Proceedings of SIGGRAPH 1999*, pp. 225–234. Publication metadata is also
available from the
[Yale Computer Graphics Group](https://graphics.cs.yale.edu/publications/modeling-and-rendering-weathered-stone).

The inspected ten-page PDF has SHA-256
`F83423249AB46E15411B01245311502785555788C387097B1B8C0E47BFFC5788`.
No reference implementation, shader, asset or donor library was inspected,
copied or ported. No additional source was necessary to freeze this deliberately
phenomenological boundary.

## What Dorsey actually represents

Dorsey confines weathering to finite surface-aligned volumetric slabs. Its
moisture model distinguishes an exterior surface, a weathered interior and a
moving moist/dry front. The mineral model associates dissolved concentration
`C_i`, diffusivity `D_i`, a dissolution/solubility constant `k_i` and a maximum
saturation `m_i` with each mineral species. Dissolved minerals are then advected
and diffused toward the surface, where later recrystallization may occur.

The paper therefore describes a larger coupled sequence:

```text
moisture pressure and moving front
-> mineral dissolution at the front
-> dissolved-mineral transport
-> surface recrystallization
```

The text treats concentration as increasing toward a saturation limit, while the
printed first-order equation on p. 230 has a sign that would drive an initially
sub-saturated value in the opposite direction. The paper also omits a complete
unit closure, calibrated constants, a deterministic time step and iteration
budget, the full initial/boundary state, and a per-voxel solid inventory update.
It calls the overall weathering model phenomenological. Jerico must therefore
study the problem but must not transplant that equation or claim physical mineral
mass conservation from it.

## Existing Jerico inputs and rejected reinterpretations

Jerico already owns two distinct upstream results:

1. schema-24 `NarrowBandMineralWeathering` stores a monotonic weathered-mineral
   response in the low byte of the existing deposition classification word;
2. schema-25 `MoistureTransport` publishes a finite normalized pressure/exposure
   field `p*` in the existing optional auxiliary GPU field.

Neither is a mineral quantity. The schema-24 byte is susceptibility/applicability,
not kilograms, moles or remaining mineral inventory. The schema-25 field is
normalized authoring exposure, not Pascals, water mass, water volume or
saturation. Reusing either as a conserved mineral store would silently change
already closed contracts.

The existing Strata classification is also a material class, not a quantitative
inventory. A dissolution stage therefore needs one explicit authored local
inventory scalar. The output must remain separate from source classification,
the Dorsey response and moisture exposure.

## Smallest coherent independent response

The selected response is one local, closed-form, normalized authoring operation.
For an active stone voxel:

```text
w       = existing schema-24 weathered response in [0, 1]
p       = existing schema-25 moisture exposure in [0, 1]
p_min   = authored minimum productive moisture exposure
a       = clamp((p - p_min) / (1 - p_min), 0, 1)
pattern = deterministic existing one-octave value noise in [0.75, 1]
dose    = authored dissolutionRatePerSecond * exposureDurationSeconds
e       = dose * a * layerStrength * regionMask * pattern
M_local = availableMineralFractionNormalized * w
D_local = M_local * (1 - exp(-e))
R_local = M_local - D_local
```

`D_local` is the new stored output. It is a dimensionless
`dissolvedMineralFractionNormalized`, not concentration or transported mineral.
`R_local` is derived for validation and inspection; it does not require a second
field.

This response has the useful local invariant:

```text
0 <= D_local <= M_local <= 1
D_local + R_local = M_local
```

That is a bounded local partition, not a claim of mineral conservation between
voxels or through later transport. It closes the missing quantitative inventory
without reopening schema-24 classification semantics.

## Finite execution and storage

The operation performs exactly one pointwise GPU dispatch and zero numerical
iterations. It is bounded by the already admitted geology volume:

```text
maximum work items = 67,108,864 voxels
peak auxiliary storage = 4 bytes per voxel
maximum auxiliary storage = 268,435,456 bytes
incremental dissolution storage = 0 bytes
```

The schema-25 moisture stage is inspected and hashed before the final dissolution
stage. The final stage may therefore read moisture from the existing auxiliary
float buffer and overwrite that same buffer with dissolved fraction. This adds no
buffer, descriptor or parallel field system and respects the existing Vulkan
storage-buffer limit.

## Explicit non-goals

This slice does not add:

- physical mineral species, molar mass, pH, temperature or reaction networks;
- Darcy flow, porous-media transport or another moisture solver;
- dissolved-mineral advection/diffusion or cross-voxel conservation;
- recrystallization, deposition, sediment, erosion or geometry removal;
- microstructure synthesis, destruction, remesh or retopology;
- runtime mutable geology, save-state, streaming state or gameplay chemistry;
- VDB, UI, presets, ML, distributed simulation or a general framework.

## Jerico conclusion

Dorsey establishes the production problem and the need to distinguish solid
mineral, dissolved mineral and later transported/recrystallized mineral, but it
does not provide a complete deterministic implementation contract. Jerico's
independent answer is the smallest bounded local partition that consumes the two
already closed authoring signals, introduces an explicit normalized inventory,
publishes one separate finite output and stops before transport or topology.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`.
