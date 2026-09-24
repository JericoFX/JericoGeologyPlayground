# F4 region-aware LOD metric, 2026-09-05

Status: metric regression PASS; actual weathered member LOD and Beauty RED.
Baseline: `15ff846d7aeaec431df365f50c4fdf59e2bacc73`.

The old nearest-vertex normal comparison ignored triangle-region ownership.
Coincident vertices belonging to different regions could therefore make a mesh
fail comparison with itself. The private topology metric now derives each
vertex's unique incident region, compares only within that region, normalizes
the dot product and uses the worst angle for equally near same-region frames.
Missing or ambiguous ownership rejects. The 35-degree gate is unchanged.
This changes acceptance measurement, not generated geometry or authored data.

Logs below are in `build/windows-vulkan-editor/`:

| Evidence | Result |
| --- | --- |
| `f4-lod-region-red.log` | CTest exit8, 2.45s; self-comparison and coincident-vertex permutation fail with old metric |
| `f4-lod-region-green-build.log` | Both topology and GPU session test targets build, exit0 |
| `f4-lod-region-green.log` | Topology CTest 1/1 PASS, 2.52s; seam, ordering, missing/ambiguous region and damaged-normal cases |
| `f4-lod-region-witness-red.log` | Actual downstream exit1, 166.977s; old worst pair already belongs to the same region, measured76.882729 degrees |
| `f4-lod-region-corrected01.log` | Actual downstream exit1, 173.223s; structural coverage/parity/cook pass, LOD still fails |
| `f4-lod-region-ordinary.log` | Ordinary `--classified-lod1` exit0, 101.727s |

Actual corrected worst pair: measured112.727115 degrees, allowed35; LOD vertex691,
dense vertex2526, distance0.021902m, both region
`E2178D96A95A2C403CBCFF8B9B720ACC`. Positions are
(0.509081,0.452118,0.405858) and (0.497325,0.464012,0.420000); normals are
(0.716976,0.199164,0.668042) and (0.257239,0.257912,-0.931295).
The metric repair does NOT explain away the real failure. Determine whether
region correspondence, local topology or normal construction causes this pair
before changing generation. Do not raise the feature-angle threshold.

Ordinary Vulkan/RTX3090 regression: LOD0 4064 vertices/8120 triangles; LOD1
1096/2184; maximum source deviation0.0858255m; LOD1 extraction199us;
hash14330100588193062977. Whole process duration includes session initialization;
it is not GPU generation time.

End-to-end audit: source/recipe, extraction, classification, exact collision and
cook are unchanged. Acceptance/debugging consumers now receive a region-aware
metric and offending pair. Actual-member LOD remains INTEGRATION REQUIRED for
runtime LOD/streaming and the M8 performance gates. No new runtime format, editor
UI, dependency, physics, scripting or gameplay contract is introduced. This
technical increment supplies no new Beauty acceptance.

## Startup continuation

The owner's question about repeated minute-long startup is substantiated by
`f4-structural-coverage-native01`: session readiness90.4418s and whole native
verification99.8646845s. The first Generate lazily creates a session; its displayed
duration includes startup, evaluation and cook. Embedded HLSL is compiled by DXC
again for a new session; the current session retains its shaders and pipelines.
Opening the tool itself is not equivalent to running Generate.

The existing pipeline overlap optimization is documented in
`M8_F4_GEOLOGY_STARTUP.md`; it is not a persistent shader cache. A Vulkan PSO cache
alone cannot remove DXC compilation. Next bounded work: reuse admitted Diligent
compiled shader archives, with content/compiler compatibility and cold/warm
fresh-process tests, then resume the real LOD witness. No startup improvement
is claimed by this metric commit.
