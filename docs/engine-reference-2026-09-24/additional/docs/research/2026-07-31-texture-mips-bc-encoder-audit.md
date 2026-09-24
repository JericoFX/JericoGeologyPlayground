# Offline texture mips and BC encoder audit

Date: 2026-07-31

## Outcome

Use **AMD Compressonator Core**, CPU-only, at the exact stable tag
`V4.5.52` / commit
`9da0bc2be8d095adfab04ba9262e94842b48dac2` as the preferred block encoder
for M6F-e. Admit only the small `CMP_Core` source subset behind a private
Jerico texture-cook wrapper. Do not build or link Compressonator Framework,
CLI, GUI, image plug-ins, OpenCL, DirectX or Vulkan helpers. The Core license is
MIT and its public block API covers BC4, BC5, BC6H and BC7.

Keep mip generation Jerico-owned and semantic-aware. The already pinned
DiligentCore can still provide format metadata, mip-count helpers and Vulkan
format mapping, but DiligentTools `TextureLoader` must remain an implementation
oracle rather than the intake, cook or runtime boundary:

- it encodes BC4/BC5 (and BC1/BC3), but does **not** encode BC6H or BC7;
- its generic mips do not decode and renormalize normal vectors;
- its sRGB filter applies the gamma-domain conversion to every stored channel,
  including alpha; and
- its DDS/KTX parsers are loaders for trusted data, not Jerico's bounded hostile
  media admission boundary.

No candidate reviewed here publishes a guarantee of byte-identical BC output
across compiler and operating system. The dependency pin is necessary but not
sufficient. M6F-e must therefore establish determinism with golden block
vectors, repeated process runs, Debug/Release equality and 1-worker/N-worker
equality before `PASS`.

## Scope and current Jerico contract

The current `.rtext` 1.0 contract is RGBA8-only and uncompressed. It may not be
silently reinterpreted. M6F-e needs a versioned reader/writer extension with
explicit GPU format, target profile, mip table, stored sizes and payload hashes.
This follows the existing
[`Compression contract`](../RUNTIME_FORMATS_AND_CHUNKS.md#compression-contract)
and texture-cook requirements in
[`ASSET_IMPORT_AND_COOKING.md`](../ASSET_IMPORT_AND_COOKING.md#fuente-cook-y-compresion).

The intended desktop profile remains:

| Semantic | Offline mip domain | GPU payload |
|---|---|---|
| Base color / LDR emissive | RGB filtered in linear light; alpha kept linear | BC7 sRGB or BC7 Linear, explicit |
| Tangent/object normal | Decode vector, filter, renormalize, re-encode XY | BC5 Linear |
| One-channel mask / height | Linear scalar, semantic filter | BC4 Linear |
| Packed ORM/ARM | Independent linear data channels | BC7 Linear by default |
| HDR environment / emissive | Linear float/half | BC6H UF16/SF16 when quality permits |

Each mip remains independently addressable and directly uploadable. Shipping
must not link any encoder or inflate BC payloads back to RGBA8.

## Exact local evidence

The configured local sources were inspected, not inferred from Diligent's
current `master` branch:

| Component | Exact commit | Local evidence root |
|---|---|---|
| DiligentTools | `260a1e47a743b0f07c9f50da3a7e31e11f815953` | `build/windows-vulkan-editor/DiligentTools/` |
| DiligentCore | `fa060f50549238578e1701b5d362d7cff2bf48c7` | `build/windows-vulkan-editor/DiligentCore/` |

Jerico currently admits only selected DiligentTools shader-packager and editor
overlay sources. `docs/DEPENDENCIES.md` and ADR-011 explicitly exclude the
`TextureLoader` target. Adding that whole target would also pull the Diligent
graphics helpers plus PNG, TIFF, Zlib and JPEG dependencies, as shown by its
[pinned CMake target](https://github.com/DiligentGraphics/DiligentTools/blob/260a1e47a743b0f07c9f50da3a7e31e11f815953/TextureLoader/CMakeLists.txt).

## Pinned DiligentTools audit

### What it actually encodes

`TextureLoadInfo::CompressMode` has two CPU compression qualities. The format is
selected only from the 8-bit source channel count:

| Input after conversion | Encoder output |
|---|---|
| R8 | BC4_UNORM |
| RG8 | BC5_UNORM |
| RGB8 expanded to RGBA | BC1_UNORM / BC1_UNORM_SRGB |
| RGBA8 | BC3_UNORM / BC3_UNORM_SRGB |

This is specified in the pinned
[`TextureLoader.h`](https://github.com/DiligentGraphics/DiligentTools/blob/260a1e47a743b0f07c9f50da3a7e31e11f815953/TextureLoader/interface/TextureLoader.h#L65-L86)
and implemented in
[`TextureLoaderImpl.cpp`](https://github.com/DiligentGraphics/DiligentTools/blob/260a1e47a743b0f07c9f50da3a7e31e11f815953/TextureLoader/src/TextureLoaderImpl.cpp#L567-L683).
BC4 and BC5 call `stb_compress_bc4_block` and `stb_compress_bc5_block`; BC1/BC3
call `stb_compress_dxt_block`. There is no BC6H or BC7 branch.

The existing upstream test verifies only that a constant RGBA8 block becomes a
non-null BC3 subresource; it does not lock BC4, BC5 or compressed bytes to a
golden hash. See the pinned
[`TextureLoaderTest.cpp`](https://github.com/DiligentGraphics/DiligentTools/blob/260a1e47a743b0f07c9f50da3a7e31e11f815953/Tests/DiligentToolsTest/src/TextureLoaderTest.cpp#L320-L357).

### What it loads but does not encode

The DDS loader maps pre-existing BC4/5/6H/7 DXGI payloads to Diligent formats,
including signed and sRGB variants. That is parse/pass-through capability, not an
encoder. See
[`DDSLoader.cpp`](https://github.com/DiligentGraphics/DiligentTools/blob/260a1e47a743b0f07c9f50da3a7e31e11f815953/TextureLoader/src/DDSLoader.cpp#L459-L492).

The KTX loader likewise recognizes BC6H/BC7 OpenGL internal formats, but its
header type and magic are explicitly KTX 1.0 (`KTX10Header` and the `KTX 11`
identifier). It is not KTX2/Basis intake. See
[`KTXLoader.cpp`](https://github.com/DiligentGraphics/DiligentTools/blob/260a1e47a743b0f07c9f50da3a7e31e11f815953/TextureLoader/src/KTXLoader.cpp#L198-L240).

### Mip behavior

The loader allocates the complete chain and invokes DiligentCore
`ComputeMipLevel` successively before block compression. Relevant behavior at
the exact DiligentCore pin:

- ordinary UNORM data uses a channel-by-channel 2x2 integer box average;
- integer data defaults to a deterministic most-frequent selector;
- sRGB data uses `FastGammaToLinear`, averages, then `FastLinearToGamma`;
- `AlphaCutoff` applies a fixed alpha boost after each mip; and
- there is no normal-map semantic, vector decode, length preservation or
  renormalization.

Sources:

- [Pinned mip dispatch and alpha remap](https://github.com/DiligentGraphics/DiligentCore/blob/fa060f50549238578e1701b5d362d7cff2bf48c7/Graphics/GraphicsTools/src/GraphicsUtilities.cpp#L322-L488)
- [Pinned `ComputeMipLevel` API](https://github.com/DiligentGraphics/DiligentCore/blob/fa060f50549238578e1701b5d362d7cff2bf48c7/Graphics/GraphicsTools/interface/GraphicsUtilities.h#L64-L141)
- [Pinned sRGB unit test](https://github.com/DiligentGraphics/DiligentCore/blob/fa060f50549238578e1701b5d362d7cff2bf48c7/Tests/DiligentCoreTest/src/GraphicsAccessories/GraphicsUtilitiesTest.cpp#L537-L575)

The sRGB implementation loops over all format components with the same
`SRGBAverage` callback. For RGBA8_SRGB this includes alpha. That differs from the
required Jerico policy: RGB is color and must be filtered in linear light, while
alpha is linear coverage. Premultiplying alpha before filtering is available and
sRGB-aware, but does not change the fact that the later sRGB mip loop gamma-filters
the alpha component.

`AlphaCutoff` is also not a measured coverage-preservation solve. It applies
`max(A_old, (A_old + 2 * cutoff) / 3)` to every coarse alpha value. Jerico should
instead define an explicit alpha semantic and, for masked materials, preserve
coverage at the authored threshold.

### Determinism and limits

The pinned linear 8-bit box path is integer arithmetic in a fixed traversal and
the pinned `stb_dxt` compression loop is serial, with no random or time input
visible in the path. This is strong evidence for repeatability within one fixed
binary. It is **not** an upstream promise of cross-toolchain equality:

- the sRGB path uses floating-point approximations;
- no upstream compressed-output SHA/golden test exists for BC4/5;
- the high-quality mode changes encoder search behavior; and
- the complete loader accepts many representations and conversions that are not
  encoded into a Jerico recipe identity.

The loader's memory estimator accounts for decoded, mip and compressed storage,
but it has no Jerico limits such as 16384 per axis, 32 mips and 512 MiB decoded
payload. Its KTX1 parser advances through header-controlled data and is not the
hostile-media pre-validator Jerico requires. Source dimensions, multiplication
overflow, allocation budgets, mip count, subresource ranges and total work must
be rejected by Jerico before any Diligent or encoder call.

### Reuse decision

| Pinned Diligent capability | M6F-e decision |
|---|---|
| `ComputeMipLevelsCount` | Reuse through already admitted DiligentCore or duplicate only if serialization needs an independent checked implementation |
| Texture format metadata / block dimensions | Reuse behind Jerico's renderer boundary |
| Vulkan BC format mapping | Reuse; Diligent maps BC4/5/6H/7 to the corresponding Vulkan block formats |
| Generic linear data mip | Algorithmic reference; Jerico wrapper must own bounds and semantic recipe |
| sRGB RGBA mip | Do not use unchanged because alpha follows the sRGB filter |
| Normal mip | Do not use unchanged; no vector renormalization |
| BC4/BC5 `stb_dxt` path | Valid fallback/oracle, but a second encoder is unnecessary if CMP_Core is admitted |
| DDS/KTX1 loader | Never expose to hostile source intake; Jerico validates first |
| Complete `TextureLoader` target | Reject for M6F-e |

DiligentCore's Vulkan backend already maps the desired formats and enables the
device feature through its normal feature negotiation:

- [Pinned Vulkan format map](https://github.com/DiligentGraphics/DiligentCore/blob/fa060f50549238578e1701b5d362d7cff2bf48c7/Graphics/GraphicsEngineVulkan/src/VulkanTypeConversions.cpp#L147-L167)
- [Pinned Vulkan BC feature request](https://github.com/DiligentGraphics/DiligentCore/blob/fa060f50549238578e1701b5d362d7cff2bf48c7/Graphics/GraphicsEngineVulkan/src/EngineFactoryVk.cpp#L865-L885)

Runtime support must still be queried per device/format and produce a visible
target-profile error; file format selection is not proof of GPU support.

## Encoder candidates

### 1. AMD Compressonator Core — recommended

**Pin:** stable tag `V4.5.52`, commit
[`9da0bc2be8d095adfab04ba9262e94842b48dac2`](https://github.com/GPUOpen-Tools/compressonator/tree/9da0bc2be8d095adfab04ba9262e94842b48dac2)

**License:** MIT for Core, recorded in the pinned
[`corelicense.txt`](https://github.com/GPUOpen-Tools/compressonator/blob/9da0bc2be8d095adfab04ba9262e94842b48dac2/license/corelicense.txt).

**Fit:** The official repository describes Core as a small, self-contained,
cross-platform, linkable block-codec library. The Core CMake target lists BC1
through BC7 sources, two CPU-extension/math sources from
`applications/_libs/cmp_math`, and three internal SIMD source libraries
(`core_simd_sse.cpp`, `core_simd_avx.cpp`, `core_simd_avx512.cpp`). It does not
link an external third-party library. The C API accepts one 4x4 block at a time:
one byte channel for BC4, two byte channels for BC5, RGBA8 for BC7 and RGB
half-float for BC6H. It exposes quality, BC6/BC7 mode masks, signed BC6 and BC7
alpha restrictions. The official Core documentation says BC4/BC5 quality values
currently have no effect, while BC6/BC7 use the full 0..1 range. It also warns
that the current BC6H codec defaults all modes on; the mode-mask effect must be
proved rather than assumed from the setter's presence.

Primary sources:

- [Official repository and platform statement](https://github.com/GPUOpen-Tools/compressonator/tree/9da0bc2be8d095adfab04ba9262e94842b48dac2#compressonator-core)
- [Pinned Core CMake source closure](https://github.com/GPUOpen-Tools/compressonator/blob/9da0bc2be8d095adfab04ba9262e94842b48dac2/cmp_core/CMakeLists.txt)
- [Pinned Core C API](https://github.com/GPUOpen-Tools/compressonator/blob/9da0bc2be8d095adfab04ba9262e94842b48dac2/cmp_core/source/cmp_core.h)
- [Pinned Core documentation](https://github.com/GPUOpen-Tools/compressonator/blob/9da0bc2be8d095adfab04ba9262e94842b48dac2/docs/source/developer_sdk/cmp_core/index.rst)
- [Official V4.5.52 release](https://github.com/GPUOpen-Tools/compressonator/releases/tag/V4.5.52)

**Why Core, not Framework:** Framework adds image file I/O, mip generation,
plug-ins and multithreaded/HPC pipelines. Jerico already owns hostile media
admission, canonical images, semantic mips, jobs and `.rtext`. Pulling Framework
would duplicate those authorities.

**Determinism caution:** Inspection of this exact pin shows that
`CompressBlockBC4`, `CompressBlockBC5`, `CompressBlockBC6` and
`CompressBlockBC7` are direct scalar C++ block entry points and do not call a
runtime ISA dispatcher. The process-global SIMD API in
`cmp_core/source/cmp_core.cpp` dispatches only BC1. The official `CMP_Core`
CMake target nonetheless always compiles and links its internal SSE, AVX2 and
AVX-512 BC1 libraries. Compiler auto-vectorization is still possible and Core
does not document cross-platform bit-equality. Jerico must initialize BC7 ramps
once on the cook coordinator, never mutate shared options during jobs, use one
immutable option context per worker and treat compiler/architecture equality as
unproven until the hash matrix passes. No `DisableSIMD` cook step is required for
BC4/5/6H/7.

### 2. Intel ISPC Texture Compressor — capable, archived fallback

**Pin:** last upstream commit
[`79ddbc90334fc31edd438e68ccb0fe99b4e15aab`](https://github.com/GameTechDev/ISPCTextureCompressor/tree/79ddbc90334fc31edd438e68ccb0fe99b4e15aab)
(the repository has no release tags).

**License:** MIT, from the pinned
[`license.txt`](https://github.com/GameTechDev/ISPCTextureCompressor/blob/79ddbc90334fc31edd438e68ccb0fe99b4e15aab/license.txt).

**Fit:** It encodes BC4, BC5, BC6H and BC7 and has Windows, Linux and macOS build
paths. Its public API works on complete surfaces and provides quality profiles.
BC6H expects FP16 HDR input; dimensions must be padded to a block multiple and
borders replicated by the caller.

Primary sources:

- [Pinned README, formats and build platforms](https://github.com/GameTechDev/ISPCTextureCompressor/blob/79ddbc90334fc31edd438e68ccb0fe99b4e15aab/readme.md)
- [Pinned surface API and block requirements](https://github.com/GameTechDev/ISPCTextureCompressor/blob/79ddbc90334fc31edd438e68ccb0fe99b4e15aab/ispc_texcomp/ispc_texcomp.h)
- [Pinned Linux SIMD targets](https://github.com/GameTechDev/ISPCTextureCompressor/blob/79ddbc90334fc31edd438e68ccb0fe99b4e15aab/Makefile.linux)

**Rejection for primary:** Intel archived and discontinued the repository in
September 2024. The build documentation still assumes old VS/Xcode SDKs and an
external ISPC compiler. Its Linux build enables `--opt=fast-math` and emits
multiple ISA variants, so cross-toolchain bit identity also needs proof. It is a
useful quality/performance oracle, but creates more maintenance and toolchain
surface than current Compressonator Core.

### 3. Khronos KTX-Software — future intake/transcode option, not M6F-e encoder

**Stable pin reviewed:** `v4.4.2`, commit
[`4d6fc70eaf62ad0558e63e8d97eb9766118327a6`](https://github.com/KhronosGroup/KTX-Software/tree/4d6fc70eaf62ad0558e63e8d97eb9766118327a6).

**Preview pin reviewed:** `v5.0.0-rc1`, commit
[`6269d2752ed04446c2d4749f54f3aad4f94555b5`](https://github.com/KhronosGroup/KTX-Software/tree/6269d2752ed04446c2d4749f54f3aad4f94555b5).

**License:** repository-owned files are generally Apache-2.0, but the project
contains a multi-license dependency closure and explicitly calls out special
cases. An admission must use its REUSE/SPDX bill of materials, not label the
entire tree simply Apache-2.0. See the pinned
[`LICENSE.md`](https://github.com/KhronosGroup/KTX-Software/blob/4d6fc70eaf62ad0558e63e8d97eb9766118327a6/LICENSE.md).

**Fit and limitation:** Stable 4.4.2 can encode Basis ETC1S/UASTC and transcode
that intermediate to BC4/BC5/BC7. Its public transcode enum has no BC6H target.
The v5 release candidate adds UASTC HDR 4x4/6x6 intermediates that transcode to
unsigned BC6H. This is a portable Windows/Linux/macOS/Vulkan-capable container
and transcode stack, but it is not the simplest direct BC4/5/6H/7 block encoder.

Primary sources:

- [Stable transcode targets](https://github.com/KhronosGroup/KTX-Software/blob/4d6fc70eaf62ad0558e63e8d97eb9766118327a6/include/ktx.h)
- [Stable supported build platforms](https://github.com/KhronosGroup/KTX-Software/blob/4d6fc70eaf62ad0558e63e8d97eb9766118327a6/BUILDING.md)
- [Official v5 release-candidate BC6H notes](https://github.com/KhronosGroup/KTX-Software/releases/tag/v5.0.0-rc1)

**Rejection for primary:** M6F-e would pay for Basis encoding plus a second
transcode step, a large dependency/license closure and a release-candidate HDR
path merely to obtain native BC payloads. Keep KTX-Software for a later,
separately admitted hostile KTX2/Basis intake/transcode slice. It must not become
the `.rtext` runtime container.

### bc7enc_rdo — useful BC7 oracle, not a complete candidate

The active untagged upstream commit reviewed is
[`b9438627eef73a1157e84201b6fa6eb2ffd6d9f0`](https://github.com/richgel999/bc7enc_rdo/tree/b9438627eef73a1157e84201b6fa6eb2ffd6d9f0).
It is attractive for fast BC7 and optional RDO/reduced-entropy output, builds on
MSVC and Linux, and its own README notes a BC7E determinism fix. The core code is
dual MIT/public-domain, while bundled BC7E is Apache-2.0.

It does not contain a BC6H encoder, so it cannot provide Jerico's complete
desktop format matrix and would force a second library. RDO also introduces
recipe parameters and cross-block/window behavior that are unnecessary for the
first native GPU-block slice. Retain it only as a later BC7 quality/size oracle.

Primary sources:

- [Pinned README](https://github.com/richgel999/bc7enc_rdo/blob/b9438627eef73a1157e84201b6fa6eb2ffd6d9f0/README.md)
- [Pinned license](https://github.com/richgel999/bc7enc_rdo/blob/b9438627eef73a1157e84201b6fa6eb2ffd6d9f0/LICENSE)
- [Pinned BC7 API](https://github.com/richgel999/bc7enc_rdo/blob/b9438627eef73a1157e84201b6fa6eb2ffd6d9f0/bc7enc.h)

## Semantic mip policy required before encoding

The encoder must never decide texture semantics. The cook recipe supplies one
explicit semantic and the mip stage produces canonical blocks before compression.

### Color and alpha

1. Decode sRGB RGB values to a deterministic linear representation.
2. Keep alpha linear.
3. For ordinary translucent color, filter premultiplied linear RGB and alpha,
   then unpremultiply safely before BC7 input to avoid transparent-edge bleed.
4. For alpha-test materials, calculate target base-level coverage at the authored
   threshold and solve a deterministic per-mip alpha scale that preserves it.
5. Encode RGB back to sRGB bytes only when the `.rtext` format is BC7_SRGB;
   retain alpha as linear bytes.

For strict cross-toolchain byte identity, use a versioned lookup/fixed-point
sRGB conversion rather than depending on compiler-specific floating-point
contraction. The lookup version and rounding rule are part of the recipe ID.

### Normal maps

1. Decode stored channels to signed vector components.
2. Reconstruct Z when the source is XY-only.
3. Average vectors in linear vector space.
4. Renormalize with a defined zero-length fallback (`+Z`).
5. Re-encode XY and compress to BC5 Linear; reconstruct Z in the shader.

Channel-wise RG averaging is not normal-aware and must fail the normal golden
test. Any future roughness adjustment from normal variance is a separate recipe
version, not an implicit side effect.

### Data, masks, ORM/ARM and HDR

- ORM/ARM channels remain linear and independent. Never sRGB-convert them.
- Height and continuous masks use a declared linear filter. Discrete IDs use a
  mode/most-frequent filter and normally should not be BC4 unless loss is
  explicitly accepted.
- HDR mips operate on finite linear floats/halves. Reject NaN/Inf and clamp only
  according to the signed/unsigned BC6H recipe before encoding.
- Odd dimensions replicate/clamp the last texel by a documented rule before
  4x4 block encoding; padding is never visible as an extra logical dimension.

## Determinism contract for M6F-e

The following tuple defines texture-cook identity:

```text
source content hash
+ canonical decoder/version
+ texture semantic and color space
+ mip algorithm/version and rounding
+ alpha policy/threshold
+ encoder name + exact commit
+ format + signedness
+ quality + mode mask + alpha restrictions
+ compiler/target profile
+ target profile
+ .rtext schema version
```

Required implementation rules:

1. Validate and fully bound the canonical image before encoder initialization.
2. Initialize third-party global tables once, on the coordinator thread.
3. Give each worker immutable or private encoder option state.
4. Parallelize by independent `(mip, layer, blockIndex)` jobs, but write results
   into precomputed offsets in canonical mip/layer/raster order. Completion order
   must never affect bytes.
5. Store no timestamp, absolute source path, host CPU name or thread count in the
   content-hashed payload.
6. Hash every mip's stored block stream and the complete `.rtext`.
7. Include encoder errors and the exact recipe in the cook report.

Required proof matrix before dependency admission is marked `PASS`:

| Gate | Minimum evidence |
|---|---|
| Block conformance | Golden encode/decode vectors for BC4, BC5, BC6H UF16/SF16 and BC7 Linear/sRGB |
| Repeatability | Ten cooks in one process plus ten fresh-process cooks produce one SHA per fixture |
| Build equality | Debug and Release `.rtext` bytes match for the accepted CPU policy |
| Thread equality | 1 worker and N workers produce identical bytes |
| Semantic mips | sRGB+alpha, masked coverage, normal renormalization, ORM channel independence and HDR finite-range fixtures |
| Corruption/bounds | Truncated table, bad format, oversized dimensions/mips/payload and invalid offsets reject before allocation/upload |
| Runtime | Source-free Shipping reopens `.rtext`, queries BC support and uploads every mip through Vulkan |
| Quality | Decode-and-measure report records per-semantic error; no silent quality claim |

If Windows x64 and Linux x64 do not produce the same BC hashes, do not hide the
difference. Make the cook artifacts explicit target/toolchain variants and
include that variant in cache identity.

## Resource and security limits

Retain the current Jerico hard ceilings unless an ADR changes them:

- width/height: 1..16384;
- mip entries: at most 32 and geometrically valid;
- total canonical decoded payload: at most 512 MiB;
- checked 64-bit arithmetic for row bytes, mip bytes, block counts and total;
- bounded wall/work budget based on total texels and encoder quality;
- cancellation between mips/block batches;
- no source-controlled output path, plug-in discovery or external process;
- no parser or encoder types in Shipping/public asset schemas.

For BC block sizes, compute storage from logical dimensions using checked
`ceil(width / 4) * ceil(height / 4) * bytesPerBlock`: 8 bytes for BC4 and 16
bytes for BC5/BC6H/BC7. Validate the result before allocation and again when the
reader checks every mip range. Small mips still occupy one complete block.

## Recommended admission shape

```text
hostile source bytes
  -> Jerico admission + canonical decoder
  -> Jerico semantic mip generator
  -> JericoTextureBlockEncoder interface
       -> private CMP_Core CPU adapter (cook/editor only)
  -> versioned .rtext writer + readback validation
  -> source-free package
  -> Diligent Vulkan native BC upload
```

The smallest sound M6F-e dependency change is:

1. write the failing format/mip/determinism tests first;
2. add an ADR and dependency record for only Compressonator Core at `V4.5.52`;
3. declare a Jerico-owned static target from the pinned Core source closure,
   rather than adding the Compressonator root project;
4. expose only a private Jerico block-encoder interface;
5. keep jobs, mips, bounds, recipe, `.rtext` and reporting Jerico-owned;
6. build the adapter only with texture cooking/editor, never Shipping; and
7. retain DiligentTools, ISPCTextureCompressor and bc7enc_rdo as comparison
   oracles, not additional production encoders.

This gives one official cook route, one modern BC encoder family, one source-free
runtime representation and no duplicate image-import or mip authority.
