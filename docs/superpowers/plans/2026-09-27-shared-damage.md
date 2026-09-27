# Shared Procedural Damage Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task by task. Sol owns all source, Git and acceptance work; Luna may only inspect or validate.

**Goal:** Make the damage tools in `docs/SHARED_DAMAGE_MATH_ALL_PROCEDURAL_SYSTEMS_2026_09_27.md` reusable across the Playground, with editable and movable non-destructive damage areas under the owner's control.

**Architecture:** Keep `SurfaceFeatures` as the sole mesh feature analyzer and extend the existing native math in `RockAddonMath` with a common `DamageField`, stable operation IDs, selectors, edge and crack operations, and tier choice. A recipe stores ordered damage operations and transforms; geology, splines and assemblies consume the same fields while retaining their own semantic parts and geometry builders. The editor manipulates the recipe and regenerates the output; it never edits the source mesh in place.

**Tech Stack:** C++20, CMake/MSBuild Release, ImGui, existing Vulkan renderer and CTest. No new external dependency.

**Spec:** `docs/SHARED_DAMAGE_MATH_ALL_PROCEDURAL_SYSTEMS_2026_09_27.md`

## Global Constraints

- `F:\JericoEngine` code is read-only; use its mathematical research as reference, without copying addon code or graphs.
- Preserve current dirty Playground changes, existing stable IDs, authored source, semantic regions, UVs and deterministic replays.
- Manual and protected selection outranks procedural selection. A recipe edit must be reversible by restoring its parameters.
- Select the least costly representation that preserves silhouette, penetration, collision, interaction and gameplay; do not claim physical Jolt behavior from static geometry.
- Do not add a private curvature, cavity, edge-distance, projector, crack or aging implementation in a consumer. Extend the shared backend only for a concrete failing fixture.
- Keep visual quality, full scene performance and Engine integration as separate acceptance claims.

## Review Focus

- Moving a damage projector across a face or over an unrelated semantic part must affect only the newly selected region; test geology, spline and assembly consumers.
- Changing mesh resolution or LOD must preserve operation identity and chosen semantic parts; test independent tessellations.
- A protected part must remain intact through operation reorder, seed changes, rebuild and serialization; test the whole round trip.
- Very thin geometry, zero scale, NaN, excessive depth or contradictory remesh features must reject or choose a safe lower tier; test each invalid fixture.
- A visible break that needs penetration or collision must never silently downgrade to a material mark; test tier choice and resulting topology/metadata.

## Phase 1 — Common fields and editable operations

**Files:** `Source/RockAddonMath.hpp/.cpp`, `Source/SurfaceFeatures.hpp/.cpp`, `Source/DamageField.hpp/.cpp` (new if the common contract cannot fit coherently in `RockAddonMath`), `CMakeLists.txt`, `Tests/RockAddonMathTests.cpp`, `Tests/SurfaceFeatureTests.cpp`, `Tests/DamageFieldTests.cpp` (new).

**Produces:** `DamageField` inputs/weights and evaluator; stable operation ID and seeded sample; metric/relative edge operation with taper; projector, cavity/through-hole and fresh-interior predicates; path crack sample; aging from shared features; tier resolver; revision-keyed feature cache; finite and budget validation. `DamageOperation` holds an editable center/orientation/scale, selection, intensity and enabled state in recipe space.

- [ ] Add focused failing fixtures for field composition, protected/manual priority, edge width semantics, projector movement and rotation, crack path, cavity thickness, interior lineage, deterministic seed and tier selection. Run the focused test binary and confirm the expected failures.
- [ ] Implement only the shared primitives those fixtures require, using the current `SurfaceFeatureFields`, `projectorCoordinates`, `sampleDamageDecal`, `segmentCrackField`, `agingFields` and `proceduralElementId`. Run focused tests to green.
- [ ] Add cache hit/miss/cost observability; prove reuse for an unchanged mesh revision and invalidation after topology or position changes. Commit this independently testable backend.

## Phase 2 — Consumers and stable geometry

**Files:** `Source/Model.hpp/.cpp`, `Source/SplineConstruction.hpp/.cpp`, `Source/ProceduralAssemblies.hpp/.cpp`, `Source/AssemblyGeneration.cpp`, relevant consumer tests, and only needed new generator files.

**Consumes:** Phase 1 field and operation contract. **Produces:** the same editable damage operation can be evaluated against geologic surfaces, canonical spline intervals and assembly Part IDs. Consumers own only representation: material/bake, bounded displacement, local SDF, prepared variant or optional physical promotion.

- [ ] Write cross-consumer RED fixtures: one edge operation on rock and a structural edge; one projector on rock and metal; one path crack on geology and a beam; shared aging on wood/metal; unchanged IDs across quality levels. Include current broken-board variation and valid tiny-board fixture.
- [ ] Migrate existing `SurfaceDamage`, `DamageDecal`, `EdgeBreakup`, spline break and assembly `resolveDamage`/board cuts through the shared field or document a concrete failing fixture before extending it. Preserve legacy recipe load and current controls where they map directly. Add door, window, hatch and prop demonstration consumers where the repository has none, using the same generic assembly/solid primitives.
- [ ] Verify topology, UV/material regions, fresh interior, protection, bounds and deterministic replay after each family. Commit each working consumer group instead of holding all changes uncommitted.

## Phase 3 — Owner controls, visual proof and handoff

**Files:** `Source/Main.cpp`, `Source/SplineLabUI.cpp`, `Source/AssemblyLabUI.cpp`, shared editor widget files if needed, evidence capture code, `GOAL.md`, existing lab result docs and the authorized Engine extraction Markdown.

**Consumes:** Phase 2 recipes. **Produces:** one consistent debug/control surface for operation choice, Auto/tier, seed, amount, edge width mode, taper, projector transform, cavity/crack controls, semantic/painted/path selection, protected/locked state and regenerate. Selection and viewport drag move an operation, not the source mesh. Diagnostic views display the fields and selected tier/IDs.

- [ ] Add failing editor/model tests for selecting an area, moving it, undoing/redoing or restoring parameters, rebuilding and saving/loading it without altering the source. Implement the common controls and viewport interaction; verify the normal executable, not only headless captures.
- [ ] Run focused and full Release CTest, bounded invalid-input gates, deterministic replay, original-resolution geology/spline/assembly captures and measured cache/generation/draw cost. Inspect beauty and report each partial visual limitation honestly.
- [ ] Review the final diff and preserved dirty-state inventory; publish scoped Playground commits and update the single authorized read-only Engine handoff Markdown with exact hashes, evidence and remaining limitations. Complete the goal only when the spec's applicable acceptance cases have evidence.
