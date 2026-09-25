# M8 RBrush Terrain Vulkan Material Checkpoint

Date: 2026-08-13

Commit: `0ac0e48 feat(render): display cooked terrain layers`

Scope: Task 8 of the active RBrush Terrain plan

State: `PASS` for Task 8; RBrush Slice 24, M8, C04 and the master Goal remain `PARTIAL / ACTIVE`

## Observable criterion

A cooked Terrain tile must render through the real Vulkan path with:

- one `Terrain/PBR_Layered` cooked shader archive;
- a Linear `MaterialWeights` `.rtext` with a diagonal two-layer mask;
- red, rough rock and white, smooth snow layer materials;
- fixed 2-4 layer binding for BaseColor, ORM, Normal and Height;
- non-flat geometry plus blended vertex displacement;
- retained offscreen evidence containing both material regions;
- stable live identity and monotonically increasing generations;
- `ResourceHotSwap` for a compatible topology and `ScopedRebind` for an incompatible topology;
- rejection of an invalid N+1 without changing the live generation;
- no regression of ordinary `Surface/PBR_Opaque` static meshes.

## RED retained

The first executable Vulkan run reached the RTX 3090, loaded the cooked assets,
uploaded the mesh and captured the back buffer. It then failed the two missing
behaviors in `TerrainMaterialRenderTests.cpp`:

```text
rockPixels > 500U: FAIL
snowPixels > 500U: FAIL
```

This was the expected product RED: the old renderer accepted the upload set but
ignored `TerrainMaterialData`, so neither authored layer appeared. Earlier CMake
scheduling-inventory failures were repository contract setup and were not counted
as product RED.

## Implemented GREEN

- Added the bounded `Terrain/PBR_Layered` HLSL/Diligent archive beside the
  existing surface shader. `Surface/PBR_Opaque` was not modified.
- Reused `StaticMeshRenderAsset`, the canonical cooked `.rmat` / `.rtext`
  structures, the existing Diligent pipeline loader and the DiligentFX GGX PBR
  shading include.
- Added one weight texture and four fixed resource slots. Only the declared 2-4
  layers contribute; weights are normalized before shading and displacement.
- Validated weight semantic/color space, layer material identities, non-recursive
  Terrain layers, ordinary PBR admission and the complete shader-resource contract.
- Made complete cooked generation replacement the Terrain live-update path.
  N+1 is created and validated before the live implementation swaps. An invalid
  candidate leaves generation N untouched.
- Classified identical index/submesh topology as `ResourceHotSwap`; changes to
  vertex count, indices, submesh ranges or submesh material identities are
  `ScopedRebind`.
- Kept replacement owner/render-thread-only and non-blocking. Destruction of N
  uses Diligent Vulkan's existing deferred native-resource release queue; no
  `IdleGPU`, second renderer or second residency manager was introduced.

## Runtime evidence

The focal test prints:

```text
render.terrain_material.pass|width=384|height=256|rock_pixels=20404|snow_pixels=20724|generation=3|indices=387
```

Retained Vulkan capture:

![Terrain PBR layered Vulkan capture](images/m8-terrain-layered-vulkan.bmp)

Capture SHA-256:

```text
070074B9D7766CC8C885830FAC457DE51877ED608C93DE3A8BED2F6856470704
```

The image was inspected at original 384x256 resolution. It shows a complete tile,
a red rock region, a white snow region, a diagonal blend and a displaced curved
silhouette. An earlier technically green capture crossed the fixture near plane
and was rejected rather than retained because it contained misleading triangular
holes.

## Verification

Debug focal and regressions:

```text
render.terrain_material: PASS (3.72 s)
render.static_mesh_asset: PASS (9.49 s)
render.shader_cook_proof: PASS (0.68 s)
repository.test_scheduling_contract: PASS (0.03 s)
```

Release focal and regressions:

```text
render.terrain_material: PASS (0.82 s)
render.static_mesh_asset: PASS (1.83 s)
render.shader_cook_proof: PASS (0.62 s)
repository.test_scheduling_contract: PASS (0.02 s)
```

Integrated build:

```text
cmake --build --preset windows-vulkan-editor --config Debug -j 4
PASS (exit 0, 71.6 s)
```

`git diff --check` passed before publication. The existing user-owned tracked
changes and all untracked attachments/showcase images were preserved.

## Boundaries and remaining work

This checkpoint does not claim a complete Terrain workspace, RBrush slice or M8.
Task 9 remains the next legal task: connect the existing Terrain transaction and
job paths to the real brush-first Terrain workspace, show Height and Material
Weight strokes in the shared World viewport, and retain Windows/Vulkan editor
evidence. Incremental package and source-free Shipping replay remain Task 10.
