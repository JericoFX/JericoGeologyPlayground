# M8 Geology Blueprint P0 DirectionalWarp — Frozen Conventions

**Slice:** `M8-GEOLOGY-BLUEPRINT-P0-DIRECTIONAL-WARP`
**Authority:** `docs/JERICO_PROCEDURAL_GEOLOGY_RESEARCH_IMPLEMENTATION_BLUEPRINT.md`

These conventions bind the DirectionalWarp CPU reference, schema-8 recipe,
Diligent/Vulkan layer, bounded inspection, mathematical tests and visible
Surface Nets proof. They extend the published PlaneCut field path and do not
define another field or noise framework.

## Domain-sampling contract

- The operator samples the prior scalar field; it does not add a scalar:
  `q = p + normalize(directionWorld) * amplitudeMeters * strength * mask(p)
  * noise(p * frequencyPerMeter, seed)` and `result(p) = source(q)`.
- Negative scalar values remain inside, zero remains the extracted surface and
  positive values remain outside.
- `strength == 0`, `amplitudeMeters == 0` or a zero mask weight returns the
  exact source sample at `p`. No noise or resampling may perturb identity.
- `strength` and mask weight are clamped independently to `[0, 1]` before they
  scale the authored amplitude. The mask is evaluated at the output position
  `p`, not the displaced source position `q`.
- Direction is normalized on CPU and before GPU upload. Scaling a non-zero
  authored direction cannot change the result.

## Parameters, coordinates and units

- The productive geology stack currently evaluates BSP geometry and layers in
  world space, so the payload names `directionWorld` explicitly.
- `amplitudeMeters` is the maximum authored displacement in meters before
  header strength and mask composition.
- `frequencyPerMeter` is an anisotropic three-axis cycles-per-meter vector.
- `octaves` is `1..8`; lacunarity is finite and positive; gain is finite in
  `(0, 1]`. One octave is the existing lattice value-noise function. Additional
  octaves use the existing normalized configured-FBM accumulation.
- The first DirectionalWarp schema admits only the identity layer transform.

## Seed and deterministic noise

- `GeologyLayerHeader::seed` is the sole authored stochastic identity.
- CPU and GPU fold its low and high 32-bit words with XOR before sampling.
- Octave seeds advance by the existing `0x9e3779b9` increment.
- No address, pointer, clock, `random_device` or external state participates.

## Sampling, filtering and bounds

- The prior linear field stores voxel-center samples at
  `int3 voxelCoordinate * voxelSizeMeters`, with `8x8x8` leaves and local order
  `x * 64 + y * 8 + z`.
- DirectionalWarp converts `q` to continuous voxel coordinates and samples the
  eight neighboring prior-field values with ordinary trilinear interpolation.
- Each trilinear corner outside the admitted leaf topology contributes the
  existing positive background value, `4 * voxelSizeMeters`. This is the
  frozen out-of-bounds/background contract; positions are not clamped to the
  edge and the field is not wrapped.
- The sampled result is clamped only to the existing representable
  `[-background, +background]` range before storage.
- Bounded inspection continues to return selected represented voxel centers
  and stage hashes only; no whole-field intermediate readback is introduced.

## Identity, dirty work and materials

- Canonical recipe identity covers schema, kind/order/header, all payload
  fields, seed, mask and quality.
- Repeating identical input must reproduce the field hash and report zero dirty
  voxels/leaves. A bounded semantic parameter change must report non-zero but
  bounded dirty voxels and `8x8x8` leaves.
- DirectionalWarp modifies only scalar sample positions. Source material GUID,
  material class, hardness and erodibility remain inherited unchanged.
- CPU/GPU parity is measured at bounded inspected positions with the existing
  `1e-4 m` tolerance.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
