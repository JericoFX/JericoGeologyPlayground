# M8 RBrush Slice 3 — lossless compression

## State

- Milestone: `M8 ACTIVE / PARTIAL`.
- Slice: `RBrush Slice 3 Compression PASS`.
- Product commit: `0a6b0feee15003c96a27a9a37583384cabf88569`.
- JericoEngine 1.0: `ACTIVE / NOT COMPLETE`.
- Scope: Tool-only lossless compression and selected-tip reads for the RBrush
  1.0 authoring container. No image/ABR importer, publication transaction,
  Editor UI, painter, runtime parser or Shipping source path is claimed.

## Observable result

`writeRBrushContainer` now writes deterministic `Raw`, `Zstd`, `PNG_L8` and
`PNG_L16` TOC entries. For each grayscale TIPD, AUTO compares the complete
canonical Raw payload, complete Zstd payload and the canonical 65-byte TIPD
prefix plus grayscale PNG; it selects the strictly smallest candidate with
stable enum-order tie-breaking. Non-TIPD metadata chunks use Zstd as required.

`readRBrushContainer` validates packed CRC before decompression, bounds both
individual and cumulative unpacked sizes, reconstructs canonical bytes and
recomputes the existing pack SHA-256. `readRBrushContainerTip` validates the
header and complete TOC layout but checks CRC and decompresses only the requested
TIPD. A two-tip test corrupts TIPD A, reads TIPD B exactly, then proves the full
reader and a selected read of A both reject the corruption.

R16 canonical samples remain little-endian in Jerico IR and are swapped to/from
PNG's required big-endian 16-bit channel representation. Synthetic R8 and R16
fixtures choose `PNG_L8` and `PNG_L16` respectively, write byte-identically,
produce `packedSize < unpackedSize`, and round-trip exact pixels plus the exact
stored canonical tip hash.

## Dependency admission

The repository had no admitted neutral Zstd or grayscale PNG encoder. The slice
admits only the two official sources behind the private Tool boundary:

- Zstandard `v1.5.7`, exact commit
  `f8745da6ff1ad1e7bab384bd1f9d742439278e99`, BSD-3-Clause choice;
- LodePNG `20260119`, exact commit
  `ed6fe5825c6a4fbb7f58ab35a4231c7543cd452a`, Zlib.

Zstandard is static compression/decompression only. Programs, tests, contrib,
shared output, dictionary builder, deprecated API, legacy decoders and
multithreading are disabled; configure reports
`ZSTD_LEGACY_SUPPORT not defined!`. LodePNG compiles only `lodepng.cpp` with
disk I/O, ancillary chunks and its C++ wrapper disabled. The feature macros are
part of the ThirdParty target's interface so the adapter and library share the
same `LodePNGState` ABI, while AssetPipeline links that target privately.

Exact license texts are retained in `ThirdParty/Licenses`; the admission contract
locks sources, pins, reduced build, licenses and absence from public Jerico and
direct Editor code. No third-party type or version enters the `.rbrush` ABI.

## Hostile-input and memory contract

- RBrush input, packed chunk, unpacked chunk and cumulative unpacked bytes are
  bounded by the existing 512 MiB limit before allocation.
- Dimension/pixel arithmetic uses checked 64-bit multiplication.
- Zstd requires one complete frame, known content size equal to TOC
  `unpackedSize`, no concatenated/trailing frame and exact decoded byte count.
- PNG requires an exact grayscale 8/16-bit IHDR matching the TIPD prefix,
  bounded dimensions/output, valid PNG/Adler CRCs and the canonical sequence
  `IHDR + IDAT(s) + IEND` with no ancillary or trailing bytes.
- PNG codecs on non-TIPD chunks and PNG_L8/PNG_L16 versus mismatched pixel
  formats are rejected.
- Packed CRC remains over the physical payload. Canonical SHA-256 remains over
  `RBrushPackIR`, independent of physical codec choice.

## TDD evidence

### RED

1. Extending `repository.asset_import_admission_contract` first failed with:

   ```text
   Asset import admission contract is missing the official Zstandard repository
   in ThirdParty/CMakeLists.txt
   ```

2. Synthetic R8/R16 product tests compiled against the Raw Slice 2 writer and
   failed because both codec fields were `Raw` and
   `packedSize == unpackedSize`.
3. The independent read test then failed compilation because
   `readRBrushContainerTip` did not exist.
4. A valid TOC/CRC mutation that appended one byte after PNG IEND was initially
   accepted. The canonical PNG preflight made this negative test GREEN.

During integration, the first large PNG encode exposed an ABI mismatch: LodePNG
feature macros were private to its target, so the adapter saw a different state
layout. Publishing those macros through the dependency target fixed the root
cause without exposing the dependency past AssetPipeline's private link.

### GREEN

The focal test proves:

- deterministic AUTO output for R8 and R16;
- literal PNG_L8/PNG_L16 selection and smaller packed payloads;
- exact canonical pack, tip hash and pixel roundtrip;
- Raw retained for a tiny TIPD when compression is larger;
- Zstd emitted for required non-TIPD chunks;
- independent selected-tip decompression with another TIPD corrupt;
- invalid magic/header CRC/TOC layout/chunk CRC/canonical SHA/reference rejection;
- unknown codec, codec/type mismatch, Zstd size mismatch and noncanonical PNG
  trailing-data rejection;
- stable logical-ID collision rejection before writing.

## Fresh verification

Commands executed from `F:\JericoEngine` against the product commit content:

```text
cmake --preset windows-vulkan-editor
cmake --build --preset windows-vulkan-editor
ctest --test-dir build\windows-vulkan-editor -C Debug --output-on-failure -R "^(repository\.(asset_import_admission_contract|shipping_source_exclusion|test_scheduling_contract)|asset_pipeline\.(rbrush_ir|rbrush_container))$"
cmake --build --preset windows-vulkan-editor --config Release --target JericoRBrushContainerTests
ctest --test-dir build\windows-vulkan-editor -C Release --output-on-failure -R "^(repository\.asset_import_admission_contract|asset_pipeline\.rbrush_container)$"
cmake --preset windows-vulkan-runtime
cmake --build --preset windows-vulkan-runtime --config Release
rg --files build\vulkan-runtime | rg -i "(zstd|lodepng|rbrush)"
git diff --check
```

Observed results:

- Windows Vulkan Editor configure: PASS; Zstd 1.5.7, legacy and multithreading
  disabled.
- Complete Editor Debug build: PASS, exit `0`.
- Debug causal matrix: `5/5 PASS`.
- Release admission/container matrix: `2/2 PASS`.
- Runtime-only Release build and its attached source-free audits: PASS, exit
  `0`; `JERICO_ENABLE_ASSET_IMPORT:BOOL=OFF`.
- Runtime-only filename inventory: `RUNTIME_CODEC_ARTIFACT_MATCHES=0` for
  `zstd|lodepng|rbrush`.
- Scheduling inventory remains `242`, SHA-256
  `ac46998526894bf961bc01f547ff4da9696c10d32e5d81f5556cd654087b899e`;
  scheduled count remains `46`.
- `git diff --check`: PASS. CRLF-conversion notices are warnings, not whitespace
  errors.

The full wider Editor CTest suite was not claimed green; its ten inherited
failures remain documented by the preceding Slice 2 checkpoint and no causal
regression was observed in this Tool-only slice.

## Runtime, Editor, C04 and visuals

- Shipping/runtime: `.rbrush`, ABR and source images remain rejected source
  inputs. Runtime-only does not configure, build or link either codec.
- Editor/UI: no UI is implemented or claimed by this slice.
- C04: the bounded in-memory codec itself has no Qt dependency. Future image/ABR
  import, compression, publication and cook must execute through cancelable
  jobs/processes with progress, ownership, late-callback discard and memory
  limits; this slice does not weaken that dependency.
- Visual evidence: none. Sponza and the user's conceptual images are not evidence
  for a binary codec and remain untouched.

## Remaining work and next legal slice

The real crack ABR size target is not claimed because ABR parsing begins in later
slices and the user fixture is not redistributable CI input. Slice 3 does not
implement image import, ABR, DDC derivatives, Editor tools, painter, SDF/PWAS,
EPLS 2.0 or JericoParticles.

The single next legal action is RBrush Slice 4 Image Importer: PNG, TGA, TIFF,
BMP, JPEG and EXR, with `single image -> RBrush preset` and
`multiple images -> one RBrush pack` as the observable PASS gate. M8 remains
`ACTIVE / PARTIAL`.
