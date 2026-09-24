# M8 Geology Blueprint P0 AnisotropicVoronoi — Frozen Conventions

**Slice:** `M8-GEOLOGY-BLUEPRINT-P0-ANISOTROPIC-VORONOI`
**Authority:** `docs/JERICO_PROCEDURAL_GEOLOGY_RESEARCH_IMPLEMENTATION_BLUEPRINT.md`

These conventions bind the reusable `AnisotropicVoronoiCells` CPU reference,
schema-8 recipe, Diligent/Vulkan structural primitive, bounded inspection,
mathematical tests and visible proof. They do not define a generator, another
lattice/noise/field framework or a later fracture/cavity consumer.

## Sites, seed and ownership

- The payload owns an explicit ordered array of `2..64` world-space sites. It
  has no density/count generator and consumes no hidden lattice.
- Site positions are meters in world space. Duplicate positions after canonical
  Vulkan `float` packing are rejected, so nearest and second always refer to
  distinct spatial sites.
- Each site owns a local-to-world unit quaternion, positive dimensionless metric
  weights `(alpha, beta, gamma)` and a positive dimensionless relative scale
  `g`.
- The full-width layer seed folds as `low32 XOR high32`. For payload index `i`
  and site count `N`, the stable non-background cell ID is:

  ```text
  id(i) = 1 + ((i + fold64(seed)) mod N)
  ```

  ID `0` is reserved for background. No clock, pointer, random device or
  external state participates.

## Anisotropic metric and orientation

For world sample `p`, site position `s`, normalized local-to-world quaternion
`q`, local coordinate `v = inverse(q) * (p - s)`, positive metric weights
`(alpha, beta, gamma)` and positive relative scale `g`:

```text
d2(p, site) = (alpha*v.x^2 + beta*v.y^2 + gamma*v.z^2) / g
```

- `d2` is a non-negative squared-metre metric. The weights and `g` are
  dimensionless; they never silently become axes, radii or frequencies.
- Each represented `weight / g` ratio must remain finite and normal in Vulkan
  `float`. Evaluation also proves that every admitted site-to-bounded-field
  offset and its conservative maximum metric remain below the finite structural
  sentinel before dispatch. Invalid authoring fails rather than changing
  ownership through overflow.
- Quaternion orientation rotates the anisotropic local frame. Scaling or
  otherwise using a non-unit authored quaternion is rejected; CPU and GPU both
  normalize the admitted value before evaluation.
- The first admitted route requires identity `GeologyTransform`; site positions
  and frames are already explicit in world space.

## Nearest, second, boundary and ties

- `nearest` is the minimum metric over all sites. Exact metric ties are resolved
  by the lower stable cell ID.
- `second` is the minimum metric from a distinct second site after the same
  deterministic ordering.
- The structural boundary metric is:

  ```text
  boundary = second - nearest
  ```

  Therefore `nearest <= second`, `0 <= boundary <= second`. A zero boundary is
  the exact anisotropic bisector under the represented metrics.
- This is not isotropic Worley noise and not cosmetic texture. Cell identity,
  nearest, second and boundary are reusable structural outputs.

## Mask, background and representation

- The existing layer-strength and regional-mask contract composes separately:

  ```text
  structuralMask(p) = clamp(strength, 0, 1)
                    * clamp(regionalMaskWeight(p), 0, 1)
  ```

  Masking does not move sites, rotate frames, change weights or alter ownership.
- At zero composed mask, the published background is cell ID `0`, nearest and
  second equal to the finite sentinel `FLT_MAX / 4`, boundary `0` and mask `0`.
  No infinity or NaN is exposed.
- CPU and shader references defensively saturate a non-representable raw metric
  to that same finite sentinel. Saturated ties still choose the lower stable ID,
  publish equal nearest/second metrics and boundary `0`; productive recipes are
  rejected earlier by validation or bounded-topology preflight.
- The productive Vulkan route stores one dedicated 16-byte structural value per
  active voxel: cell ID bit pattern plus nearest, second and boundary `float`
  metrics. The composed mask is stored separately.
- The primitive preserves the scalar SDF, extracted topology and source material
  GUIDs. It neither aliases the existing StrataField nor adds material channels.
- Bounded inspection decodes selected values and stage hashes only; it never
  reads back a whole structural field.

## Schema, dirty work and scope

- `AnisotropicVoronoiCells` uses the published schema 8. Schemas 1–7 reject it.
  Canonical serialization and hashing cover ordered cells, positions,
  quaternions, weights, scales, full-width seed, strength, mask and layer order.
- Every packed scalar must be finite and preserved in the productive Vulkan
  `float` domain. Weights and relative scale must remain strictly positive, and
  each represented `weight / relativeScale` ratio must remain finite and normal.
- Repeated identical evaluation preserves scalar and structural hashes and
  reports zero dirty work. Structural cell/metric/mask changes participate in
  exact voxel comparison and the existing `8x8x8` dirty-leaf aggregation.
- This slice implements no `FractureCells`, `CavityCarve`, weathering consumer,
  P1/P2/M9/Task23 work, VDB route, removed UI or replacement framework.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
