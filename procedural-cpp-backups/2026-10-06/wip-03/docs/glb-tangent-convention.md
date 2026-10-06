# Native GLB tangent-space convention

## Export contract

The native exporter writes positions and normals in the proper rotation
`(x,y,z) -> (x,z,-y)`. Texture coordinates use `TEXCOORD_0=(u,1-v)` for glTF's
top-left image origin. PNG files have top-down rows.

The normal texture is the original C++-generated `textures.normal` PNG16,
with OpenGL +Y tangent-space components. It is embedded without changing green.
`TANGENT.xyz` is the normalized U derivative orthogonalized against the loop
normal. `TANGENT.w` is **minus** the raw derivative-frame handedness calculated
from the serialized float32 glTF UVs. This is a single coordinate-convention
conversion for every part, including mirrored UV islands, not a per-recipe fix.

The old `textures.normal_gltf` green-inverted diagnostic image may remain in a
bake directory. The production GLB exporter no longer reads or embeds it.

## Why the pair matters

For an explicit glTF tangent frame, `B = cross(N,T.xyz) * T.w`. Given a decoded
normal sample `(nx,ny,nz)`, the physical normal is proportional to
`T*nx + B*ny + N*nz`.

Changing both `T.w` and `ny` to their negatives preserves this vector. Our former
pair (raw derivative W plus green-inverted texture) was physically consistent
for a renderer honoring explicit tangents. However, the Blender 4.3.2 importer
reverses V, does not import the supplied TANGENT attribute, and connects the
normal image directly to a Blender Normal Map node. Regenerated Blender
bitangents then need the original green channel. The selected pair is also the
convention documented by the MikkTSpace glTF binding and used by Blender's
exporter.

The glTF specification does not require an arbitrary explicit tangent frame to
have one particular sign relative to raw texture-coordinate derivatives. The
native validator's sign test checks **this documented export contract**, not a
universal extra requirement on all glTF files.

## Primary sources

- [glTF 2.0 specification, primitive data and tangent reconstruction](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#meshes-overview)
- [glTF 2.0 specification, textures and upper-left origin](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#textures)
- [MikkTSpace binding: converting tangent signs for glTF](https://github.com/donmccurdy/mikktspace-wasm/blob/main/README.md)
- [Khronos-maintained Blender exporter, primitive attributes](https://github.com/KhronosGroup/glTF-Blender-IO/blob/main/addons/io_scene_gltf2/blender/exp/primitive_extract.py)
- [Khronos sample-model discussion of the coordinate-convention distinction](https://github.com/KhronosGroup/glTF-Sample-Models/issues/174)

Installed Blender 4.3.2 was inspected directly: `imp/mesh.py:629-630` reverses V;
`imp/material_utils.py:170-195` binds the PNG directly to Normal Map;
`exp/primitive_extract.py:1505-1520` retains Blender's bitangent sign when
converting Z-up to Y-up. Import code does not consume TANGENT.

## Recorded independent evidence

On 2026-10-06, the wood-reinforced native scene was sampled at 53,325 triangle
corners, using each part's actual packed/rotated atlas UVs, source positions,
loop normals, PNG16 samples, and decoded GLB tangent data. MetricUV is **not**
a substitute for the packed atlas because the packer rotates islands.

The pre-conversion GLB used for this test had SHA-256
`f78474d248fb1f4c6712ea24915036faf6360e08b95dd298f63147369306a21e`.

| Comparison | Mean angle | RMS angle | Maximum angle |
| --- | ---: | ---: | ---: |
| Former explicit GLB pair vs reconstructed native normal | 0.00048358° | 0.00095719° | 0.02977716° |
| Equivalent pair change: opposite W plus original PNG green | 0.00044606° | 0.00088292° | 0.00174856° |
| Incorrectly retaining inverted green after Blender regenerates its basis | 3.16752° | 4.98482° | 56.24053° |

The first residual includes float32 position/UV quantization. The second includes
separate 16-bit quantization of complementary green channels. Angular differences
are invariant under the same rigid node rotation, so this tests the physical
normal independently of object orientation.

A separate diagnostic GLB changed only the pair, then underwent unmodified,
read-only Blender import and rendering with the native scene's camera, bounds,
resolution, seed, and 64 samples. For the wood scene, native-vs-imported image
MAE improved from **0.0015004146** to **0.0001370134**; RMSE improved from
**0.0037539824** to **0.0009441941**. No material-node repairs or preview modifiers
were applied. These measurements establish this Blender interoperability case;
they are not a claim of integration with an untested engine.

## Regression coverage and reproduction

`tests/export_tests.cpp` checks the convention directly on normal and mirrored
UVs, verifies original normal-PNG embedding, and rejects UV islands that collapse
in float32. Two reduced real bevel failures still require fail-fast rejection of
incompatible source loop normals.

`tools/validate_native_glb.py` independently parses GLB buffers and PNGs. It keeps
all strict checks: finite float32 geometry, nondegenerate triangles and UVs,
positive `dot(Ns,Ng)`, unit/orthogonal tangents, homogeneous +/-1 W within each
triangle, tangent U-derivative alignment, packed atlas and MetricUV preservation,
exact native image bytes, hierarchy, pivots, sockets and grips. Its self-test also
rejects a W-only inversion, alongside the other deliberately corrupted files.

Run after the CMake build:

```sh
ctest --test-dir build --output-on-failure
python3 tools/validate_native_glb.py build/export-test/native.glb \
  --scene build/export-test/scene.json --self-test
```

To inspect an already generated production asset, use the same validator with
its adjacent native `scene.json`. Rendering is an offline consumer check; neither
Blender nor Python participates in native geometry, baking, or GLB export.
