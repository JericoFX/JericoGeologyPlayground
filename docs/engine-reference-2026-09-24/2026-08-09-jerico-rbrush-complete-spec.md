# JericoEngine — RBrush Complete Specification
## Native Brush Asset, ABR/Image Import, Brush Packs, Surface Paint, SDF and Rock Generation

**Status:** REQUIRED IMPLEMENTATION SPEC<br>
**Scope:** JericoEngine + JericoEditor<br>
**Native brush format:** `.rbrush`<br>
**External brush sources:** `.abr`, `.png`, `.tga`, `.tif/.tiff`, `.bmp`, `.jpg/.jpeg`, `.exr`<br>
**Shipping policy:** external sources NEVER ship; Editor `.rbrush` NEVER ships by default; static results are baked/cooked.<br>
**Implementation policy:** Everything described in this document is in current scope. Do not defer features as “future work”.

---

# 0. Core decision

JericoEngine must have its own native brush system.

The native concept is:

```text
RBRUSH
```

Photoshop `.abr` is only one import source.

Images are also first-class brush sources.

```text
                    EXTERNAL SOURCES
                           │
       ┌───────────────────┼────────────────────┐
       │                   │                    │
       ▼                   ▼                    ▼
     .ABR              IMAGE FILES        GENERATED MASKS
                         PNG/TGA/
                       TIFF/BMP/JPG/
                           EXR
       │                   │                    │
       └───────────────────┼────────────────────┘
                           ▼
                 Brush Source Pipeline
                           ▼
                 Canonical Brush IR
                           ▼
                        .rbrush
                           │
          ┌────────────────┼─────────────────────┐
          ▼                ▼                     ▼
   Surface Painter      SDF Tools          Procedural Systems
          │                │                     │
   PBR channels       Rocks/cracks          EPLS/terrain/
   materials          chips/height          decals/instances
          │                │                     │
          └────────────────┼─────────────────────┘
                           ▼
                    World Authoring
                           ▼
                         Cook
                           ▼
                    .epak Shipping

NO .abr
NO source PNG/TIFF/etc used only as brush source
NO Editor .rbrush
```

---

# 1. Architectural rule

Keep these concepts separate:

```text
BRUSH
    HOW and WHERE coverage is generated.

PAYLOAD
    WHAT is applied.

TARGET
    WHERE the result goes.
```

Examples:

```text
Crack RBrush
+ Roughness +0.25
+ Mesh Surface
```

```text
Crack RBrush
+ BrokenConcrete material
+ Mesh Surface
```

```text
Crack RBrush
+ Subtract 0.03 m
+ Rock SDF
```

```text
Scatter RBrush
+ Moss Include
+ EPLS Mask
```

```text
Rock RBrush
+ Rock prefab set
+ Instance Scatter
```

RBrush must never be hard-coded as “a decal”.

A decal is only one possible target/output.

---

# 2. Goals

The system MUST:

1. Import `.abr`.
2. Import common image formats.
3. Import one image or many images at once.
4. Create a new `.rbrush` pack from selected images.
5. Add selected images to an existing `.rbrush` pack.
6. Extract selected presets from one `.rbrush` into another `.rbrush`.
7. Extract multiple brushes from packed image channels.
8. Allow the user to select alpha, luminance, R, G, B, A or combinations as brush coverage.
9. Create Jerico brush dynamics for image-derived brushes.
10. Edit imported ABR/image brush parameters inside JericoEditor.
11. Preserve stable preset GUIDs.
12. Detect duplicate sources and duplicate canonical tips.
13. Update existing brushes without breaking World references.
14. Support brush groups, folders, tags and search metadata.
15. Use RBrush in Surface Painter.
16. Use RBrush for scalar PBR painting.
17. Use RBrush for textures.
18. Use RBrush for full materials.
19. Use RBrush for masks.
20. Use RBrush for terrain.
21. Use RBrush for EPLS inclusion/exclusion.
22. Use RBrush for decal distribution.
23. Use RBrush for generic instance distribution.
24. Use RBrush for SDF sculpting.
25. Use RBrush for SDF rock generation.
26. Use RBrush for cracks, chips, erosion masks and surface breakup.
27. Derive 2D SDF representations from mask brushes.
28. Support deterministic procedural brush sets.
29. Compress `.rbrush` strongly and losslessly by default.
30. Never require the imported ABR/image source again after Embed/Consume import.
31. Never ship `.abr`.
32. Never ship consumed source images.
33. Never ship Editor `.rbrush` for static content.
34. Validate Shipping so Editor-only brush dependencies cannot leak into `.epak`.
35. Preserve known Photoshop dynamics.
36. Preserve unknown Photoshop descriptor properties.
37. Preserve unknown ABR sections when safe boundaries are known.
38. Provide complete import/update reports.
39. Be robust against malformed external files.
40. Be fuzz-tested and sanitizer-tested.

---

# 3. RBrush is a pack format

A single `.rbrush` can contain one or many presets.

Example:

```text
Rock_Damage.rbrush

Cracks/
    crack_01
    crack_02
    crack_03

Chips/
    chip_01
    chip_02

Surface/
    pits_01
    strata_01
```

These folders are metadata inside the pack.

Each preset has:

```text
PresetGUID
name
group/path
tags
tip reference
brush settings
source metadata
```

Worlds reference:

```text
RBrushAssetGUID + PresetGUID
```

Never:

```text
filename + list index
```

---

# 4. External source types

Supported now:

```text
Adobe:
    .abr

Images:
    .png
    .tga
    .tif
    .tiff
    .bmp
    .jpg
    .jpeg
    .exr
```

All of these must route into the same canonical Brush IR.

Architecture:

```cpp
class IBrushSourceImporter
{
public:
    virtual ~IBrushSourceImporter() = default;

    virtual bool CanImport(const BrushSourceFile&) const = 0;

    virtual BrushImportResult Import(
        const BrushImportRequest&) = 0;
};
```

Implement now:

```cpp
class AbrBrushImporter final : public IBrushSourceImporter;
class ImageBrushImporter final : public IBrushSourceImporter;
```

---

# 5. Folder structure

Engine-wide brush inbox/library:

```text
Engine/
    Import/
        Brushes/
            Inbox/

    EditorAssets/
        Brushes/
```

Project-local:

```text
Project/
    Import/
        Brushes/
            Inbox/

    EditorAssets/
        Brushes/
```

The exact physical root may integrate into Jerico Asset Registry, but the logical roles are mandatory:

```text
Import/Brushes/Inbox
    external consumable inputs

EditorAssets/Brushes
    canonical RBrush assets
```

---

# 6. Editor entry points

Brush Library toolbar:

```text
[ Import Sources... ]
[ Import ABR... ]
[ Import Images... ]
[ Scan Inbox ]
[ Create Empty Pack ]
[ Add To Pack ]
[ Extract To New Pack ]
[ Update ]
[ Reimport Selected ]
```

All are current-scope features.

---

# 7. Import destinations

When importing one or many source files:

```text
DESTINATION

● Create New RBrush Pack
  Name: [ Rock_Damage ]

○ Add To Existing RBrush Pack
  Pack: [ ____________________ ▼ ]
```

When selecting presets already inside Jerico:

```text
[ Extract To New RBrush Pack ]
```

Example:

```text
Free_Cracks.rbrush
    121
    122
    123
    ...
```

Select:

```text
121
124
128
132
```

Result:

```text
Favorite_Cracks.rbrush
    121
    124
    128
    132
```

The new pack gets its own AssetGUID and each copied preset gets a new PresetGUID.

---

# 8. Source handling modes

Source handling must be explicit.

```cpp
enum class BrushSourceRetention
{
    EmbedAndConsume,
    EmbedAndKeep,
    ReferenceAndKeep
};
```

## 8.1 EmbedAndConsume

Use for:

```text
Import/Brushes/Inbox
```

Jerico:

```text
reads source
embeds canonical data
verifies RBrush
deletes source
```

This is the intended ABR workflow.

## 8.2 EmbedAndKeep

Use when the user wants a self-contained RBrush but does not want Jerico to delete the source.

## 8.3 ReferenceAndKeep

Allowed only for existing Project/Engine image assets.

Example:

```text
Project/Assets/Textures/Rocks/crack.png
```

Jerico must NEVER automatically delete an existing project asset.

Reference mode keeps the external asset dependency inside Editor authoring.

Cook still resolves it and excludes it when it was used only as authoring input.

### ABR rule

ABR must always become embedded canonical RBrush data.

Never create:

```text
RBrush -> requires .abr at runtime/editor reopen
```

---

# 9. Transactional consume

A consumed source is deleted only after verification.

Required sequence:

```text
1. Open source.
2. Hash source.
3. Parse/decode completely.
4. Validate dimensions/types/data.
5. Build canonical Brush IR.
6. Compress RBrush chunks.
7. Write <name>.rbrush.tmp.
8. Flush.
9. Reopen using normal RBrush reader.
10. Validate header/TOC/chunks.
11. Validate per-tip canonical hashes.
12. Validate preset references.
13. Atomically replace final .rbrush.
14. Register/refresh Asset Registry.
15. Reopen from Asset Registry.
16. Only now consume/delete source if policy requires it.
```

On failure:

```text
source remains
existing RBrush remains
temporary output is removed
report records failure
```

---

# 10. Source manifests and duplicate detection

Every RBrush stores:

```cpp
struct RBrushImportManifest
{
    Guid        assetGuid;

    std::vector<RBrushSourceRecord> sources;

    uint32_t importerVersion;
};
```

Source record:

```cpp
struct RBrushSourceRecord
{
    std::string logicalName;

    BrushSourceType type;

    Sha256 sourceHash;

    BrushSourceRetention retention;

    uint16_t abrVersion;
    uint16_t abrSubversion;

    std::vector<RBrushSourcePresetRecord> presets;
};
```

Per preset:

```cpp
struct RBrushSourcePresetRecord
{
    Guid presetGuid;

    std::string sourcePresetName;

    std::string sourceInternalId;

    Sha256 canonicalTipHash;

    Sha256 semanticSettingsHash;
};
```

Exact source hash already imported:

```text
ALREADY IMPORTED
```

Do not duplicate.

---

# 11. Update behavior

Matching priority:

```text
1. exact internal source ID / sampledData UUID
2. source-specific stable ID
3. source logical identity + stable semantic name
4. canonical tip hash
5. prior stable index only as last fallback
```

Existing Jerico PresetGUID MUST survive updates.

Added source preset:

```text
create new PresetGUID
```

Removed source preset:

```text
SourceMissing = true
```

Do not silently delete it.

If referenced by World or procedural tools, keep it.

---

# 12. ABR fixture inspected

Reference local test file:

```text
Free Cracks Photoshop Brushes 10.abr
```

Observed:

```text
size            ~22.35 MiB
ABR version     6
subversion      2

8BIM samp       ~23,425,228 bytes
8BIM patt       0 bytes
8BIM desc       ~10,028 bytes

sample count    15
tip size        2500x2500
depth           8-bit grayscale
compression     PackBits/RLE
```

Preset names:

```text
121
122
123
124
125
126
127
128
129
130
131
132
133
134
135
```

All observed presets use:

```text
Diameter       2500 px
Angle          0°
Roundness      100%
Spacing        25%
Interpolation  true
Flip X         false
Flip Y         false
```

Observed dynamics/toggles in this pack are disabled.

Critical observation:

> Physical `samp` ordering does not match logical descriptor preset ordering.

Therefore:

```text
NEVER pair desc[i] with samp[i] by assumption.
```

Resolve by `sampledData` UUID/internal ID.

---

# 13. ABR parser module

Location:

```text
Engine/
    AssetPipeline/
        Importers/
            ABR/
                AbrReader.*
                AbrParser.*
                AbrDescriptorParser.*
                AbrSampleDecoder.*
                AbrPackBits.*
                AbrSemanticMapper.*
                AbrImportReport.*
                AbrToBrushIR.*
```

Editor/AssetPipeline only.

No ABR parser code in Shipping runtime.

---

# 14. Safe ABR binary reader

```cpp
class AbrReader
{
public:
    uint8_t  ReadU8();
    int8_t   ReadI8();

    uint16_t ReadU16BE();
    int16_t  ReadI16BE();

    uint32_t ReadU32BE();
    int32_t  ReadI32BE();

    uint64_t ReadU64BE();
    int64_t  ReadI64BE();

    double   ReadF64BE();

    FourCC   ReadFourCC();
    AbrID    ReadID();

    std::u16string ReadUnicodeString();

    ByteSpan ReadBytes(size_t);

    uint64_t Tell() const;
    uint64_t Remaining() const;

    void Seek(uint64_t);

    ScopedReadBoundary PushBoundary(uint64_t bytes);
};
```

No read may escape its active block boundary.

Every size multiplication must be overflow checked.

---

# 15. ABR versions

Support:

```text
v1
v2
v6+
```

Unknown/new version behavior:

```text
safe detection
safe block-bound parsing
preserve recoverable unknown blocks
report unsupported structure
never guess unchecked offsets
```

---

# 16. ABR 8BIM blocks

Generic:

```cpp
struct AbrSection
{
    FourCC signature;
    FourCC key;

    uint64_t sourceOffset;
    uint64_t payloadSize;

    ByteBuffer rawPayload;
};
```

Known:

```text
samp
desc
patt
```

Unknown blocks:

```text
preserve compressed when safe
```

Known huge `samp` data:

```text
decode/canonicalize
do not duplicate original sample payload
```

---

# 17. ABR sampled brush data

Canonical tip formats:

```cpp
enum class RBrushPixelFormat
{
    R8_UNORM,
    R16_UNORM,

    RGB8_UNORM,
    RGBA8_UNORM,

    R16_FLOAT
};
```

For masks/SDF:

```text
0 = no coverage
1 = full coverage
```

Polarity conversion must be explicit.

Store source polarity metadata.

---

# 18. ABR PackBits/RLE

Required exact row-bounded decoder.

```text
0..127
    copy next n+1 bytes

-127..-1
    repeat next byte 1-n times

-128
    no-op
```

Validate:

```text
scanline compressed sizes
expected row output
source boundaries
destination boundaries
```

---

# 19. Photoshop generic Descriptor parser

Do not implement brush settings by string scanning.

Parse generic descriptor values first.

```cpp
struct AbrProperty
{
    AbrID    key;
    FourCC   osType;
    AbrValue value;

    uint64_t sourceOffset;
    uint64_t sourceLength;

    ByteBuffer rawEncoding;
};
```

Support known descriptor types:

```text
bool
long
comp
doub
UntF
TEXT
enum
Objc
GlbO
VlLs
tdta
type
GlbC
alis
obj
```

Unknown type:

```text
preserve safely bounded raw encoding
or preserve containing descriptor/block if exact child boundary is unknown
```

---

# 20. Descriptor IDs

Do not assume 4-char only.

```cpp
struct AbrID
{
    std::string value;
    bool isFourCC;
};
```

Support:

```text
charID
stringID
```

---

# 21. ABR semantic mapping

Map at least:

## Brush Tip Shape

```text
sampledData
diameter
angle
roundness
hardness
spacing
flip X
flip Y
interpolation
antialias behavior
```

## Shape Dynamics

```text
size jitter
minimum diameter
angle jitter
roundness jitter
minimum roundness

controls:
    off
    random
    fade
    pen pressure
    pen tilt
    stylus wheel
    pen rotation
    direction
    initial direction
    unknown
```

## Scatter

```text
amount
both axes
control
count
count jitter
count control
```

## Transfer

```text
opacity
opacity jitter
opacity control
minimum opacity

flow
flow jitter
flow control
minimum flow

wetness
mix
their controls where present
```

## Response curves

```cpp
struct BrushResponseCurve
{
    std::vector<Vec2> points;
};
```

Generic mapping supports:

```text
pressure -> size
pressure -> opacity
pressure -> flow
tilt -> angle
direction -> angle
rotation -> angle
etc.
```

## Dual Brush

```text
enabled
secondary tip
blend mode
diameter
spacing
scatter
both axes
count
angle
roundness
hardness
flip
```

## Texture

```text
pattern ID
pattern name
scale
invert
brightness
contrast
mode
depth
minimum depth
depth jitter
control
texture each tip
```

## Color Dynamics

```text
foreground/background jitter
hue jitter
saturation jitter
brightness jitter
purity
control
apply per tip
```

## Brush Pose

```text
tilt X
tilt Y
rotation
pressure
override flags
```

## Other known state

```text
noise
wet edges
build-up/repeat
smoothing
airbrush
protect texture
```

---

# 22. Unknown ABR preservation

Status categories:

```text
SUPPORTED
PARSED_UNSUPPORTED
UNKNOWN_KEY
UNKNOWN_TYPE
UNKNOWN_SECTION
MALFORMED
RECOVERED
```

Current implementation requirement:

```text
PARSED_UNSUPPORTED data MUST still be stored.
```

There is no “ignore now, add later” policy.

If Jerico does not execute a known Photoshop feature yet, its complete semantic data still lives in RBrush and the importer report.

Unknown raw data is compressed in `UNKN`.

---

# 23. Image importer

Location:

```text
Engine/
    AssetPipeline/
        Importers/
            BrushImage/
                BrushImageImporter.*
                BrushImageChannelExtractor.*
                BrushImageNormalizer.*
                BrushImageReport.*
```

It must use Jerico's common image decoding stack where possible.

---

# 24. Image import modes

For each input image:

```text
Coverage Source:

Auto
Alpha
Luminance
Red
Green
Blue
RGB Average
RGB Max
RGB Min
```

`Auto`:

```text
if alpha contains meaningful variation:
    Alpha
else:
    Luminance
```

User can override per file or apply to all selected files.

---

# 25. Extract multiple brushes from channels

An image can create multiple presets.

UI:

```text
CREATE BRUSHES FROM CHANNELS

[x] Red        -> crack
[x] Green      -> chip
[x] Blue       -> dirt
[x] Alpha      -> erosion
```

Example source:

```text
RockMasks_RGBA.png
```

Result:

```text
RockMasks.rbrush

crack
chip
dirt
erosion
```

Each extracted channel gets its own canonical tip hash and PresetGUID.

---

# 26. Image alpha/luminance interpretation

Options:

```text
[ ] Invert coverage
```

Normalization:

```text
None
Min/Max Normalize
Auto Levels for coverage
```

Default:

```text
None
```

Do not silently alter artist values.

---

# 27. Trim/crop

Image import supports:

```text
Trim empty borders
Padding
Preserve original canvas
```

Default for brush masks:

```text
Trim empty borders = ON
Padding = 8 px
```

Record crop transform in metadata.

This is needed so import/update is deterministic and explainable.

---

# 28. Image bit depth

Options:

```text
Preserve
Force R8
Force R16
```

Default:

```text
Preserve meaningful mask precision
```

Examples:

```text
8-bit PNG alpha -> R8
16-bit TIFF grayscale -> R16
16/32-bit EXR scalar -> R16/R16F canonical depending on declared mode
```

Do not keep unused RGB channels when the user selected only Alpha.

---

# 29. JPEG policy

JPEG is supported.

Report:

```text
LOSSY SOURCE
```

because compression artifacts may damage SDF edges.

The importer still works.

The preview/import report should recommend:

```text
PNG/TIFF/TGA/EXR for SDF-critical masks
```

but does not block JPEG.

---

# 30. Default brush settings for images

Images have no Photoshop dynamics.

Create a Jerico preset with defaults:

```text
Spacing          25%
Hardness         100%
Angle            0°
Roundness        100%

Size Jitter      0%
Angle Jitter     0%
Roundness Jitter 0%

Scatter          0%
Count            1

Opacity          100%
Flow             100%

Pressure         Off
Dual Brush       Off
Texture          Off
```

The user edits these in RBrush Editor.

---

# 31. RBrush Editor

This is current scope.

JericoEditor must contain a dedicated editor.

```text
RBRUSH EDITOR
────────────────────────────────────

Pack:
Rock_Damage.rbrush

Preset:
Cracks/crack_01

[ Preview ]

TIP
    Source             R8 mask
    Dimensions         1024x1024
    Hardness           100%
    Spacing            25%
    Angle              0°
    Roundness          100%
    Flip X/Y

SHAPE DYNAMICS
    Size Jitter
    Size Control
    Minimum Diameter

    Angle Jitter
    Angle Control

    Roundness Jitter
    Roundness Control
    Minimum Roundness

SCATTER
    Amount
    Both Axes
    Control
    Count
    Count Jitter

TRANSFER
    Opacity
    Opacity Jitter
    Control
    Min Opacity

    Flow
    Flow Jitter
    Control
    Min Flow

PRESSURE/CURVES
    graph editor

DUAL BRUSH
    Enabled
    Secondary preset
    Mode
    Diameter
    Spacing
    Scatter
    Count

TEXTURE
    Enabled
    Pattern
    Scale
    Depth
    Mode

SDF DEFAULTS
    Compatible
    Threshold
    Invert
    Default Depth
    Edge Width
    Profile Gamma

METADATA
    Group
    Tags
    Source
    Internal Source ID
```

---

# 32. Brush preview test canvas

RBrush Editor includes a live test canvas.

Inputs:

```text
mouse
tablet pressure
tablet tilt
direction
rotation when device provides it
```

The preview must visualize:

```text
spacing
scatter
jitter
pressure
opacity
flow
dual brush
```

No need to enter a World to test a brush.

---

# 33. Pack management

Brush Library supports:

```text
Rename Pack
Rename Preset
Move Preset To Group
Duplicate Preset
Delete Preset
Extract Presets To New Pack
Merge Packs
Add Sources To Pack
Sort
Tag
Search
```

Delete must check references.

Referenced preset deletion requires either:

```text
cancel
replace references
force delete with broken-reference report
```

Default is cancel.

For JericoEngine 1.0, `force delete with broken-reference report` retains the
unresolved `RBrushAssetGUID + PresetGUID` in every affected World and appends a
canonical `rbrushBrokenReferenceReports` record to that same World descriptor.
The record contains the deleting Automation command GUID, the exact source pack
content hash and the fixed reason `PresetForceDeleted`. Pack deletion and all
World report records publish as one reversible ProjectAsset transaction.

Force Delete must never silently erase the unresolved reference, invent a
replacement or claim the World is healthy. Cook/validation may therefore detect
the retained unresolved reference and require explicit repair. Cancel remains
the default and Replace References remains the safe resolution.

---

# 34. Groups/folders

Metadata groups:

```text
Cracks/
Chips/
Surface/
Erosion/
Rock/
Terrain/
Decals/
```

No separate physical files required.

Store group path as normalized metadata.

---

# 35. Tags

Each preset supports tags.

Example:

```text
crack
rock
sdf
subtract
small
large
chip
erosion
terrain
roughness
```

Procedural tools can query tags.

```cpp
struct RBrushTagQuery
{
    std::vector<StringId> require;
    std::vector<StringId> any;
    std::vector<StringId> exclude;
};
```

Example:

```text
Require: crack, sdf
Exclude: large
```

---

# 36. Canonical Brush IR

```cpp
struct RBrushPackIR
{
    Guid assetGuid;

    RBrushImportManifest manifest;

    std::vector<RBrushTipIR>     tips;
    std::vector<RBrushPresetIR>  presets;
    std::vector<RBrushPatternIR> patterns;

    AbrDescriptorTree sourceDescriptorTree;

    std::vector<RBrushUnknownData> unknownData;
};
```

Preset:

```cpp
struct RBrushPresetIR
{
    Guid presetGuid;

    std::string name;
    std::string groupPath;

    std::vector<StringId> tags;

    Guid primaryTipGuid;

    RBrushTipShape      tipShape;
    RBrushShapeDynamics shapeDynamics;
    RBrushScatter       scatter;
    RBrushTransfer      transfer;

    std::optional<RBrushDualBrush>     dualBrush;
    std::optional<RBrushTexture>       texture;
    std::optional<RBrushColorDynamics> colorDynamics;
    std::optional<RBrushPose>          pose;

    RBrushSdfDefaults sdfDefaults;

    RBrushMiscFlags misc;

    RBrushSourcePresetInfo sourceInfo;
};
```

---

# 37. RBrush binary format

Magic:

```text
RBRUSH
```

Header:

```cpp
struct RBrushHeader
{
    char        magic[8];          // "RBRUSH\0"

    uint16_t    versionMajor;
    uint16_t    versionMinor;

    uint32_t    headerSize;
    uint32_t    flags;

    uint64_t    fileSize;
    uint64_t    tocOffset;

    uint32_t    tocCount;
    uint32_t    presetCount;

    Guid        assetGuid;

    Sha256      canonicalContentHash;

    uint32_t    headerCrc32;
};
```

Jerico-native binary data can be little-endian.

ABR parsing stays big-endian.

---

# 38. RBrush chunks

```cpp
enum class RBrushChunkType : uint32_t
{
    META,
    PRST,
    STRS,
    TIPD,
    PATT,
    DESC,
    UNKN,
    THMB
};
```

TOC:

```cpp
struct RBrushChunkEntry
{
    uint32_t type;
    uint32_t codec;

    uint64_t logicalId;

    uint64_t offset;
    uint64_t packedSize;
    uint64_t unpackedSize;

    uint32_t crc32;
    uint32_t flags;
};
```

Chunks independently addressable.

---

# 39. Compression

Default is lossless.

Per-tip compression.

```cpp
enum class RBrushCodec
{
    Raw,
    Zstd,
    PNG_L8,
    PNG_L16
};
```

For R8:

```text
encode PNG_L8
encode Zstd canonical bytes
choose smaller
```

For R16:

```text
encode PNG_L16
encode Zstd
choose smaller
```

Metadata/descriptor/unknown:

```text
Zstd
```

Do not store GPU preview mip chains inside RBrush.

Those belong in Derived Data Cache.

---

# 40. Real crack ABR compression target

The inspected crack ABR:

```text
source ABR         ~22.35 MiB
decoded grayscale  ~89.41 MiB
```

Lossless optimized grayscale PNG experiment:

```text
~16.53 MiB total tip payload
```

Regression target for this specific fixture:

```text
final .rbrush approximately <= 18 MiB
```

Do not hardcode this as a universal requirement.

Import report records actual result.

---

# 41. Canonical tip deduplication

Hash:

```text
SHA-256(
    pixel format
    width
    height
    canonical pixels
)
```

Identical tips store TIPD once.

Multiple presets may point to same TipGUID.

---

# 42. Source data deduplication

If the same image is imported twice into one pack and canonical coverage is identical:

```text
do not duplicate TIPD
```

The two presets may still have different dynamics/settings.

---

# 43. Derived Data Cache

RBrush stores canonical data.

DDC stores:

```text
preview thumbnails
GPU R8/R16 copies
mips
BC4/BC5 preview derivatives
2D SDF textures
distance transforms
brush atlas pages
temporary paint tiles
```

DDC is disposable.

---

# 44. Brush Engine

Shared module:

```text
Engine/
    Painting/
        Brush/
            RBrushAsset.*
            RBrushReader.*
            BrushEngine.*
            BrushDynamics.*
            BrushResponseCurve.*
            BrushSampler.*
```

Input:

```text
stroke path
RBrush preset
tool overrides
input device state
seed
```

Output:

```cpp
struct BrushDab
{
    Vec3 position;
    Vec3 normal;
    Vec3 tangent;

    float radius;
    float aspect;
    float rotation;

    float opacity;
    float flow;

    Guid tipGuid;

    uint64_t seed;
};
```

---

# 45. Brush deterministic behavior

All random behavior derives from stored seed.

Includes:

```text
scatter
size jitter
angle jitter
roundness jitter
count jitter
dual brush scatter
procedural brush selection
```

Same:

```text
preset version
stroke
settings
seed
```

must reproduce same dabs.

---

# 46. Brush overrides

```cpp
struct BrushOverrides
{
    std::optional<float> size;
    std::optional<float> spacing;

    std::optional<float> opacity;
    std::optional<float> flow;

    std::optional<float> scatter;

    std::optional<float> sizeJitter;
    std::optional<float> rotationJitter;

    std::optional<bool> invert;
};
```

Do not clone a preset only because one tool needs a different size/depth.

---

# 47. Surface Painter

Jerico Surface Painter uses Brush Engine output.

UI:

```text
SURFACE PAINT
────────────────────────────────

Brush:
[ Rock_Damage/Crack 01 ▼ ]

Size        0.80 m
Strength    0.65

WHAT TO PAINT

● Channel
○ Texture
○ Material
○ Mask
○ SDF / Geometry
○ Decal / Instances
```

---

# 48. PBR channel painting

Minimum:

```text
Base Color
Normal
Roughness
Metallic
Height
Ambient Occlusion
Emissive
Opacity
```

Jerico semantic channels:

```text
Wetness
Snow
Dirt
Moss
Rust
Damage
Burn

Custom Scalar
Custom Vector
```

Material definitions expose valid custom channels.

---

# 49. Value-only painting

No payload texture required.

Example:

```text
Brush:
    Crack 01

Target:
    Roughness

Operation:
    Add

Value:
    +0.20
```

The RBrush tip is coverage only.

---

# 50. Texture painting

Separate brush mask from payload texture.

```text
Brush:
    Crack 01

Payload Texture:
    ScratchNormal

Target:
    Normal
```

Projection:

```text
UV
Planar
Triplanar
Camera
World
Object
```

---

# 51. Material painting

```text
Brush:
    Crack 01

Material:
    BrokenConcrete

Channels:
[x] BaseColor
[x] Roughness
[x] Normal
[x] Height
[ ] AO
```

One brush coverage mask drives selected material channels.

---

# 52. Scalar operations

```text
Replace
Add
Subtract
Multiply
Min
Max
Smooth
Blur
Sharpen
```

Correct data semantics:

```text
Roughness       linear
Metallic        linear
AO              linear
Height          linear
Masks           linear
Base Color      use correct sRGB/linear conversion
Normal          use normal-specific blend
```

---

# 53. Surface Height vs Geometry Height

Separate targets.

## Material Height

```text
Material.Height
```

Affects visual/material height.

## Geometry/SDF

```text
Geometry/SDF
```

Actually changes editable shape.

Never conflate the two.

---

# 54. Paint layers

World authoring:

```cpp
struct PaintLayer
{
    Guid id;

    PaintTarget target;
    PaintPayload payload;

    BlendSettings blend;

    std::vector<PaintStroke> strokes;

    bool visible;
    bool locked;
};
```

Stroke:

```cpp
struct PaintStroke
{
    RBrushPresetRef brush;

    std::vector<StrokePoint> points;

    BrushOverrides overrides;

    uint64_t seed;
};
```

World stores RBrush references.

Never source ABR/image paths.

---

# 55. EPLS integration

RBrush can paint:

```text
Include
Exclude
Density
Strength
Material mask
```

Final mask:

```text
Procedural
+ Manual Include
- Manual Exclude
= Final EPLS
```

Same RBrush is reusable across Surface Paint and EPLS.

---

# 56. Terrain integration

Targets include:

```text
Terrain Height
Terrain Material Weight
Terrain Hole Mask
Biome Mask
Foliage Include
Foliage Exclude
Wetness
Snow
EPLS
```

Terrain Height supports:

```text
Raise
Lower
Replace
Flatten
Smooth
Noise-modulated stamp
```

---

# 57. Decal and instance distribution

RBrush is a distribution mask.

Example:

```text
RBrush scatter
+ decal material
+ surface target
```

Result:

```text
Decal Instances
```

Instance scatter target:

```text
RBrush
+ prefab set
+ placement rules
```

Result:

```text
rock/foliage/object instances
```

---

# 58. SDF integration

RBrush is first-class input to:

```text
SDF Sculpt Tool
Rock Generator
Cave detail tools
Damage/chipping
Crack generation
Surface breakup
Erosion masks
```

---

# 59. Mask -> 2D SDF

Derived pipeline:

```text
R8/R16 coverage
    │
    ├── invert option
    ├── threshold
    ├── edge cleanup
    ▼
continuous/binary mask
    ▼
distance transform
    ▼
2D signed distance field
    ▼
DDC
```

Store canonical mask, not derived distance field, in RBrush.

---

# 60. SDF convention

```text
distance < 0   inside solid
distance = 0   surface
distance > 0   outside
```

Boolean:

```text
Union        min(A,B)
Intersect    max(A,B)
Difference   max(A,-B)
```

Smooth equivalents supported by SDF subsystem.

---

# 61. Crack cutter

```text
RBrush crack mask
    ↓
2D SDF
    ↓
project on surface tangent frame
    ↓
apply depth/profile
    ↓
3D cutter field
    ↓
Difference
```

Settings:

```cpp
struct SdfBrushStampSettings
{
    SdfBrushOperation operation;

    float worldSize;
    float depth;

    float threshold;
    float edgeWidth;
    float profileGamma;

    float rotation;
    float normalOffset;

    bool invert;

    SdfProjectionMode projection;

    float smoothK;
};
```

---

# 62. SDF operations

Implement now:

```cpp
enum class SdfBrushOperation
{
    Add,
    Subtract,

    SmoothAdd,
    SmoothSubtract,

    SurfaceDisplace,

    CrackCutter,
    ChipCutter,

    Smooth,
    Flatten,

    MaskOnly
};
```

---

# 63. SDF projection

Implement:

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

Rock defaults:

```text
Surface Tangent
Triplanar
Spherical/Radial
```

---

# 64. SDF crack profile

Concept:

```text
coverage = SampleBrush(uv)

strength =
    pow(
        saturate(
            (coverage - threshold) /
            max(1 - threshold, epsilon)
        ),
        profileGamma
    )

depthAtPoint = depth * strength
```

Derived 2D SDF is preferred for clean crack boundaries and true cutter construction.

---

# 65. Combined geometry + material stamp

One editor action can create:

```text
SDF crack
+
material crack detail
```

Example:

```text
Geometry:
    subtract 0.02 m

Material:
    darker exposed concrete
    roughness +0.15
    normal/height crack detail
```

Represent as coordinated authoring layers sharing:

```text
transform
brush reference
seed
```

Do not put material payload inside RBrush itself.

---

# 66. Rock Generator

UI:

```text
SDF ROCK GENERATOR
────────────────────────────────

Base Shape
    Sphere / Ellipsoid / Primitive

Macro Shape
    Noise
    Warp

Surface Breakup
    Brush Set / Noise

Cracks
    Enabled

    Brush Query:
        tags: crack, sdf

    Count          8..20
    Scale          0.2..1.3 m
    Depth          0.01..0.08 m
    Rotation       random
    Projection     Surface Tangent
    Smooth K       0.002

Chips
    Brush Query:
        tags: chip, sdf

Erosion
    Brush Query:
        tags: erosion
```

---

# 67. Weighted brush sets

```cpp
struct WeightedRBrushRef
{
    RBrushPresetRef brush;

    float weight;

    FloatRange scale;
    FloatRange depth;
    FloatRange rotation;

    float threshold;
    float gamma;

    bool invert;
};
```

Reusable set representation:

```cpp
struct RBrushSet
{
    std::vector<WeightedRBrushRef> entries;

    RBrushTagQuery query;

    uint64_t seed;
};
```

The tag query can populate entries dynamically from a pack/library.

---

# 68. Deterministic procedural placement

```cpp
struct SdfBrushLayer
{
    uint64_t seed;

    RBrushSet brushes;

    uint32_t count;

    DistributionRules distribution;
};
```

Same seed/settings/assets => same output.

---

# 69. RBrush as procedural mask source

Supported use patterns:

```text
Noise
-> select positions
-> RBrush cracks
-> SDF subtract
```

```text
Curvature
-> RBrush dirt
-> Roughness mask
```

```text
Humidity
-> RBrush moss
-> EPLS include
```

```text
Slope
-> RBrush distribution
-> rock instance scatter
```

```text
Damage authoring
-> RBrush
-> decals + SDF + material
```

---

# 70. SDF World authoring

```cpp
struct SdfRBrushModifier
{
    RBrushPresetRef brush;

    Transform localTransform;

    SdfBrushStampSettings settings;

    uint64_t seed;
};
```

SDF object stack:

```text
Base Shape
Noise
RBrush Crack Layer
RBrush Chip Layer
Erosion
Smooth
```

Editable until cook.

---

# 71. RBrush source preview metadata

Each preset stores:

```text
source type
source logical name
source internal ID
source dimensions
source bit depth
crop
channel extraction mode
invert
ABR version/subversion if ABR
source hash
canonical tip hash
```

---

# 72. Import report

For every import:

```text
RBRUSH IMPORT REPORT
────────────────────────────────

Destination Pack:
Rock_Damage.rbrush

Sources:
3 images
1 ABR

Created Presets:
...

Updated Presets:
...

Skipped Duplicates:
...

Canonical Tips:
...

Tip Compression:
...

Unknown ABR Keys:
...

Unknown ABR Sections:
...

Consumed Sources:
...

Kept Sources:
...

Final RBrush Size:
...

Validation:
PASS
```

ABR-specific detail:

```text
version
subversion
8BIM sections
sample count
descriptor preset count
resolved sampledData IDs
unresolved IDs
```

Image-specific detail:

```text
dimensions
source channels
selected coverage channel
bit depth
crop
normalization
lossy source warning
```

---

# 73. Brush Library

Example:

```text
BRUSH LIBRARY
─────────────────────────────────────

Search [ crack________________ ]

Location
[x] Engine
[x] Project

Type
[x] Imported ABR
[x] Imported Image
[x] Jerico Edited

Tags
[x] crack
[x] sdf

[preview] crack_01
[preview] crack_02
[preview] crack_03

─────────────────────────────────────

Pack:
Rock_Damage.rbrush

Preset:
Cracks/crack_01

Tip:
1024x1024 R8

Spacing:
25%

SDF:
Compatible

Tags:
crack, rock, sdf, subtract

Source:
PNG / Embedded

[ Edit ]
[ Duplicate ]
[ Extract ]
[ Report ]
```

---

# 74. Source inbox status

Statuses:

```text
NEW
ALREADY IMPORTED
UPDATE AVAILABLE
IMPORT ERROR
UNSUPPORTED DATA
```

Consumed source absence is normal.

Do not show it as broken.

Use provenance:

```text
SourceState = ConsumedExternal
```

---

# 75. Update button

`Update`:

```text
Scan Engine and Project Brush Inboxes.
Compare source hashes/internal IDs against RBrush manifests.
Update matching presets.
Add new presets.
Mark source-removed presets SourceMissing.
Preserve Jerico GUIDs.
```

`Reimport Selected` lets the user point to a replacement source manually.

---

# 76. Merge packs

Brush Library supports:

```text
Select packs
-> Merge Into New Pack
```

Deduplicate identical TIPD data.

Preserve preset settings.

New pack/presets receive new GUIDs.

---

# 77. Create empty pack

Current-scope feature.

```text
Create Empty RBrush Pack
```

Then:

```text
Add Images
Add ABR
Duplicate/Edit presets
```

This allows Jerico-native packs with no external source.

---

# 78. Create preset from existing texture asset

From Content Browser:

```text
Right Click Texture
-> Create RBrush Preset
```

Dialog:

```text
Destination Pack
Coverage Channel
Embed or Reference
Invert
Trim
Bit Depth
Default Dynamics
Tags
```

If `Embed`:

```text
RBrush becomes self-contained
```

If `Reference`:

```text
RBrush Editor reference is retained
```

Never delete the original Content Browser asset.

---

# 79. Create brushes from one packed texture

Content Browser:

```text
Right Click RGBA texture
-> Create RBrush Presets From Channels
```

Example:

```text
R -> crack
G -> chips
B -> pits
A -> erosion
```

All four are created in one transaction.

---

# 80. Preview generation

Create:

```text
256x256 or 384x384
```

Preview can live in:

```text
THMB
or DDC
```

Default:

```text
DDC
```

unless offline browsing requires embedded thumbnail.

Do not store large preview pyramids.

---

# 81. Import safety limits

Configurable hard limits:

```text
source file size
ABR section size
sample count
descriptor item count
descriptor recursion depth
string length
list length
image width/height
pixel count
decompressed bytes
RLE row output
RBrush chunk count
RBrush unpacked chunk size
```

No unchecked allocation.

---

# 82. Sanitizers/fuzzing

Required:

```text
ASan
UBSan
libFuzzer or equivalent
```

Targets:

```text
AbrReader
AbrDescriptorParser
AbrPackBits
AbrSamp parser
Image channel extractor
RBrush reader
RBrush TOC parser
```

---

# 83. RBrush reader validation

Validate:

```text
magic
version
file size
header CRC
TOC bounds
chunk overlap
packed/unpacked sizes
codec IDs
chunk CRC
canonical hash
preset references
tip references
pattern references
string references
source manifest
```

---

# 84. No third-party asset auto-commit

The crack ABR used for local development may have redistribution restrictions.

Do not automatically commit it.

CI must use:

```text
synthetic fixtures
or redistributable fixtures
```

Local regression test may use the user-provided file.

---

# 85. Cook behavior

Authoring graph:

```text
World
-> RBrush
-> strokes/modifiers
```

Cook:

```text
evaluate/bake
remove Editor-only dependency
```

Examples:

### Surface

```text
Paint Layers
-> bounded derived textures/masks/material atlases
```

JericoEngine 1.0 implements Virtual Texture Paging through the shared
Virtualized World Runtime defined by
`docs/JERICO_VIRTUALIZED_WORLD_RUNTIME_1_0.md`. Surface-paint authoring emits
bounded `DirtySpatialRegion` updates and cook invalidates only the affected
texture pages and dependency closure. Conventional fully resident `.rtext`
storage remains an admitted fallback on unsupported hardware, but it uses the
same stable asset/page identity and must not create a second authoring or cook
path.

### Terrain

```text
Terrain brush authoring
-> cooked height/weights
```

### SDF

```text
SDF + RBrush modifiers
-> cooked mesh/SDF representation
```

### EPLS

```text
RBrush manual/procedural masks
-> cooked masks/instances
```

### Decals/Instances

```text
RBrush distribution
-> cooked instances
```

---

# 86. Shipping validator

Shipping must reject:

```text
*.abr
*.rbrush
Import/Brushes/
EditorAssets/Brushes/
```

and any unresolved Editor brush source dependency.

Error example:

```text
COOK ERROR:
World/Cave01 references Editor-only RBrush:
Rock_Damage.rbrush / crack_03

Expected:
paint/SDF dependency should have been baked.
```

---

# 87. Runtime-dynamic paint cook

Runtime painting is supported through a cooked derivative.

Never ship full Editor RBrush automatically.

Cook only used runtime data:

```text
tip mask
required dynamics
response curves
minimal metadata
```

Internal cooked resource may be stored inside `.epak`.

Use cases:

```text
blood
mud
snow tracks
vehicle tracks
paint
damage
```

This feature is part of the architecture now; the cooker/resource path must exist.

---

# 88. Vertical implementation plan

## Slice 1 — Canonical Brush IR

Implement:

```text
Tip
Preset
Dynamics
Tags
Groups
Source manifest
Stable GUIDs
```

PASS:

```text
construct/save/load IR in memory tests
```

---

## Slice 2 — RBrush binary container

Implement:

```text
header
TOC
META
PRST
STRS
TIPD
PATT
DESC
UNKN
CRC/hash
```

PASS:

```text
write/read exact canonical roundtrip
```

---

## Slice 3 — Compression

Implement now:

```text
Zstd
PNG_L8
PNG_L16
AUTO selection
```

PASS:

```text
lossless tip hashes
independent per-tip decompression
```

---

## Slice 4 — Image importer

Implement:

```text
PNG
TGA
TIFF
BMP
JPEG
EXR
```

PASS:

```text
single image -> RBrush preset
multiple images -> one RBrush pack
```

---

## Slice 5 — Image channel extraction

Implement:

```text
Auto
Alpha
Luminance
R
G
B
RGB Average
RGB Min
RGB Max
```

PASS:

```text
RGBA input -> four presets from R/G/B/A in one import
```

---

## Slice 6 — Image import processing

Implement:

```text
invert
trim
padding
preserve canvas
bit-depth conversion
normalization modes
```

PASS:

```text
metadata exactly records conversion
```

---

## Slice 7 — ABR inspect

Implement:

```text
abr_inspect
```

PASS on crack fixture:

```text
v6.2
samp/desc/patt
15 samples
15 descriptors
2500x2500 R8
PackBits
```

---

## Slice 8 — ABR sample decode

PASS:

```text
15 correct lossless canonical masks
stable hashes
```

---

## Slice 9 — Generic descriptor parser

PASS:

```text
names 121..135
sampledData IDs
spacing/diameter/angle/roundness
known toggles
unknown reporting
```

---

## Slice 10 — ABR semantic mapper

Implement all sections listed in this document:

```text
tip
shape dynamics
scatter
transfer
pressure curves
dual brush
texture
color dynamics
brush pose
misc
```

PASS:

```text
known values survive RBrush write/read
```

---

## Slice 11 — ABR ID resolution

PASS:

```text
desc preset -> samp tip using sampledData ID
not position
```

---

## Slice 12 — Transactional import + consume

PASS:

```text
source consumed only after verified RBrush reopen
failure leaves source untouched
```

---

## Slice 13 — Source manifests/duplicate detection

PASS:

```text
drop exact same source again
no duplicated presets
```

---

## Slice 14 — Update

PASS:

```text
same source ID changed pixels/settings
existing PresetGUID survives
World ref survives
```

---

## Slice 15 — Brush Library

Implement:

```text
search
group
tags
source status
preview
pack navigation
```

PASS:

```text
browse RBrush after original sources are gone
```

---

## Slice 16 — Pack management

Implement:

```text
new pack
empty pack
add sources
merge
extract selected
duplicate preset
move group
rename
delete with reference check
```

PASS:

```text
all actions preserve valid asset graph
```

---

## Slice 17 — RBrush Editor

Implement all editable properties listed in this document.

PASS:

```text
image preset can be turned into scatter/pressure/jitter brush entirely in Jerico
```

---

## Slice 18 — Brush test canvas

PASS:

```text
spacing/scatter/jitter/pressure visible without World
```

---

## Slice 19 — Brush Engine

PASS:

```text
deterministic dabs
spacing
scatter
jitter
pressure
```

---

## Slice 20 — Surface Painter scalar channel

First validation:

```text
Roughness
```

PASS:

```text
paint only roughness
other channels unchanged
undo/redo
```

---

## Slice 21 — Material Height

PASS:

```text
paint Material.Height
do not change SDF geometry
```

---

## Slice 22 — Texture paint

PASS:

```text
RBrush mask + independent payload texture
```

---

## Slice 23 — Material paint

PASS:

```text
one brush mask applies selected channels from PBR material
```

---

## Slice 24 — Terrain

PASS:

```text
same RBrush paints terrain height and terrain material weights
```

---

## Slice 25 — EPLS

PASS:

```text
RBrush Include/Exclude modifies procedural final mask
```

---

## Slice 26 — Decal/instance scatter

PASS:

```text
RBrush distributes deterministic decal/instance set
```

---

## Slice 27 — Derived 2D SDF

PASS:

```text
R8 crack mask -> valid 2D signed distance representation
```

---

## Slice 28 — SDF stamp

PASS:

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

---

## Slice 29 — SDF projections

PASS:

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

---

## Slice 30 — Crack geometry

PASS:

```text
uploaded-style crack brush cuts actual SDF rock geometry
undo/redo
```

---

## Slice 31 — Combined geometry/material stamp

PASS:

```text
same action creates SDF crack + coordinated material detail
```

---

## Slice 32 — Rock Generator brush sets

PASS:

```text
tag/query/weighted RBrush sets
deterministic cracks/chips
```

---

## Slice 33 — Procedural source usage

PASS:

```text
RBrush functions as non-mouse mask source for procedural placement
```

---

## Slice 34 — Cook static Surface Paint

PASS:

```text
World renders correct baked output with RBrush dependency removed
```

---

## Slice 35 — Cook static SDF

PASS:

```text
rock result works with RBrush dependency removed
```

---

## Slice 36 — Runtime dynamic derivative

Implement minimal cooked runtime brush resource.

PASS:

```text
runtime sample project paints using cooked derivative
Editor .rbrush absent from Shipping
```

---

## Slice 37 — Shipping validator

PASS:

```text
.epak inspection shows:
NO ABR
NO brush-source-only images
NO Editor RBrush
NO EditorAssets references
```

---

# 89. Acceptance criteria

The feature is complete only when all are true:

## Native asset

- [ ] `.rbrush` is a versioned chunked format.
- [ ] One `.rbrush` can contain many presets.
- [ ] Each pack has stable AssetGUID.
- [ ] Each preset has stable PresetGUID.
- [ ] Groups/folders work.
- [ ] Tags work.
- [ ] Search works.
- [ ] TIPD deduplication works.
- [ ] Per-tip compression works.
- [ ] RBrush reader validates all chunks.

## Images

- [ ] PNG supported.
- [ ] TGA supported.
- [ ] TIFF supported.
- [ ] BMP supported.
- [ ] JPEG supported.
- [ ] EXR supported.
- [ ] One image can create a preset.
- [ ] Many images can create one pack.
- [ ] Images can be added to existing pack.
- [ ] RGBA channels can produce multiple presets.
- [ ] Alpha extraction works.
- [ ] Luminance extraction works.
- [ ] R/G/B/A extraction works.
- [ ] RGB average/min/max work.
- [ ] Invert works.
- [ ] Trim works.
- [ ] Padding works.
- [ ] Bit-depth preservation/conversion works.
- [ ] Existing Project texture assets are never deleted.
- [ ] Embed mode produces self-contained RBrush.
- [ ] Reference mode works for existing image assets.

## ABR

- [ ] v1 supported.
- [ ] v2 supported.
- [ ] v6+ supported.
- [ ] `samp` handled.
- [ ] `desc` handled.
- [ ] `patt` handled.
- [ ] PackBits handled.
- [ ] R8 handled.
- [ ] R16 path handled.
- [ ] Samples resolve by internal ID.
- [ ] Descriptor parser is generic/recursive.
- [ ] Spacing mapped.
- [ ] Hardness mapped.
- [ ] Scatter mapped.
- [ ] Rotation mapped.
- [ ] Size jitter mapped.
- [ ] Opacity jitter mapped.
- [ ] Flow mapped.
- [ ] Pressure mappings mapped.
- [ ] Response curves mapped.
- [ ] Dual Brush mapped.
- [ ] Texture mapped.
- [ ] Color Dynamics mapped.
- [ ] Brush Pose mapped.
- [ ] Unknown descriptor fields preserved.
- [ ] Unknown sections preserved when safe.
- [ ] Import report complete.
- [ ] Crack ABR fixture imports all 15 presets correctly.

## Pack workflows

- [ ] Create empty pack.
- [ ] Create pack from images.
- [ ] Create pack from ABR.
- [ ] Add images to pack.
- [ ] Add ABR presets to pack.
- [ ] Extract selected presets to new pack.
- [ ] Merge packs.
- [ ] Duplicate preset.
- [ ] Move preset to group.
- [ ] Rename pack/preset.
- [ ] Delete checks references.
- [ ] Exact duplicate source does not duplicate data.
- [ ] Update preserves PresetGUID.

## RBrush Editor

- [ ] Tip settings editable.
- [ ] Spacing editable.
- [ ] Hardness editable.
- [ ] Flip editable.
- [ ] Shape Dynamics editable.
- [ ] Scatter editable.
- [ ] Transfer editable.
- [ ] Response curves editable.
- [ ] Dual Brush editable.
- [ ] Texture settings editable.
- [ ] SDF defaults editable.
- [ ] Tags/groups editable.
- [ ] Live test canvas works.

## Surface Paint

- [ ] Scalar PBR channels paint.
- [ ] Value-only paint works.
- [ ] Texture payload works.
- [ ] Material paint works.
- [ ] Channel selection works.
- [ ] Operations work.
- [ ] Normal-specific blending works.
- [ ] Material Height is separate from Geometry Height.
- [ ] Paint layers are non-destructive.
- [ ] Undo/redo works.

## Terrain/EPLS

- [ ] Terrain Height works.
- [ ] Terrain material weights work.
- [ ] EPLS Include works.
- [ ] EPLS Exclude works.
- [ ] Deterministic results.

## SDF

- [ ] Derived 2D SDF works.
- [ ] Add/Subtract work.
- [ ] Smooth booleans work.
- [ ] SurfaceDisplace works.
- [ ] CrackCutter works.
- [ ] ChipCutter works.
- [ ] Smooth works.
- [ ] Flatten works.
- [ ] MaskOnly works.
- [ ] All projection modes in this spec work.
- [ ] Crack brush cuts real rock geometry.
- [ ] Combined geometry/material stamp works.
- [ ] Rock Generator uses weighted/tagged RBrush sets.
- [ ] Procedural placement is deterministic.

## Cook/Shipping

- [ ] Static Surface Paint bakes.
- [ ] Static Terrain brush effects bake.
- [ ] Static EPLS brush effects bake.
- [ ] Static SDF brush effects bake.
- [ ] Static decal/instance results bake.
- [ ] Runtime dynamic derivative exists.
- [ ] `.abr` never ships.
- [ ] source-only brush images never ship.
- [ ] Editor `.rbrush` never ships by default.
- [ ] Shipping validator blocks leaked Editor-only dependencies.

---

# 90. Implementation boundaries

```text
ABR parser
    AssetPipeline only

Image brush importer
    AssetPipeline only

RBrush
    canonical Editor-native asset

RBrush Editor / Brush Library
    JericoEditor

Brush Engine
    shared coverage/dab engine

Surface Painter
    material/channel/texture/mask target

Terrain/EPLS
    brush targets

SDF Adapter
    brush -> distance/geometry operations

Rock Generator
    procedural RBrush sets

World
    non-destructive strokes/modifiers

Cooker
    bakes/removes Editor-only dependencies

Shipping
    cooked results only
```

---

# 91. Codex execution rule

Implement this as part of JericoEngine's real Asset Pipeline, Editor, World IR and Cooker.

Do not create disconnected demos.

Do not stop after parsing images.

Do not stop after showing ABR thumbnails.

Do not defer image pack support.

Do not defer RBrush Editor.

Do not defer SDF operations.

Do not defer tag queries.

Do not defer pack merge/extract.

Do not defer source consume/update logic.

Do not defer Shipping validation.

Work in vertical slices and require executable/editor evidence.

Git commits should correspond to verified PASS slices.

Do not mark a slice PASS from static code inspection alone when runtime/editor evidence is possible.

---

# 92. Final workflow — ABR

```text
1. Put .abr in Brush Inbox.
2. Jerico detects NEW.
3. Import into new/existing RBrush.
4. Parse samples/descriptors/patterns.
5. Resolve internal IDs.
6. Canonicalize tips/settings.
7. Preserve unknown data.
8. Compress RBrush.
9. Verify RBrush by reopening.
10. Register asset.
11. Consume ABR when selected.
12. Use only RBrush from then on.
```

---

# 93. Final workflow — images

```text
1. Select one or many images.

2. Choose:
   Create New Pack
   or Add To Existing Pack.

3. Choose coverage:
   Auto
   Alpha
   Luminance
   R/G/B
   RGB Avg/Min/Max.

4. Optionally extract multiple channels.

5. Choose:
   Embed + Consume
   Embed + Keep
   Reference + Keep
   according to source context.

6. Apply import processing:
   invert
   trim
   padding
   bit depth
   normalization.

7. Jerico creates presets with default dynamics.

8. Open RBrush Editor.

9. Add:
   spacing
   scatter
   pressure
   jitter
   dual brush
   SDF defaults
   tags/groups.

10. Use in Surface Paint/SDF/Terrain/EPLS/etc.
```

---

# 94. Final workflow — SDF rock

```text
Rock Generator
    ↓
Base SDF
    ↓
Macro noise/warp
    ↓
Query RBrush tags:
    crack + sdf
    ↓
Weighted deterministic placement
    ↓
2D mask -> 2D SDF
    ↓
Surface projection
    ↓
Crack/Chip Boolean operations
    ↓
Optional material paint pairing
    ↓
Editable World modifier stack
    ↓
Cook
    ↓
Final mesh/SDF representation
```

---

# 95. Final system summary

```text
                   JERICO RBRUSH SYSTEM
                          │
         ┌────────────────┼────────────────┐
         │                │                │
         ▼                ▼                ▼
      Photoshop        Images         Jerico Native
        .abr       png/tga/tiff/...    edited/generated
         │                │                │
         └────────────────┼────────────────┘
                          ▼
                 Canonical Brush IR
                          ▼
                       .rbrush
                  pack + subassets
                          │
         ┌────────────────┼────────────────┐
         │                │                │
         ▼                ▼                ▼
   Surface Painter      SDF/Rocks      Procedural
   PBR/materials      cracks/chips     terrain/EPLS
         │                │            decals/instances
         └────────────────┼────────────────┘
                          ▼
                     World Authoring
                          ▼
                         Cook
                          ▼
                     .epak Shipping

                 Source files are gone
                 Editor RBrush is gone
                 Only cooked results remain
```

---

# 96. Definition of done

RBrush is DONE only when a user can perform this full scenario:

```text
A.
Drop the provided Photoshop crack ABR into Jerico.

B.
Import it into an RBrush pack.

C.
Delete/consume the ABR.

D.
Close JericoEditor.

E.
Reopen the project.

F.
All crack presets still work from RBrush alone.

G.
Select several ordinary PNG/TIFF masks.

H.
Add them into the same pack or a new pack.

I.
Extract RGBA channels into individual brush presets.

J.
Edit spacing/scatter/pressure/tags inside Jerico.

K.
Use one brush to paint Roughness.

L.
Use the same brush to paint a material.

M.
Use the same brush to cut an actual crack into an SDF rock.

N.
Use a tagged set of crack/chip brushes in Rock Generator.

O.
Regenerate with the same seed and obtain the identical result.

P.
Cook the World.

Q.
Run the cooked game and see the correct surface/SDF result.

R.
Inspect Shipping output and verify:
   no .abr
   no source-only brush images
   no Editor .rbrush
   no unresolved Editor brush dependency.
```

Only then is the RBrush system complete.
