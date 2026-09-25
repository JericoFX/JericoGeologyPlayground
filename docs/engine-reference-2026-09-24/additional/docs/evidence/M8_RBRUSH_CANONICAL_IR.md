# M8 — RBrush Canonical IR

Date: 2026-08-11

RBrush vertical Slice 1 status: **PASS**

Milestone M8 status: **ACTIVE / PARTIAL**

JericoEngine 1.0 Goal status: **ACTIVE / NOT COMPLETE**

## Observable result

`Jerico::Tools::AssetPipeline` now owns one canonical authoring model for an
RBrush pack, its presets, tip payloads, dynamics and source manifest. The model
reuses `Jerico::Guid` for pack, preset and tip identity and
`Result<EngineError>` for visible validation failures.

The in-memory schema preserves those identities and exact source-manifest
references, emits deterministic little-endian bytes and round-trips back to an
equal value. It rejects zero or duplicate identities, missing tip references,
mismatched manifests, invalid normalized values, malformed group/tag metadata,
wrong pixel sizes, truncation and trailing bytes.

This is the canonical payload foundation only. It is not the physical
`.rbrush` container and does not add its header, TOC, chunks, CRC or content
hash.

## Reused authority and product seams

- Identity: `Jerico::Guid` from `Engine/Core`; no second GUID type.
- Error contract: existing `Result<EngineError>` with stage
  `asset.rbrush.ir`.
- Tool ownership: existing `Jerico::Tools::AssetPipeline`; no Editor or runtime
  dependency.
- Future publication: `EditorProjectContext` and
  `ProjectTransactionCoordinator` remain the only project/asset transaction
  path. This slice performs no project publication.
- Digests: a Jerico-owned 32-byte digest value records already-computed source
  and semantic identities. No hashing library or alternate asset identity was
  introduced.

## RED evidence

The first targeted build reached the new product test and failed because the
canonical product interface did not exist:

```text
RBrushIRTests.cpp(1,10): fatal error C1083: cannot open include file
'Jerico/AssetPipeline/RBrushIR.hpp': No such file or directory
```

Before that compile, CMake correctly stopped once because the checked test
inventory had changed. The scheduling manifest was then updated from the
observed inventory (`241`, SHA-256
`0556b31f83629f909e809d8fc3895bdd25020d4e568df7fea5ed10e7d81092f2`)
without changing the scheduled set (`46`). The next run produced the authentic
missing-interface RED above.

## GREEN evidence

Targeted Debug build:

```text
cmake --build --preset windows-vulkan-editor --config Debug \
  --target JericoRBrushIRTests -- /m:2 /nr:false
Exit code: 0
```

Direct product execution:

```text
build/windows-vulkan-editor/Tests/AssetPipeline/Debug/JericoRBrushIRTests.exe
Exit code: 0
```

CTest focal:

```text
asset_pipeline.rbrush_ir: 1/1 PASS, 0 failures
```

Fresh Debug regression/boundary matrix:

```text
repository.asset_import_admission_contract PASS
repository.test_scheduling_contract        PASS
repository.shipping_source_exclusion       PASS
asset_pipeline.gltf_import                  PASS
asset_pipeline.smart_import_asset_recipe   PASS
asset_pipeline.rbrush_ir                    PASS
6/6 PASS, 0 failures, 6.46 s
```

Fresh Release build of the three AssetPipeline targets passed. The same Release
matrix passed `6/6`, `0` failures in `6.05 s`.

`git diff --cached --check` passed over the complete staged slice immediately
before the final verification builds and is repeated before publication.

A complete Debug build also passed. The unconstrained complete CTest preset was
executed and is not green: `229/241` passed. Repeating its 12 failures serially
recovered `editor.world_level_bake_transaction` and
`editor.material_instance_workspace`; `10/12` still failed. None of those ten
tests links or exercises RBrush IR, and no Editor/runtime product source changed
in this slice. They remain an explicit global regression/debt finding rather
than evidence for this RBrush PASS boundary. The applicable six-test RBrush,
AssetPipeline, scheduling, import-admission and Shipping matrices remain green
in both Debug and Release. Their final post-suite rerun passed `6/6` in `3.07 s`
for Debug and `6/6` in `2.49 s` for Release.

## C04, Editor, runtime and Shipping boundary

This slice is a bounded in-memory value model and codec. It performs no file
I/O, Qt work, background scheduling, import, compression, cook, bake or package
operation, so it has no job to dispatch and no UI/runtime proof to claim.

RBrush import, compression, physical container loading and future cook remain
subject to `C04_ASYNC_OWNER_CONTRACT`: cancelable bounded work off the Qt thread,
immutable snapshots, owner-thread result application, stale callback discard,
safe shutdown, determinism and responsiveness evidence.

`JericoAssetPipeline` remains Tool-only and
`repository.shipping_source_exclusion` passed in both Debug and Release. No
RBrush reader or source payload was linked into Shipping.

No concept art or user-owned untracked file was used as evidence.

## Scope boundary and next legal action

M8 is not complete. Terrain, Smart Materials, painter/adapters, Sponza, EPLS
baseline, fog and bounded water/ocean remain future M8 work. Full SDF/PWAS/EPLS
2.0 and JericoParticles were not advanced.

The next legal RBrush step is Slice 2: wrap this exact canonical payload in the
specified deterministic `.rbrush` binary container with header, TOC, chunks,
CRC and content hash. It must not create a second IR or bypass the existing
asset transaction path.

Estado: PASS for RBrush vertical Slice 1 only; M8 ACTIVE/PARTIAL

Resultado: canonical bounded RBrush IR with deterministic in-memory roundtrip

Pruebas ejecutadas: focal RED/GREEN, Debug and Release AssetPipeline regressions,
scheduling, import admission and Shipping/source exclusion

Evidencia: this file and the exact commands/results above

Faltantes: RBrush Slices 2+, the remaining M8 terrain/surface deliverables and
the complete JericoEngine 1.0 Goal

Riesgos: the wider Debug suite retains ten unrelated failures (two Editor
unit/integration, Smart Import/Qt deployment and six editor visual proofs); later
container/import/cook work is heavy and must satisfy C04 without moving project
publication or callbacks onto unsafe owners

Siguiente slice: RBrush Slice 2 deterministic binary container
