# M8 Terrain cooked outputs — incremental package and Shipping replay

Date: 2026-08-13

State: **RBrush Slice 24 PASS; M8 and JericoEngine 1.0 remain ACTIVE**

RBrush is editor-only in this proof: it authors the Terrain change and is absent
from every cooked package and runtime binary. The verified
incremental/package/Shipping behavior belongs to the resulting Terrain-owned
runtime assets, not to RBrush. `RenderDevice` now exposes a bounded four-query
duration pool, CPU frame measurement and a single-call non-blocking poll. The
Shipping replay retains real CPU/GPU frame times from Diligent's Vulkan duration
query; it never calls `WaitForIdle` to obtain telemetry.

## Product behavior

- A complete cook and an incremental cook share the same persisted adaptive
  refinement topology. `refinementRegions` describe authored topology;
  `dirtyRegions` describe only the current invalidation; `fullCook` is explicit.
- One bounded Height + Material Weight edit invalidates exactly one Terrain tile
  and seven adaptive pages. Every emitted edge explains:
  `Dirty Terrain page intersects authored change`.
- Page cook keys depend on the page samples, its bounded normal halo and the
  material-weight samples it actually consumes. A recipe revision, whole-world
  content hash, whole-Terrain content hash, another area of the same patch or
  an unrelated neighbor patch no longer changes unrelated page keys.
- RuntimeFormats admits bounded, hashed `.rterrain` v2 through a Qt-free reader.
  PackageTool recognizes Terrain plus its mesh, weight, material, collision and
  navigation closure.
- `.epak` now distinguishes a complete package from a delta with canonical base
  build GUIDs. Package Changed emits only the changed cooked set. AssetRegistry
  admits a delta only after every declared base, with an explicit reason and a
  strictly higher priority for same-kind GUID replacement.
- Compact Packages resolves the visible overlay inventory. Every compacted
  runtime asset hash equals the clean full-cook inventory.
- Shipping deletes the three generated cooked directories before replay. Two
  independent VFS/AssetRegistry instances mount only `base.epak + delta.epak`,
  load `.rterrain`, render its `.rmesh/.rtext/.rmat` with Vulkan, raycast its
  `.rcoll` through Jolt, and query/path its `.rnav` through Detour.
- Packages reject authoring payload markers including `SourceAssets`,
  `.terrain.toml`, `.rbrush`, `.abr`, Terrain evaluator/cooker names and Qt.
  The runtime-only binary inventory contains none of those authoring symbols.
- There is deliberately no `.rbrush` cook, runtime derivative, package entry or
  Shipping loader. Only the static result selected by the Terrain adapter is
  cooked.
- Frame telemetry measures the CPU submission span between explicit begin/end
  calls. GPU duration is returned later from `QUERY_TYPE_DURATION`; if four
  queries are still pending, the bounded path records CPU-only data instead of
  allocating without limit or waiting for the GPU. Present/VSync wait remains a
  separate future renderer-foundation metric.

## Metrics

Fresh Release proof output:

```text
terrain.shipping.incremental_succeeded|changed_tiles=1|changed_pages=7|changed_cook_ms=2.3709|base_bytes=665189|delta_bytes=420821|compact_entries=43|source_free_replays=2|terrain_hash=14906042553592363501|capture_hash=18121064807415065818|draw_calls=1|triangles=8|cpu_frame_ms=0.5542|gpu_frame_ms=0.01536|gpu_query=duration_nonblocking
```

The fixture intentionally renders the deepest changed page. The broader editor
proof retains the 28-page / 3,584-triangle / 28-draw scene. The small Shipping
page isolates package overlay identity, two-material weights, collision and nav
without hiding a whole-world cook.

## Visual evidence

Retained Vulkan Shipping capture:

- [`images/m8-terrain-shipping-overlay-vulkan.bmp`](images/m8-terrain-shipping-overlay-vulkan.bmp)
- SHA-256:
  `A5C386FD9372662253780281FF6C73EE943E14A307A491E761F1FE9178B6D461`
- 512 x 512, inspected at original resolution.
- The central page is visibly distinct from the blue clear color and carries
  the cooked Rock-to-Soil weight gradient. The two replay captures have the
  same pixel hash.

This supplements the full JericoEditor before/Height/Material captures in
`M8_RBRUSH_TERRAIN_EDITOR_PREVIEW.md`; it is not concept art.

## Verification

Fresh commands and results:

```powershell
cmake --build --preset windows-vulkan-editor --target JericoRenderDeviceTests JericoTerrainShippingProof
ctest --test-dir build\windows-vulkan-editor -C Debug -R "^(terrain\.|editor\.terrain_|render\.(terrain_|vulkan_device)|repository\.test_scheduling_contract|architecture\.)" --output-on-failure
# 32/32 PASS in 16.16 s

ctest --test-dir build\windows-vulkan-editor -C Debug -R "^(runtime_formats\.epak|package_tool\.(integration|overlay)|assets\.registry)$" --output-on-failure
# 4/4 PASS in 0.90 s

cmake --build build/windows-vulkan-editor --config Release --target JericoRenderDeviceTests JericoTerrainShippingProof
ctest --test-dir build\windows-vulkan-editor -C Release -R "^(terrain\.|editor\.terrain_|render\.(terrain_|vulkan_device))" --output-on-failure
# 11/11 PASS in 6.33 s

cmake --build --preset windows-vulkan-runtime
# PASS in 70.8 s

ctest --test-dir build\windows-vulkan-editor -C Debug -R "^repository\.(test_scheduling_contract|texture_codec_shipping_boundary|runtime_only_target_discovery|shipping_source_exclusion)$" --output-on-failure
# 4/4 PASS in 75.02 s
```

Additional checks:

- `render.vulkan_device` + `terrain.shipping_incremental`: Debug 2/2 PASS in
  4.65 s; Release 2/2 PASS in 2.19 s.
- `runtime_only_forbidden_symbols=none` across runtime `.exe/.dll` for Terrain
  authoring/cooker/evaluator, source Terrain/RBrush extensions and Qt markers.
- `git diff --check`: no whitespace errors; only existing LF/CRLF warnings.

## Failures retained

1. The first overlay RED did not compile because no delta request, package
   compaction, base build metadata or priority mount policy existed.
2. Initial compact-vs-clean comparison failed because page cook keys contained
   global recipe/world/Terrain hashes and whole-neighbor patch hashes. Page-local
   inputs fixed the false invalidation.
3. The first Vulkan capture was only clear color: the proof transform inverted
   Terrain winding. The next capture exposed a black page because the light
   direction opposed the transformed normal. Both defects were fixed and a
   visible-pixel assertion now prevents that false PASS.
4. The first Release matrix reported five `Not Run` tests because the plan built
   only the Shipping proof. Building the five referenced Release targets made
   the same matrix 10/10 PASS.
5. Final review added a RED proving that a sample change in one page left no
   stable key elsewhere in the same patch. The existing whole-patch source hash
   failed that test. The GREEN hashes only the page grid plus its bounded normal
   halo and the weight samples consumed by that page.
6. The telemetry RED failed compilation because `RenderDeviceInfo`,
   `FrameTelemetrySample` and begin/end/poll APIs did not exist. The first GREEN
   then exposed a real lifecycle regression: retained duration-query references
   kept the Vulkan logical device alive after `shutdown()`, so the existing
   second-device test failed. Releasing active/pending/available queries before
   the device fixed the regression; Debug and Release now pass.

## Slice result and next legal work

All eleven acceptance items in the accepted Terrain design now have durable
product, editor, runtime, Shipping, visual, timing and regression evidence.
RBrush Slice 24 is `PASS`. This does not make the broader C18 Terrain family,
M8, C04 global, Virtualized World Runtime, JSVGI or the JericoEngine master Goal
complete: their remaining gates stay active.

The next legal slice is RBrush Slice 25 EPLS. Its first observable contract is:
`Final EPLS = Procedural + Manual Include - Manual Exclude`, with manual
exclusion winning deterministically. Design and RED must reuse the canonical
RBrush, Region Mask, transaction, C04 and bounded dirty-region authorities;
they must not open SDF stamps, decal scatter, Vegetation or M9.
