# M8 RBrush Slice 29 — SDF Projection Design

Date: `2026-08-14`

Status: accepted under the standing autonomous-decision rule after Slice 28
published its canonical `SdfBrushStampPoint` consumer.

## Objective

Map a validated RBrush-derived `MaskSignedDistanceField2D` onto bounded samples
of an immutable `SparseSdfField` through every projection required by the
RBrush authority:

```text
Surface Tangent
Planar
Triplanar
Spherical
Cylindrical
Radial
Object Local
World
```

The result is the sorted canonical point stream already consumed by
`applySdfBrushStamp`. Slice 29 owns projection, continuous mask sampling,
bounded sample enumeration and construction of an extruded stamp operand.
Slice 30 owns the uploaded-style crack workflow, actual visible crack result and
Undo/Redo proof.

## Chosen boundary

Add one authoring-only deep module:

```text
SdfBrushProjection
```

It consumes:

- one immutable, validated `SparseSdfField`;
- one validated `MaskSignedDistanceField2D`;
- one projection mode and metric settings;
- one orthonormal frame;
- one finite world-space affected bound; and
- an explicit candidate-brick inventory.

It produces:

- sorted `SdfBrushStampPoint` values;
- scanned/emitted sample counts; and
- one canonical projection hash.

It does not mutate or reopen the source field. The caller passes the returned
points to Slice 28 with the desired Boolean/material operation.

## Alternatives rejected

### Expose only `worldPosition -> uv`

Rejected because a coordinate helper would not prove that a real derived mask
enumerates a bounded sparse field or that Slice 28 can consume the result.

### Apply the stamp inside the projector

Rejected because projection and field mutation would become one monolith and
duplicate the accepted Slice 28 owner/protection/hash boundary.

### Wire viewport picking and Undo/Redo now

Rejected because those are the visible crack workflow in Slice 30. Slice 29 is
the deterministic, Qt-free producer that the future editor job will schedule.

## Public model

```cpp
enum class SdfProjectionMode : std::uint8_t
{
    SurfaceTangent = 1U,
    Planar = 2U,
    Triplanar = 3U,
    Spherical = 4U,
    Cylindrical = 5U,
    Radial = 6U,
    ObjectLocal = 7U,
    World = 8U
};

struct SdfBrushProjectionFrame
{
    Vec3d origin;
    Vec3d tangent;
    Vec3d bitangent;
    Vec3d normal;
};

struct SdfBrushProjectionSettings
{
    SdfProjectionMode mode{SdfProjectionMode::SurfaceTangent};
    double worldSizeMeters{1.0};
    double depthMeters{0.05};
    double normalOffsetMeters{};
    double rotationRadians{};
    double radialReferenceMeters{0.5};
    double triplanarSharpness{4.0};
    std::uint64_t maximumPoints{sdfBrushStampMaximumPoints};
};

struct SdfBrushProjectionRequest
{
    const SparseSdfField& source;
    const MaskSignedDistanceField2D& mask;
    SdfBrushProjectionFrame frame;
    Bounds3d affectedBounds;
    std::span<const SdfBrickCoord> candidateBricks;
    SdfBrushProjectionSettings settings;
};

struct SdfBrushProjectionResult
{
    std::vector<SdfBrushStampPoint> points;
    std::uint64_t scannedSamples{};
    std::uint64_t emittedSamples{};
    std::uint64_t projectionHash{};
};
```

The entry point is:

```cpp
Result<SdfBrushProjectionResult> projectSdfBrushToSparseField(
    const SdfBrushProjectionRequest& request,
    std::stop_token stopToken = {});
```

The hard point limit remains Slice 28's `1,048,576`. `maximumPoints` lets an
owner impose a lower per-job budget but can never exceed the hard limit.

## Canonical frame

The frame must be finite, unit length, mutually orthogonal and right-handed:

```text
cross(tangent, bitangent) == normal
```

The projector does not silently repair an invalid frame. Surface Tangent gets
this frame from an admitted surface pick later; Object Local gets the admitted
object transform; Planar gets an explicit authored plane.

Rotation is applied in the selected 2D projection plane before mask sampling.

## Metric mask rule

`worldSizeMeters` is the metric span of the mask's largest dimension. The
smaller dimension preserves source aspect ratio:

```text
texelMeters = worldSizeMeters / max(mask.width, mask.height)
widthMeters = mask.width  * texelMeters
heightMeters = mask.height * texelMeters
```

This keeps the Slice 27 Euclidean texel distance isotropic in world space.
Centered, non-periodic modes return no point outside this footprint.

Mask sampling is bilinear. Periodic axes wrap before interpolation; bounded
axes reject coordinates outside `[0,1]`. No second mask representation is
created.

## Projection semantics

Let frame-local coordinates be `(x,y,z)` and the sampled field distance be
`sourceDistance`.

### Surface Tangent

Centered `(x,y)` mapping through tangent/bitangent. Depth follows
`sourceDistance`, so the slab conforms to the current SDF surface rather than
remaining one infinite flat cutter.

### Planar

Centered `(x,y)` mapping. Depth is explicit plane coordinate `z`.

### Triplanar

Samples centered `YZ`, `XZ` and `XY` projections. Weights are the absolute
field-normal components in the canonical frame, raised to
`triplanarSharpness` and normalized. The three signed mask distances are
blended using those weights. Depth follows `sourceDistance`.

### Spherical

Uses equirectangular mapping in the canonical frame:

```text
u = atan2(z, x) / (2*pi) + 0.5   // wrapped
v = acos(clamp(y / radius)) / pi // bounded
```

Depth is `radius - radialReferenceMeters`. Metric texel scale is the conservative
minimum of equatorial and pole-to-pole texel spans.

### Cylindrical

The frame normal is the cylinder axis. `u` is wrapped azimuth and `v` is the
centered axial coordinate over `worldSizeMeters`. Depth is radial distance minus
`radialReferenceMeters`.

### Radial

Uses a polar mask on the frame plane: wrapped azimuth is `u`, normalized radius
`radius / radialReferenceMeters` is bounded `v`. Depth is explicit plane
coordinate `z`.

### Object Local

Uses the explicit object frame and the same centered non-periodic metric mapping
as Planar. Its distinct mode identity prevents an object-local stamp from being
silently reinterpreted after an object transform changes.

### World

Uses absolute world `X/Z` coordinates tiled by `worldSizeMeters`; both axes
wrap. Depth follows `sourceDistance`. It ignores frame origin/orientation for
mapping but the frame remains validated so one request shape is canonical.

## Extruded 3D operand

For projection depth coordinate `d`, the one-sided cutter interval is:

```text
[normalOffsetMeters - depthMeters, normalOffsetMeters]
```

Its slab signed distance is:

```text
slab = max(
    d - normalOffsetMeters,
    normalOffsetMeters - depthMeters - d)
```

The final projected operand is:

```text
stampDistanceMeters = max(maskDistanceMeters, slab)
```

Only samples whose operand is below the source narrow-band maximum are emitted;
the value is clamped to that band. This prevents irrelevant positive-influence
points from colliding with Slice 28 protection while retaining every sample
that can affect its Boolean.

## Enumeration, determinism and cancellation

Candidate bricks are copied, sorted and duplicate-checked. Missing bricks fail
closed. Samples are enumerated by sorted brick coordinate then sample index,
filtered by inclusive `affectedBounds`; output is therefore directly canonical
for Slice 28.

The projection hash domain-separates and includes:

- source/mask content hashes and mask derivation key;
- mode, frame, bounds and every setting;
- sorted candidate bricks; and
- every emitted brick/sample/distance/influence value.

Cancellation is checked before allocation and throughout validation,
enumeration, mask sampling and hash traversal. No partial result escapes.

Stable errors use stage `procedural.sdf.projection`:

```text
procedural.sdf.projection_invalid
procedural.sdf.projection_budget
procedural.sdf.projection_cancelled
```

## Verification

The focal test must:

1. project one real Slice 27 mask into a real sparse-field sample with every
   mode;
2. pass every result to Slice 28 and prove a real changed/reopened field;
3. prove mode-distinct projection hashes;
4. verify exact orientation, rotation, aspect, wrap and triplanar behavior with
   an asymmetric mask;
5. prove candidate order independence, bounds filtering and lower owner budget;
6. reject invalid mask/frame/mode/settings/bricks and duplicates; and
7. prove pre-cancellation and source immutability.

## Explicit non-goals

Slice 29 does not add:

- World picking, Qt UI, overlay or a C04 job owner;
- threshold/profile-gamma/depth authoring UI;
- uploaded ABR crack selection;
- application/transaction/Undo/Redo;
- visible crack/chip geometry evidence;
- DDC persistence;
- runtime or Shipping RBrush data; or
- Procedural 3D Layout.
