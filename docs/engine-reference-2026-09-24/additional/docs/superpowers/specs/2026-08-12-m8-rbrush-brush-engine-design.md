# M8 RBrush Brush Engine Design

Status: accepted continuation design under the RBrush master specification and
the user's autonomous-decision rule.

## Scope

This vertical implements RBrush Slice 19 as a shared, deterministic 3D Brush
Engine. It consumes an immutable stroke, normalized brush preset, tool
overrides, device state and a stored seed, then emits canonical `BrushDab`
values. The RBrush test canvas becomes a real consumer of this engine and no
longer owns dynamics or sampling math.

It does not paint a surface, derive a 2D SDF, modify a World, persist strokes,
cook brush data, introduce Virtual Texture pages or claim Surface Painter,
Slice 20, M8 or the master Goal complete.

## Architectural boundary

Create the Engine-layer target `JericoPaintingBrush` under
`Engine/Painting`. It depends only on `Jerico::Core` and C++20. Its public API
contains no Qt, filesystem, AssetPipeline, `.rbrush`, editor catalog, raster
image or runtime-renderer type.

`RBrushBrushAdapter` remains in JericoEditor. It converts the canonical
authoring IR already loaded by the admitted RBrush library into the neutral
Engine preset and converts canvas samples into 3D stroke samples. This is the
only RBrush-to-Brush-Engine mapping. Shipping never reads `.rbrush` and the
generic engine target contains no source-format parser.

Directly linking an Engine target to `Jerico::AssetPipeline` was rejected
because it would invert the authoring/runtime boundary. Keeping the evaluator
inside JericoEditor was rejected because Surface Painter, SDF, Terrain and
future tools would not share one canonical dab contract.

## Public data contract

The module uses namespace `Jerico::Painting` and owns a small `Vec3` value so
the contract does not depend on a renderer or procedural-tool math type.

```cpp
struct Vec3
{
    float x{};
    float y{};
    float z{};
};

struct BrushDeviceState
{
    float pressure{1.0F};
    float tiltX{};
    float tiltY{};
    float rotation{};
    float stylusWheel{};
};

struct BrushStrokeSample
{
    Vec3 position;
    Vec3 normal{0.0F, 0.0F, 1.0F};
    Vec3 tangent{1.0F, 0.0F, 0.0F};
    BrushDeviceState device;
};

struct BrushDab
{
    Vec3 position;
    Vec3 normal;
    Vec3 tangent;
    float radius{};
    float aspect{1.0F};
    float rotation{};
    float opacity{1.0F};
    float flow{1.0F};
    Guid tipGuid;
    std::uint64_t seed{};
};
```

`BrushPreset` contains preset GUID/revision, primary tip GUID, diameter,
normalized spacing, aspect/rotation, size/angle/aspect dynamics, scatter,
opacity/flow and an optional dual tip with its own shape/scatter values.
Response curves and control-source enums are neutral Engine values, not aliases
of RBrush IR types. The RBrush adapter uses the admitted pack content hash as
the revision input; candidate control values remain explicit preset fields, so
unapplied editor previews are deterministic without inventing a persisted
revision.

`BrushOverrides` contains the authority fields: optional size, spacing,
opacity, flow, scatter, size jitter, rotation jitter and invert. `size` is a
diameter in the caller's coordinate unit; output `radius` is half that value.
Spacing and scatter are multiples of effective diameter. Invert remains a
stroke/tool semantic consumed alongside the returned dabs; it does not alter
dab geometry and is retained in `BrushStrokeResult::invert`.

## Sampling and 3D frame

`BrushSampler` walks the polyline by accumulated 3D arc length. Primary and
enabled dual tips each use their effective spacing and therefore produce
independent event streams. Events are merged by path distance; primary sorts
before secondary at an equal distance.

Position and device channels interpolate linearly. Normal and tangent are
interpolated, normalized and orthogonalized. Binormal is `normal x tangent`.
Scatter displaces across binormal and, when `bothAxes` is true, along tangent.
The output frame remains finite, normalized and right-handed.

One-sample strokes emit one event. Repeated positions do not create duplicate
distance. Spacing has a positive lower bound so zero/very small imported values
cannot create an unbounded dab stream.

## Dynamics and pressure

Control sources are Off, Random, Fade, Pen Pressure, Pen Tilt, Stylus Wheel,
Pen Rotation, Direction and Initial Direction. Unknown controls fail
validation instead of silently behaving as Off.

Response curves use clamped piecewise-linear interpolation. Size, angle and
aspect combine control, curve, minimum and jitter. Opacity and flow combine
their base value, control, curve, minimum and jitter. Tool overrides replace
the corresponding base value for the stroke without mutating or cloning the
preset.

Scatter count is evaluated per event. Count control first maps the admitted
base count into `[1, baseCount]`; count jitter then deterministically reduces
that result but never below one. The same rule applies independently to dual
brush scatter.

## Determinism

All random values derive from a stored 64-bit root seed. The engine uses a
specified SplitMix64-style mixer and never exposes implementation RNG state.

The hierarchy is:

```text
root seed + preset GUID + preset revision
  -> event seed(path distance ordinal, primary/dual branch)
  -> dab seed(scatter ordinal)
  -> property value(fixed property slot)
```

Each `BrushDab::seed` is the derived dab seed. Property slots are fixed for
count jitter, size, angle, aspect and both scatter axes. Adding a future random
property cannot shift existing results. Given identical preset revision,
stroke samples, overrides and root seed, the ordered dabs compare exactly.
Changing the seed changes at least one random-dependent field while preserving
all validity invariants.

## Validation, cancellation and bounds

The public entry point is:

```cpp
Result<BrushStrokeResult> evaluateBrushStroke(
    const BrushEngineRequest& request,
    std::stop_token stopToken = {});
```

It validates before allocating output:

- `1..4096` finite stroke samples;
- non-zero preset and tip GUIDs;
- finite, non-degenerate normal/tangent frames;
- positive finite diameter and spacing;
- normalized aspect, jitter, opacity, flow and device channels;
- scatter/count and response-curve limits;
- finite, in-range overrides;
- maximum `16384` emitted dabs.

Cancellation is checked while sampling and emitting. Failure uses structured
`painting.brush.*` errors. The engine is synchronous and bounded for live input;
future batch painters may invoke it from jobs without changing this contract.

## Canvas migration

`RBrushTestCanvasModel` retains only:

- RBrush IR to neutral `BrushPreset` adaptation through
  `RBrushBrushAdapter`;
- 2D canvas samples to XY-plane 3D stroke conversion;
- R8 tip lookup and RGBA rasterization of returned canonical dabs.

Its preview PRNG, response-curve evaluation, spacing walker, scatter and
dynamics functions are removed. Canvas telemetry displays canonical dab count
and seed information. Existing mouse/tablet, Demo/Clear, zero-write and visual
tests remain green against the shared Engine output.

## Observable acceptance

Pure Engine tests use literal expected dabs and prove:

- identical request yields byte/equality-identical ordered dabs;
- different root seed changes random-dependent output;
- spacing changes event count and exact positions;
- primary and dual scatter/count are deterministic;
- size/angle/aspect jitter and pressure curves affect the documented fields;
- overrides change one stroke without changing the preset;
- 3D frames, bounds, invalid data, cancellation and dab-limit errors work.

The real Windows canvas must still make spacing, scatter, jitter and pressure
visible, now using `JericoPaintingBrush`. Debug/Release Engine and RBrush
matrices, target-boundary gates and source-free Shipping discovery must pass.

PASS closes only RBrush Slice 19. It does not claim paint tiles, channel writes,
undo/redo for surface painting, SDF stamping, renderer integration, M8 or the
JericoEngine 1.0 Goal complete.
