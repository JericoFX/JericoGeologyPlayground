# M8 Geology Blueprint P2 — Dorsey Narrow-Band Mineral Weathering Research

**Date:** 2026-08-29
**Slice:** `M8-GEOLOGY-BLUEPRINT-P2-DORSEY-NARROW-BAND-MINERAL-WEATHERING`
**Selected boundary:** `BOUNDED_INTERIOR_MINERAL_RESPONSE_BAND`

## Question

What is the smallest independent Jerico capability that retains Dorsey's useful
surface-aligned volumetric idea, consumes an existing mineral/material
classification, publishes a useful bounded weathered-mineral response through
the existing classified Vulkan vertical, terminates under hard ceilings and
leaves the designer in control without implementing moisture transport,
mineral dissolution or a second field system?

## Primary source

The binding primary source is Julie Dorsey, Alan Edelman, Henrik Wann Jensen,
Justin Legakis and Hans Koeling Pedersen,
[*Modeling and Rendering of Weathered Stone*](https://www.cs.jhu.edu/~misha/ReadingSeminar/Papers/Dorsey99.pdf),
*Proceedings of SIGGRAPH 1999*.

The inspected ten-page PDF has SHA-256
`F83423249AB46E15411B01245311502785555788C387097B1B8C0E47BFFC5788`.
No reference implementation, shader, tool, source asset or library was
inspected, copied or ported.

## Confirmed representation and problem

Dorsey et al. observe that most visible stone weathering is concentrated near
the boundary even though it has volumetric effects. Their slab combines surface
and volume representations: a quadrilateral surface cover is extruded near the
surface, each eight-corner trilinear volume owns a local `(u, v, w)` frame, and
adjacent slabs share boundary corners. Samples inside that narrow region retain
density and material/mineral state.

The paper distinguishes the surface, a weathered interior and a moving moist/dry
front. Its full simulator evolves water pressure and a front, transports
dissolved mineral concentrations, recrystallizes material at the surface and
uses a decay index to alter stone density. The authors explicitly characterize
the result as phenomenological rather than an exact physical prediction.

The reusable representation idea is:

```text
surface boundary
-> finite surface-aligned interior neighborhood
-> material state stored beside density
-> bounded appearance/shape consumers
```

The paper's extruded mesh slabs, manual intersection correction, porous-media
equations, moving front, dissolution, advection/diffusion, recrystallization,
probabilistic erosion and Monte Carlo subsurface renderer are not Jerico
implementation donors and are outside this slice.

## What the source does not define for Jerico

- canonical recipe syntax, schema migration or deterministic identity;
- how an existing implicit scalar field supplies the surface-aligned coordinate;
- a minimal mineral input/output that does not require transport or dissolution;
- finite work/memory ceilings, overflow or invalid-input behavior;
- cancellation and no-partial publication;
- immutable preview/diff and explicit designer acceptance;
- Jerico Surface Nets, `JRMS GEOL`, cook, package or PBR material lineage.

## Existing Jerico facts

Jerico already owns the representation needed for the bounded adaptation:

```text
typed GeologyStackRecipe + full-width layer seed
-> resident signed-distance field
-> co-resident packed geological classification field
-> classified Surface Nets
-> GeologySurfaceMesh
-> JRMS 1.1 GEOL + JRCL + material/texture cook
-> source-free package-only static PBR Vulkan runtime
```

The signed distance is a direct surface-aligned depth coordinate. It removes
the need to build or edit extruded mesh slabs. The packed classification is an
eight-byte secondary field on the same active topology, so it removes the need
for another mineral grid.

`Strata` already produces the canonical normalized `strataClass` from its
material-class range. This is the admitted mineral/material-class input. The
packed `deposition` byte is an existing history-dependent semantic channel that
survives classified Surface Nets and `GEOL`. The Vulkan PBR material consumer
already routes five responses in this order:

```text
fracture, erosion, deposition, cavity, fresh
```

Therefore the `deposition` response can carry a normalized weathered-mineral
product amount and select an existing designer-authored PBR palette entry. No
runtime format, material database, renderer or second classification stream is
needed.

The existing layer stack already provides masks, strength, stable seed,
canonical hashing and explicit recipe choice. `GeologyGpuSession` already
provides topology/device admission, cancellation before publication, resident
scalar/classification ping-pong, inspection, deterministic result identities
and bounded Surface Nets extraction. These seams must be deepened, not
reimplemented.

## Independent Jerico conclusion

Select `BOUNDED_INTERIOR_MINERAL_RESPONSE_BAND`:

- one cumulative schema-24 layer, not a new source, simulation or field system;
- one earlier active `Strata` layer supplies the existing mineral/material class;
- one normalized susceptible mineral class and positive class half-width;
- one finite inside-surface depth, maximum weathered-mineral fraction and
  isotropic weathering frequency;
- existing layer strength, regional mask and full-width seed;
- one deterministic pointwise pass over the admitted active topology;
- only samples with `-interiorBandMeters <= signedDistance <= 0` and a matching
  mineral class receive a weathered-mineral candidate;
- the signed-distance field and every existing classification channel except
  `deposition` remain bit-exact;
- `deposition` changes monotonically by taking the maximum of its previous value
  and the bounded candidate;
- the existing deposition response selects a designer-authored PBR material;
- the full GPU result is the immutable preview, a pure payload/seed diff reports
  authored changes, and the accepted recipe remains the designer decision.

The hard work ceiling is `67,108,864` active-voxel evaluations. The layer adds
zero variable GPU bytes and no descriptor: its five scalar controls reuse the
existing fixed 368-byte per-layer constant buffer and it writes the already
admitted classification ping-pong buffer. Invalid data, missing active Strata,
arithmetic/budget exhaustion or cancellation produce no result.

This boundary implements no water/moisture state, porous-media flow, moving
front, mineral dissolution/recrystallization, conservation solver, density
erosion, microstructure synthesis, dedicated UI, runtime mutation or save state.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`
