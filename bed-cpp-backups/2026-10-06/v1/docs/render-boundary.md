# Preview boundary
The repository generates asset geometry and materials in C++. Blender may read generated arrays/textures and render with frozen cameras/lights. It is not Jerico Engine, Playground, a Diligent renderer or runtime integration.

Historical Diligent files located in the workspace are Windows build logs from older studies. No working cloud Diligent harness was found at the first audit. Native CLI output is reusable asset data, not an engine renderer.

Reference renders may execute the frozen Python/Blender authoring code solely as an offline oracle. Production-side preview must never run the reference generator, add a bevel/normal modifier, calculate an asset procedural shader, repair UVs or insert missing geometry. Those operations belong in C++.

The family preview also supports `--mode glb --glb asset.glb`. It invokes Blender's ordinary glTF importer, asserts that the imported objects have no modifiers, and renders without any geometry, UV, tangent, material or normal repair. Camera, stage, lights, color management and sampling are shared with the original and direct-native preview paths. The GLB itself embeds the textures and buffers created by C++.
