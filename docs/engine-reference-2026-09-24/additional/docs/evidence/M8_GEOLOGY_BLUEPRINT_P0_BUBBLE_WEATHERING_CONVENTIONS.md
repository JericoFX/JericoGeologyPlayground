# M8 Geology Blueprint P0 BubbleWeathering — Frozen Conventions

**Slice:** `M8-GEOLOGY-BLUEPRINT-P0-BUBBLE-WEATHERING`
**Authority:** `docs/JERICO_PROCEDURAL_GEOLOGY_RESEARCH_IMPLEMENTATION_BLUEPRINT.md`

These conventions bind the Step 11 CPU reference, canonical recipe,
productive Diligent/Vulkan pass, bounded inspection, mathematical tests and
visible proof. BubbleWeathering extends the existing `SurfaceWeathering` kind
without replacing Jerico's recipe, sparse scalar field, ping-pong, sampling,
regional masks, dirty tracking, inspection, structural channels or Surface
Nets extraction.

## Model, schema and units

- Schema 11 tags the new `SurfaceWeathering` model as `Bubble`. Schemas 1–10
  retain their existing legacy SurfaceWeathering payload and productive Vulkan
  behavior; the new model does not reinterpret old recipes.
- `exposureRadiusMeters` is finite, positive and exactly an integer multiple of
  the recipe voxel size. This slice admits radii of one through four voxels.
- `maximumWearMetersPerIteration` is finite and lies in `[0, voxelSizeMeters]`.
- `durability` is a finite dimensionless scalar in `[0,1]`: zero is weak and
  one is fully resistant.
- `iterations` lies in `[0,64]`. The Bubble model is deterministic and accepts
  only seed zero and identity transform; location comes from the source field
  and optional existing regional mask.

Unknown models, pre-schema Bubble state, fractional/out-of-range radii,
non-finite values, out-of-range durability/wear/iterations, nonzero seed or a
non-identity transform reject instead of producing ambiguous identity.

## Classification and exposure kernel

Jerico retains the existing negative-inside convention:

```text
rock/solid: S <= 0
air/void:   S > 0
```

For an authored integer radius `r` in voxels, the kernel samples the 26
non-centre offsets of the `3x3x3` Moore shell, scaled by `r`. A rock sample is a
surface sample when at least one shell neighbor is air. Its exposure is:

```text
airCount = count(S(p + r * offset) > 0 for all 26 offsets)
exposure = airCount / 26                     when S(p) <= 0 and airCount > 0
exposure = 0                                 otherwise
```

The positive finite sparse-field background is air. Existing bounded field
sampling owns out-of-domain lookup; BubbleWeathering adds no edge cache or
sampling framework. Small-grid contracts cover the admitted radius-one through
radius-four endpoints, including a point visible only at the second shell;
productive Vulkan parity directly covers radii one, two and four.

## Scalar update, durability and iterations

For each iteration and rock sample:

```text
weight  = clamp(layerStrength, 0, 1)
        * clamp(existingRegionalMask(p), 0, 1)
removal = maximumWearMetersPerIteration
        * weight
        * exposure
        * (1 - clamp(durability, 0, 1))
S_next  = S_current + removal
```

Increasing the negative-inside scalar removes real solid volume. A convex
corner with more air neighbors therefore recedes faster than the centre of a
broad flat surface. The productive Vulkan route evaluates every iteration from
one immutable input field into the other existing scalar ping-pong field, then
swaps them. No iteration reads values it has already written. Fixed input is
therefore deterministic, and later iterations observe only the fully completed
previous field.

Zero strength and zero iterations are exact identity endpoints. Zero
iterations schedules no SurfaceWeathering dispatch. The shared layer shader
retains one full-domain transient exposure binding as safe bounded scratch, but
performs no Bubble write, inspection readback or dirty work for that endpoint.
Durability one or wear zero also yields zero scalar removal under the same
formula.

## Masks, inspection and channels

The weight composes the existing compiled regional-mask route, including the
admitted localized `RBrushLocalField` mask. Mask identity/inversion flags and
the Bubble model flag coexist in the existing layer constants; changing mask
coverage produces non-empty bounded dirty work.

BubbleWeathering writes only the scalar ping-pong field plus the transient GPU
exposure scratch. That scratch is sized to the bounded evaluation domain and
counted by device-memory preflight because the shared optimized shader requires
a safe full-domain binding even when Bubble is inactive. Bounded inspection
copies only explicitly selected samples. The scratch is not a persistent
`DurabilityField`, material channel or structural channel. Existing material
GUIDs and Strata/structural scalar, mask, cell and metric data remain unchanged.

Canonical schema-11 serialization and recipe hashing cover radius, per-
iteration wear, durability, iterations, strength, mask and all inherited layer
identity. Exact reevaluation preserves the field hash and reports zero dirty
leaves/voxels; semantic changes report non-empty work bounded by the existing
`8x8x8` leaves and workload/device-memory guards.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
