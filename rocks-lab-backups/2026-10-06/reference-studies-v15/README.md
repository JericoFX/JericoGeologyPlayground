# Mathematical rock reference studies v15

Independent experimental profiles: `lava_rope_lobes` and `angular_scree`. New analytic C++ evaluators reuse only the closed-heightfield extraction and neutral OBJ render pipeline. Legacy candidates remain unchanged. No images, textures or private reference assets are bundled or sampled at runtime.

Build: `bash build_direct.sh`.
Generate: `build/rocks_cli generate --recipe recipes/lava_rope_lobes-48-v15.json --out out/lava_rope_lobes-48` (substitute `angular_scree` for the second study).
Render: `blender -b -t 8 -P tools/render_reference_studies.py -- --profile lava_rope_lobes --size 900 --samples 40`.

Footprint 2 m; grid 256; source 1025×1025; neutral clay; soft AO 0.30 / 0.12 m. Control recipes set featureDepth to zero. The legacy viewer UI is unchanged; these experiments use the CLI.

Approximate mathematical morphology studies, not reconstructions or geological simulations. Closed-backed 2.5D patches have no undercuts or detached fragments.

Verified: 24 C++ test executables plus intersection fixtures, 50 Python tests, 9 Node tests. Both final meshes: 67,074 vertices, 134,144 triangles, zero degenerate faces and zero detected triangle intersections. Legacy sample digests and positional aggregate initialization remain unchanged. Inactive study controls do not allocate unused geometry. Render cache identity includes all renderer dependencies. Render provenance verifies unmodified OBJ geometry and normals.
