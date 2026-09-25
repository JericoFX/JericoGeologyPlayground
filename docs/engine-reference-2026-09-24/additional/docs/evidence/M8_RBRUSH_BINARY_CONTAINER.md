# M8 RBrush Slice 2 — deterministic binary container

## State

- Milestone: `M8 ACTIVE / PARTIAL`.
- Slice: `RBrush Slice 2 binary container PASS`.
- JericoEngine 1.0: `ACTIVE / NOT COMPLETE`.
- Scope: Tool-only deterministic byte container around the accepted canonical
  RBrush IR. No ABR/image importer, compression, file publication, Editor UI,
  painter, runtime asset loading or Shipping parser is claimed.

## Observable result

`Jerico::Tools::AssetPipeline::writeRBrushContainer` now writes the version 1.0
`.rbrush` ABI and `readRBrushContainer` reconstructs the exact canonical
`RBrushPackIR`. A representative pack with two presets sharing one canonical tip
writes byte-identically twice and reads back value-identically.

The fixed little-endian ABI is:

- 96-byte header with exact `RBRUSH\0\0` magic, version 1.0, zero flags,
  declared file size, TOC offset/count, preset count, AssetGUID, canonical
  SHA-256 and IEEE header CRC32;
- 48-byte TOC entries with explicit type, codec, logical ID, offset,
  packed/unpacked size, IEEE chunk CRC32 and flags;
- deterministic order `META`, zero or more `PRST`, `STRS`, zero or more `TIPD`,
  `PATT`, `DESC`, `UNKN`;
- one independently addressable `PRST` per sorted PresetGUID and one `TIPD` per
  sorted TipGUID;
- real separate manifest, preset, string-table and tip payloads;
- canonical zero-count `PATT`, `DESC` and `UNKN` payloads because Slice 1 does
  not yet define corresponding IR values;
- Raw codec only. `THMB` is omitted because previews belong to DDC.

The canonical content identity is SHA-256 of
`writeRBrushPackIRMemory(pack)`. It is therefore independent of physical chunk
packing and remains stable when Slice 3 introduces lossless codecs.

## Reuse and dependency decision

The container reuses the existing `RBrushPackIR`, stable `Jerico::Guid`,
`Result<EngineError>`, canonical in-memory codec and AssetPipeline Tool boundary.
No parallel identity, IR, asset transaction or publication path was created.

A repository inventory found no Jerico Core/Tool CRC32 or SHA-256 primitive.
The existing C++ SHA-256 use is Qt-bound inside JericoEditor, and Diligent's
admitted xxHash is not SHA-256. Slice 2 adds no dependency and keeps compact
private IEEE CRC32/SHA-256 implementations inside the Tool-only container source.
The SHA implementation is locked against an independently calculated known
answer: canonical empty-pack IR SHA-256
`66160f3b0613b6434299b3363f59b125d848d0e8b0fb68f670c7622e80a24b69`.

## TDD evidence

### RED

1. Registering `asset_pipeline.rbrush_container` first made configuration fail
   truthfully with:

   ```text
   repository.test_scheduling.full_inventory_changed
   expected_count=241
   actual_count=242
   actual_sha256=ac46998526894bf961bc01f547ff4da9696c10d32e5d81f5556cd654087b899e
   ```

   The durable scheduling producer and consumer were updated to that observed
   inventory; scheduled tests remain 46.

2. The first product build failed at the deliberately absent boundary:

   ```text
   fatal error C1083: 'Jerico/AssetPipeline/RBrushContainer.hpp': No such file or directory
   ```

3. Adding declarations only advanced the same RED to the expected unresolved
   `writeRBrushContainer` and `readRBrushContainer` linker symbols.

4. Corruption tests then exposed one classification defect: a dangling
   `primaryTipGuid` returned generic `asset.rbrush.ir_invalid`. GREEN added the
   required container reference validation before final IR validation; the test
   now receives `asset.rbrush.container_reference_invalid`.

### GREEN and negative matrix

The focused test proves:

- deterministic repeated bytes and exact canonical roundtrip;
- literal header fields, sizes and required TOC type order;
- two `PRST` chunks reference one deduplicated `TIPD`;
- SHA-256 known answer and independently recomputed header CRC32;
- invalid magic rejection;
- header CRC mismatch rejection;
- non-Raw codec rejection;
- noncanonical/overlapping chunk offset rejection;
- chunk CRC mismatch rejection;
- canonical SHA-256 mismatch after a valid string/CRC mutation;
- dangling tip-reference rejection after a valid PRST CRC repair;
- stable logical-ID collision rejection before writing.

The reader checks bounds and canonical layout before payload allocation, requires
contiguous non-overlapping chunks, validates packed/unpacked sizes and flags,
limits counts and total input to 512 MiB, consumes every chunk exactly, validates
all string/GUID references, runs canonical IR validation and recomputes SHA-256.

## Fresh verification

Commands executed in `F:\JericoEngine`:

```text
cmake --preset windows-vulkan-editor
cmake --build build\windows-vulkan-editor --config Debug --target JericoRBrushContainerTests --parallel
ctest --test-dir build\windows-vulkan-editor -C Debug --output-on-failure -R "^asset_pipeline\.rbrush_(ir|container)$"
cmake --build --preset windows-vulkan-editor --parallel
ctest --test-dir build\windows-vulkan-editor -C Debug --output-on-failure -R "^(repository\.(asset_import_admission_contract|test_scheduling_contract|shipping_source_exclusion)|asset_pipeline\.(gltf_import|smart_import_preview|smart_import_asset_recipe|rbrush_ir|rbrush_container))$"
cmake --build build\windows-vulkan-editor --config Release --target JericoRBrushIRTests JericoRBrushContainerTests --parallel
ctest --test-dir build\windows-vulkan-editor -C Release --output-on-failure -R "^asset_pipeline\.rbrush_(ir|container)$"
git diff --check
```

Observed results:

- configure: PASS with 242-test inventory and unchanged 46-test scheduled set;
- complete Windows Vulkan Editor Debug build: PASS;
- Debug RBrush IR/container: `2/2 PASS`;
- Debug repository/AssetPipeline causal matrix: `8/8 PASS`;
- Release RBrush IR/container: `2/2 PASS`;
- `git diff --check`: PASS.

The first causal-matrix run was `7/8`: the scheduling manifest producer already
contained 242/new SHA while `VerifyTestScheduling.cmake` still expected three
241-era literals. Systematic tracing identified that exact producer/consumer
drift; after the single consumer correction the isolated scheduling test and
the full 8-test matrix pass.

The wider Editor suite was not used as an acceptance claim. Its ten documented
pre-existing failures from the preceding RBrush checkpoint remain wider-suite
debt outside this Tool-only modified surface unless a future causal regression
demonstrates otherwise.

## Runtime, Editor, Shipping, C04 and visuals

- Editor/runtime: not applicable to this in-memory Tool boundary; no empty UI or
  runtime window is treated as evidence.
- Shipping: `repository.shipping_source_exclusion` passes. Shipping still has no
  `.rbrush`, ABR, raw image or SourceAssets parser.
- C04 Jobs/Async: no Qt/UI call or heavy import/cook is introduced. Operations
  are synchronous bounded byte codecs for unit use; future import, compression,
  publication and cook remain required to run as cancelable owned jobs/processes.
- Visual evidence: none. Sponza and concept art are not evidence for this binary
  format slice and remain untouched.

## Remaining work and next legal slice

Slice 2 does not implement Zstd/PNG_L8/PNG_L16, per-tip codec selection or the
real crack-ABR size target. The single next legal action is RBrush Slice 3:
lossless per-tip compression with exact roundtrip and independently verified
size/codec behavior. M8 remains active after this checkpoint.
