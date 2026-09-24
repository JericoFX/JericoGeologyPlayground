# M8 Geology Blueprint P0 PlaneCut — Frozen Conventions

**Slice:** `M8-GEOLOGY-BLUEPRINT-P0-FOUNDATION-PLANECUT`
**Authority:** `docs/JERICO_PROCEDURAL_GEOLOGY_RESEARCH_IMPLEMENTATION_BLUEPRINT.md`

These conventions bind the PlaneCut CPU reference, Diligent/Vulkan layer,
inspection output, mathematical tests and visible proof. They reuse current
Jerico behavior and do not define a second field framework.

## Scalar and composition

- Negative scalar value means inside solid material.
- Zero is the extracted surface.
- Positive scalar value means outside material.
- A normalized plane is
  `plane(p) = dot(p, normalWorld) - offsetMeters + roughness(p)`.
- The retained half-space is `dot(p, normalWorld) <= offsetMeters`.
- With full layer strength, PlaneCut is the existing negative-inside
  intersection: `max(source(p), plane(p))`.
- Header strength zero is exact identity. Strength in `(0, 1]` linearly blends
  from the source scalar to that intersection; strength one is the exact cut.
- PlaneCut produces a scalar implicit field. It is not claimed to remain an
  exact metric SDF after roughness or partial-strength blending.

## Coordinates, transforms and units

- The productive geology field currently evaluates BSP geometry and layers in
  world space. PlaneCut therefore names and stores `normalWorld` explicitly.
- `offsetMeters` and `roughnessMeters` are meters.
- `roughnessFrequencyPerMeter` is a three-axis cycles-per-meter vector; it is
  the blueprint's roughness frequency and anisotropic scale expressed without
  ambiguous unitless scale.
- `normalWorld` is normalized by both CPU and GPU evaluators. A non-finite or
  near-zero normal is invalid authoring data.
- The first PlaneCut schema admits only the identity `GeologyTransform`; world
  orientation is fully controlled by `normalWorld`. Transform support must not
  be silently different between CPU and GPU.

## Roughness and seed composition

- Roughness perturbs the plane; it never replaces the structural plane.
- `roughnessMeters == 0` disables noise exactly and must produce the analytic
  plane independent of frequency and seed.
- Non-zero roughness reuses the current layer-shader lattice value-noise
  formulation. The CPU reference mirrors that admitted formula only for
  semantic parity; it does not create a new noise framework.
- The sole authored stochastic identity is `GeologyLayerHeader::seed`.
- CPU and GPU fold the low/high 32-bit seed words with XOR before lattice
  sampling, matching the current layer path. No address, pointer, clock or
  `random_device` state participates.

## Material channels

- PlaneCut modifies only the scalar/density channel.
- Material identity, material class, hardness and erodibility are preserved from
  the input stage.
- The current Surface Nets path assigns the source GUID to generated triangles;
  the planar face follows that same rule. This slice does not invent a second
  material buffer or face-material policy.

## Voxel and finite-difference conventions

- One GPU leaf is exactly `8 x 8 x 8 == 512` voxels.
- A voxel center is `int3 voxelCoordinate * voxelSizeMeters`, matching the
  current source, layer and Surface Nets shaders.
- Linear local order is `x * 64 + y * 8 + z`.
- Requested inspection points are snapped to the nearest represented voxel
  center, and the returned sample reports that actual world position.
- Where later operators need gradients, central differences use one voxel as
  `h`; under negative-inside distance fields the normalized gradient points
  outward. PlaneCut itself uses the analytic plane normal and needs no finite
  difference.

## Intermediate inspection and parity

- Inspection is explicit and opt-in on `GeologyGpuEvaluationRequest`.
- A request may contain at most 64 finite world-space points inside the admitted
  topology bounds.
- The result records the source stage and every enabled, non-zero layer stage in
  recipe order: recipe layer index, stage name, GPU field hash and bounded scalar
  probes.
- Each probe records requested position, actual snapped voxel-center position,
  signed distance and inside/outside classification.
- Inspection copies only selected field words and the two-word stage hash. It
  never performs a hidden whole-field readback.
- PlaneCut CPU/GPU parity compares the GPU value at the returned snapped
  position against the CPU reference with the existing `1e-4 m` geology
  tolerance. Tolerances are not weakened to hide formula differences.

## Quality, identity and dirty reporting

- Recipe quality and voxel size remain owned by `GeologyQuality`.
- Recipe identity covers layer kind/order/header, PlaneCut payload, seed,
  quality and schema through canonical recipe serialization/hashing.
- Field identity is the real GPU field hash after each inspected stage.
- Dirty reporting remains the existing exact changed-voxel count plus changed
  8x8x8 leaf count. Repeating identical input must report zero dirty work.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
