<p align="center">
  <img src="docs/branding/jerico-j.svg" alt="Jerico modular J logo" width="104">
</p>

<h1 align="center">Jerico Playground</h1>

<p align="center"><em>An isolated laboratory for procedural geometry, surface math and physics experiments.</em></p>

> **Research status — October 24, 2026:** This repository is still named `JericoGeologyPlayground`. Its latest recorded implementation work is from September 28, 2026; later October commits document new mathematical scope and a separate rock-lab handoff. The historical geology goal is **PAUSED / PARTIAL**. Experimental results are not equivalent to finished assets or functionality integrated into JericoEngine.

## What this repository is

A C++20 Windows research application with a Diligent/Vulkan viewport and ImGui tools, originally linked to the **earlier** JericoEngine source/build tree. It exercises geometry algorithms, reproducible recipes, local physics demonstrations and visual/debug captures. It is not the O3DE-based JericoEngine, an independent production editor or a shipping runtime.

| Area | Historically implemented or tested here |
| --- | --- |
| Geometry and surface fields | Procedural primitive stacks, FBM/Voronoi/warp, layered rock, CSG cuts, surface-feature fields, local mesh deformation and reduction |
| Damage and fracture | Reusable damage fields, edge/cavity masks, crack paths, cut volumes, fresh interior regions and bounded fracture demonstrations |
| Splines and construction | Arc-length paths, stable frames, UV sweeps, pipes/rails/bridges/catwalks and other modular assembly test cases |
| Physics | Selected assembly-fragment and web/strand demonstrations using Jolt; not a gameplay destruction system |
| UV and baking | Experimental automatic UVs, high-to-low normal/feature maps, region masks, LOD comparisons and Vulkan captures |
| Reproducibility | Deterministic seeds, JSON recipes, Undo/Redo in selected labs, CTest cases, logs, metrics and saved evidence |

The September 28 evidence records **26/26 CTest checks passing** in the original Windows/Release setup, along with bounded Vulkan and Jolt demonstrations. These are historical results, **not tests rerun for this README**. Several cliffs and rock variants still had visibly artificial shapes; closed meshes and passing numerical tests did not settle artistic quality or all self-intersection cases.

## Current research boundary

The [October procedural-math directive](docs/PROCEDURAL_MATH_PLAYGROUND_2026_10_03.md) focuses on reusable mathematics: edge graphs, chains, curve frames, sockets, snap hysteresis, routing, supports, clearance, surface projection and shared damage. It describes research direction, not a claim that every listed solver is implemented.

A [separate rocks-only handoff](docs/DOT_ROCKS_PLAYGROUND_HANDOFF_2026_10_05.md) was prepared for Punto/DOT. It does **not** establish that another laboratory was created, executed or validated.

Transfer to the current [JericoEngine](https://github.com/JericoFX/Jerico_Engine/tree/development) is staged and selective: verify source and licensing, compare mathematical behavior, adapt it to an existing `JE::` owner, and validate editor, asset, physics, cook and runtime consumers there. The laboratory's window, renderer and application architecture are not integration targets. See the [integration inventory](https://github.com/JericoFX/Jerico_Engine/blob/development/docs/PLAYGROUND_TO_JE_INTEGRATION_MASTER.md).

## Reproducing the historical Windows build

The current CMake configuration requires **Visual Studio 2022**, CMake 3.25+, an older `JericoEngine` source checkout and its already-built dependencies. It is not a stand-alone build against the current O3DE-based engine. Set the paths to your own compatible local copies:

```powershell
cmake -S . -B build-surface -G "Visual Studio 17 2022" -A x64 -DJERICO_SOURCE="F:/JericoEngine" -DJERICO_DEPS="F:/JericoEngine/build/windows-vulkan-editor"
cmake --build build-surface --config Release -j 2
ctest --test-dir build-surface -C Release --output-on-failure
```

The historical executable was launched from `build/Release/JericoGeologyPlayground.exe` or `Run.cmd` where those artifacts existed. Existing `Evidence/` snapshots are not proof that this workspace can be rebuilt today without its original dependencies.

## License

JericoFX-owned original code is offered under **GNU GPL v3.0 only** ([full GPLv3 license](LICENSE); [scope and exceptions](LICENSE_POLICY.md)). This does not relicense third-party files or the older engine/dependencies used by historical builds. Redistribution of a linked executable must be checked against those separate licenses; no complete legal-release audit is claimed here.

## Reference material

- [Laboratory goal and pause status](GOAL.md)
- [Mathematical/back-end contract](BACKEND_CONTRACT.md)
- [Procedural assemblies and damage](docs/PROCEDURAL_ASSEMBLIES_DAMAGE_BRIDGES_2026_09_27.md)
- [Physical chain and rock-edge mathematics](docs/PHYSICAL_CHAIN_AND_ROCK_EDGE_MATH_2026_10_03.md)
- [Spline construction results](docs/SPLINE_CONSTRUCTION_LAB_RESULTS_2026_09_27.md)
- [Recorded test and visual evidence](Evidence/2026-09-28-spline-shared-path/README.md)
- [Project design and known limits](DESIGN.md)

This repository deliberately separates **implemented experiments**, **recorded verification**, **unresolved limitations**, and **proposed engine integration**. Its older source and evidence remain available for technical traceability.
