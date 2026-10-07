# Jerico Shelf Props C++

Three original procedural shelf accessories in metres, right-handed Z up and front -Y: an open old tin mug, an unlit oil-style lantern, and a dirty amber glass bottle. These complete the accessory group of the bookshelf study. Reference images and third-party addon implementations are not distributed.

C++17 owns geometry, native texture generation, normals, UVs and GLB/OBJ export. Blender is only a read-only importer/render viewer. The models have closed two-manifold wall surfaces, including the open mouth of the mug and bottle. Separate touching metal parts are intentional assembly interfaces, not a single fused Boolean mesh.

## Build

Requires a C++17 compiler, CMake 3.16+ and zlib development package. JSON is vendored with its license.

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build -j4
    ctest --test-dir build --output-on-failure
    build/jerico_shelf_props --out output --bake 384

A compiler-only build script is also provided for minimal Linux environments. No Python or Blender is required to build or generate the assets. Python QA scripts are read-only checks.

## Boundaries

There is no light source, fire, liquid, physics, gameplay, interaction binding or engine integration. The lantern is unlit. Stable item IDs, OBBs, support planes and grasp groups are metadata for a consuming engine. Material appearance, transparency and physical interaction still need integration testing in the target engine. OBJ cannot preserve the full glTF transmission extension or metadata; GLB is the preferred viewing format.

## Recipe and placement API

The JSON recipe accepts version 1, signed integer seed, wear from 0 to 1, 32–128 radial segments, and independent mug_scale / lantern_scale / bottle_scale from 0.70 to 1.30. Scaling preserves the wall/contact proportions. Four built-in shared procedural materials are old tin, oxidized iron, amber glass and dirty clear glass; seed and wear parameterize their appearance. Unknown fields and out-of-range values are rejected.

- build_shelf_props(recipe): raw independently closed meshes plus item/pivot/OBB/grasp metadata.
- evaluate_shelf_props(scene): native smooth angle-weighted normals and UV data.
- bake_shelf_props(scene, directory, resolution): four shared C++ PNG16 material sets.
- plan_shelf_prop_placement(bookshelf_scene, props_scene): derives full-height free X intervals from actual compartment bounds and item OBBs. It uses per-ID deterministic choice and rejects insufficient space.

To create a new combined preview using an existing evaluated/horneado bookshelf JSON:

    build/jerico_shelf_props --out output --bake 384 --shelf-scene bookshelf-scene.json

Existing mesh data, books, item IDs and texture maps are preserved. Only supported accessories are appended. Required free space is 6 mm on both sides, 8 mm front/back and 10 mm above. A no_fit result remains explicit; nothing is moved or hidden to make room. The placement output is recipe-dependent and cannot be reused blindly after changing the shelf dimensions or contents. Exported GLBs embed their images and work without the original working directory.
