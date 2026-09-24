# ADR-042: Explicit GPU geology value-noise semantics

Status: accepted and behaviorally verified in M8 F3; published at72ea1148.
Owner authorization: Astra execution and bounded F2 repairs, 2026-09-05.

## Problem and decision

At `3e469c15`, productive GPU geology accepts the recipe label `OpenSimplex2`
but computes hashed cubic-interpolated lattice values. The CPU geology reference
computes the same value noise. Their agreement does not establish OpenSimplex
semantics. The FastNoise2 admission guard correctly rejects this undocumented
second implementation of an allegedly identical algorithm.

Name the existing GPU primitive **ValueLatticeV1**. It is the single bounded
GPU geology noise semantic implementation, not a new recipe graph or evaluator.
The private FastNoise2 backend behind `Jerico::ProceduralField` continues to own
the actual OpenSimplex2S/Perlin/Cellular procedural field algorithms. Neither
path may claim numerical parity with a different named algorithm. No third-party
nodes, types or libraries enter shaders, runtime formats or Shipping.

Geology recipe schema 28 explicitly admits `ValueLatticeV1` (enum value 4) for
Noise layers. A schema-28 Noise layer carrying an older algorithm label must
fail validation. Schemas 1-27 retain their canonical bytes and existing behavior;
the old GPU `OpenSimplex2` label is a compatibility encoding of ValueLatticeV1,
not a claim to implement OpenSimplex. New editor authoring must select schema 28
and the explicit value-noise name. Upgrading an old recipe is an explicit
Preview/Apply operation: canonical recipe identity changes, but the existing
numeric field and mesh are expected to remain identical. Do not bulk-migrate
accepted products or replace their hashes.

## Frozen scalar and composition contract

- Positions are world-space meters multiplied component-wise by XYZ frequency
  in cycles per meter. Frequency is not a reciprocal UI "scale".
- The scalar primitive samples the eight corners of `floor(position)`, uses
  cubic weights `t*t*(3-2*t)`, then interpolates X, Y, Z in that order.
- Lattice hash starts with seed XOR the three signed coordinates reinterpreted
  modulo 2^32, multiplied by `8da6b343`, `d8163841`, `cb1ab31f`. The existing
  mix uses shifts 16/15/16 and multipliers `7feb352d`/`846ca68b`. The low 24 bits
  map to [-1,1] with denominator 16777215.
- Existing layer seed packing remains unchanged. Detail folds a 64-bit seed by
  XORing low/high words. This is deterministic, not collision-free: flipping
  identical bits in both words aliases. Do not promise unique output per seed.
- Configured FBM uses normalized amplitude sum, octave seed step `9e3779b9`,
  authored lacunarity and gain. Ridged squares `1-abs(sample)`, adds coordinate
  offsets (11.7,-7.3,5.1) per octave and remaps the normalized sum to [-1,1].
- Domain warp uses the existing three decorrelated FBM seed domains
  `68bc21eb`, `02e5be93`, `967a889b` and bounds the vector length to one.
  Displacement, remap, masks and represented-scale guards remain unchanged.
- Noise supplies detail/irregularity. Existing structural operators own form.
  Signed distance remains negative inside, positive outside, zero at surface.
- CPU double reference versus Vulkan float execution requires explicit bounded
  coordinates and finite intermediates. Primitive probes include negative cell
  boundaries, large representable coordinates and high seed bits. Existing
  field parity tolerance (1e-4 meters for Detail) must not be relaxed.

## Required proof and guard

GPU lattice-coordinate admission uses the actual packed layer constants and
inclusive planned leaf bounds, after finite-value admission and before the
affected layer buffer/dispatch. Conservative absolute bounds include every
evaluated octave, Ridged offsets, domain warp, weather trace and the packed
GradientDetailWarp transform. Intermediate bounds round outward to float.
Every sampled coordinate must fit below 2^31 with room for the upper lattice
corner; 2147483520 is the largest float strictly below that boundary. Conservative
rotation/trace/warp bounds may reject a borderline request; they never clamp or
wrap it into a different result. Trace/warp unit-component bounds include a
factor-two float safety envelope. Rejection reports
`procedural.geology.gpu_noise_coordinates_unsupported`. This does not claim
arbitrary-world-coordinate precision or validate unrelated operator arithmetic.

Retain the admission RED at the commit fence. A semantic test must fail before
schema 28/explicit noise admission exists; round-trip, legacy identity and
rejection tests must pass afterward. Exercise the shared productive shader
primitive on Vulkan and every affected composition branch against independent
CPU expectations. Require known-answer samples as well as repeatability;
comparing two copies of an unnamed algorithm is insufficient.

The original ban on generic `valueNoise()`/`geologyNoise()` remains. Extract
only the existing primitive into one named shader seam, and constrain its
definition/consumption through the repository guard. A spelling change alone
does not close F3. Pins, licenses, private inclusion and runtime-only/Shipping
guards remain mandatory. No current Beauty result is promoted by this ADR.

## End-to-end completeness and semantic lineage

`source recipe/seed -> field/operator -> classified mesh -> cook -> rmesh/rcoll
-> renderer/physics -> observed surface` remains the existing product path.

| Integration | Applicability and ownership |
|---|---|
| Source, authoring, serialization, testing, docs, failure handling | Required in F3: explicit versioned noise identity, validation, canonical round trips, deterministic values and diagnostics. Existing recipe/session owners. |
| CPU, GPU, shaders, debugging | Required in F3: one primitive, semantic reference, actual Vulkan probes, unchanged represented-scale limits. Existing GeologyGpuSession and CPU reference owners. |
| Cook, materials, collision, runtime representation, renderer, LOD | Regression boundary: preserve old numeric results, classifications, structural fields, accepted cooked identities and source-free consumption. No new format or renderer. |
| Tooling/UI, cancellation, profiling/performance | INTEGRATION REQUIRED: next M8 rock-family authoring slice must expose seed and structural sliders in the existing editor; cancellable candidate regeneration, comparison, explicit Apply, Undo/Redo and measured latency. Owner: JericoEditor; input: canonical recipes and current session bridge. Depends on F3 semantic admission. |
| ECS, animation, Luau/events, Sequencer, save/load, streaming, destruction, navigation, particles, audio | No new noise consumer in F3. Preserve their existing upstream cooked-data interfaces; F2 IR records own later integrations. No public source-recipe dependency is introduced. |

The later runtime-host, fuzz/security and measured optimization repairs remain
authorized follow-ups in `docs/audits/M8_F2_ENGINE_ARCHITECTURE_PERFORMANCE_AUDIT.md`.
They do not become simultaneous work inside the F3 implementation slice.
