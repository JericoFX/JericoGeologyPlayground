# M8 Geology Blueprint P0 Detail — Frozen Conventions

**Slice:** `M8-GEOLOGY-BLUEPRINT-P0-DETAIL`
**Recipe contract:** existing schema-1 `Noise` payload; no schema bump
**Backend:** CPU mathematical reference plus productive Diligent/Vulkan path

## Purpose and boundary

Detail adds bounded micro/meso variation to an already recognizable geological
scalar field. It reuses the admitted `Noise` layer, recipe identity, RBrush
masks, scalar ping-pong, trilinear field sampling, bounded inspection, dirty
tracking, material attribution and live structural channels. `FBM`, `Ridged`
and small domain warp cannot own the macro silhouette or replace strata,
faults, fractures, cavities or weathering.

This slice does not add another noise graph or field framework, restore VDB or
the removed UI, implement the gradient-based P1 detail warp, persist a
DurabilityField, introduce material transport or enter Step 14 proof recipes.

## Existing canonical identity

No recipe-schema extension was required. The existing `Noise` payload already
round-trips and hashes all semantic inputs:

- noise and fractal kind;
- frequency, amplitude, octave count, lacunarity and gain;
- domain-warp frequency and amplitude;
- displacement mode, surface band and clamp;
- linear remap;
- layer enabled state, strength, full-width seed, transform and mask reference.

The productive path admits identity-transform `OpenSimplex2` with `None`,
`FBM` or `Ridged`, one through eight octaves, lacunarity from one through four
and canonical gain in `(0, 1]`. Evaluation folds the canonical 64-bit seed by
XOR of its two 32-bit halves, matching the established CPU/GPU noise contract;
the full 64-bit value remains part of recipe identity.

## Sign, units and represented scale

- The scalar field is negative inside rock and positive outside.
- World positions, amplitudes, clamps, bands and warps use metres.
- `frequencyPerMeter` and `domainWarpFrequencyPerMeter` use inverse metres.
- For frequency `f`, the lattice wavelength is `1/f` metres.
- Each productive base wavelength must be no larger than 16 voxels.
- The finest wavelength after all octaves must be at least one voxel.
- Value amplitude, source-domain amplitude, noise-domain warp amplitude and
  scalar clamp are independently limited to at most two voxels.
- The active surface band is from one through eight voxels.

These bounds are checked against the recipe voxel size before any productive
Noise dispatch. Non-finite state continues to be rejected by canonical recipe
validation. Large isotropic silhouette noise is outside this contract.

## Shared noise primitives

The CPU and HLSL paths use the existing deterministic lattice value noise and
configured FBM. FBM is the amplitude-normalized sum:

```text
FBM(p) = sum(valueNoise(p * lacunarity^i, seed_i) * gain^i)
       / sum(gain^i)
```

Ridged reuses that same primitive and schedule:

```text
ridge_i = (1 - abs(valueNoise(p_i, seed_i)))^2
Ridged(p) = 2 * sum(ridge_i * gain^i) / sum(gain^i) - 1
```

Both are finite and normalized to `[-1, 1]` before the existing linear remap.
No third-party implementation was copied and no dependency or licence record
changes.

## Noise-domain warp

When `domainWarpAmplitudeMeters` is non-zero, three decorrelated normalized FBM
channels form a vector whose length is clamped to one. The primary noise is
then evaluated at:

```text
p_noise = p + boundedVector(fbmX, fbmY, fbmZ)
              * domainWarpAmplitudeMeters
detail = configuredNoise(p_noise * frequencyPerMeter)
```

This bends only the coordinate used to generate Detail. It does not sample the
geological source field at a displaced point and is not the P1 gradient-based
detail warp.

## Value displacement versus source-domain deformation

The common weight is:

```text
weight = clamp(layerStrength) * clamp(regionalMask)
       * clamp(1 - abs(F(p)) / surfaceBandMeters)
```

`SurfaceNormal` is negative-inside value displacement:

```text
delta = clamp(detail * amplitudeMeters * weight,
              -clampMeters, +clampMeters)
F_next(p) = F(p) - delta
```

Positive Detail expands rock; negative Detail removes it. The update changes a
scalar value at the current world position and does not deform the source
domain.

`Vector` is true source-domain deformation. Three decorrelated remapped noise
channels form a vector bounded to unit length, then the existing trilinear
sampler reads the immutable input field at:

```text
q = p + boundedVector(detailX, detailY, detailZ)
        * amplitudeMeters * weight
F_next(p) = F(p) + clamp(F(q) - F(p),
                         -clampMeters, +clampMeters)
```

The operation never uses a scalar-noise value as a fake vector displacement.
It remains a small non-gradient domain deformation bounded by the same
micro/meso ceiling.

## Identity, ordering and channels

- A disabled Detail layer is not scheduled and contributes zero topology
  expansion; disabling all Detail layers reproduces the exact pre-detail field
  hash.
- Zero layer strength is the same exact no-dispatch identity.
- Zero amplitude is an exact mathematical scalar identity.
- Non-zero layers read one immutable scalar field and write the existing other
  ping-pong field before swapping.
- Regional masks multiply the Detail weight; they do not modify structural
  channels.
- Material GUIDs and unrelated Strata/AnisotropicVoronoi/Fracture structural
  values remain unchanged.
- Selected-sample inspection reuses the bounded readback path. Detail adds no
  whole-field CPU readback.
- Identical reevaluation produces the same field hash and zero dirty work;
  changed semantic inputs use the existing bounded dirty-leaf comparison.

Surface Nets uses atomic append counters, so raw vertex/index append order is
not a canonical identity. Exact disable proof therefore uses the scalar field
hash plus equivalent extracted counts, world bounds and material attribution.

## Required proof

Mathematical tests must prove the negative-inside sign, finite deterministic
FBM/Ridged output, physical frequency scaling, bounded noise/source domain
warps and exact zero controls. Productive proof must additionally demonstrate
real scalar and extracted-topology changes for all admitted modes, bounded
CPU/GPU parity, exact all-disabled field/hash identity, localized masking,
zero dirty work for repeated input, bounded changed-input dirty work,
cancellation, memory/workload guards, channel preservation and recognizable
inherited macro/meso geology.
