# M8 Geology Blueprint P0 CurvatureWeathering — Frozen Conventions

**Slice:** `M8-GEOLOGY-BLUEPRINT-P0-CURVATURE-WEATHERING`
**Schema:** 12
**Backend:** CPU mathematical reference plus Diligent/Vulkan productive path

## Purpose and boundary

CurvatureWeathering is bounded scalar-field erosion selected by signed mean
curvature. It reuses the admitted sparse field, sampling, regional masks,
BubbleWeathering exposure/durability/update behavior, scalar ping-pong, dirty
tracking, bounded inspection, material attribution and structural channels.

It does not add a persistent independent durability field, material transport,
mineral chemistry, rockfall, colluvium, sediment, Detail, VDB or UI work.

## Scalar sign and units

- The scalar field is negative inside rock and positive outside.
- World positions and signed distances use metres.
- Curvature uses inverse metres (`1/m`).
- `sampleSpacingMeters` is an exact integer multiple of the recipe voxel size
  from one through four voxels.
- `curvatureResponseLengthMeters` is a finite physical length from one through
  64 voxels. Multiplying it by curvature produces a dimensionless response.
- `maximumWearMetersPerIteration` is finite and lies in `[0, voxelSizeMeters]`.

## Stencil and epsilon

For spacing `h = sampleSpacingMeters`, the centred gradient is:

```text
gradient(F, p) = (
    (F(p + hX) - F(p - hX)) / (2h),
    (F(p + hY) - F(p - hY)) / (2h),
    (F(p + hZ) - F(p - hZ)) / (2h))
```

The unit normal is `gradient / |gradient|` when `|gradient| > 1e-6`; otherwise
it is exactly zero. The curvature stencil evaluates that same normal at the six
axial neighbours and computes:

```text
kappa = (Nx(p+hX)-Nx(p-hX)
       + Ny(p+hY)-Ny(p-hY)
       + Nz(p+hZ)-Nz(p-hZ)) / (2h)
```

This is `div(normalize(gradient(F)))`, not the pre-existing Laplacian-like
`meanCurvature` helper. Sparse out-of-domain lookup continues to use the
admitted positive finite background value. Curvature evaluation adds no edge
cache or alternative sampler.

## Sign and mode

With the negative-inside convention:

- a solid convex sphere has positive curvature;
- a concave cavity has negative curvature;
- a plane is near zero curvature;
- `Convex` selects `max(kappa, 0)`;
- `Concave` selects `max(-kappa, 0)`.

The bounded response is:

```text
response = clamp(selectedCurvature * curvatureResponseLengthMeters, 0, 1)
```

No absolute-value mode silently merges convex and concave behavior.

## Exposure, durability, mask and update

CurvatureWeathering reuses BubbleWeathering's 26-neighbour Moore-shell surface
exposure at the authored spacing. The scalar removal for one iteration is:

```text
removalMeters = maximumWearMetersPerIteration
              * response
              * exposure
              * layerStrength
              * regionalMaskWeight
              * (1 - durability)

S_next = S_current + removalMeters
```

Increasing a negative-inside scalar removes rock. Every factor is clamped by
its admitted contract. Durability is a scalar recipe input in `[0,1]`; this
slice does not persist a separate DurabilityField.

Zero strength, zero mask or zero iterations produces the source field exactly.
Zero iterations schedules no `SurfaceWeathering` pass. Every non-zero iteration
reads one immutable scalar field and writes the other through the existing
ping-pong route before swapping.

## Determinism, channels and inspection

- Schema 12 hashes and round-trips mode, physical spacing, response length,
  maximum wear, durability and iteration count.
- Seed is zero and transform is identity because the model has no stochastic or
  transformed domain.
- Non-finite, fractional-voxel, out-of-range or ambiguous state is rejected.
- The productive pass changes only the scalar ping-pong field and a transient
  inspection metric tuple `(exposure, gradientMagnitude, curvature, response)`.
- Material GUIDs and live structural scalar/mask/cell channels are preserved
  exactly.
- Re-evaluation with identical state is deterministic and reports zero dirty
  work; changed inputs produce bounded dirty leaves/voxels.
- Inspection reads only requested samples. The transient metric field is
  included in the device-memory preflight and is never a persistent authored
  asset.

## Required proof

The mathematical RED/GREEN suite uses small analytic fields and proves:

- plane curvature near zero;
- convex and concave fields with opposite signs;
- physical-scale response under changed voxel size;
- finite results at zero-gradient points;
- deterministic output;
- strength, mask, exposure, durability and safe scalar update composition.

The productive proof must additionally demonstrate CPU/GPU parity, actual
scalar and extracted-topology changes for both modes, safe iterations,
localized mask behavior, exact structural/material preservation, cancellation,
device-memory bounds and real Diligent/Vulkan execution.
