# M8 RBrush Slice 36 — Runtime Dynamic Derivative

Status: `PASS` for Slice 36 only. M8 and the JericoEngine 1.0 master Goal remain
`ACTIVE / NOT COMPLETE`.

## Demonstrated boundary

One canonical RBrush pack with a `9x9` R8 tip and pressure/random/fade dynamics
is projected by the tools-only `RBrushRuntimeDerivativeCooker` into a versioned
`RuntimePaintBrush` component embedded in one `.rpref`. No runtime file format,
package kind, registry or source parser was added.

The runtime sample mounts one `.epak` through VFS/AssetRegistry, loads the
prefab, decodes exactly one component, evaluates the existing Brush Engine and
rasterizes a deterministic `512x256` mask. The proof deletes Authoring,
SourceAssets, staging, provenance and fixture arguments before launch and runs
the sample twice.

## Product contracts

- `.rbrush`, ABR and tip source images remain editor/source-only.
- Supported runtime data is one primary R8 tip plus bounded shape, size, angle,
  aspect, scatter, opacity, flow and response curves.
- Unsupported semantics fail cook explicitly; they are never silently lost.
- Raw controls, dimensions, counts, curves, size and normalized hash fail
  closed before evaluation/allocation.
- The final component respects the existing `.rpref/.rlevel` 1 MiB component
  envelope. A `1024x1024` R8 tip is rejected by the focal regression.
- Full canonical `packContentHash` rejects stale snapshots but is excluded from
  output identity, preserving a cache hit after unrelated preset changes.
- Cancellation returns no derivative.

## Fresh Shipping evidence

Debug:

- package entries: `1` Prefab;
- dabs: `63`;
- changed texels: `4150`;
- coverage hash: `1358154781762946768`;
- CPU: `3.059 ms` and `3.181 ms`;
- package SHA-256:
  `2EB61EE1D07D37722741A22C7F8351B8FC31D36C04FD1F9827BB13A64DB6B2AD`;
- capture SHA-256:
  `F192184F510A0471237455F9F32E0716F941B58929F454192FA1E937D2043D68`;
- `source_free=true`.

Release:

- dabs: `63`;
- changed texels: `4150`;
- coverage hash: `1358154781762946768`;
- CPU: `0.390 ms` and `0.373 ms`;
- package and capture hashes are identical to Debug;
- `source_free=true`.

The two captures in each configuration are byte-identical and the package is
unchanged by runtime execution. AssetDump validates Shipping and reports exactly
one Prefab with zero dependencies. Binary scanning rejects the unique source
sentinels, `.rbrush`, `.abr`, source-image extensions, `SourceAssets`,
`EditorAssets` and `Import/Brushes`.

Retained runtime result:

![Runtime dynamic RBrush paint](images/m8-rbrush-runtime-dynamic-paint.bmp)

The retained BMP is an actual runtime output: a legible, non-uniform grayscale
stroke with visible per-dab variation. It is not concept art and does not claim
renderer/PBR, static Surface Paint, Sparse SDF or the final RBrush Shipping
validator.

## Verification

- Authentic RED: absent codec/cooker headers, then explicit malformed-control,
  unsupported-semantics, stale-hash and 1 MiB envelope failures.
- `painting.runtime_brush_derivative`: Debug/Release `PASS`.
- `painting.brush_engine` and `painting.brush_mask_rasterizer`: `PASS`.
- `JericoRuntimePaintShippingProof`: Debug/Release `PASS` with two source-free
  replays per configuration.
- Independent Terra review: initial P1/P2 findings corrected; re-review `PASS`.

## Limits and next legal action

This slice proves one minimal runtime-compatible derivative. It does not make
all RBrush authoring semantics runtime-compatible, introduce a reusable brush
asset extension, or complete the RBrush family. Slice 37 Shipping Validator is
the next legal capacity and must validate the complete packaged-runtime
inventory rather than only this focused fixture.
