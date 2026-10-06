# Jerico Cobweb Physics Lab — experimental checkpoint

**WIP / EXPERIMENTAL: BLOCKED on panel stability; final visual acceptance is incomplete.** This snapshot preserves a working standalone three-class strand assay and the in-progress dense-panel implementation. It is not production-ready gameplay physics.

The C++17 reference solver tests verify fixed anchors, tension-only response, permanent cuts/contact removal, Z-up gravity and floor, swept sphere contact, physical overload timing, deterministic ordering, exact state replay, invalid-input rejection, and three artistic breaking strengths (0.25 / 1 / 6). They do not certify the whole generated panel. No biological silk calibration is claimed.

The panel reuses original strand/node provenance, all 33 original world-anchor IDs and exact positions, shared static damage, and conservative local UV/mesh cuts. Decorative detail has representative support/contact. Millimetric graph clustering is an explicit approximation: only connected junctions within 5 mm are contracted; distinct world anchors never merge; all source-node IDs survive in version 2checkpoints. The original visual rest mesh is not reshaped by clustering.

## Build and focused checks

```sh
./scripts/build.sh
./build/core_tests
./build/physics_solver_tests
./build/physics_binding_tests
python -m unittest discover -s tests -p 'test*.py'
```

No additional runtime dependencies are needed beyond a C++17 compiler and Python 3. Blender is optional for rendering exported frames. CMake configuration is supplied but the shell build is the verified route.

## Run and replay

```sh
./build/cobweb_sim --recipe recipes/physics_panel.recipe --out outputs/new_run
./build/cobweb_sim --recipe recipes/physics_panel.recipe --resume outputs/new_run/03_pretear_or_final/replay.checkpoint --ticks 800 --out outputs/new_replay
```

Every output folder must be new. Recipe files use `key=value`. Script commands are `force`, `pull` (stops after a new break), `sphere`, and explicit `cut`; the exact script is exported. The final fixture release in the default script is labeled scripted rather than presented as load-driven failure. Stage metadata reports when a short run has no observed tear/release.

`final.checkpoint` and per-frame `replay.checkpoint` bind full state to the exact recipe and interaction script. A frame's `checkpoint.txt` is a lower-level C++ solver state, not a CLI replay file. Physics defaults, graph-budget limits and numerical validation remain experimental.

## Known panel blocker at this checkpoint

Tiny original junction edges made the initial 4×2 substep/pass target accumulate unphysical residual energy and sometimes break under gravity. More iterations alone did not solve it. 5 mm connected-junction contraction removed the observed spontaneous failures in the latest probes, and 8×8passed one seed’s 3% energy bound, but two-seed panel acceptance and the substantial gravity sag remain under investigation. Do not interpret passing strand tests or a settled picture as panel approval.

`tests/panel_convergence_regression.cpp` is an intentionally failing diagnostic, not a passed acceptance test. It uses the original tighter conditioning target and explicit intact-hold criteria to expose the unresolved issue. Build/run it separately:

```sh
g++ -std=c++17 -O2 -Iinclude tests/panel_convergence_regression.cpp src/geometry.cpp src/fibers.cpp src/damage.cpp src/physics_graph.cpp src/xpbd_solver.cpp -o build/panel_check
build/panel_check
```

No images, generated outputs, binary builds, private manifests, account information or machine-specific paths are needed by this source checkpoint. No engine/Jolt integration, bending or self-collision is included. Future snapshots may change the diagnostic recipe after numerical evidence supports it.
