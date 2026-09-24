# Jerico-owned Geometry Nodes Library Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use
> `superpowers:executing-plans` to implement this plan task-by-task in the current
> folder owner. Subagent execution is excluded by the repository's single-writer
> policy unless the user explicitly changes it.

**Goal:** Convert the existing original Jerico Blender generators into a
versioned, composable node-group library whose complete control contract is emitted
as deterministic `GeneratorSchema` data for the M10.5 Qt front.

**Architecture:** Keep the established headless worker and seven generators.
Introduce one pure-Python registry as the single source of generator, group,
control, panel, unit, dependency and composition metadata. Blender builders consume
that registry and emit Jerico-owned node groups; the transformer catalogue and
runtime schema consume the same registry so M10.5 never reverse-engineers Python or
BLEND files. Native job lifecycle, DDC, transactions and Qt remain external owners.

**Tech Stack:** Python 3, Blender 5.2.1 `bpy`/Geometry Nodes, `unittest`, canonical
JSON, the existing Jerico Blender worker protocol.

**Spec:** `Tools/BlenderWorker/PRIVATE_REFERENCE_TECHNIQUE_AUDIT.md`

## Execution checkpoint — 2026-09-08

Tasks 1, 2 and 5 are implemented. Task 3 has all eight Jerico-owned reusable
datablocks and integrates the owned noise/write-mask groups into all seven
generators; extracting every remaining logical composition stage into a nested
group is still open. Focused Blender tests pass 4/4, the seven-generator matrix
passes 7/7, ten clean deterministic replays match and a changed seed differs.
The CPU-light regression suite passes 123/123 before the visual-manifest tests.

Task 6 now has a fail-closed manifest validator and an inspected 84-image
1024×1024 base matrix. The first run was rejected for solid wireframe silhouettes;
the corrected `node-library-visual-20260908-1430-v3` run uses native Eevee
wireframes and passes every generator/pass/view cell. Overall Task 6 remains
PARTIAL because protected-volume, LOD1, seed-pair and high-to-low bake variants
are not yet captured. See `evidence/OWN_NODE_LIBRARY_VISUAL_ACCEPTANCE.md`.

## Global Constraints

- Write only under `Tools/BlenderWorker/**`; no Engine, Editor, shared docs or Git mutation.
- Addons, their code, node groups and assets are private study material only; production uses Jerico-owned code and IDs.
- Keep Blender/GPU/build/heavy execution closed until the active M9 owner opens it explicitly.
- Preserve `VerifyBlenderJobLifecycleAndCancellation` as the first shared native integration RED; do not create a scheduler, DDC or journal here.
- Qt widgets remain M10.5 work; this plan emits only data contracts and binding documentation.
- Every graph is headless, offline, deterministic and source-free for Shipping.
- Selection, BSP allowed/protected regions, keep-clear and provenance remain authoritative inputs and outputs.
- No production code is written before its focused test has failed for the expected missing behavior.

---

### Task 1: Single source of truth for own generators and controls

**Files:**

- Create: `Tools/BlenderWorker/node_library.py`
- Modify: `Tools/BlenderWorker/generators.py`
- Modify: `Tools/BlenderWorker/transformer/presentation.py`
- Modify: `Tools/BlenderWorker/transformer/suite.py`
- Test: `Tools/BlenderWorker/tests/test_node_library.py`

**Interfaces:**

- Consumes: current `GROUPS`, `PARAMETERS`, `QUALITY`, parameter validation and
  `control_contract` output shape.
- Produces: `generator_definition(generator_id) -> dict`,
  `generator_definitions() -> list[dict]`, `control_definition(generator_id,
  socket_name) -> dict`, and the unchanged `PARAMETERS` compatibility view.

- [ ] **Step 1: Write the failing registry test**

```python
def test_every_generator_control_is_complete_and_front_bindable(self):
    for generator in self.library.generator_definitions():
        self.assertRegex(generator["id"], r"^jerico\.[a-z0-9_.-]+/v1$")
        self.assertTrue(generator["nodeGroup"].startswith("JRC_GN_"))
        for control in generator["controls"]:
            for key in ("id", "socket", "label", "description", "type", "unit",
                        "default", "minimum", "maximum", "panel", "options",
                        "dependencies", "binding"):
                self.assertIn(key, control)
            self.assertEqual(control["binding"]["recipePath"],
                             "layers[].parameters." + control["id"])
```

- [ ] **Step 2: Run the test and verify RED**

Run:

```powershell
& $python -B -m unittest Tools.BlenderWorker.tests.test_node_library -v
```

Expected: FAIL because `node_library` and the complete registry do not exist.

- [ ] **Step 3: Implement immutable definitions**

Define the seven production generator entries with stable Jerico IDs, current
node-group datablock names, version `1.0.0`, panels, descriptions, units, ranges,
dependencies, geometry effect, masks and composition group IDs. Build `GROUPS` and
`PARAMETERS` from these definitions without changing validator behavior.

- [ ] **Step 4: Make transformer catalogue consume the registry**

Replace the duplicate inference loop in `catalogue()` with the exact control
definitions. Preserve existing operation catalogue behavior and its JSON shape.

- [ ] **Step 5: Run focused tests and verify GREEN**

Run:

```powershell
& $python -B -m unittest Tools.BlenderWorker.tests.test_node_library Tools.BlenderWorker.tests.test_transformer -v
```

Expected: all tests PASS; catalogue JSON is finite and every generator control has
one binding.

### Task 2: Deterministic GeneratorSchema and M10.5 view model

**Files:**

- Create: `Tools/BlenderWorker/generator_schema.py`
- Modify: `Tools/BlenderWorker/nodes.py`
- Modify: `Tools/BlenderWorker/worker.py`
- Test: `Tools/BlenderWorker/tests/test_generator_schema.py`

**Interfaces:**

- Consumes: `generator_definition`, Blender `tree.interface.items_tree`, runtime
  socket identifiers and canonical JSON utilities.
- Produces: `schema_from_definition(generator_id) -> dict` outside Blender and
  `schema_from_tree(generator_id, tree) -> dict` inside Blender. Both share
  `schemaVersion=1`, `generatorId`, `generatorVersion`, `nodeGroup`, ordered panels,
  typed controls, bindings, masks, constraints and composition.

- [ ] **Step 1: Write schema RED tests**

Assert exact stable ID, panel order, enum options, unit, dependency, recipe path,
socket name, rebuild policy and identical canonical hashes across ten clean Python
processes. Reject duplicate IDs, invalid defaults/ranges, unknown panels and a
runtime socket missing from the definition.

- [ ] **Step 2: Run schema tests and verify RED**

Expected: FAIL because `generator_schema` is absent.

- [ ] **Step 3: Implement pure schema generation and validation**

Generate M10.5 controls without importing `bpy`. Map types to Qt-neutral widgets:
`boolean -> check`, `integer -> spin`, `number -> scalar`, `vector3 -> vector3`,
`points -> spline-points`, `enum -> choice`. Keep widget policy advisory; the
authoritative fields remain type/range/unit/options/dependencies/binding.

- [ ] **Step 4: Reconcile runtime interface identifiers**

Change `nodes.schema` into a compatibility wrapper over `schema_from_tree` and pass
the generator ID from `worker.py`. Runtime reconciliation must fail closed on
missing/extra/type-mismatched sockets and store the actual Blender socket
`identifier` beside the stable recipe parameter ID.

- [ ] **Step 5: Run focused non-Blender tests and verify GREEN**

Run schema, protocol, transformer and worker tests that do not launch Blender.

### Task 3: Reusable Jerico node groups

**Files:**

- Create: `Tools/BlenderWorker/own_node_groups.py`
- Modify: `Tools/BlenderWorker/nodes.py`
- Test: `Tools/BlenderWorker/tests/test_own_node_groups.py`
- Test: `Tools/BlenderWorker/tests/test_worker.py`

**Interfaces:**

- Consumes: existing `Graph`, deterministic `sub_seed`, source geometry, Selection,
  protected volumes and the registry's composition IDs.
- Produces: `ensure_own_node_groups() -> dict[str, bpy.types.GeometryNodeTree]` and
  Jerico datablocks:
  `JRC_GN_Core_WriteMask_v1`, `JRC_GN_Mask_Direction_v1`,
  `JRC_GN_Mask_Gradient_v1`, `JRC_GN_Mask_Noise_v1`,
  `JRC_GN_Mask_Combine_v1`, `JRC_GN_Mapping_Triplanar_v1`,
  `JRC_GN_Detail_MultiscaleDisplacement_v1` and `JRC_GN_UV_BoxProject_v1`.

- [ ] **Step 1: Write Blender RED tests**

For every required group, assert exact tree type, stable name, socket set/types,
nonempty description, no `GeometryNodeIsViewport`, Viewer, Simulation, arbitrary
Object Info or private reference name, and expected native node families. Evaluate
fixture values for direction, gradient, noise independence, mask combination,
triplanar weight sum, selection write mask and UV attribute domain.

- [ ] **Step 2: Run under pinned Blender and verify RED**

This step waits for explicit Blender-window coordination. It must fail because the
own reusable group factory does not exist.

- [ ] **Step 3: Implement group factories from scratch**

Create interfaces and links exclusively through `bpy`. Reuse the factory by name
and version within one job; reject an existing mismatched datablock rather than
silently mutating it. Record a deterministic structural digest from interface,
node types, relevant properties and links.

- [ ] **Step 4: Integrate groups into the seven generators**

Use nested GeometryNodeGroup nodes for actual generation: multiscale noise and
direction masks in rocks/cliffs, write masks around displacement, triplanar/box UV
before export, and mask combine for layer outputs. Preserve existing geometry and
semantic contracts unless a dedicated behavior RED authorizes the change.

- [ ] **Step 5: Run Blender tests and verify GREEN**

Run the focused group tests, then all seven generator fixture tests and the ten
clean-process determinism gate.

### Task 4: Layer composition, Decal3D and destructive spline bindings

**Files:**

- Modify: `Tools/BlenderWorker/node_library.py`
- Modify: `Tools/BlenderWorker/generator_schema.py`
- Modify: `Tools/BlenderWorker/generators.py`
- Modify: `Tools/BlenderWorker/nodes.py`
- Test: `Tools/BlenderWorker/tests/test_node_library.py`
- Test: `Tools/BlenderWorker/tests/test_worker.py`

**Interfaces:**

- Consumes: ordered recipe layers, selection source IDs, protected volumes, masks
  and current `destructive_spline`/`decal3d` parameters.
- Produces: explicit `selection`, `spatialConstraints`, `inputMasks`, `outputMasks`,
  `affectedBounds` and `rebuildScope` schema sections for every layer.

- [ ] **Step 1: Write failing layer-binding tests**

Prove two ordered layers retain earlier masks; unselected sources remain identical;
Decal3D attribute-only changes semantics only; spline cut changes geometry and emits
fresh-break provenance; protected/keep-clear rejection survives both operations.

- [ ] **Step 2: Verify RED for missing schema sections**

Existing geometry tests may pass; the RED must be the absent explicit schema and
binding fields, not an unrelated Blender error.

- [ ] **Step 3: Implement layer/front bindings**

Expose mask pickers as attribute IDs, selection source as GUID lists, spline points
as an ordered point model, and affected bounds/diff as backend result data. M10.5
edits a draft recipe; Preview submits a new immutable job; Apply uses the native
transaction owner. No Qt signal or widget is implemented here.

- [ ] **Step 4: Verify focused and regression tests**

Run schema/catalogue tests CPU-only; run worker geometry tests only in the coordinated
Blender window.

### Task 5: M10.5 integration contract and source-free evidence

**Files:**

- Create: `Tools/BlenderWorker/M10_5_GENERATOR_CONTROLS.md`
- Create: `Tools/BlenderWorker/examples/generator-schema.json`
- Create: `Tools/BlenderWorker/examples/layered-geology-recipe.json`
- Modify: `Tools/BlenderWorker/README.md`
- Test: `Tools/BlenderWorker/tests/test_generator_schema.py`

**Interfaces:**

- Consumes: exact schema emitted by Task 2 and layer result protocol from Task 4.
- Produces: Qt-neutral control mapping, dependency evaluation, validation/error flow,
  Preview/Cancel/Apply event sequence and concrete JSON examples.

- [ ] **Step 1: Write documentation/example RED tests**

Parse both examples; validate them through production functions; assert every
documented field exists in the emitted schema and every generator is represented.

- [ ] **Step 2: Verify RED**

Expected: missing M10.5 contract and examples.

- [ ] **Step 3: Write exact consumer contract**

Document model roles, widget suggestions, units, dependency rules, error display,
stale-generation fencing, preview/diff, cancellation and Apply transaction handoff.
State that Qt never loads Blender, addons or node groups.

- [ ] **Step 4: Run final CPU and Blender gates**

Run all pure Python tests, `git diff --check`, private-name/source-free scan and,
when coordinated, pinned Blender group/generator tests. Record partial status if the
Blender window remains closed; never convert static checks into visual PASS.

### Task 6: Original-resolution visual acceptance

**Files:**

- Create: `Tools/BlenderWorker/evidence/visual-node-library/<run-id>/manifest.json`
- Create: `Tools/BlenderWorker/evidence/visual-node-library/<run-id>/*.png`
- Create: `Tools/BlenderWorker/evidence/OWN_NODE_LIBRARY_VISUAL_ACCEPTANCE.md`
- Test: `Tools/BlenderWorker/tests/test_visual_manifest.py`

**Interfaces:**

- Consumes: pinned Blender runtime, deterministic fixtures, final node-group
  digests, generated meshes, LODs, masks, bake outputs and keep-clear volumes.
- Produces: immutable original-resolution PNG evidence and a manifest binding every
  image to generator version, seed, camera, quality, mesh/semantic hash and graph
  structural digest.

- [ ] **Step 1: Write the visual-manifest RED**

Validate that each admitted generator has `UNSHADED`, `WIREFRAME`, `BEAUTY` and
`MASKS` captures from front, profile and three-quarter cameras; verify dimensions,
nonempty image payload, unique view IDs, seed, graph digest, geometry/semantic hash,
LOD identity and keep-clear result. Require high-to-low normal/AO/mask bake evidence
for generators that displace or fracture geometry.

- [ ] **Step 2: Verify RED before capture**

Expected: FAIL because no visual run manifest exists for the new library version.

- [ ] **Step 3: Capture deterministic original-resolution evidence**

After explicit Blender/GPU window coordination, render each family at the resolution
declared in the manifest. Use fixed cameras, lighting, world, color management and
seed. Retain an unshaded silhouette pass so a material cannot conceal thin sheets,
holes, intersections or poor mass; retain mask AOVs and wireframe separately.

- [ ] **Step 4: Compare correctness variants**

Capture seed replay, changed seed, protected/keep-clear fixture, LOD0/LOD1 and baked
low mesh. Fail on protected penetration, missing masks, LOD semantic loss, inverted
normal bake, sheet/fabric cliff reading, disconnected cave walls or a Beauty result
whose quality exists only in the shader.

- [ ] **Step 5: Validate and publish visual status**

Run the visual-manifest validator and record PASS/PARTIAL/FAIL per generator and view.
Do not claim the backend node library complete while any required image or visual
criterion is absent.
