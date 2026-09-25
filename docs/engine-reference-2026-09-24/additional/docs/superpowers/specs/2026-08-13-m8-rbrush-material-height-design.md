# M8 RBrush Material Height Design

**Status:** Accepted under the owner's standing autonomy rule on 2026-08-13.

## 1. Objective

Complete RBrush Slice 21 by extending the admitted Surface Painter vertical to
the linear `Material.Height` channel. The same canonical RBrush dab stream and
non-destructive World recipe used by Slice 20 must paint a deterministic height
mask, support Preview/Cancel/Apply/Undo/Redo, cook through the existing texture
cooker, and drive the already admitted M6F Height renderer in source-free
Shipping.

The observable separation is mandatory:

```text
Material.Height -> visual Parallax or MicroDisplacement
Geometry/SDF    -> editable structural shape
```

Painting `Material.Height` must not change SDF/BSP/Terrain geometry, collision,
navigation, PWAS/EPLS topology or World-cell topology.

## 2. Authorities and inherited contracts

- `GOAL.md` and `AGENTS.md`: deterministic, previewable, cancelable,
  transaction-safe and source-free authoring.
- `docs/superpowers/specs/2026-08-09-jerico-rbrush-complete-spec.md`, sections
  47-54 and Slice 21: Height is linear, material Height is distinct from
  Geometry/SDF, and Slice 21 must paint the former without changing the latter.
- `docs/superpowers/specs/2026-08-02-m6f-height-displacement-design.md`: Height
  uses a Linear `ScalarMask` `.rtext`, midpoint `0.5`, POM as the preferred
  visual mode, and MicroDisplacement as render-only vertex displacement.
- Slice 20 Surface Painter: one World-owned recipe, canonical Brush Engine dabs,
  immutable evaluation, project transaction, C04 job and `TextureCooker` path.
- Virtualized World and JSVGI: a bounded edit emits one Jerico-owned dirty
  region; RBrush records intent and never edits residency, shadow or GI storage
  directly.

## 3. Alternatives and decision

### 3.1 Extend the existing SurfacePaint vertical by channel — selected

The canonical recipe admits `MaterialHeight`, the evaluator changes the existing
R16 height plane, and the existing cooker emits a separate Linear ScalarMask
`.rtext`. This reuses spacing, dynamics, transactions, cancellation and texture
cook while preserving the correct runtime format.

### 3.2 Add a Height-specific painter/cooker — rejected

It duplicates recipe ownership, rasterization, scheduling and cancellation, and
would let Roughness and Height diverge for the same stroke.

### 3.3 Pack Height into ORM — rejected

Jerico's ORM alpha is not the Height contract. Reusing it would break existing
materials, compression semantics and the M6F runtime binding.

## 4. Canonical authoring contract

`SurfacePaintChannel` gains the stable value:

```cpp
MaterialHeight = 2U
```

Slice 21 continues to admit only `SurfacePaintScalarOperation::Add`. `value` is
finite in `[-1, 1]`; positive values raise normalized material height and
negative values lower it. Brush coverage, opacity, flow, pressure, invert,
layer/stroke order, visibility, locking, limits and projection retain the Slice
20 contracts.

Recipe compatibility is explicit:

- schema 1 remains Roughness-only and serializes byte-for-byte as before;
- schema 2 admits Roughness and MaterialHeight;
- a writer emits schema 2 when at least one MaterialHeight layer exists;
- a schema-1 reader rejects a Height layer rather than interpreting it;
- a schema-2 recipe can contain both channels and preserves their order;
- no derived raster bytes or source texture path enter the recipe.

This is a format admission, not a second recipe or World facet.

## 5. Deterministic evaluation

`PbrSurfaceTile::heightR16` is the canonical authoring plane. Each texel is one
little-endian normalized unsigned 16-bit value. Neutral Height is `32768`, the
nearest R16 representation of the M6F midpoint `0.5`.

For MaterialHeight layers the evaluator applies:

```text
height' = clamp(height + value * coverage, 0, 1)
```

and quantizes with deterministic round-to-nearest to `[0, 65535]`. It changes
only the two Height bytes of covered texels. BaseColor, Normal, all four ORM
bytes, Emissive and Opacity remain byte-exact. Roughness layers continue to
change only ORM.G. A mixed recipe evaluates once in authoritative layer order,
with each layer writing only its declared plane.

The evaluator reports the union of changed texels and dabs. Repeats and
different admitted caller worker counts must produce identical tiles and dirty
bounds.

## 6. Dirty-region and structural-separation contract

The intrinsic dependency mask for a MaterialHeight texture edit is:

```text
DirtySpatialDependency::SurfacePages
```

It never contains `GeometryPages`, `Collision`, `Navigation` or
`WorldCellDependencies`. This is what proves the paint operation did not become
an SDF/structural edit.

Shadow and JSVGI invalidation are consumer-dependent: an admitted `.rmat` using
MicroDisplacement may fan the changed Height resource out to `ShadowPages` or
`GiBricks` according to its shadow/GI participation. That fanout belongs to the
canonical dependency graph and must not be guessed by the Qt painter. Slice 21
therefore retains the exact intrinsic `SurfacePages` mask and records the Height
texture GUID so later consumers can answer why their derived cache is dirty.

## 7. Cooked output

`SurfacePaintCooker` remains the only adapter. Its request selects one admitted
output target:

```cpp
enum class SurfacePaintCookTarget : std::uint8_t
{
    RoughnessOrm = 1U,
    MaterialHeight = 2U
};
```

For `RoughnessOrm`, existing behavior and bytes remain unchanged. For
`MaterialHeight`, the cooker:

1. evaluates the immutable recipe through the same evaluator;
2. converts each R16 value to canonical RGBA8 by deterministic rounded R16-to-R8
   reduction in R, replicating R to RGB and writing A=255;
3. calls `TextureCooker` with `TextureSemantic::ScalarMask`,
   `TextureColorSpace::Linear`, and the target Height GUID;
4. preserves the existing desktop BC4/RGBA8 admission and M6F reader contract;
5. includes target, GUID and exact Height plane in source/recipe identities;
6. validates readback before publishing a result.

The cooker never writes files or packages. Incremental Cook may schedule only
the output whose dependency key changed; there is no hidden full cook and no
parallel texture-cook implementation.

## 8. Editor and transaction flow

The Surface Paint workspace changes its channel control from a fixed Roughness
label to the two admitted choices `Roughness` and `Material Height`. Selecting
Height changes the layer channel and the preview legend, not the active World,
brush or transaction path.

Height preview must make the channel legible: the before/preview surface uses
the painted Height plane to perturb the displayed surface lighting/relief while
also exposing an optional grayscale Height inspection. It must not claim that
the CPU canvas is the Vulkan runtime proof. The retained evidence includes both
the editor paint result and a real Vulkan POM/MicroDisplacement replay.

Preview freezes all Slice 20 identities plus the selected channel and Height
output identity. Preview and Cancel write nothing. Apply, Undo, Redo and reopen
use the existing two-root ProjectAsset transaction and reproduce exact schema-2
recipe/World bytes. Evaluation remains off the Qt owner thread; generation
fencing, cancellation, owner destruction and late callback rules remain
unchanged.

## 9. Runtime and Shipping proof

The first runtime proof reuses the M6F `.rmat` 1.3 contract and renderer rather
than adding a new material path. It packages:

- one SurfacePaint-produced Height `ScalarMask` `.rtext`;
- one `.rmat` referencing that exact GUID in Parallax mode;
- one admitted `.rmesh`/scene surface.

The source-free runtime must visibly differ from the neutral Height baseline.
A second MicroDisplacement replay demonstrates silhouette movement while exact
SDF/BSP/Terrain, collision and navigation fixture hashes remain unchanged. The
package contains no `.rbrush`, `.abr`, SurfacePaint recipe, stroke samples,
source image or importer.

## 10. Failure handling

Fail closed for:

- unknown channel/operation or Height inside schema 1;
- malformed/odd-sized R16 data or invalid tile dimensions;
- non-finite values, invalid projection, empty coverage or cancellation;
- missing/zero Height output GUID;
- a Height target producing anything except Linear ScalarMask;
- stale World, recipe, RBrush, base tile, material or project generation;
- any attempted partial publication after evaluation/cook/readback failure.

## 11. Acceptance

Slice 21 is `PASS` only when fresh evidence on the published HEAD proves:

1. Schema-1 Roughness bytes remain unchanged and schema 2 round-trips mixed
   Roughness/MaterialHeight layers strictly.
2. One admitted stroke changes only `heightR16`; every non-Height plane is
   byte-exact, with positive/negative clamp and deterministic repeats.
3. Roughness regression still changes only ORM.G.
4. The dirty region is bounded and exactly `SurfacePages`; geometry, SDF,
   collision and navigation fixture identities remain unchanged.
5. Preview/Cancel write nothing; Apply/Undo/Redo/reopen restore exact canonical
   schema-2 World/recipe bytes.
6. One/four-worker cooks emit identical reader-valid Linear ScalarMask `.rtext`
   bytes through the existing TextureCooker.
7. The Qt thread stays responsive; Cancel and owner destruction discard late
   callbacks.
8. A retained real editor capture visibly identifies Material Height and shows
   its painted relief/diagnostics.
9. Real Vulkan Parallax and MicroDisplacement consume the painted texture; the
   latter changes render silhouette without changing structural fixture hashes.
10. Debug/Release focal and integration gates plus source-free Shipping replay
    pass; packages contain no authoring/source payload.

A selectable combo box, changed R16 bytes, a compile-only shader or concept art
does not establish PASS.

## 12. Explicit boundaries

This slice does not add Geometry/SDF sculpting, terrain structural Height,
additional scalar blend modes, texture-payload painting, material multi-channel
painting, VT residency, JSVGI evaluation, new shadows or final RBrush UX polish.
It establishes the correct authored data and invalidation contracts those later
systems consume.
