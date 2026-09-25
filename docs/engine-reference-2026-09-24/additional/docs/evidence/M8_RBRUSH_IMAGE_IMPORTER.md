# M8 RBrush Slice 4 — image importer

## State

- Milestone: `M8 ACTIVE / PARTIAL`.
- Slice: `RBrush Slice 4 Image Importer PASS`.
- Product commit: `19c168ddc23a4060f32dcd36157a12def0bebf1a`.
- JericoEngine 1.0: `ACTIVE / NOT COMPLETE`.
- Scope: Tool-only in-memory import of PNG, TGA, TIFF, BMP, JPEG and EXR
  sources into canonical RBrush IR. No channel extraction, image processing,
  ABR, file publication, Editor UI, painter, cook or runtime parser is claimed.

## Observable result

`importRBrushImages` accepts an owned request containing a non-zero destination
AssetGUID and one or more named source byte arrays. One image produces one
source record, one canonical tip and one preset. Six images in either selection
order produce one byte-identical pack containing six linked source records,
tips and presets; the result survives exact IR serialization and `.rbrush`
container write/read.

The importer preserves the channels representable by Slice 1 IR without
advancing Slice 5: R8/R16 grayscale, RGB8, RGBA8 and scalar R16F. Lossless
fixture samples are asserted byte-for-byte for TGA R8, TIFF R16 and EXR R16F.
Image-derived presets receive the section-30 defaults: 25% spacing, full
hardness/roundness, zero angle/jitter/scatter, count one, full opacity/flow and
all controls off.

Source order never defines identity. Tip and preset GUIDs are domain-separated
SHA-256 derivatives of the destination AssetGUID plus case-normalized logical
source name. Source bytes, canonical tip bytes and default semantics receive
separate hashes; tips, presets and source records are sorted according to the
canonical IR contract before validation.

## Dependency admission

The existing memory-only stb_image 2.29 adapter now enables its already pinned
BMP and TGA decoders beside PNG/JPEG. TIFF and EXR use two official, exact,
Tool-only pins behind private Jerico adapters:

- libtiff 4.7.2 at
  `d01a94be176f5f6a87f7ee1c0b32e65416aa2b4d`;
- TinyEXR v1.0.13 at
  `4946b5d92e13bcc8102ac2c8efd129596a90bf75`.

libtiff is static C only. Tools, tests, contrib, docs, install, deprecated API,
C++, OpenGL and every optional external codec are disabled; the admitted subset
retains uncompressed, LZW and PackBits scanlines. TinyEXR bypasses its upstream
compile-test project and builds only `tinyexr.cc` plus bundled miniz with
threads, OpenMP and ZFP disabled. Exact pinned license clauses are retained in
`ThirdParty/Licenses`; libtiff Markdown trailing spaces were removed solely to
satisfy the repository whitespace gate.

No upstream type enters a public Jerico header or serialized ABI. The repository
contract scans public/runtime headers and direct Editor consumers for libtiff,
TinyEXR and their symbols.

## Hostile-input and memory contract

- Source names are bounded normalized project-relative paths; empty, absolute,
  parent and duplicate case-normalized names are rejected.
- Extension and magic/structural signature must agree before a decoder runs.
- Encoded source, decoded working set, canonical image and aggregate canonical
  pack payloads are bounded at 512 MiB with checked arithmetic.
- Width and height are non-zero and at most 16384 pixels.
- TIFF admits one contiguous scanline directory, UINT grayscale/RGB/RGBA,
  top-left or bottom-left orientation and NONE/LZW/PackBits compression.
- EXR admits one non-deep, non-tiled, single-part HALF/FLOAT scalar channel;
  NaN, infinity and values that overflow finite R16F are rejected.
- The request owns immutable source snapshots. `std::stop_token` is checked
  before validation, before each source, after stb decode and while
  canonicalizing TIFF/EXR rows or samples. Failure returns no partial IR.
- All public failures are normalized to actionable
  `asset.rbrush.image_*` errors.

## TDD evidence

### RED

1. The dependency admission test was extended first and failed with:

   ```text
   Asset import admission contract is missing the official libtiff repository
   in ThirdParty/CMakeLists.txt
   ```

2. The complete public interface and six-format product test were added before
   implementation. The test compiled and failed only at link with
   `LNK2019/LNK1120` for the missing `importRBrushImages` symbol.
3. The first GREEN execution exposed a corrupt synthetic JPEG literal. Windows'
   native decoder independently reported `SOS before SOF`; replacing only the
   fixture with a valid in-memory 2x1 JPEG preserved the product contract.
4. Adding test 243 first failed the repository scheduling inventory. The
   producer and verifier now share count `243` and the fresh inventory hash.

### GREEN

The focal test proves:

- all six required formats import from bounded in-memory fixtures;
- exact format, dimensions, non-empty pixels and lossless scalar samples;
- one image creates one preset and many images create one deterministic pack;
- non-zero stable GUID/source/tip/settings hashes and exact manifest links;
- IR serialization equality across reversed selection order;
- exact `.rbrush` container round-trip;
- zero GUID, empty source list/bytes, unsafe names, duplicate names, invalid
  retention, extension/magic mismatch, unsupported extension, truncation,
  oversized dimensions, unsupported TIFF compression, multipart EXR and
  non-finite EXR rejection;
- pre-cancellation returns `asset.rbrush.image_cancelled` without partial IR.

The SHA-256 implementation was moved, not copied, from the container into the
private `RBrushHash` seam. Existing container tests prove no digest/ABI drift.

## Fresh verification

Commands executed from `F:\JericoEngine` against the product commit content:

```text
cmake --build build/windows-vulkan-editor --config Debug -- /m:2 /nr:false
ctest --test-dir build/windows-vulkan-editor -C Debug -R "^(repository\.(asset_import_admission_contract|test_scheduling_contract|texture_codec_shipping_boundary|shipping_source_exclusion|runtime_only_target_discovery)|asset_pipeline\.rbrush_(ir|container|image_importer))$" --output-on-failure
cmake --build build/windows-vulkan-editor --config Release --target JericoRBrushImageImporterTests -- /m:1 /nr:false
ctest --test-dir build/windows-vulkan-editor -C Release -R "^(repository\.asset_import_admission_contract|asset_pipeline\.rbrush_(ir|container|image_importer))$" --output-on-failure
cmake --preset windows-vulkan-runtime
cmake --build build/vulkan-runtime --config Release -- /m:2 /nr:false
rg -a -l -i <forbidden-symbol> build/vulkan-runtime -g "*.exe" -g "*.dll" -g "*.lib"
git diff --check
```

Observed results:

- Complete Windows Vulkan Editor Debug build: PASS, exit `0`.
- Debug causal/Shipping matrix: `8/8 PASS` in 70.37 s.
- Release admission plus RBrush IR/container/importer: `4/4 PASS`.
- Runtime-only configure and complete Release build: PASS;
  `JERICO_ENABLE_ASSET_IMPORT:BOOL=OFF`.
- Runtime solution/project inventory: zero AssetPipeline, libtiff, TinyEXR,
  stb_image decoder or RBrush importer targets.
- Runtime binary symbol inventory: zero `TinyEXR`, `TIFFClientOpen`,
  `stbi_load`, `importRBrushImages` or `RBRUSH` matches. Diligent's unrelated
  `stb_image_write` header/license remains only in its generated ThirdParty
  build tree and is not an importer target or runtime binary symbol.
- Scheduling inventory: `243`, SHA-256
  `c70619e366bc9c94ca0e8c5121ae5d147878ea5cc87f0318b87bf5591ed47ce3`;
  scheduled count remains `46`.
- Staged `git diff --check`: PASS after retaining license clauses without
  upstream Markdown trailing spaces.

One transient `LNK1236` occurred when three tests sharing AssetPipeline were
requested as parallel top-level MSBuild targets. MSVC `dumpbin` validated both
the object and archive; rebuilding the same artifact once and linking targets
serially passed without a source change. Verification therefore builds the
shared target once or uses `/m:1` for multi-target focal commands.

## Runtime, Editor, C04 and visuals

- Shipping/runtime: no source image parser, RBrush importer, libtiff or TinyEXR
  target exists in runtime-only; `.rbrush` remains Editor-native.
- Editor/UI: this pure importer seam has no Qt, file dialog, modal, catalog or
  publication side effects. ADR-032's large typed import modals remain future
  UI work in roadmap order.
- C04: this slice supplies an owned immutable request, bounded memory,
  cooperative cancellation and all-or-nothing result. It does not claim the
  future Qt job scheduling/progress/late-callback/shutdown integration; C04
  remains `PARTIAL` with GAP-10/PB-06.
- Visual evidence: none applies. Sponza and the user's conceptual images were
  not used as proof and remain untouched.

## Remaining work and next legal slice

Slice 4 does not select Alpha/Luminance/R/G/B or split packed channels. It does
not invert, trim, pad, normalize, add to an existing asset transaction, parse
ABR, expose UI or feed painter/SDF/PWAS/EPLS.

The single next legal action is RBrush Slice 5 Image Channel Extraction:
`Auto`, `Alpha`, `Luminance`, `R`, `G`, `B`, `RGB Average`, `RGB Min` and
`RGB Max`, with one RGBA input producing four requested R/G/B/A presets in one
import as the observable PASS gate. M8 and JericoEngine 1.0 remain active.
