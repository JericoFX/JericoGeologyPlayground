# Jerico Procedural C++ Lab — WIP

Native C++17 recipe-driven asset generation. No Python interpreter, Blender process, bpy, input mesh or fixed input texture is linked or invoked by the executable. Python/Blender files under `fixtures/sources` are frozen original-work reference oracles only; production code is under `src` and `include`.

## Build and test

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build -j2
    ctest --test-dir build --output-on-failure

C++17, a compiler, CMake >=3.16 and the zlib development library are required. Geometry and material evaluation are native C++; zlib is used only for lossless PNG encoding. The vendored nlohmann JSON header is MIT licensed. Material noise math derives from official Blender Cycles Apache-2.0 and BSD-3-Clause source; see source attribution in materials.cpp and third_party. No proprietary add-ons are included.

## Generate

    build/jerico_procedural --family fracture --recipe fixtures/sources/JericoGlassFractureStudy/recipes/industrial_window_broken.json --out output/source.json
    build/jerico_procedural --family fracture --recipe fixtures/sources/JericoGlassFractureStudy/recipes/industrial_window_broken.json --out output/native/scene.json --evaluate 1 --bake 1024 --glb output/native/window.glb

Keep each retained asset in its own output directory: native JSON references the accompanying texture files. A GLB embeds those images and is self-contained.

Families linked: fracture, industrial (plain/vented/louvered/window with local cuts, cracks and edge wear), finewear, wood, singlepane, damp, furniture. Core C++ generates all source vertices, faces, metric UVs, material roles, stable IDs, pivots, sockets and grips. `--evaluate 1` creates native rounded bevel geometry and corner normals. `--bake N` generates new native atlas textures from the procedural C++ material samplers; it never reads baked Blender textures. The atlas names have explicit parameter channel descriptions. Add `--obj output/native/asset.obj` to emit native world-space OBJ/MTL with relative PNG textures. The accompanying README explains OBJ limits: no hierarchy/secondary metric UV layer or physical transmission, and Blender 4.3.2 ignores the PBR `norm` statement. GLB is the primary complete material transport.

## Verified native gates

- 26/26 integrated CTest suites pass, including executable generation, complete offline source oracles, evaluated mesh tests, material tests, native PNG, GLB and OBJ/MTL tests, a CLI OBJ smoke test, and independent GLB/OBJ validators.
- 144 fracture, 142 Boolean/crack, 406 localized-damage and 20 finewear oracle cases pass, including matching explicit rejections. Base seeds are signed64; derived positive offsets are computed safely up to65bits rather than overflowing.
- 25 wood/single-pane,16 furniture and12 damp-wood full fixtures match identifiers, positions, topology, metric UVs, materials, sockets/grips and semantic metadata at2e-12 tolerance. Furniture drawer motion and continuous clearance checks pass.
- 60/60 family recipes generate valid native evaluated meshes.544 representative pieces pass manifold/volume/UV/unit-normal and shading-normal hemisphere checks and remain valid after float32 conversion. AddressSanitizer and UndefinedBehaviorSanitizer were also run on the evaluated-piece suite; leak checking was not available in this environment.
- Native scalar material samplers are compared with measured original Blender graphs. The tests include original noise/LUT behavior and signed64 material seeds.
- The first full fracture-window material comparison uses the same camera/lights/64-sample render and native PNG16 atlas at1024: RGB MAE0.000218 in0–1 units on the final regression, versus the frozen original. This is a measured close match for that scene, not bitwise identity or all-family aesthetic approval.
- Native GLB generation and independent buffer/PNG/tangent validation pass; Blender import is read-only and makes no geometry, material or tangent repairs. The command also succeeds with an empty environment and empty PATH; no Python/Blender runtime is linked.

## Boundaries and remaining gates

This is WIP. Source geometry parity is separate from evaluated bevel topology and final material/render parity. The native bevel uses three-segment rounded edges and shared surface-fan/corner construction. Its tessellation is not identical to Blender's; sampled surface differences of up to 0.514 mm remain in the selected difficult timber/cap pieces. Source flat-face normals and fully smooth weighted normals are now native; mixed smooth/flat hardware retains the earlier calibrated native normal construction. All evaluated family recipes pass geometry validity. Thin atlas islands now retain valid samples, and the wood normal filter uses a camera-independent physical texel footprint checked against Cycles and at two viewing distances. The four representative integrated material previews have whole-image RGB MAE from 0.000618 to 0.001590 against their frozen references; projected asset-bounds errors are recorded separately. Portable GLB reimport shading and distinct damage variants have their own gates, rather than being inferred from these images.

Preview rendering uses Blender strictly as a viewer of C++ output. It is not a Jerico/Diligent/Playground engine renderer. No engine integration, physics runtime, collision build or interactive runtime behavior has been tested here. No main-branch merge or deployment is implied.

Read `docs/parity-matrix.md`, `docs/geometry-coverage.md`, `docs/material-coverage.md`, `docs/glb-tangent-convention.md`, `docs/assets-size-budget.md` and `docs/render-boundary.md` for gates and residuals. The original snapshots remain unchanged.

## Reproduce the four-family visual check

    python3 tools/run_material_visual_gate.py --out output/visual-check --render-reference --bake 512 --jobs 2

This optional offline harness requires Blender 4.3.2 for original-reference rendering and native-output preview. The harness invokes the standalone C++ executable for all native asset data. Its four recipes are portable files under `fixtures/visual`; it writes exact input hashes, commands, native GLBs, independent binary validation and matched-camera PNGs. The separate `--render-reference` path uses only the frozen original source snapshots.

The quality-reference GLBs are authoring exports with per-part PNG16 atlases, not runtime-optimized content. Raw GLB sizes are 24–95 MB; lossless ZIP reduces the metal-door example below 20 MB but does not make all assets fit that limit. Mesh/texture counts, decoded memory costs and a rejected lossless PNG-filter optimization are recorded in `docs/assets-size-budget.md`.

## Source backup scope

This checkpoint preserves native code, tests, recipes, generated numeric fixtures, frozen offline reference sources and licenses. Preview images, models, evidence logs, delivery metadata and coordination plans are excluded. The C++ port remains WIP at the documented parity boundaries; springs are not implemented in this checkpoint. Python is optional for the six offline oracle/format-validation CTest suites and is not a runtime dependency of the generator.
