# Geology backend implementation plan

**Goal:** Finish a reusable, tested geology generator inside the independent Playground, with structural rock shapes and explicit engine-facing outputs; preserve the active M9 checkout and user recipes.

**Architecture:** Keep Recipe -> generator -> Jerico TriangleMesh independent of ImGui/Diligent. Reuse the engine's public solid validation, plane splitting and linear subdivision where they link cleanly. Produce true plane-cut surfaces and sliced strata, stable region identities, scalar fields and diagnostics. The Playground is one frontend of that library.

**Tech Stack:** C++20, existing Jerico solid/FBM sources and admitted prebuilt dependencies, Diligent/Vulkan, ImGui. No Blender or addon implementation dependency.

**Spec:** DESIGN.md plus Evidence/VISUAL_GAP_ANALYSIS.md and the owner's explicit continuation/standalone clarification on 2026-09-15.

## Constraints

- All writes/builds/evidence remain in F:/JericoGeologyPlayground. Engine files and dependencies are read-only inputs.
- Preserve the live user window and recipes. Never overwrite the running executable; use the verified build path or a new output directory when it is in use.
- No new dependency, alternate engine checkout, branch, task, production format change or M8 closure claim.
- The owner authorized implementation and all tests; Astra is the sole writer, auxiliaries read only.

## Tasks

### 1. Native geometry seam and validity

Files: CMakeLists.txt, Source/SolidGeometry.hpp/.cpp, Tests/SolidGeometryTests.cpp.

- [ ] RED: engine-native validation and plane cut must accept our closed primitive with complete region IDs, conserve volume across both halves and preserve source/cut regions.
- [ ] Compile/link the minimal original source/public contracts. Add cancellation and bounded triangle limits to the adapter.
- [ ] Prove determinism, invalid/open input rejection, cap identity and dense planar subdivision.

### 2. Structural rock and strata

Files: Source/Model.hpp/.cpp, Source/SolidGeometry.hpp/.cpp, Tests/ModelTests.cpp.

- [ ] RED: plane formation adds several coherent planar regions and changes silhouette; strata are closed slices of the input with distinct profiles and persistent cap/stratum metadata.
- [ ] Implement bounded seeded plane cuts, source-preserving strata slices, directed coordinate/output weights and well-scaled defaults.
- [ ] Preserve old JSON recipes through explicit version migration; never silently reinterpret old layer formation.
- [ ] Retain existing stack order, masking, cancellation and async snapshot invariants.

### 3. Frontend consumption and visual families

Files: Source/Main.cpp, Source/Renderer.cpp, Tests/ModelTests.cpp.

- [ ] Expose the structural operations and independent XYZ weights in the existing UI.
- [ ] Use region-aware corner normals so cut faces keep their planes without making every subdivided triangle a visual facet.
- [ ] Build family presets from the public stack; validate multiple seeds, topology, region lineage and bounds before final publication.
- [ ] Capture original-resolution Lit/Normals/Wireframe/field output and inspect silhouette, slab edges and cracks. Correct defects without weakening gates.

### 4. Reusable backend delivery

Files: Source/public API as needed, Tests/BackendConsumerTests.cpp, README.md, integration guide, Evidence/.

- [ ] Build a consumer with no ImGui/Diligent dependency that loads a recipe, evaluates, validates engine mesh/regions and saves/reloads deterministically.
- [ ] Describe units, spaces, region/field meanings, ownership, cancellation, error handling, version migration and frontend responsibilities.
- [ ] Run final tests, seed matrix, Vulkan proof, provenance hashes and focused read-only review.
- [ ] Distinguish ready backend operations from any remaining M8 cook/runtime/material/collision integration. Do not call untested downstream wiring complete.
