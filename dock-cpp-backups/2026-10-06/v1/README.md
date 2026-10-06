# JericoDockCppLab

An isolated native C++17 procedural dock study. No Blender or Python runs in the geometry, damage, material-bake or export pipeline. Blender is used only to import the exported GLB, set the presentation stage and render evidence. Python tools are independent read-only QA.

## Build and run

Requires a C++17 compiler, CMake 3.16+ and zlib development headers. nlohmann ordered_json is bundled. Preserve third-party license notices.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j4
ctest --test-dir build --output-on-failure
./build/jerico_dock --out output/dock --resolution 512
./build/jerico_dock --recipe fixtures/straight.json --out output/straight --resolution 256
```

The command writes GLB, OBJ/MTL with relative PNG16 textures, the full evaluated Scene JSON, normalized authoring metadata and timing/validation report. Pass `--no-obj` to omit OBJ. Native texture paths in the authoring JSON are local to its output folder; GLB is self-contained and OBJ references sibling images. Keep the complete OBJ folder together.

## Default authored layout

- About 8 m main Catmull–Rom route, 2.4 m width, deck height 1.5 m, 75 mm timber, approximately 200 mm board pitch
- Adjustable branch, landing and explicit stair flight to a 0.15 m lower datum
- Adaptive sampling, arc-length placement and parallel-transport frame adapted from the user's existing spline implementation
- Individual stable-ID wood profiles with chamfered edges, restrained warp, actual longitudinal grooves, irregular breaks and attached splinters
- Individual piles, separate collars, crossbeams, longitudinal beams and braces. Deliberate bearing overlaps are joints, not accidental wholesale intersection
- Authored main-route damage interval, with independent severity. Severity zero is intact; piles, beams, stairs and their IDs remain unchanged by deck damage
- Six shared weathered wood texture variants plus iron. No per-board texture atlas. Original WIP03 exports are extended only in this isolated copy to deduplicate images/materials

The values are authoring defaults. Reference models were normalized and did not establish real-world dimensions.

## Contracts and limitations

Source scene uses right-handed metres, Z-up. GLB converts to Y-up. Stable part IDs, source-board IDs, bearing references, disabled sockets for lost deck wood and authoring metadata survive GLB extras. OBJ has no live hierarchy or sockets; retain Scene JSON. Some OBJ consumers ignore the `norm` normal-map extension; GLB is the preferred portable PBR format.

This is an authoring generator, not an engine physics implementation or structural engineering certification. Sockets identify preserved/invalidated attachments; they do not simulate forces or fracture propagation. Supports are protected by default. `allow_support_damage=true` is explicitly rejected until a corresponding support-failure recipe and connectivity contract are implemented, rather than silently breaking an occupied load path.

Open Catmull–Rom paths are supported by this increment. It does not replace the existing editor, import fused source GLB meshes as reusable component kits, or implement arbitrary mesh booleans. Extreme bends are rejected if deck boards cannot fit without overlap. Fine detail is finite-resolution and is not claimed equivalent to the fused reference assets.

## Provenance

Core geometry, evaluation, material noise, native PNG16 and GLB/OBJ support are copied from the immutable WIP03 checkpoint. Original snapshots, fixtures and the user's local Playground/engine are unchanged. The spline polynomial, adaptive tests and double-reflection transport were adapted from the supplied DockSystem/SplineConstruction reference handoff; source Y-up conventions were adapted to this core's Z-up basis.

Reference GLBs, source transfer archive and historical screenshots remain private review inputs and are excluded from source/backups. No proprietary addon source was copied. The repository does not claim a new redistribution license for external reference assets.

Continuous edge deformation is independently reusable in `deform_timber_edges`: local grain coordinates, exact endpoint/bearing masks, bounded transverse displacement and rejection of invalid topology. Final default affects 53 pieces while retaining 2,823 protected vertices.

Delivery: the preserved GLB512 uses 512×512 shared maps and the 1200 px renders use that version. The separately named lightweight GLB384 and OBJ384 use 384×384 maps with identical geometry; their raster material data and sampling are not byte-identical to GLB512.

This source backup preserves code, tests, recipes, QA tools and license notices. It excludes coordination documents, generated assets and verification logs. Rebuild and run the four native CTest suites with the commands above; independent QA tools operate on newly generated output.
