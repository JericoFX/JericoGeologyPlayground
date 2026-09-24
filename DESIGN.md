# Jerico Geology Playground — standalone design and v2 extension

## Purpose and boundary

An independent C++ executable to test geological operations, their visual value and cost against the M8 approach. This does not establish that the experiment is better than M8: that requires matched recipes, scale, camera, geometry budgets and measurements.

The owner approved design and implementation from `Codex — Jerico Geology Playground.md`, then explicitly requested reuse of actual engine code. Project and build live outside `F:/JericoEngine`. Engine sources and its admitted dependency installation are read-only inputs. No changes to production rendering, geology, editor, formats, physics, navigation or gameplay. No Blender dependency, addon assets or copied addon implementation.

## Design

The application has an editable ordered mesh operation stack. Local fields supply displacement and mask values. Domain Warp modifies sampling coordinates for subsequent field operations; it does not silently displace the mesh. Structural layers replicate and flatten the incoming shape into oriented slabs with per-layer thickness, scale and lateral offsets. This makes layers actual geometry rather than a sinusoidal bump. It is not a geological simulation or Boolean solid union.

Actual Jerico code is compiled/referenced from its original paths: native Window, TriangleMesh/Vec3d and the public geology FBM evaluator. Other reuse candidates are evaluated for clean boundaries. The full RenderDevice product renderer carries asset/runtime dependencies; a small Diligent rendering host is allowed if it is not cleanly reusable. Diligent/ImGui use the same pinned dependency build as the engine. There is no engine dependency on this project.

Architecture: Recipe/Layer data -> ordered evaluator -> Jerico TriangleMesh with normals and scalar field -> Diligent buffers -> viewport. CPU reference evaluation is the correctness baseline for reused code. GPU local evaluation is optional and measured separately; topology stays CPU. A worker builds previews from immutable recipe snapshots; stale generations never replace newer edits. UI remains responsive and preview resolution is lower while editing.

UI: operations left, viewport center, parameters right, telemetry bottom. Add/select/enable/duplicate/delete/reorder, reset, seeds, recipe save/load, presets composed of ordinary editable layers. Orbit/pan/zoom, Lit/Normals/Wireframe, final/up-to-selected/selected-field inspection. Errors are shown without discarding the last valid mesh or recipe. Save/load uses a versioned experimental JSON schema and validates bounds before adopting a file.

## Acceptance

- Four bases: cube, sphere, cylinder, cone.
- Transform, FBM normal and vector displacement, Voronoi F1/F2/edge, structural layers, domain warp, six masks, surface-fracture experiment and mesh-neighbor relaxation.
- Reordering noncommuting operations changes geometry; disabled layers contribute nothing.
- Same recipe/seed reproduces the same CPU result; saved/loaded recipe reproduces geometry.
- Fields and up-to-layer views reveal intermediate behavior.
- A real Diligent/Vulkan window renders distinct rock and layered examples with readable controls.
- Geometry counts, CPU generation time, GPU timing where available and estimated buffer bytes are explicitly distinguished.
- Negative recipe cases, reset and failed load preserve valid state.

## System completeness audit

Applicable now: input recipe, authoring workflow, CPU geometry/field evaluation, optional GPU local evaluation, renderer/shaders, experimental serialization, camera/debug visualization, metrics, cancellation/stale-result fencing, error handling, tests and documentation. Materials are preview lighting and field colors; no product material pipeline.

Intentionally outside this standalone experiment: ECS, product asset cook, collision/physics, animation, Luau/events, Sequencer, production save/load, streaming/LOD, physical destruction, navigation, particles and audio. Future adoption requires explicit owners/contracts and matched M8 acceptance; no compatibility claim beyond the shared source paths actually exercised.

## Implementation plan

1. RED tests for base geometry, operation ordering and recipe roundtrip; implement model/evaluator using shared sources.
2. Independent CMake target, native engine window and Diligent/ImGui host; real viewport and camera.
3. Full operation editing, intermediate field views, async preview, save/load and presets.
4. Bounded GPU experiment with CPU reference comparison and separate timing; no forced GPU topology.
5. Focused tests, real runtime captures, build/source provenance and concise use guide. Preserve all engine files and existing work.

## Implemented v2 extension

The owner asked to continue the Playground and prepare tested code for later M8 adoption. The adopted five-step proposal is Shared Macro Warp, variable layer thickness, correlated drift, macroform envelope and edge-specific breakup. Each has an isolated RED/green contract and paired Vulkan captures. Existing v1 recipes are preserved and checked against retained geometry hashes.

Native plane cuts and linear subdivision reuse the existing engine adapter. The isolated evaluator remains a C++ library with no Diligent/ImGui dependency; its consumer test covers multiple seeds, source lineage, serialization, cancellation and the original recipes. Rendering normals split at region boundaries after topological operations; geometry remains welded. See `BACKEND_CONTRACT.md` for the exact equations, public seam, metadata and integration audit.

## New attached surface / Manifold / ice proposal

`E:/este/JERICO_GEOLOGY_PLAYGROUND_SURFACE_MANIFOLD_ICE.md` was received as a proposal while v2 validation was in progress. It is not evidence that those capabilities exist. Its Surface -> Solidify -> Difference -> Ice order is technically consistent with the standalone boundary. The locally installed Manifold headers report 3.5.2.

Implemented in v3: Plane source, explicit Surface/Solid state, surface solidification, Strata Relief, Ridged Noise and a real BSP selection input seam. Solid cliff uses fixed-direction extrusion after visual inspection revealed folds from large normal offsets. Native reduction is quality-gated and may return its input unchanged. See `Evidence/SURFACE_V3_VERIFICATION.md` for measured results and limitations.

Still NOT IMPLEMENTED: arbitrary procedural cutter Booleans, ice accumulation and icicle growth. A plane must not enter the closed-solid adapter before solidification and validation. Ice should reuse accumulation/masks/directional growth and preserve material semantics, without implying a physical freezing simulation. Heightmap and RBrush are explicitly document-only for future M8, as are the full product recipe-management requirements in `docs/M8_GEOLOGY_BACKEND.md`. This result does not declare the whole attached proposal or M8 complete.

## Implemented v4 extension

Bounded owner-approved extension: signed curvature, crease proximity, finite-radius contact masks, seeded inward wear and two diagnostic presets. User additionally requested real high-to-low normal baking: implemented CPU projection/atlas/export, asynchronous preview and Vulkan tangent-space rendering. Existing versioned recipes remain supported. Normal baking retains the high as source and does not claim silhouette preservation or production UV quality.

The current evidence is `Evidence/surface-features-v4/VERIFICATION.md`; reusable interfaces and downstream M8 requirements are in `BACKEND_CONTRACT.md`. No Engine edits, addon installation, production integration or M8 closure were performed. Arbitrary cutters, ice, heightmaps and RBrush remain outside this extension.

## v5 native research coverage

The owner clarified that addon references must become native C++ mathematics, without running Blender. v5 adds projector damage as an actual recipe operation/preset, derived surface masks, independent AO and thickness measurements, and specialist maps projected from high to low. Standalone kernels cover additional damage composition, crack/sphere fields, semantic lattices and aging; these kernels are not full integrated authoring systems.

The existing completeness audit still applies. Source/CPU/recipe serialization, debug fields, tests and export are exercised locally; normal rendering consumes the baked normal. Other specialist outputs stop at export and require a production material consumer. Continuous fields retain their source mesh domain; topology-preserving damage retains region GUIDs. Region preview colors cannot replace semantic IDs. No new runtime/physics/destruction integration is claimed.

The complete addon-research request remains PARTIAL: advanced remesh/segmentation, Boolean interiors and full material/UV/bake production capabilities remain unimplemented. BACKEND_CONTRACT.md lists the concrete native seam and limits. M8 should consume verified code with its required downstream tests; this standalone work does not authorize an untested integration.
