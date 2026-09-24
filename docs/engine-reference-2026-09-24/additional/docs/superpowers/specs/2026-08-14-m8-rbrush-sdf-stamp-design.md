# M8 RBrush Slice 28 — SDF Stamp Design

Date: `2026-08-14`

Status: accepted by the standing autonomous-decision rule after the versioned
RBrush authority fixed Slice 28 as the next legal capacity.

## Objective

Apply the ten RBrush SDF operations to a real immutable `SparseSdfField` and
return a validated derived field or mask result:

```text
Add
Subtract
SmoothAdd
SmoothSubtract
SurfaceDisplace
CrackCutter
ChipCutter
Smooth
Flatten
MaskOnly
```

This slice owns operation algebra, field mutation, ownership, protection,
determinism and cancellation. Slice 29 owns mapping a 2D mask to sample points
through Surface Tangent, Planar, Triplanar, Spherical, Cylindrical, Radial,
Object Local or World projection.

## Chosen boundary

`SdfBrushStamp` consumes a sorted canonical set of already projected stamp
points. Each point identifies one real sparse-field sample and carries only the
projection result required by the operation:

```text
brick coordinate
sample index 0..728
stamp signed distance in meters
influence 0..1
flatten target distance in meters
```

The request also carries stable RBrush provenance:

```text
RBrush asset GUID
preset GUID
RBrush pack content hash
Slice 27 mask derivation key
operation GUID
stamp region GUID when geometry may change
```

This seam is deliberate. Slice 28 can prove every operation against a real
field without inventing a temporary projection API. Slice 29 will produce the
same point set from the derived 2D field and a projection frame.

## Alternatives rejected

### Implement every projection inside Slice 28

Rejected because it collapses Slice 29 into this slice, expands the RED across
World picking and coordinate frames, and creates a monolithic stamp path.

### Test only a scalar `combine(a, b)` helper

Rejected because a passing arithmetic helper would not prove that a sparse
field changes, that protected samples are preserved, that region ownership is
valid, or that the result reopens through `SparseSdfField::create`.

## Public model

The authoring-only module lives in `Jerico::ProceduralAuthoring`:

```cpp
enum class SdfBrushOperation : std::uint8_t
{
    Add = 1U,
    Subtract = 2U,
    SmoothAdd = 3U,
    SmoothSubtract = 4U,
    SurfaceDisplace = 5U,
    CrackCutter = 6U,
    ChipCutter = 7U,
    Smooth = 8U,
    Flatten = 9U,
    MaskOnly = 10U
};

struct SdfBrushStampSettings
{
    SdfBrushOperation operation{SdfBrushOperation::Add};
    float smoothK{0.05F};
    float displacementMeters{0.05F};
};

struct SdfBrushStampProvenance
{
    Guid rbrushAssetGuid;
    Guid presetGuid;
    std::uint64_t rbrushPackContentHash{};
    std::uint64_t maskDerivationKey{};
};

struct SdfBrushStampPoint
{
    SdfBrickCoord brick;
    std::uint16_t sampleIndex{};
    float stampDistanceMeters{};
    float influence{1.0F};
    float flattenTargetDistanceMeters{};
};

struct SdfBrushStampMaskSample
{
    SdfBrickCoord brick;
    std::uint16_t sampleIndex{};
    float value{};
};

struct SdfBrushStampRequest
{
    const SparseSdfField& source;
    Guid operationGuid;
    Guid stampRegionGuid;
    SdfBrushStampProvenance provenance;
    SdfBrushStampSettings settings;
    std::span<const SdfBrushStampPoint> points;
};

struct SdfBrushStampResult
{
    SparseSdfField field;
    std::vector<SdfBrushStampMaskSample> mask;
    std::uint64_t changedSamples{};
    std::uint64_t operationHash{};
    std::uint64_t contentHash{};
};
```

The operation accepts at most `1,048,576` points. Input order is not semantic:
the implementation sorts a private copy by brick and sample index, and rejects
duplicates.

## Operation semantics

All calculations use the immutable source sample. Boolean candidates are
blended toward the source by `influence` and clamped to the source field's
narrow-band distance.

```text
Add              candidate = min(source, stamp)
Subtract         candidate = max(source, -stamp)
SmoothAdd        candidate = smoothMin(source, stamp, smoothK)
SmoothSubtract   candidate = smoothMax(source, -stamp, smoothK)
SurfaceDisplace  result    = source - displacementMeters * influence
CrackCutter      candidate = max(source, -stamp)
ChipCutter       candidate = max(source, -stamp)
Smooth           target    = average of source plus available six-axis neighbours
Flatten          target    = point.flattenTargetDistanceMeters
MaskOnly         field unchanged; emit the canonical positive influence values
```

`CrackCutter` and `ChipCutter` deliberately share Difference algebra in Slice
28 but retain distinct operation identity and hashes. Slice 29/30 differentiates
their projected/profiled operand shapes; the core must not hardcode one crack
shape.

Polynomial smooth minimum:

```cpp
h = max(k - abs(a - b), 0) / k;
smoothMin = min(a, b) - h * h * k * 0.25;
smoothMax(a, b, k) = -smoothMin(-a, -b, k);
```

Smooth samples the source field at exact `±cellSize` offsets through
`SparseSdfField::sampleDistance`, so brick boundaries use the existing field
query rather than a second neighbourhood implementation.

## Ownership and protection

The source field is never mutated.

For geometry-changing operations, each changed sample is assigned to
`stampRegionGuid`. If that region is new, the region table is re-sorted and all
existing owner indices are remapped before publication. A zero region GUID is
invalid for geometry operations; `MaskOnly` does not add a region.

Any positive-influence point whose source sample has `Locked` or `NoGenerate`
fails the complete request before mutation. Missing bricks, invalid sample
indices, duplicate points, non-finite values, invalid operation values and
missing provenance fail closed. No partial field is returned.

## Determinism and hashing

Canonical domain-separated hashes include explicit little-endian fields:

```text
operationHash = source content hash
              + operation/provenance/settings
              + sorted projected points

field content hash = source content hash
                   + operation hash
                   + changed sample keys/distances/owners

result content hash = field content hash
                    + sorted MaskOnly samples
```

Geometry results also derive a new recipe hash from the source recipe hash and
operation hash. `sourceHash` remains the original source lineage hash.
`MaskOnly` preserves the complete field hashes and bytes.

## Cancellation and errors

The function accepts `std::stop_token`, checks before allocation and during
validation, sorting/application and hash traversal, and never spawns work. A
future Editor owner must schedule it through the established C04 coordinator.

Stage:

```text
procedural.sdf.stamp
```

Stable error families:

```text
procedural.sdf.stamp_invalid
procedural.sdf.stamp_budget
procedural.sdf.stamp_protected
procedural.sdf.stamp_cancelled
```

## Verification

One focused test builds a valid sparse brick fixture and demonstrates every
operation changes the expected real sample or emits the expected mask. It also
proves:

- input field immutability;
- result reopening through `SparseSdfField::create`;
- order-independent bytes/hashes;
- changed-sample ownership and existing-owner remap;
- distinct operation hashes for Subtract, CrackCutter and ChipCutter;
- cross-brick-capable Smooth through the public field query;
- locked and NoGenerate rejection without partial output;
- duplicate/missing/budget/non-finite/provenance failures;
- pre-cancellation;
- neighbouring Sparse SDF/session/cook regressions;
- runtime-only and Shipping source exclusion.

## Explicit non-goals

Slice 28 does not add:

- any projection mode from Slice 29;
- 2D mask sampling or tangent-frame construction;
- World picking, Qt UI, editor overlay or job owner;
- crack/chip geometry showcase from Slice 30;
- DDC persistence;
- runtime or Shipping RBrush data;
- a second SDF field implementation.
