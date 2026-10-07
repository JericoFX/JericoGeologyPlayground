# Procedural aged iron bed, native C++17

Original procedural study inspired by an aged institutional iron bed. The reference is aesthetic guidance, not measured dimensions. No reference image, imported mesh, external texture, add-on or engine modification is included.

## Build and reproduce

Requires C++17, CMake 3.16+, and zlib development headers. The JSON header and applicable third-party licenses are included.

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build -j2
    ctest --test-dir build --output-on-failure
    build/jerico_bed_cli recipes/bed/complete.json output/complete 512
    build/jerico_bed_cli recipes/bed/bedding.json output/bedding 512
    build/jerico_bed_cli recipes/bed/iron_only.json output/iron_only 512

All mesh construction, bevel evaluation, normal construction, textures, PNG16 encoding and GLB/OBJ exports run in C++. Python tools are optional QA oracles, packaging or read-only Blender viewers. They are never a generator fallback. The bed family adds `src/bed.cpp`, `src/bed_materials.cpp`, `src/bed_main.cpp` and `include/jerico/bed.hpp` to the retained core. Existing GLB/OBJ sharing changes are reused from the tested dock increment.

## Parameters and presets

- `preset`: `iron_only`, `bedding` (mattress and pillow), or `complete` (also blanket)
- `width`: 0.70–1.60 m, default 1.02; tube centreline spacing, not outermost feet
- `length`: 1.70–2.40 m, default 2.04; head-to-foot tube centreline spacing
- `head_height`: 0.85–1.45 m, default 1.12; upper tube centreline
- `foot_height`: 0.69–1.08 m, default 0.81; no taller than head
- `tube_radius`: 0.014–0.032 m, default 0.021
- `bar_count`: 3–12 per end; combined width/radius/count combinations with inadequate clearance are rejected
- `wear`: 0–1, default 0.8; controlled local tube dent depth
- `seed`: signed 64-bit integer, default 71

All presets use the same dimensional recipe and exact structural IDs/geometry. The coordinate system is metres, right-handed Z-up, with foot at -Y. GLB uses the documented native basis conversion. Overall width/length includes tube radii and foot ferrules beyond the nominal centreline dimensions.

## What is authored

Connected curved head/foot tubes, vertical bars, discrete weld collars, bevelled side rails, eight mattress supports, gussets, corner plates, hex bolts, retaining tabs and foot ferrules. Textile objects are separate closed solids. Mattress piping, pillow piping, wrinkles, blanket folds/drape, edge waviness and sparse attached loose threads are geometry. Local iron dents are geometry; roughness, corrosion, dirt and textile weave are procedural materials.

There are only three shared material families: iron, linen and blanket. The complete preset writes 21 native PNG16 images, independent of bar count, and embeds 18 of those in GLB. OBJ writes relative shared texture paths. The supplied assets use 512 × 512 textures; the CLI supports 32–2048. This is an authoring-quality export, not a draw-call or LOD optimization.

## Validation and limitations

The full core suite and added bed tests cover deterministic generation, parameter/range guards, preset stability, manifold positive-volume geometry, unit corner normals, support contact, retaining-tab clearance, native atlas sharing and actual GLB/OBJ export. Independent tools verify serialized float32 geometry, UVs, normals/tangent handedness, image provenance and read-only reimport. `tools/bed_contact_oracle.py` uses NumPy/SciPy to test actual triangle support beneath the textile meshes.

The pillow and blanket are analytical static shapes. No physical cloth simulation, softness, sleeping interaction, damage gameplay, collisions or engine integration is claimed. Tubes intentionally penetrate weld collars, mounting plates and support joints. Contact checks address authored static support and protected bar/retainer envelopes, not unrestricted dynamic collisions. A small support tolerance is retained for the rounded mattress.

GLB is the primary portable PBR format. OBJ/MTL has importer-dependent PBR/normal-map support and does not carry the GLB hierarchy, metric secondary UV layer, sockets or grips. See the generated OBJ README. Retain GLB extras for the source-coordinate metadata. Rail grip sockets are optional authoring hints; the engine owns all runtime choices.

The images are Cycles previews of the exported GLB, without asset modifiers, geometry repair or material replacement. They are not engine screenshots. No photorealistic match to the aesthetic reference is claimed.

## Attribution

Bed geometry/material code is original project work. Inherited core noise math includes official Blender Cycles derivations under the notices and Apache/BSD licenses in `third_party/blender_cycles`; nlohmann JSON retains its MIT license. No proprietary third-party add-on code or source art is distributed. Historical Python fixtures are original-work offline reference oracles only, not production generators.
