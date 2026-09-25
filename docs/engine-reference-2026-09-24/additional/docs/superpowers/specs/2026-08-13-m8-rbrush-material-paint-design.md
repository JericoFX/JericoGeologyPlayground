# M8 RBrush Material Paint Design

**Status:** Accepted under the owner's standing 30-second autonomy rule and
self-reviewed against the published Slice 22 checkpoint on 2026-08-13.

## 1. Objective

Complete RBrush Slice 23 with one logical Material Paint layer whose single
RBrush coverage mask applies the selected channels from one admitted cooked PBR
material to one admitted target surface material.

The representative vertical selects:

```text
BaseColor + Roughness + Normal + MaterialHeight
```

The selected source material, target material, RBrush asset/preset, referenced
source textures and target textures retain separate stable identities. Material
Height remains a surface/material value and never mutates geometry, Sparse SDF,
collision or navigation.

## 2. Authorities and inherited contracts

- `GOAL.md` and `AGENTS.md`: deterministic, non-destructive, previewable,
  cancelable, transaction-safe, batch/cook capable and source-free authoring in
  the single authoritative checkout.
- `docs/superpowers/specs/2026-08-09-jerico-rbrush-complete-spec.md` sections
  47-54 and Slice 23: one brush mask drives the selected channels of a PBR
  material; channel semantics and Material Height/Geometry Height separation are
  mandatory.
- Slices 19-22: canonical `BrushDab`, World-owned Surface Paint recipe,
  immutable C04 worker snapshots, bounded `SurfacePages`, shared World viewport,
  transaction, multi-generation preview, `TextureCooker`, Vulkan replacement and
  source-free Shipping.
- Incremental Cook/Delta Epak: a stroke records authoring intent and bounded
  dirty metadata. It never performs a hidden full cook or package. Normal work is
  `Cook Changed -> Package Changed`; Full Clean Cook is validation/recovery.
- Virtualized World/JSVGI: Surface Paint reports bounded dirty surface regions;
  downstream residency, shadow and GI owners consume that contract later.

## 3. Alternatives and decision

### 3.1 One atomic Material Paint layer — selected

Persist one source material identity, one target material identity, one channel
mask and one ordered stroke set. The Brush Engine evaluates every stroke once;
all selected channels consume the same dabs and coverage. Validation and result
publication are atomic.

This exactly represents the authority's user concept and prevents selected PBR
channels from drifting apart.

### 3.2 Expand one action into hidden per-channel layers — rejected

This reuses the single-channel evaluator but duplicates strokes, seeds and layer
state. Undo, visibility, reimport and later editing could desynchronize channels,
so the stored authoring model would contradict “one brush coverage mask”.

### 3.3 Bake the source material into one composite payload — rejected

A private packed payload would erase PBR texture semantics, source identities,
color-space rules and dependency diagnostics. It would also create a second
material/texture pipeline instead of reusing `.rmat`, `.rtext` and the project
catalog.

## 4. Canonical schema 4 authoring contract

Schema 4 adds a logical material channel and stable channel bits:

```cpp
enum class SurfacePaintChannel : std::uint8_t
{
    Roughness = 1,
    MaterialHeight = 2,
    Normal = 3,
    Material = 4
};

enum class SurfacePaintMaterialChannel : std::uint16_t
{
    BaseColor       = 1U << 0U,
    Roughness       = 1U << 1U,
    Metallic        = 1U << 2U,
    Normal          = 1U << 3U,
    MaterialHeight  = 1U << 4U,
    AmbientOcclusion= 1U << 5U,
    Emissive        = 1U << 6U,
    Opacity         = 1U << 7U
};

struct SurfacePaintMaterialPayload
{
    Guid sourceMaterialGuid;
    std::uint64_t sourceMaterialContentHash;
    Guid targetMaterialGuid;
    std::uint64_t targetMaterialContentHash;
    SurfacePaintMaterialChannels selectedChannels;
};
```

A Material layer requires `SurfacePaintOperation::Replace`, zero scalar value,
no texture payload and exactly one material payload. The channel mask is nonzero
and contains only admitted bits. Source and target material GUIDs must be
different for the retained vertical; their hashes pin exact catalog generations.

Schema compatibility is strict:

- schemas 1-3 serialize byte-for-byte as before;
- schema 4 is emitted when a Material layer exists and may contain legacy
  Roughness, MaterialHeight and Normal layers in order;
- schemas 1-3 reject material payload keys;
- schema 4 rejects zero/stale identities, empty or unknown masks, a texture and
  material payload on one layer, non-Replace Material operation and unknown
  keys;
- files store no source image, `.abr`, import path or decoded raster bytes.

## 5. Resolved material snapshots

JericoEditor resolves source and target `ProjectMaterialAsset` rows through the
existing `MaterialProjectCatalog`. The worker receives immutable cooked material
values and only the exact cooked textures referenced by selected channels.

The Surface Paint evaluator consumes material-owned snapshots equivalent to:

```cpp
struct ResolvedSurfacePaintMaterialTexture
{
    RuntimeFormats::MaterialTextureSemantic binding;
    Guid textureGuid;
    std::uint64_t contentHash;
    RuntimeFormats::TextureSemantic semantic;
    RuntimeFormats::TextureColorSpace colorSpace;
    std::uint32_t width;
    std::uint32_t height;
    std::vector<std::byte> rgba8;
};

struct ResolvedSurfacePaintMaterial
{
    Guid materialGuid;
    std::uint64_t contentHash;
    RuntimeFormats::PbrMaterialData pbr;
    std::vector<ResolvedSurfacePaintMaterialTexture> textures;
};
```

The job checks the material GUID/hash, every selected binding, referenced texture
GUID/hash, texture semantic, color space and decoded first mip before evaluation.
It uses the existing bounded TextureCook decode path outside Qt. Missing or stale
selected input rejects the complete Preview. Both material snapshots require
valid canonical PBR data; an editor display card or loose texture collection is
not a material payload.

Source constants may supply BaseColor, Roughness, Metallic, Emissive and Opacity
when the source material lacks a corresponding texture. Normal, Height and AO
are selectable only when the source has an admitted reference. Every selected
channel requires a compatible target texture in Slice 23; creating missing
target slots is Material Studio authoring and is not performed silently by a
stroke.

## 6. Independent native resolutions

Material textures are not required to share dimensions. A new material
evaluation path inside the existing Surface Paint evaluator preserves each
target texture at native resolution rather than resampling all channels into the
legacy same-size `PbrSurfaceTile`.

The Brush Engine evaluates each stroke once. Its immutable dabs are then
rasterized against every selected target texture using the same continuous
world/UV projection, seed, tip coverage, opacity, flow and falloff. Per-texture
texel grids may differ, but the authored mask and world-space dirty bounds are
shared.

All source and target snapshots validate before output mutation. Evaluation
returns either the complete selected texture set or one error; partial channel
results are never published.

## 7. PBR channel semantics

Coverage is:

```text
coverage = RBrush tip * dab opacity * dab flow * source payload alpha where applicable
```

Selected channels apply Replace under that coverage:

- **BaseColor:** decode target/source sRGB RGB to linear, lerp, encode sRGB;
  preserve target alpha unless Opacity is selected.
- **Roughness:** linear lerp of the G component of the canonical ORM target.
- **Metallic:** linear lerp of the B component of ORM.
- **Ambient Occlusion:** linear lerp of ORM R; a separate admitted AO reference
  overrides the source material's packed R for this channel. The target may be
  ORM R or a dedicated admitted ScalarMask; grouping follows the target GUID.
- **Normal:** normalized tangent-space replacement using the proven Slice 22
  decode, lerp, normalize and quantize rules; preserve target alpha.
- **MaterialHeight:** linear scalar replacement in the target Height texture;
  never emits geometry/SDF dirtiness.
- **Emissive:** sRGB-to-linear RGB replacement and re-encode; preserve alpha.
- **Opacity:** linear alpha replacement from BaseColor texture/factor; preserve
  target RGB when BaseColor is not selected.

Unselected components and textures remain byte-exact. If Roughness, Metallic and
AO share one ORM target GUID, the evaluator mutates its selected components once
and the cooker emits one texture. BaseColor and Opacity behave the same way when
they share one target. A shared target GUID is legal only for compatible packed
components; the complete target binding map is validated before any mutation.

`changedTexels` is reported per target texture/channel and as a total changed
sample count. Dabs are retained once, not duplicated per channel. Cancellation
is checked between strokes, dabs, texture rows and cook units.

The intrinsic dependency remains exactly:

```text
DirtySpatialDependency::SurfacePages
```

## 8. Multi-output cook inside the canonical cooker

`SurfacePaintCooker` gains a material overload/request; no second cooker or
format is introduced. It evaluates the Material layer once, groups selected
channels by target texture GUID and invokes the existing `TextureCooker` once per
changed target:

```text
BaseColor/Opacity -> Color + sRGB
Roughness/Metallic/AO -> ORM or ScalarMask + Linear, matching target binding
Normal -> TangentNormal + Linear
MaterialHeight -> ScalarMask + Linear
Emissive -> Color + sRGB
```

Each output preserves its original target GUID, semantic, color space and native
dimensions. Source/target material identities, channel mask, every source/target
texture identity, World/recipe/RBrush hashes and quality participate in the
deterministic cook key. The result is sorted by target GUID and read back before
publication. One/four-worker output bytes must match.

The `.rmat` is unchanged because its references already point to the target GUIDs.
`Cook Changed` publishes only changed target `.rtext` assets; failure/cancel
publishes none. Package mutation remains a separate `Package Changed` operation.

## 9. Viewport and editor workflow

The shared World remains dominant. Picking returns geometry, entity, mesh,
material, basis and UV without requiring Height and Normal simultaneously. The
Material Paint owner resolves the selected target bindings from the hit material
and rejects unavailable channels before starting a stroke.

The inspector preserves the accepted brush-first hierarchy:

```text
PAINT · MATERIAL / PBR CHANNELS

BRUSH TIP · SHAPE / MASK
[ large circular tip ]

MATERIAL PAYLOAD
[ large material card + selector/View ]

CHANNELS
[x] Base Color  [x] Roughness  [x] Normal  [x] Height
[ ] Metallic    [ ] AO         [ ] Emissive[ ] Opacity

STROKE CONTROLS
Radius / Strength / Spacing / Falloff
```

The technical Slice 22 screenshot is backend evidence, not the UX target. UX
acceptance requires the tool to read immediately as a brush: a visible tip
library, one large live tip preview, a clear active-tip state, grouped Radius /
Falloff / Strength / Flow controls, a visually separate material card and
channel toggles, and a dominant unobstructed World viewport. Raw diagnostic
labels, a tiny anonymous circle or a texture card presented as the brush do not
pass this gate. The later repository-wide UX/UI pass may restyle the shell, but
this slice must already establish that information architecture instead of
leaving a known misleading layout for later.

Unavailable source/target channels are visible but disabled with an exact reason.
Changing material or mask cancels the old generation and clears stale preview.
The in-render cursor reads `MATERIAL · N CH` and uses the real surface hit.

Preview replaces every selected target Vulkan generation as one owner-thread
commit. If any replacement fails, already prepared replacements are discarded
and the previous complete generation remains active. Cancel restores all
baselines. Apply/Undo/Redo/reopen reuse the existing World + Surface Paint
transaction and exact schema-4 bytes. No second viewport or Material preview
scene substitutes for the World proof.

## 10. C04 and interaction budget

Material/texture catalog capture is brief and owner-thread only. Decode,
evaluation and cook run in the existing cancelable worker with immutable
snapshots, bounded workers/memory, progress by stage and late-generation discard.

The interactive gate remains:

```text
max Qt heartbeat gap <= 50 ms
late callback after owner destruction = false
cancel/shutdown bounded
```

No stroke invokes shader cook, texture cook, package, `.epak`, residency rebuild,
SDF, PWAS or Nav work. The existing observed `5 ms` gap is the baseline, not a
license to regress toward the 50 ms ceiling.

## 11. Shipping proof

The source-free proof uses an admitted multi-channel PolyHaven material as source
and a distinct cooked target material. It changes at least BaseColor, Roughness
and Normal in Shipping; the editor/world vertical additionally proves
MaterialHeight. The target `.rmat` remains byte-identical while its referenced
target `.rtext` generations change under their original GUIDs.

After package creation, delete SourceAssets, decoded/cooked authoring payloads,
Cache, Procedural recipes and EditorAssets. Two Vulkan replays must be
byte-identical. The package must not contain the source material payload when no
runtime asset references it, source PNG names, `.abr`, `.rbrush`, material-paint
recipe, stroke samples or import paths.

Full Clean Cook in this proof is a validation/recovery gate with measured timing.
It does not redefine the daily workflow or hide a full recook behind Apply.

## 12. Failure handling

Fail closed for:

- zero, identical, missing or stale source/target material identity;
- empty/unknown channel mask or unsupported layer/schema combination;
- selected source/target channel absent or semantically incompatible;
- duplicate/conflicting texture bindings or identity generations;
- wrong sRGB/Linear contract, malformed dimensions/mips or decode failure;
- target texture GUID reused with incompatible semantics;
- invalid projection, brush, target hit, bounds or no-effect result;
- cancellation, stale catalog/World/document generation or owner destruction;
- partial multi-texture Vulkan preparation, cook, readback or publication;
- any attempt to treat MaterialHeight as Geometry/SDF.

Errors identify material, channel and texture GUID and advise reselect/reimport or
create the missing target in Material Studio. They never choose another channel,
fall back to source files or publish a subset.

## 13. RED and acceptance

The first product RED compiles against the intended schema-4 Material layer and
must fail because that API does not yet exist. Slice 23 becomes `PASS` only when
fresh evidence on the published HEAD proves:

1. Schemas 1-3 remain exact; schema 4 round-trips mixed legacy/Material layers
   and rejects invalid identities, masks, operations and keys.
2. One Material layer retains one stroke/dab set while at least four selected
   channels change under identical coverage.
3. Targets with different native resolutions are evaluated without resizing or
   duplicated strokes.
4. BaseColor/Emissive use correct sRGB-linear conversion, scalars remain linear,
   Normal remains normalized and MaterialHeight never changes geometry planes or
   dirty dependencies.
5. Selected channels change; every unselected component/texture is byte-exact.
6. Missing/stale source or target material/texture generations reject atomically.
7. Dirty bounds are bounded and dependencies equal exactly `SurfacePages`.
8. C04 Preview/Cancel, Apply/Undo/Redo/reopen, owner destruction and late callback
   discard pass with a responsive Qt loop.
9. The real editor shows brush, material and channels as distinct concepts and
   the World visibly responds through all selected PBR targets. A reviewer can
   identify the active brush tip, radius/falloff/strength/flow and active material
   without consulting logs or implementation notes.
10. One/four-worker multi-output cook is deterministic, reader-valid and changes
    only selected target `.rtext` GUIDs while leaving `.rmat` bytes exact.
11. Vulkan runtime and source-free Shipping replay the cooked material paint
    twice with no authoring/source payload leak.

Checkboxes, a material card, duplicated hidden single-channel layers, CPU-only
bytes, debug tint, compile success or a concept image do not establish PASS.

## 14. Explicit boundaries

This slice does not create missing target material slots silently, add new scalar
operations beyond Replace for Material Paint, add non-UV projection, modify
`.rmat` references, implement Terrain, EPLS, SDF/Geometry stamps, decals,
instances, VT residency backend, JSVGI evaluation, new renderer lighting, M9 or
Penumbra-specific content. It establishes the atomic multi-channel Surface Paint
contract those later slices reuse.
