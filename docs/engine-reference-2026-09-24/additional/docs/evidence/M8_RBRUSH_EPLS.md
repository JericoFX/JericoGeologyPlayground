# M8 RBrush Slice 25 — EPLS incremental cook and Shipping

**Date:** 2026-08-14
**State:** RBrush Slice 25 `PASS`; C26 EPLS, C04 global, Virtualized World,
JSVGI, M8 and the master Goal remain `PARTIAL / ACTIVE`.

## Observable result

A real RBrush-backed EPLS Wetness page is composed from Procedural, Include and
Exclude U16 masks. `EplsCooker` converts the immutable final page to the
canonical page-local `.rtext` representation: `ScalarMask`, Linear, BC4,
Compressonator Core 4.5.5.2. The stable runtime texture GUID derives from the
recipe, layer and page identities.

`Cook Changed` intersects only dirty regions that request `SurfacePages`. The
fixture changes the first of two pages and emits exactly one candidate with the
answer:

```text
Dirty EPLS mask page intersects authored change
```

No hidden full recook precedes the delta. The cook key covers recipe/world/
layer/surface/page identity, page bounds and dimensions, procedural/include/
exclude hashes, the final local sample hash, semantic, toolchain/target/runtime
pins, quality and the exact canonically sorted dependent material GUIDs.

## Incremental package and Shipping proof

The retained Debug run produced:

```text
base_bytes=100501
delta_bytes=3813
delta_entries=1
compact_clean_equal=true
source_free_replays=2
texture_hash=7489705050044168139
capture_hash=7492913355706331529
```

The delta contains only the changed EPLS texture. A base+overlay package mount
resolves the new stable GUID generation; compacted base+delta inventory equals
a clean full-cook package inventory. Both independent Vulkan Shipping replays
load the cooked mesh, PBR material and BC4 mask through VFS/AssetRegistry and
produce identical non-zero texture/capture hashes, one draw and two triangles.

The material binds the scalar mask through the admitted `Height`/parallax
semantic. The retained image is therefore evidence of a real cooked Vulkan
Shipping load/render, not a claim of final EPLS visual art direction or the
future MeshBlend backend:

![EPLS Shipping replay](images/m8-rbrush-epls-shipping.bmp)

## Source-free boundary

Before replay, the fixture deletes `SourceAssets`, `Procedural`, editable
`.epls` and editor brush directories. Base, delta, compact and clean packages
are scanned and reject `.epls`, `.rbrush`, `.abr`, brush strokes,
`EplsPaintWorkbench`, `EplsCooker`, SourceAssets, EditorAssets and Qt markers.
Shipping consumes only cooked `.rmesh`, `.rmat`, `.rtext` and `.epak` bytes.

## Authentic RED and causal GREEN

The product RED failed in `EplsFixtureBuilder.cpp:3` because
`Jerico/Epls/EplsCooker.hpp` did not exist. Two scheduling-contract REDs first
recorded the intentional inventory changes `283→285` tests and `65→67`
governed tests. The first compile then exposed an invalid generic hash helper;
it was reduced to the established integral/enum byte hashing pattern.

The first Shipping integration correctly rejected a double-sided material and
then rejected binding a Linear scalar mask as sRGB BaseColor. The fixture was
fixed at the caller boundary: the admitted opaque single-sided material binds
the BC4 scalar mask as Height/parallax. Renderer validation was not weakened.

## Verification

- Debug focal cooker: `1/1 PASS`, final run `0.09 s`.
- Debug cooker + Shipping: `2/2 PASS`, retained run `2.71 s`.
- Debug EPLS/editor/procedural/scheduling/architecture matrix: `31/31 PASS`,
  `11.37 s`.
- Release proof target: `PASS`; base/delta/hashes matched Debug.
- Release EPLS/editor matrix: `9/9 PASS`, `4.81 s`.
- Runtime-only Vulkan build: `PASS`, including runtime-only source audits.
- Runtime-only target discovery + Shipping source exclusion: `2/2 PASS`,
  `31.06 s`.
- Scheduling contract: `285` tests, `67` governed, `21` serial.
- `git diff --check`: clean for the owned Task 6 paths.

## Scope and next legal work

This proves the RBrush EPLS Include/Exclude acceptance sentence and closes
RBrush Slice 25 only. It does not complete C26 EPLS: material/decal/instance/
mesh-patch/static-particle/tag outputs, PWAS delegation, MeshBlend, sky,
atmosphere, weather, volumes, VT residency and the full EPLS DoD remain open.

The next legal RBrush capacity is Slice 26 Decal/instance scatter: one RBrush
must distribute a deterministic decal/instance set using the existing World
shell and canonical systems. It does not authorize PWAS 2.0, later SDF stamps,
JericoParticles, Vegetation or M9.
