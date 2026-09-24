# Geological families — approved continuation

## Current user directive — 2026-09-08

The user explicitly changed the distribution assumption: "Tu usa código de los
addons, los compre y me pertenecen el engine no va a ser open source". The previous
independent-only implementation preference below is historical, not the current
restriction. Prefer reuse of the purchased installed addon implementations in the
private Blender backend when technically suitable. Preserve third-party authorship,
license notices and provenance; do not relabel third-party code or graphs as
original Jerico work. Python licensing and bundled BLEND/material/texture licensing
are audited separately. See `PRIVATE_ADDON_REUSE_DIRECTIVE_20260908.md` for concrete
component findings and the native owner's reconciliation boundary.

This changes the local implementation preference, not shared Engine authorities:
the main owner must reconcile those before dependency integration. No Engine,
Editor, shared authority or Git mutation is authorized in this backend task.

Date: 2026-09-08. This is an extension of the approved own-node work, not a new
engine subsystem or replacement scheduler. The owner requires broad geological
coverage and no loss of quality relative to private addon references. Current v2
Beauty is rejected. Only backend files are writable; Blender requires a new
coordinated window. The user explicitly requested continuing implementation.

## Implementation sequence and observable gates

1. Structural cells and variable beds: original graphs using Blender's native
   topology, extrusion, scaling and volume operators. No new Delaunay/boolean
   library. CPU RED checks real interface binding and nonuniform metric bed layout;
   Blender RED checks nonempty closed output, changed seed, source/void preservation
   and actual masks. Visual RED: no regularly sawn loaf, uniform pancake stack or
   detached flakes as a substitute for rock structure.
2. Karst cave wall and glacier-ice wall: BSP supplies the cave's solid wall and
   traversable void. The backend changes surface morphology, never invents the
   level layout. Flow-oriented concavity, scallops and melt channels are geometric;
   ice cannot be a blue recolor of a rock. Negative voids and keep-clear survive.
3. Follow with columnar joints attached to a coherent mass, weathered granite,
   limestone/chalk, shale/slate, basalt/lava, scree/talus, stalactites/stalagmites,
   flowstone, frozen waterfalls/icicles and glacier crevasses. Each requires its
   own geometry, semantics and image gates, not just a renamed preset.

## Files and responsibilities

- `geology_layout.py`: seeded metric bed boundaries, bounded before Blender.
- `geology_library.py`: new original family identities and complete controls.
- `geology_nodes.py`: native Blender graphs and deterministic job-local helpers.
- Existing `nodes.build`: one dispatch into the existing worker; old v1/v2 remain.
- `tests/test_geology_cells.py`: CPU behavior and validation; no mock geometry.
- Blender evaluation tests/captures: actual worker and retained immutable inputs.
- `GEOLOGY_FAMILY_RESEARCH.md`: primary-source findings and explicit coverage status.

All distances use the existing Jerico/Blender conversion. Seeds use `sub_seed` and
layer identity; helpers are rebuilt through the existing recipe/job route. UI
remains M10.5. No paid graph, Python, texture, material or BLEND is redistributed.

## Historical independent-only design choices (superseded preference)

Selected: shared native GN operations with family-specific constructions. Pure
noise displacement is insufficient for structural rock. Copying a paid graph
through different Python is not independent authorship. Native Blender operations
are used directly; bought presets are private visual/behavioral comparators only.

For cell-based rock, native triangulation/dual topology and individual extrusion
produce connected bevels and recessed seams. Variable beds are explicit metric
intervals, not a periodic sine/sawtooth. Basal mass and BSP intersection constrain
structural helpers. Exact algorithm/controls remain candidate until evaluated.

## End-to-end completeness boundary

Source: closed BSP solid plus source IDs, materials/physical IDs, selection and
protected volumes. Processing: deterministic family graph and layer masks. Output:
existing worker mesh/LOD/semantic/GLB/collision candidates. Engine, cook/runtime,
Jolt, navigation, destruction, audio/surface responses, native materials, streaming,
undo/apply and source-free Shipping require their existing native owners.
`INTEGRATION REQUIRED` at M10.5/native backend admission: consume the established
schema/job/result interfaces; do not introduce an alternate bridge.

Geological hardness is not a substitute for physical-material authority. Ice
opacity, transmission, absorption and roughness are preview material data until
native mapping/baking is admitted. No geometry test closes these downstream gates.

## Execution checklist

- [ ] Retain CPU RED before new implementation.
- [ ] Implement controls and deterministic helpers; run focused CPU gates.
- [ ] Implement candidate native graphs without opening Blender prematurely.
- [ ] Coordinate and execute actual worker/visual REDs and successive corrections.
- [ ] Audit original-resolution images against private references and report gaps.
- [ ] Extend remaining named families only with distinct morphological evidence.

This checklist tracks work; it is not a claim that all geological forms already
exist or that a finite preset list covers every possible landscape.
