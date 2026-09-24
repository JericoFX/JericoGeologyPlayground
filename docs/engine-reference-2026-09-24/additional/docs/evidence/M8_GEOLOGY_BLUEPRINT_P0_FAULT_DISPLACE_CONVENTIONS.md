# M8 Geology Blueprint P0 FaultDisplace — Frozen Conventions

**Slice:** `M8-GEOLOGY-BLUEPRINT-P0-FAULT-DISPLACE`
**Authority:** `docs/JERICO_PROCEDURAL_GEOLOGY_RESEARCH_IMPLEMENTATION_BLUEPRINT.md`

These conventions bind the FaultDisplace CPU reference, schema-8 recipe,
Diligent/Vulkan layer, bounded inspection, mathematical tests and visible
StrataField proof. They extend the published PlaneCut, DirectionalWarp and
StrataField route without defining another field, noise, mask or structural
channel framework.

## Domain-deformation contract

- FaultDisplace evaluates in world space and deforms where the previous scalar
  and structural fields are sampled. It never adds a random value to the SDF.
- The authored non-zero `faultNormalWorld` and `slipDirectionWorld` are
  normalized before use. Scaling either authored axis cannot change the result.
- For output position `p`, normalized fault normal `n`, plane offset `o`,
  transition half-width `w`, optional roughness `r`, normalized slip direction
  `d`, authored displacement `D`, layer strength `L` and regional mask `M(p)`:

  ```text
  h = dot(p, n) - o
  hRough = h + r * FBM3(p * roughnessFrequencyPerMeter, fold64(seed))
  side = smoothstep(-w, +w, hRough)
  amount = D * side * clamp(L, 0, 1) * clamp(M(p), 0, 1)
  q = p + d * amount
  scalarOut(p) = scalarIn(q)
  structuralOut(p) = structuralIn(q)
  ```

- `hRough <= -w` is the unchanged negative side. `hRough >= +w` receives the
  full authored positive-side slip before strength and mask composition. The
  plane center evaluates to selector `0.5` when roughness is zero.
- The frozen transition profile is the ordinary cubic Hermite smoothstep
  `t*t*(3-2*t)`, with `t = clamp((hRough+w)/(2*w), 0, 1)`.
- `transitionHalfWidthMeters` is half the finite transition thickness; the full
  transition spans `2*w` meters. It must be finite and strictly positive.
- `displacementMeters` is a finite non-negative magnitude. Direction is owned
  only by `slipDirectionWorld`, so a negative magnitude is rejected as
  ambiguous.
- Zero displacement, zero layer strength or zero regional mask weight is exact
  identity and must avoid a noise/resampling path that could perturb the input.

## Parameters, units and coordinates

- Canonical payload names are `faultNormalWorld`, `planeOffsetMeters`,
  `slipDirectionWorld`, `displacementMeters`,
  `transitionHalfWidthMeters`, `boundaryRoughnessMeters` and
  `roughnessFrequencyPerMeter`.
- All positions, directions and planes are world-space. Offset, displacement,
  transition half-width and boundary roughness are meters.
- `roughnessFrequencyPerMeter` is an anisotropic three-axis cycles-per-meter
  vector. Every component must be finite and strictly positive.
- Every non-zero dimensional scalar control packed for Vulkan must remain
  finite and normal in the productive `float` domain. Values that overflow or
  enter its subnormal range are rejected before conversion rather than silently
  changing the CPU/GPU contract. Plane and slip axes are normalized robustly in
  double precision before their normalized components are packed; component
  rounding remains governed by the published CPU/GPU tolerance.
- The first admitted FaultDisplace route requires identity
  `GeologyTransform`; world orientation is fully explicit in the payload.

## Roughness, seed and mask composition

- Boundary roughness perturbs only the side selector's plane coordinate. It
  never becomes scalar/SDF displacement and never changes the slip direction.
- `boundaryRoughnessMeters == 0` is the exact analytic plane and performs no
  noise evaluation, independent of frequency and seed.
- Non-zero roughness reuses the admitted normalized Jerico three-octave FBM:
  lacunarity `2.0`, gain `0.5` and octave seed increment `0x9e3779b9`.
- CPU and GPU fold the full-width layer seed by XOR of its low and high 32-bit
  words. No pointer, clock, `random_device` or external state participates.
- The existing regional mask is evaluated at output position `p`. Strength and
  mask scale the slip amount after the fault selector; they do not move the
  plane or change roughness.

## Sampling, background and reusable StrataField

- The previous scalar field is sampled at continuous voxel coordinate
  `q / voxelSizeMeters` using the existing eight-corner trilinear filter.
- Every out-of-bounds scalar corner contributes the existing positive
  background `4 * voxelSizeMeters`; the field is not clamped or wrapped.
- The represented scalar result remains clamped only to the existing
  `[-background, +background]` storage range.
- If a reusable StrataField exists before FaultDisplace, its two admitted
  components—periodic scalar and composed mask—are sampled at the same `q`
  through equivalent trilinear filtering. Out-of-bounds structural corners are
  `(0,0)`.
- Structural resampling uses the existing StrataField channel with bounded
  ping-pong storage. It does not create a new channel framework. A Fault before
  any StrataField deforms only the scalar field and exposes no fabricated
  structural identity.
- Bounded inspection may return selected scalar/structural samples and hashes
  after Strata and Fault stages. It must never read back a whole intermediate
  field.

## Recipe identity, dirty work and materials

- `FaultDisplace` is added to the admitted schema-8 payload route. Schemas 1–7
  reject it; existing schema-8 recipes remain byte-for-byte canonical.
- Canonical serialization and hashing cover kind/order/header, every payload
  field, full-width seed, generic mask and quality.
- Repeating identical input must preserve scalar and faulted structural hashes
  and report zero dirty scalar voxels/leaves.
- A bounded displacement, plane or roughness change must report non-empty but
  bounded dirty scalar voxels and `8x8x8` leaves.
- FaultDisplace preserves source material GUIDs and existing material channels.
  It implements no durability, weathering, fracture-cell or later consumer.
- CPU/GPU parity is measured at bounded represented voxel centers with the
  existing `1e-4 m` scalar tolerance and `1e-4` structural tolerance. No
  tolerance is weakened.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
