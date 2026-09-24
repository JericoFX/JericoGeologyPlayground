# Geological families and single-mesh mixing implementation plan

> Execution: inline in the existing task under the explicit Tools/BlenderWorker
> exception. No subagent writers, worktree, Git mutation or Engine/Editor changes.
> Use executing-plans and TDD with real Blender evaluation, not source assertions.

Goal: visibly distinct native geology families and regional mixtures on a single
mesh, with real captures and preserved source/selection/attributes.

Architecture: surface branches evaluate the same original topology. A native
wrapper samples each branch's position/attributes by point index, combines bounded
weighted offsets, and returns the original topology with layer-namespaced masks.
Topology-changing solids/attachments remain separate explicitly validated routes;
joining objects alone does not satisfy the continuous-surface mixing contract.

Spec: GEOLOGY_FAMILY_RESEARCH.md sections 1–7 and the user's explicit continuation
of 2026-09-10. Tech: pinned Blender 5.2.1, bpy/native GN, existing _Group/supervisor.

## Constraints

- Owner-renewed window 2026-09-10 13:43–15:13 UTC; at most 40 geometry children
  and 59 CPU renders in total, including failures/repeats, 512x512, one child at
  a time, two BELOW_NORMAL threads, 30 seconds per child and 20 minutes aggregate.
  The last renewal permits only the parallel-axis fix, two evaluations and two
  renders; all earlier consumption remains included.
  The retained 0.412356-second overlap incident and strictly serial replacement
  are documented in FAMILY_MIX_STATUS.md.
- Only native independently authored code; reference directories read-only.
- Preserve the accepted relief factory until a tested change requires it.
- Geometry proof is not Engine/cook/runtime/LOD/collision admission.
- Every new output remains under _local; do not overwrite earlier evidence.

## 1. Regional surface mixer

Files: create surface_family_mix.py and tests/probe_family_gallery.py; extend
tests/probe_surface_relief.py only to dispatch the new tests/captures and hash
their dependencies. Preserve its existing suite and render behavior.

Interface: build_surface_mix(layers) -> GeometryNodeTree. Each layer has a unique
id, existing style, POINT mask attribute name and optional socket values.
All branches receive the original support. Missing masks mean zero influence.
Normalize overlaps only when the sum exceeds one; retain partial influence below
one. Store each layer's masks separately and preserve source face IDs and UV.

- [x] RED: a fixture with two disjoint masks and a shared transition must fail
  because the mixer is absent. Core checks exercise evaluated mesh data:

```python
assert list(mixed.polygons[0].vertices) == list(source.polygons[0].vertices)
assert mixed.uv_layers['UVMap'].data[0].uv[:] == source.uv_layers['UVMap'].data[0].uv[:]
assert (mixed.vertices[left].co - only_left.vertices[left].co).length < 1e-5
assert (mixed.vertices[right].co - only_right.vertices[right].co).length < 1e-5
assert (mixed.vertices[blend].co - (only_left.vertices[blend].co + only_right.vertices[blend].co)*.5).length < 1e-5
```

- [x] GREEN: build bounded native branches, Sample Index transfers and one final
  Set Position; reject duplicate layer/mask names, unknown styles and nonfinite
  or out-of-range values before creating graph data. Expose real layer controls.
- [x] Regression: original 17 tests; missing masks, reversed layer order,
  unchanged input, overlap bounds and separate attributes on actual output.
- [x] Capture: clay and material mixtures, one connected mesh, identical geometry
  across the pair. Record recipe, full mesh digest, dependencies and point/face IDs.

## 2. Distinct surface morphology families

Files: surface_family_nodes.py and its family definitions; reuse the mixer and
gallery harness rather than create another supervisor/protocol.

- [x] RED per morphology: ensure changing a family/control changes real positions
  without changing selected topology or moving protected points; reject aliases
  whose sole difference is material/color. Evaluate at least two seeds.
- [x] Implement the designed strata/block/cliff, weathered/fractured, column-end,
  karst, slate, lava, flowstone and ice surface fields with metric scales and
  explicit primary/secondary forms. Do not call surface column ends full columns.
- [x] Capture every experimental surface form, then mixed strata/karst/ice on one
  continuous support. Validate all masks and source lineage on the returned mesh.

## 3. Solid and attachment coverage

- [x] Exercise and improve closed boulder/column routes against real silhouette,
  components, interior/cut faces, volume and source-envelope requirements.
- [ ] Build distinct native profiles for stalactites, stalagmites, flowstone,
  icicles/frozen falls; explicit anchors and clearance. Talus remains opt-in.
- [ ] Demonstrate topology-changing composition with new-face provenance and
  actual joined/contact behavior; do not reuse index-based surface transfer after
  changing topology. Reject unsupported mixtures explicitly.
- [ ] Capture each remaining planned form with its correct route. Update a
  coverage matrix with evidence paths and accurate unimplemented/rejected states.

## 4. Acceptance and resource closure

- [x] Repeat seeded evaluation independently with hashes of source, recipe,
  geometry, UV, IDs, masks and implementation. Inspect original-resolution images.
- [x] Run focused regressions, inspect owned diff and list remaining native
  integration obligations. Preserve failed evidence and report limits honestly.
- [x] Record file hashes, consumed children/time and zero remaining children for
  the owner. Resource exhaustion requires renewal, not a false completion claim.
