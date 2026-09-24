# M8 Classified Geology LOD1 Source-Free Vertical Closure

Date: 2026-08-29

Slice: `M8-GEOLOGY-CLASSIFIED-LOD1-SOURCE-FREE-VERTICAL`

Status: `PASS`

Input boundary: `2a2ac1ead8ae824d0f9534f4d1db8281cad3ab25`

## Observable result

One checked `GeologyGpuTopologyProfile` now evaluates exactly one coarser
classified topology from the same resident final scalar and packed
classification fields as dense `LOD0`. `GeologyCook` writes both levels into
one compatible JRMS `GEOL + VISI` product while collision remains dense-only.
The existing conventional CPU and Diligent/Vulkan visibility paths select
exactly one level per `lodSetId` near, far and across the two-pixel hysteresis
transition.

No recipe/operator, second field evaluator, remesher dependency, alternate
format, material/Beauty system, collision LOD or streaming policy was added.

## Retained RED evidence

1. The focal evaluator contract expected one topology level and initially
   observed `topologyLevels.empty()` because no implementation consumed
   `topologyProfiles`.
2. The JRMS focal test initially rejected the valid level-one visibility record
   (`first.hasValue() == false`).
3. The first Vulkan near/far proof submitted both records in the set:
   `draws=2`, `triangles=10304` at both distances.
4. After GPU selection was repaired, the added renderer-fallback RED still
   reported `near_draws=2`, `far_draws=2`, and `10304` triangles. This exposed
   that the conventional renderer had not integrated the existing CPU oracle.

Each RED was run before its corresponding production change.

## Producer and cook metrics

```text
backend=Diligent/Vulkan
device=NVIDIA GeForce RTX 3090
lod0_vertices=4064
lod0_triangles=8120
lod1_vertices=1096
lod1_triangles=2184
maximum_source_deviation_m=0.0858255
lod1_extraction_us=129
lod1_hash=2620618399222834365
mesh_bytes=413470
collision_bytes=146466
```

LOD1 is non-empty and cheaper than LOD0, remains inside its deviation,
feature-angle and triangle budgets, and has one classification, normal and
tangent for every vertex. Invalid dense/non-coarser cell size, zero or non-finite
deviation, out-of-range feature angle, zero triangle budget and multiple
profiles are rejected. Repeat evaluation and repeat cook produce identical
hashes and bytes.

The cooked artifact identities are:

```text
classified JRMS SHA-256 = ccf850e1277e9e7a6fcda2716b999a30d7b9cdb67701dde78d684de9ddfaa62f
dense-only JRCL SHA-256 = 0f8a7dd0957878876e37bf1c37e40794218c211efeb3cf4aef6db5ea60ac4dd0
visibility RMAT SHA-256 = 70a7999d36fc2139b4c2077724484ddf4def0a8a6f0dc4c9387c6acf8642e186
```

The LOD cook's collision bytes equal the legacy dense-only cook exactly. No LOD1
triangle enters collision.

## JRMS compatibility

`VISI` remains chunk version 1. A non-zero `lodSetId` now admits contiguous,
unique levels starting at zero with strictly decreasing projected-diameter
thresholds. Gapped, duplicate and non-monotonic sets fail. The classified
product has two submeshes and exactly two records in one set (`LOD0/LOD1`) plus
one shared `GEOL` descriptor/classification stream. Ordinary, single-level and
golden JRMS bytes remain unchanged and readable.

## Direct CPU/Vulkan proof

The real Vulkan renderer loaded the generated JRMS with two records, four
classified materials and zero fallback textures. Final diagnostics were:

```text
CPU near: draws=1 triangles=8120 projected_rejected=1
CPU far:  draws=1 triangles=2184 projected_rejected=1
CPU transition: held_triangles=2184 released_triangles=8120
Vulkan near: draws=1 triangles=8120
Vulkan far:  draws=1 triangles=2184
Vulkan transition_lod1_scale=0.042251
Vulkan transition_lod0_scale=0.0424902
fallbacks=0
readback=true
```

The conventional path uses the same checked CPU plan only when valid `VISI`
exists. The pre-existing no-`VISI` direct fallback still draws its ordinary
submeshes. Vulkan retains `CullA -> DepthA -> BuildHiZ -> CullB` and rejects the
unselected level as projected-size LOD work.

## Single-package source-free replay

One complete package contains the classified JRMS, visibility archive and the
existing geology palette/material textures:

```text
package=geology-lod1.epak
entries=9
bytes=7917052
SHA-256=9e7eaba37420c5c71bada10dda8cebc877a40d66eb66100aa3a38786f76d9950
shipping_valid=true
no_abr=true
no_brush_source_images=true
no_editor_rbrush=true
no_editorassets=true
source_free=true
```

The replay directory contained only that complete package. Package-only GPU and
conventional results agreed exactly:

| Path/view | Draws | Triangles | Projected rejects | Capture SHA-256 |
|---|---:|---:|---:|---|
| GPU near | 1 | 8120 | 1 | `0ef4be7840ce19f09705e66cf171ed73c4dcac4c2d3fa7a047e622e7ff39d538` |
| GPU far | 1 | 2184 | 1 | `5150d1d79937b48193a1b0e4bf2394019405870d2320a26860e1f6e03d633539` |
| CPU near | 1 | 8120 | 1 | `0ef4be7840ce19f09705e66cf171ed73c4dcac4c2d3fa7a047e622e7ff39d538` |
| CPU far | 1 | 2184 | 1 | `5150d1d79937b48193a1b0e4bf2394019405870d2320a26860e1f6e03d633539` |

The two GPU captures repeated byte-for-byte, and CPU/GPU captures at the same
view are byte-identical. Both inspected 960x720 images are non-empty. This slice
does not claim a new Beauty acceptance; the existing classified PBR response is
preserved and Beauty changes were explicitly excluded.

## Verification

The closure runs the following configured gates:

- focused `JericoGeologyGpuSessionTests --classified-lod1` with repeat cook;
- full `JericoSdfCookTests` geology/collision suite;
- full `JericoMeshAssetTests`, including legacy golden and malformed LOD sets;
- full `JericoStaticMeshVisibilityPlanTests` near/far/transition hysteresis;
- focused real-Vulkan `JericoStaticMeshRenderAssetTests
  --geology-lod1-visibility` for GPU and conventional paths;
- `JericoAssetDump --shipping-validate` on the one complete package;
- package-only Vulkan and conventional near/far replay with validation enabled;
- repeated capture hashes and `git diff --check`.

All exit `0`; no Diligent error/fatal diagnostic is accepted.

## End-to-end completeness and exclusions

The repeated completeness/data-lineage audit is appended to
`M8_GEOLOGY_SYSTEM_COMPLETENESS_AUDIT.md`. The implemented lineage ends at one
selected classified StaticPbr draw. Dedicated LOD UI, `LOD2+`, HLOD, chunks,
world cells, geometry pages, streaming/residency, Jolt class properties,
collision LOD, runtime modification/destruction/save, navigation, PWAS/EPLS,
gameplay/Luau/events, VFX and audio remain explicitly `INTEGRATION REQUIRED`
under later owners.

The 182-entry foreign inventory remains outside this publication with exact
LF-joined, no-final-newline SHA-256:

```text
2e996b1669df5ad1dd4e0cb9c3cbaf1b8bfc73ba6bc734abbad0a49419954030
```

This bounded vertical is `PASS`. JericoEngine 1.0 is not complete.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
