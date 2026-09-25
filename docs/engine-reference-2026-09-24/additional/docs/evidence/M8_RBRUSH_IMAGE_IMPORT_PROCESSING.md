# M8 RBrush Slice 6 — image import processing

## State

- Milestone: `M8 ACTIVE / PARTIAL`.
- Slice: `RBrush Slice 6 Image Import Processing PASS`.
- Product commit: `0047894 feat(asset-pipeline): process rbrush image imports`.
- Design commit: `058128b docs(rbrush): design image import processing`.
- Plan commit: `3507f81 docs(rbrush): plan image import processing`.
- JericoEngine 1.0: `ACTIVE / NOT COMPLETE`.
- Scope: Tool-only processing of already admitted and channel-extracted image
  coverage. No ABR inspect/decode, UI, publication, painter, target adapter,
  cook/runtime, SDF, PWAS/EPLS 2.0 or JericoParticles is claimed.

## Observable result

`RBrushImageExtraction` now owns one typed `RBrushImageProcessing` request:

```text
normalization = None | MinMax | AutoLevels
canvas        = PreserveOriginal | Trim
bit depth     = Preserve | ForceR8 | ForceR16
padding       = 0..16384
invert        = false | true
```

The authoritative default is `None`, `Trim`, `Preserve`, padding `8`, invert
false. A synthetic 6x5 R8 TGA contained this 2x2 coverage center:

```text
10 20
30 40
```

The product test requested MinMax, Trim, padding 1 and ForceR16. It requires the
exact 4x4 result below after R8 normalization and exact R8-to-R16 expansion:

```text
0000 0000 0000 0000
0000 4040 8080 0000
0000 BFBF FFFF 0000
0000 0000 0000 0000
```

The same test requires crop `(x=1,y=0,w=4,h=4)`, source `(R8,6x5)`, extracted
`R8`, levels `(0,40)`, final `(R16,4x4)`, requested/resolved Luminance and the
complete processing request in both manifest and preset metadata. Changing to
PreserveOriginal/None keeps tip and preset GUIDs stable while changing semantic
settings and canonical tip hashes.

## Processing contract demonstrated

The fixed operation order is channel extraction, normalization, inversion,
canvas selection/padding and bit-depth conversion.

- `None` preserves artist samples. It never silently levels or clamps.
- `MinMax` uses the full scalar range. A flat positive range becomes one and a
  flat non-positive range becomes zero.
- `AutoLevels` keeps non-positive samples at zero and maps the positive range.
  Literal R8 input `00 0A 14 1E` produces `00 00 80 FF` and records levels
  `10/30`.
- R8/R16 uses deterministic nearest integer arithmetic. Exact TIFF R16
  `34 12 CD AB` forced to R8 produces `12 AB`; EXR R16F `0.25/1.0` produces
  `40 FF`. R8 forced to R16 expands by `value * 257`.
- Inversion happens before trim. A zero border becomes full coverage and forces
  the resulting crop to preserve the complete source canvas.
- Trim finds positive coverage and expands padding only within the source
  canvas. Default padding 8 on the 6x5 fixture clips to the full 6x5 canvas.
- PreserveOriginal records and outputs `(0,0,sourceWidth,sourceHeight)`; padding
  remains an explicit retained trim preference but has no pixel effect.
- All-zero trimmed masks fail as `asset.rbrush.image_empty_coverage`.
- Inversion or forced integer conversion of unnormalized R16F outside `[0,1]`
  fails as `asset.rbrush.image_coverage_range`; no silent clamp occurs.

## Identity, metadata and formats

`RBrushImageImportMetadata` serializes requested/resolved channel, processing,
decoded source format/dimensions, extracted scalar format, applied levels,
applied crop and final format/dimensions. Canonical validation proves enum,
range, canvas, depth and tip consistency and exact equality between manifest and
preset views. Auto is restricted to its real Alpha/Luminance resolution.

Stable GUID material remains AssetGUID + normalized logical source + requested
channel. Display name and processing do not replace identity. The canonical tip
hash covers final pixels/format/dimensions; semantic settings hash covers the
complete requested processing policy but not source-dependent levels/crop. The
image importer manifest version advances from 2 to 3.

Canonical memory IR advances from schema 1 to schema 2. `.rbrush` advances from
1.0 to 1.1 because META/PRST gain optional typed image metadata. Writers emit
only schema 2/container 1.1. Tests generate and read real schema-1/container-1.0
bytes, synthesize absent metadata and verify the original schema-1 canonical
SHA-256. Crossed 1.0/schema2 input is rejected. TIPD framing remains compatible.

No third-party ABI or dependency was added. Existing libtiff, TinyEXR,
stb_image, Zstd and LodePNG adapters remain private Tool-only dependencies.

## TDD evidence

### Product RED

The final public settings/metadata interface and literal 6x5 product assertions
were added before processing implementation. The focal executable compiled, then
CTest failed with exit `1` only because Slice 5 still returned the unprocessed
tip and absent metadata:

```text
tip pixel format: expected R16, observed R8
tip width/height: expected 4x4, observed 6x5
tip pixels: expected normalized/cropped R16 bytes, observed source R8 bytes
preset sourceInfo.imageImport: absent
manifest source preset imageImport: absent
```

### Compatibility and validation REDs

- IR test failed on schema `1` vs expected `2`, lost metadata and accepted
  inconsistent/non-finite metadata.
- Container test failed on minor `0` vs expected `1`, lost image metadata and
  changed canonical golden hash.
- The legacy-container test linked against the declared private versioned writer
  and failed with unresolved `writeRBrushContainerVersion` before its GREEN.
- A focused validator RED proved that Auto incorrectly admitted a resolved Red
  channel in synthetic RGBA metadata; the validator now permits only Alpha or
  Luminance.

### GREEN

The private `RBrushImageProcessor` performs the bounded scalar transformation,
checks cancellation in pixel passes and returns one complete image plus metadata
or one typed failure. `importRBrushImages` resolves/extracts each channel, runs
the processor, enforces the aggregate 512 MiB IR budget, builds final hashes and
commits metadata to both canonical views only after success.

The focal product test then passed `1/1`. IR/container/importer passed `3/3`,
including defaults, operation order, levels, bit depths, determinism, stable
identity, typed negatives, schema/container compatibility and all Slice 4/5
format/channel/hostile-media regressions.

## Fresh verification

Commands executed from `F:\JericoEngine` against product commit content:

```text
cmake --build build/windows-vulkan-editor --config Debug --target JericoRBrushImageImporterTests -- /m:1 /nr:false
ctest --test-dir build/windows-vulkan-editor -C Debug -R "^asset_pipeline\.rbrush_image_importer$" --output-on-failure
cmake --build build/windows-vulkan-editor --config Debug --target JericoRBrushIRTests JericoRBrushContainerTests JericoRBrushImageImporterTests -- /m:1 /nr:false
ctest --test-dir build/windows-vulkan-editor -C Debug -R "^asset_pipeline\.(rbrush_ir|rbrush_container|rbrush_image_importer)$" --output-on-failure
cmake --build build/windows-vulkan-editor --config Debug -- /m:2 /nr:false
ctest --test-dir build/windows-vulkan-editor -C Debug -R "^(repository\.(asset_import_admission_contract|test_scheduling_contract|texture_codec_shipping_boundary|shipping_source_exclusion|runtime_only_target_discovery)|asset_pipeline\.rbrush_(ir|container|image_importer))$" --output-on-failure
cmake --build build/windows-vulkan-editor --config Release --target JericoRBrushIRTests JericoRBrushContainerTests JericoRBrushImageImporterTests -- /m:1 /nr:false
ctest --test-dir build/windows-vulkan-editor -C Release -R "^(repository\.asset_import_admission_contract|asset_pipeline\.rbrush_(ir|container|image_importer))$" --output-on-failure
cmake --preset windows-vulkan-runtime
cmake --build build/vulkan-runtime --config Release -- /m:2 /nr:false
git diff --check
```

Observed results:

- Product RED: expected behavioral failure, exit `1`.
- Product GREEN: `1/1 PASS`.
- RBrush IR/container/importer Debug: `3/3 PASS` in `0.65 s` on the final focal
  rerun.
- Complete Windows Vulkan Editor Debug build: PASS, exit `0` in `33.8 s`.
- Debug causal/Shipping matrix: `8/8 PASS` in `71.72 s`.
- Release admission plus RBrush IR/container/importer: `4/4 PASS` in `0.62 s`.
- Runtime-only configure and complete Release build: PASS;
  `JERICO_ENABLE_ASSET_IMPORT:BOOL=OFF`.
- Runtime project inventory: zero AssetPipeline, image processor/importer,
  libtiff, TinyEXR or stb decoder hits.
- Runtime binary inventory: zero `importRBrushImages`, `processRBrushImage`,
  `TIFFClientOpen`, TinyEXR, `stbi_load` or `RBRUSH` hits.
- Scheduling contract: inventory remains `243`, SHA-256
  `c70619e366bc9c94ca0e8c5121ae5d147878ea5cc87f0318b87bf5591ed47ce3`;
  scheduled count remains `46`.

## Runtime, Editor, C04 and visuals

- Shipping/runtime: unchanged Tool-only boundary. No source image parser,
  processor or Editor `.rbrush` reader is linked.
- Editor/UI: no modal, workspace or publication control is implemented. ADR-032
  remains the future shared-World/modal integration authority.
- C04: owned immutable requests, bounded settings/output, cooperative
  cancellation and all-or-nothing packs are implemented. Qt job scheduling,
  progress, late-callback discard, concurrency/memory coordination and shutdown
  remain `PARTIAL` under GAP-10/PB-06.
- Visual evidence: none applies. Sponza and the user's conceptual images were
  not used as proof and remain untouched.

## Remaining work and next legal slice

Slice 6 does not inspect or decode ABR, add UI/publication, paint, cook/runtime,
SDF, PWAS/EPLS 2.0 or JericoParticles.

The single next legal action is RBrush Slice 7 ABR Inspect: add `abr_inspect` and
prove on the crack fixture `v6.2`, `samp/desc/patt`, 15 samples, 15 descriptors,
2500x2500 R8 and PackBits. M8 and JericoEngine 1.0 remain active.
