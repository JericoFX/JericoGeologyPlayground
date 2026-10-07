# Native execution evidence

The compiled C++ CLI was run with an empty environment and empty PATH, taking only a JSON recipe and producing a new evaluated scene, new material textures and a new GLB in its output directory. It succeeded. Linked libraries were libz, libstdc++, libm, libgcc, libc and the platform dynamic loader; no Python or Blender runtime was linked.

Command form:

    /usr/bin/env -i PATH= /absolute/path/build/jerico_procedural --family fracture --recipe recipe.json --out scene.json --evaluate 1 --bake 32 --glb native.glb

Output of the independent GLB validator for this smoke test:

-52 meshes,52 materials,312 embedded PNG16 images
-29,640 vertices,9,880 triangles
-3,355,860 bytes with lossless native PNG compression
-SHA256 d180f27fea48cf80323f569bf1888ab3fcefd36c20d415a26d2cd78d3b3f743b
-Source geometry, binary buffers, UVs, normals, tangents and texture packing passed

This32-resolution run is a functional independence smoke test, not the final visual-resolution asset. Higher-resolution material comparisons are separate.

A syscall trace could not be collected because the environment disallows ptrace. No tracing bypass was attempted. The evidence is the isolated PATH run, linked-library inspection, source audit, and independent binary validation; it is not represented as a full syscall audit.
