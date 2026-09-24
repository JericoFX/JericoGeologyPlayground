# F4 native geology diagnostic preview

**Goal:** edit existing geology values and capture actual Vulkan results through
the already admitted Diligent ImGui integration, as explicitly authorized by the
owner on2026-09-05. This is a bounded F4 diagnostic tool; final editor UI is M10.5.
**Architecture:** Tool-only native host of existing RenderDevice/RenderEditorOverlay,
GeologyGpuSession and GeologyCook. Immutable loaded assets, draft recipe, explicit
generation/cancellation, renderer replacement only after success. Captures include
the exact rendered recipe/cooked identities. No alternate evaluator or mesher.
**Tech Stack:** C++20, native Windows window, admitted Diligent/Vulkan and ImGui.
**Spec:** latest owner direction and `docs/work/ACTIVE_SLICE.md` F4 contracts.

One Astra writer in the authoritative checkout; auxiliaries READ_ONLY/CMake.
No new task, branch, checkout or dependency; preserve all189 preexisting entries.
Implementation paths: `Tools/GeologyPreview/`, surgical `Tools/CMakeLists.txt`,
current evidence/ACTIVE_SLICE/HANDOFF. The demonstrated concurrent queue defect
also requires the existing ThirdParty patch mechanism and dependency register:
the pinned Diligent GetQueue ignores its queueIndex argument. No Qt panels or
Shipping dependency change; the backend correction preserves the admitted pin.

- [x] RED: native host loads matched recipe/mesh/material/textures, renders the
  real rock, and its bounded verification fails on missing ImGui controls.
- [x] Implement existing-layer scalar controls, Generate/Cancel/restore, camera
  controls, and clean captures through the existing render/cook interfaces.
- [x] Verify real ImGui input changes the saved recipe and resulting mesh;
  cancellation preserves the displayed result; restore is exact; captures exclude
  UI and retain recipe/mesh/collision. Keep failure diagnostics visible.
- [x] Build, inspect native original-resolution images, review preservation and
  diff, selectively commit/push. Do not call the rock Beauty accepted.

Pre-implementation completeness audit: source/recipe and existing cooked input
identities are validated; generated field/classification/structural data flow
through GeologyCook to mesh/collision and the existing renderer/material path.
The tool owns only its new output directory, never a project scene or source
asset. Render thread owns uploads; one worker owns the compute session, with
cancellation and no concurrent evaluation. Runtime formats, physics consumers,
LOD/streaming, scripting/events/Sequencer/animation/audio/particles remain unchanged;
their actual-member F4 gates stay open. No Shipping/whole-editor UI acceptance
or new performance budget follows from this diagnostic host.
