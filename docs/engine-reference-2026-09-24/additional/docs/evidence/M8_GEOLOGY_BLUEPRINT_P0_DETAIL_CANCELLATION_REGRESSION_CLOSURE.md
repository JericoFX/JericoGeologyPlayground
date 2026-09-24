# M8 Geology Blueprint P0 Detail Cancellation Regression Closure

Date: 2026-08-29

Slice: `M8-GEOLOGY-BLUEPRINT-P0-DETAIL-CANCELLATION-REGRESSION`

Local status: `PASS`

Blueprint status: unchanged `PARTIAL`; only the repeated complete-blueprint exit
audit may decide blueprint `PASS` after fresh source-free Vulkan/Beauty proof.

## Observable result

The existing Release and Debug P0 Detail gates now cancel at the intended
checkpoint after bounded Detail dispatch and before dense Surface Nets
extraction. The cancellation still returns
`procedural.geology.gpu_cancelled`, publishes no evaluation result and leaves
the accepted Detail calculation unchanged.

No recipe, noise formula, CPU reference, shader, field, mask, topology,
Surface Nets, work budget, memory budget, cook format or runtime semantic
changed.

## Preserved RED and actual poll-10 cause

Before the repair, the existing Release `--detail` gate exited `1` twice at the
unchanged exact-cause assertion. Fresh bounded instrumentation printed the
actual cause returned when the test's tenth cancellation poll became true:

```text
The designer cancelled between bounded GPU geology dispatches.
```

The failed run still emitted the frozen productive witness:

```text
baseline_hash=15244624087024613006
fbm_hash=16937973488845065865
ridged_hash=14312245170672981847
vector_hash=10423780136070414906
parity_max_m=3.87942e-08
ridged_dirty_voxels=4074
mask_dirty_voxels=12
```

The temporary diagnostic print was removed after localization.

## First causal divergence

The original Detail closure at `ae1328f87` used the tenth poll to reach the
existing pre-Surface-Nets checkpoint. Commit `c6f2a7b98`, which admitted the P1
DirectedCrack Vulkan path, later added this cancellation checkpoint after its
resource upload:

```text
The designer cancelled after DirectedCrack resource upload.
```

That check was outside the `DirectedCrack` layer condition. It therefore ran
for every admitted layer, including `Noise/Detail`, consumed one cancellation
poll and shifted poll 10 into Detail's multi-dispatch loop. The returned cause
was accurate for that shifted location, but the extra checkpoint and its
DirectedCrack-specific identity were semantically invalid for Detail.

This is an evaluator-stage semantic defect, not a Detail-math defect and not a
reason to weaken or rename the test expectation.

## Minimal repair

`GeologyGpuSessionDiligent.cpp` now evaluates the post-upload DirectedCrack
cancellation checkpoint only when the active layer kind is `DirectedCrack`.
All generic per-layer, per-dispatch, dirty-comparison and pre-Surface-Nets
checkpoints remain unchanged.

The existing P0 Detail RED test is the regression contract: removing the guard
reproduces the exact failure, while the guard restores poll 10 to
`The designer cancelled before dense GPU surface extraction.`. The test also
requires the cancelled evaluation to have no value, preserving no-publication.

## GREEN witnesses

Fresh direct builds of `JericoGeologyGpuSessionTests` passed in Release and
Debug. Both `--detail` runs exited `0` with the exact frozen witness:

| Witness | Release | Debug |
|---|---:|---:|
| baseline hash | `15244624087024613006` | `15244624087024613006` |
| FBM hash | `16937973488845065865` | `16937973488845065865` |
| Ridged hash | `14312245170672981847` | `14312245170672981847` |
| Vector hash | `10423780136070414906` | `10423780136070414906` |
| maximum parity | `3.87942e-08 m` | `3.87942e-08 m` |
| Ridged dirty voxels | `4,074` | `4,074` |
| masked dirty voxels | `12` | `12` |

Additional focused regressions exited `0`:

- complete Release `JericoGeologyRecipeTests` CPU recipe/reference suite;
- Release `JericoGeologyGpuSessionTests --directed-crack`, preserving field,
  mesh, dirty-work and `2.83122e-09 m` parity witnesses;
- `git diff --check` for the owned publication paths.

A read-only focused review found no critical, important or minor issue and
accepted the one-condition repair.

## End-to-end system completeness audit

Data lineage for this regression repair is:

```text
P0 Detail cancellation predicate
-> generic source/layer checkpoints
-> bounded Detail GPU dispatches
-> pre-Surface-Nets cancellation checkpoint
-> stable EngineError code/cause
-> no GeologyGpuEvaluationResult publication
```

Applicable categories were GPU orchestration, cancellation/failure handling,
deterministic diagnostics, testing and documentation. Source data, recipe
serialization, CPU/GPU Detail math, semantic fields, masks, Surface Nets,
materials, renderer/shaders, physics/collision, cook/package, ECS/runtime
representation, save/load, streaming/LOD, destruction, navigation, scripting,
events, Sequencer, particles, audio, editor tooling/UI and profiling budgets
were not modified.

No upstream semantic geology information was added, removed, compressed or
reinterpreted. The exact hashes, parity, masks and dirty-work witnesses prove
that the repaired control flow reaches the same productive data.

`INTEGRATION REQUIRED` remains unchanged for the repeated current source-free
renderer/Beauty replay and the broader downstream geology integrations already
recorded by the complete-blueprint audit. This regression slice does not claim
or attempt those results.

## Scope and continuity

The 182 foreign default-status entries remain untouched with SHA-256
`2e996b1669df5ad1dd4e0cb9c3cbaf1b8bfc73ba6bc734abbad0a49419954030`.
No renderer/CMake repair, other P0/P1/P2 operator, transport,
recrystallization, VDB/UI, bespoke preset, general framework or production
expansion was opened.

Next slice: exactly one `M8-GEOLOGY-BLUEPRINT-EXIT-AUDIT` successor repeats the
complete audit and alone decides the eventual blueprint status.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
