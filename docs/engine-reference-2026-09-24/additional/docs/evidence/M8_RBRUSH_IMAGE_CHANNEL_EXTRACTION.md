# M8 RBrush Slice 5 — image channel extraction

## State

- Milestone: `M8 ACTIVE / PARTIAL`.
- Slice: `RBrush Slice 5 Image Channel Extraction PASS`.
- Product commit: `079e137b6209c35ef3ef5a8ba76cdb2e75b9dfc5`.
- Design commit: `77fe065d98454f937da2f2b8bbd158d2d7fbd9da`.
- Plan commit: `dcbbf3485482c99b4133c8b23b9720ff4c5f9538`.
- JericoEngine 1.0: `ACTIVE / NOT COMPLETE`.
- Scope: Tool-only selection of Auto, Alpha, Luminance, R, G, B, RGB
  Average, RGB Minimum and RGB Maximum from the canonical image decoded by
  Slice 4. No Slice 6 processing, ABR, UI, publication, painter, target adapter,
  cook or runtime parser is claimed.

## Observable result

`RBrushImageSource` now owns one or more `RBrushImageExtraction` requests. The
default is one `Auto` request. One synthetic 2x1 RGBA TGA requested Red, Green,
Blue and Alpha with the preset names `crack`, `chip`, `dirt` and `erosion`; one
import returned one source record linked to four canonical R8 tips and four
presets.

The source pixels were hand-authored as `(10,20,30,40)` and
`(200,150,100,250)`. The test requires these literal outputs:

```text
Red          0A C8
Green        14 96
Blue         1E 64
Alpha        28 FA
Luminance    13 9D
RGB Average  14 96
RGB Minimum  0A 64
RGB Maximum  1E C8
```

The multi-channel pack survives exact `.rbrush` write/read. Reversing the
requested mode order produces byte-identical serialized IR. Changing only a
preset display name preserves tip GUID, preset GUID and semantic settings hash.
Unnamed multi-mode requests derive readable `<source stem> <mode>` names.

## Channel contract

- Auto selects Alpha only when an RGBA alpha plane varies; uniform alpha and
  RGB input fall back to Luminance.
- Luminance uses deterministic BT.709 integer weights
  `(13933*R + 46871*G + 4732*B + 32768) >> 16`.
- RGB Average rounds to nearest integer with `(R + G + B + 1) / 3`; minimum and
  maximum are exact.
- RGB/RGBA selection outputs only R8 coverage; unused channels are discarded.
- R8, R16 and R16F scalar input accepts Auto/Luminance as identity. Exact TIFF
  R16 bytes `34 12 CD AB` and EXR R16F bytes `00 34 00 3C` survive both modes.
- Alpha on RGB, color selections on scalar, duplicate/unknown modes, zero
  selections and names above 256 bytes fail through actionable
  `asset.rbrush.image_*` errors with no partial IR.
- No path silently inverts, normalizes, crops, pads or converts scalar depth.

## Identity, metadata and limits

Selections are validated and sorted before decode. Each image is decoded once.
Mode-specific tip/preset GUIDs derive from destination AssetGUID, normalized
source name and requested mode; display name, request order and source pixels do
not define identity. A sole default Auto request retains the Slice 4 GUID
purpose strings. `sourceInternalId` and semantic settings hash record the
requested mode and Auto rule. The image importer manifest version is now `2`;
the canonical IR schema and `.rbrush` container schema do not change.

The private `RBrushImageChannelExtractor` receives immutable canonical pixels.
It validates layout and checked sizing, checks cancellation during alpha scans,
scalar copies and extraction loops, and returns a complete scalar image or one
typed failure. The importer preflights each extracted output against the
existing 512 MiB aggregate IR limit before allocation. Existing 16384 dimension,
hostile extension/magic, decoder and encoded-input limits remain unchanged.

No dependency was added. libtiff, TinyEXR and stb_image remain private Tool-only
adapters already admitted by Slice 4.

## TDD evidence

### RED

The public enum/request types and complete product assertions were written
before channel extraction implementation. The focal executable compiled against
the extended interface, then CTest failed with exit `1` because the Slice 4
implementation ignored the request:

```text
manifest source presets: expected 4, observed 1
pack tips: expected 4, observed 1
pack presets: expected 4, observed 1
requested display names absent
invalid/duplicate/unavailable modes accepted
```

This was the intended behavioral RED, not a fixture, compiler or linker error.

### GREEN

After adding the private extractor and integrating multi-preset IR, the same
test passed `1/1`. The causal RBrush trio then passed `3/3` and includes:

- exact R/G/B/A, luminance and RGB aggregate samples;
- Auto alpha variation and luminance fallback;
- exact R16/R16F scalar preservation;
- deterministic request-order normalization;
- display-name-independent stable identity;
- derived names and all manifest links;
- typed invalid/duplicate/unavailable selection rejection;
- cancellation without partial IR;
- six source-format regressions, hostile-media regressions and exact IR/
  container round-trips.

## Fresh verification

Commands executed from `F:\JericoEngine` against the product commit content:

```text
cmake --build build/windows-vulkan-editor --config Debug --target JericoRBrushImageImporterTests -- /m:1 /nr:false
ctest --test-dir build/windows-vulkan-editor -C Debug -R "^asset_pipeline\.rbrush_image_importer$" --output-on-failure
cmake --build build/windows-vulkan-editor --config Debug --target JericoAssetPipeline -- /m:2 /nr:false
cmake --build build/windows-vulkan-editor --config Debug --target JericoRBrushImageImporterTests JericoRBrushContainerTests JericoRBrushIRTests -- /m:1 /nr:false
ctest --test-dir build/windows-vulkan-editor -C Debug -R "^asset_pipeline\.(rbrush_image_importer|rbrush_container|rbrush_ir)$" --output-on-failure
cmake --build build/windows-vulkan-editor --config Debug -- /m:2 /nr:false
ctest --test-dir build/windows-vulkan-editor -C Debug -R "^(repository\.(asset_import_admission_contract|test_scheduling_contract|texture_codec_shipping_boundary|shipping_source_exclusion|runtime_only_target_discovery)|asset_pipeline\.rbrush_(ir|container|image_importer))$" --output-on-failure
cmake --build build/windows-vulkan-editor --config Release --target JericoRBrushImageImporterTests -- /m:1 /nr:false
ctest --test-dir build/windows-vulkan-editor -C Release -R "^(repository\.asset_import_admission_contract|asset_pipeline\.rbrush_(ir|container|image_importer))$" --output-on-failure
cmake --preset windows-vulkan-runtime
cmake --build build/vulkan-runtime --config Release -- /m:2 /nr:false
git diff --check
```

Observed results:

- Focal RED: expected failure, exit `1`, for ignored extraction behavior.
- Focal GREEN: `1/1 PASS`.
- RBrush IR/container/importer Debug: `3/3 PASS`.
- Complete Windows Vulkan Editor Debug build: PASS, exit `0`.
- Debug causal/Shipping matrix: `8/8 PASS` in `71.15 s`.
- Release admission plus RBrush IR/container/importer: `4/4 PASS` in `0.54 s`.
- Runtime-only configure and complete Release build: PASS;
  `JERICO_ENABLE_ASSET_IMPORT:BOOL=OFF`.
- Runtime solution/project inventory: zero AssetPipeline, channel extractor,
  RBrush image importer, libtiff, TinyEXR or stb decoder targets.
- Runtime binary inventory: zero `importRBrushImages`,
  `extractRBrushImageChannel`, `TIFFClientOpen`, TinyEXR parse, `stbi_load` or
  `RBRUSH` symbols.
- Scheduling inventory remains `243`, SHA-256
  `c70619e366bc9c94ca0e8c5121ae5d147878ea5cc87f0318b87bf5591ed47ce3`;
  scheduled count remains `46`.

One verification invocation used an accidental `5050 ms` command timeout and
aborted its parent build. The immediate overlapping retry showed a locked MSBuild
tracking file and one ShaderCook custom action failure. The exact failing render
target passed isolated without a source change, then the single non-overlapping
complete Debug build passed. This was diagnosed as verification-process overlap,
not recorded as a product regression.

## Runtime, Editor, C04 and visuals

- Shipping/runtime: unchanged Tool-only boundary; no source image parser,
  extractor or Editor `.rbrush` reader is linked.
- Editor/UI: this slice exposes no modal, workspace or publication control.
  ADR-032 remains the future integration authority.
- C04: owned immutable requests, bounded memory, cooperative cancellation and
  all-or-nothing results are implemented. Qt job scheduling, progress,
  late-callback discard and shutdown are not claimed; C04 remains `PARTIAL`
  with GAP-10/PB-06.
- Visual evidence: none applies. Sponza and the user's conceptual images were
  not used as proof and remain untouched.

## Remaining work and next legal slice

Slice 5 does not invert, trim, pad, preserve a canvas, normalize or convert bit
depth. It does not add ABR, UI, publication, painter, cook/runtime, SDF,
PWAS/EPLS 2.0 or JericoParticles.

The single next legal action is RBrush Slice 6 Image Import Processing:
`invert`, `trim`, `padding`, `preserve canvas`, bit-depth conversion and
normalization modes. Its observable PASS is exact metadata recording of every
conversion. M8 and JericoEngine 1.0 remain active.
