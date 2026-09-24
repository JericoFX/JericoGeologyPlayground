# M8 Geology Blueprint P0 StrataField — Frozen Conventions

**Slice:** `M8-GEOLOGY-BLUEPRINT-P0-STRATA-FIELD`
**Authority:** `docs/JERICO_PROCEDURAL_GEOLOGY_RESEARCH_IMPLEMENTATION_BLUEPRINT.md`

These conventions bind the StrataField CPU reference, schema-8 recipe,
Diligent/Vulkan layer, bounded inspection, mathematical tests and visible
proof. They extend the published PlaneCut and DirectionalWarp field path and do
not define another field, noise, mask or material framework.

## Structural-field contract

- StrataField evaluates in world space. The authored non-zero
  `directionWorld` is normalized before use; scaling it cannot change the
  result.
- For world position `p`, normalized axis `a`, physical spacing `s`, authored
  phase `phaseOffsetCycles` and warped coordinate `h`, the periodic phase is:

  ```text
  h = dot(p, a) + warpAmplitudeMeters * FBM3(
          p * warpFrequencyPerMeter, fold64(seed))
  phaseCycles = h / s + phaseOffsetCycles
  ```

- `spacingMeters` is the distance in meters between equivalent phase points.
  `phaseOffsetCycles` is measured in cycles; adding one is exactly periodic.
- The frozen periodic profile is the symmetric triangle wave:

  ```text
  unitPhase = frac(phaseCycles)
  periodicScalar = 1 - abs(2 * unitPhase - 1)
  ```

  `periodicScalar` is finite and bounded to `[0, 1]`, equals zero at integer
  phase and equals one at half-cycle phase.
- The reusable structural mask is:

  ```text
  profileMask = smoothstep(
      profileMinimum, profileMaximum, periodicScalar)
  composedMask = profileMask
      * clamp(layerStrength, 0, 1)
      * clamp(regionalMaskWeight, 0, 1)
  ```

  Both mask values are finite and bounded to `[0, 1]`. Inverting or feathering
  the existing regional mask remains owned by the admitted generic mask route.
- The structural result exposes phase, periodic scalar, profile mask and
  composed mask independently from the optional SDF response. Geometry,
  durability, material, weathering and fracture-bias consumers can therefore
  share the same structural meaning; this slice implements only the existing
  optional geometry response and does not implement those later consumers.

## Warp, seed and deterministic noise

- `warpAmplitudeMeters` displaces the layer coordinate in meters before
  division by `spacingMeters`. Zero amplitude is the exact analytic field and
  performs no noise evaluation that can perturb the result.
- `warpFrequencyPerMeter` is an anisotropic three-axis cycles-per-meter vector.
- The existing normalized Jerico `FBM3` configuration is frozen for this
  schema-8 Strata payload: three octaves, lacunarity `2.0`, gain `0.5` and the
  existing `0x9e3779b9` octave seed increment. No new noise abstraction is
  introduced.
- The full-width layer seed is folded by XOR of its low and high 32-bit words,
  exactly as PlaneCut and DirectionalWarp. No pointer, clock, `random_device`
  or external state participates.

## Recipe, transforms and range validation

- Schema 8 canonical identity continues to cover kind/order/header, every
  existing `GeologyStrataPayload` field, full-width seed, generic mask and
  quality through canonical serialization and hashing.
- The first admitted StrataField route uses an identity layer transform because
  its payload explicitly names world-space coordinates. Non-identity transforms,
  zero axes, non-positive spacing/frequency, negative warp amplitude, unordered
  profile thresholds or non-finite state are rejected rather than interpreted
  ambiguously.
- Existing optional geometry controls (`shapeDisplacementMeters`,
  `grooveDepthMeters`, `grooveWidth`, `surfaceBandMeters`) remain meter/bounded
  responses to the structural field. They do not define the field itself.
- Existing material-class, hardness and erodibility payload ranges remain
  serialized for compatibility but are not applied by this slice.

## Lattice, SDF composition and inspection

- The structural channel uses the existing world-space voxel-center lattice,
  `8x8x8` leaves and local order `x * 64 + y * 8 + z` used by the productive
  Diligent/Vulkan scalar field.
- StrataField does not resample the preceding SDF and introduces no separate
  out-of-bounds/background rule. The preceding scalar field retains its existing
  positive background `4 * voxelSizeMeters` and represented
  `[-background, +background]` range.
- Optional geometry response is composed only inside the existing mutable
  narrow surface band and generic regional mask. The reusable structural field
  itself is position-defined and is not clipped to the surface band.
- Bounded stage inspection may return selected structural values and hashes.
  It must not read back the whole structural or scalar field between stages.

## Identity, dirty work and future consumers

- Repeating identical input must reproduce recipe, scalar-field and structural
  identities and report zero dirty SDF voxels/leaves.
- A bounded orientation, spacing or warp change with a non-zero optional
  geometry response must report non-empty but bounded dirty voxels and
  `8x8x8` leaves.
- StrataField does not change source material GUIDs. Later material, durability,
  weathering and fracture-bias stages may consume the structural channel only
  in their own legal slices.
- CPU/GPU structural parity is measured at bounded inspected voxel centers with
  the existing `1e-4` numerical tolerance. No tolerance is weakened.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
