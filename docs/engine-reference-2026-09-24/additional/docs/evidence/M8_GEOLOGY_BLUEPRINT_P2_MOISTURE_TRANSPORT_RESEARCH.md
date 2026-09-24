# M8 Geology Blueprint P2 — Bounded Moisture Transport Research

**Date:** 2026-08-29
**Slice:** `M8-GEOLOGY-BLUEPRINT-P2-MOISTURE-TRANSPORT`
**Selected boundary:** `BOUNDED_NORMALIZED_MOISTURE_PRESSURE_PROFILE`

## Question

What is the smallest independently designed Jerico capability that represents a
finite moisture source, stores and transports one unambiguous bounded quantity,
publishes a deterministic consumed result through the existing geology vertical,
and does not implement mineral transport, dissolution, recrystallization, erosion
or a general porous-media simulator?

## Primary source

The binding primary source is Julie Dorsey, Alan Edelman, Henrik Wann Jensen,
Justin Legakis and Hans Koeling Pedersen,
[*Modeling and Rendering of Weathered Stone*](https://graphics.stanford.edu/~henrik/papers/sig99/sig99.pdf),
*Proceedings of SIGGRAPH 1999*, pp. 225–234,
[DOI 10.1145/311535.311560](https://doi.org/10.1145/311535.311560).

The already inspected ten-page PDF is also retained by the preceding slice under
SHA-256 `F83423249AB46E15411B01245311502785555788C387097B1B8C0E47BFFC5788`.
No reference implementation, shader, source asset or donor library was inspected,
copied or ported. No additional scientific source was necessary to freeze this
bounded phenomenological response.

## What Dorsey actually represents

Dorsey confines weathering to surface-aligned volumetric slabs (§4.1, printed
p. 228). The moisture model (§4.3.1–4.3.2, printed pp. 228–229) distinguishes the
exterior surface, a weathered interior and a moving moist/dry front. It evolves:

```text
p     water pressure in the interior
phi   spatially varying porosity
front position
```

It does not define water mass, water volume or voxel saturation as the stored
quantity. During a wet cycle, surface pressure is prescribed from a wetness map;
during a dry cycle it is set to zero. Pressure is always zero at the moving
interior front. The paper then evolves pressure and advances that front using a
Darcy-style velocity.

The published equations are:

```text
dp/dt = -div(phi grad p)
v     = -(K / eta) (grad p - rho g)
```

The paper does not supply a complete SI closure for the pressure equation, a time
step, maximum iterations, convergence tolerance, lateral slab boundary condition,
formal initial pressure/front, or deterministic termination. It explicitly calls
the overall weathering model phenomenological in §7 (printed p. 233). Therefore
Jerico must not claim that Dorsey provides a mass-conserving water solver or a
complete calibrated porous-media model.

## Existing Jerico seams and rejected reuse

Jerico already owns the cumulative typed recipe, stable source/recipe identity,
resident signed-distance/classification/structural fields, immutable preview,
cancellation, checked Vulkan admission, field inspection, Surface Nets, cook and
source-free package lineage.

The existing heightfield hydraulic path is not a moisture-volume seam:

```text
GeologyHeightfieldErosionInput
-> rectangular XZ height samples
-> waterMeters + pipe flow + discharge + sediment
-> hydraulic/thermal erosion result
```

It stores surface-water depth in meters, derives volume from horizontal cell area,
and couples flow to erosion and sediment. It is a separate 2D heightfield problem,
not pressure transport inside Jerico's 3D signed-distance band. Its budget,
hashing, cancellation and failure patterns are reusable; its water state and
solver semantics are not.

The packed eight-byte geological surface classification is also not a moisture
store. Its channels already mean strata class/strength, fracture, erosion,
deposition, cavity, curvature, exposure and fresh. Reinterpreting any of them as
pressure would destroy existing data lineage. The admitted Dorsey layer's
deposition response remains a weathered-mineral authoring signal and is not
transported mass.

## Independent bounded conclusion

Select `BOUNDED_NORMALIZED_MOISTURE_PRESSURE_PROFILE`:

- one schema-25 cumulative layer after the admitted schema-24 Dorsey layer;
- one dimensionless normalized pressure-potential output `p*` in `[0, 1]`, not
  physical Pa, water mass, volume or conserved saturation;
- one surface source controlled by maximum normalized pressure, existing layer
  strength/mask, deterministic source frequency and full-width seed;
- one finite interior signed-distance band of depth `L` meters;
- one effective transport diffusivity `D` in square meters per second and one
  finite exposure duration `t` in seconds;
- characteristic transport distance `lambda = sqrt(D t)`;
- a bounded exponential depth profile normalized so `p*(0)` equals the authored
  source and `p*(L) = 0`;
- no lateral transport, moving front, wet/dry state history or numerical
  iterations;
- one pointwise CPU/Vulkan pass and one co-resident scalar output buffer inside
  the existing `GeologyGpuSession` field system;
- scalar geometry, packed classification, structural Strata state and durability
  remain bit-exact;
- field inspection and a dedicated moisture hash consume the authoring output;
- the existing cooked mesh records the changed canonical recipe hash but does not
  serialize the moisture field as runtime state.

This is an independently defined, finite pressure-response profile motivated by
Dorsey's source/pressure/front separation. It is not presented as a closed-form
solution of Dorsey's incomplete pressure/front system.

## Scope and overclaiming guards

- `p*` must never be called quantity of water, saturation, mass or volume.
- `D` is an authored effective response scale, not mineral permeability `K`.
- the result does not conserve water and cannot drive dissolution or
  recrystallization in this slice;
- the fixed inner zero boundary is a Jerico authoring decision, not Dorsey's
  moving front;
- no runtime mutable weather, save state, remesh, physical erosion, mineral
  chemistry or multi-scale microstructure is admitted;
- no dedicated UI or final bespoke preset is admitted;
- the designer previews and explicitly accepts the canonical recipe.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`
