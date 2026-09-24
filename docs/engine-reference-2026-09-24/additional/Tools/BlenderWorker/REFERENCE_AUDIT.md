# Local Blender references — read-only capability study

Current precedence: [backend completeness audit](BACKEND_COMPLETENESS_AUDIT.md).
Complete, integrate and test the nonvisual backend before the post-M9 M8 focus;
then select concrete addon capabilities and adapt GeneratorSchema/bindings.
Qt front belongs to M10.5. This inventory is historical reference research, not
production admission or a direction to prioritize further addon controls now.

Only the nine Markdown files in [PLAN.md](PLAN.md) enter this documentary
publication. Reference addons, code, JSON, binaries, `evidence/`, `_local/` and
generated reports are local paths excluded from it.

Dates: initial inventory 2026-09-07; expanded Python/control study 2026-09-08.
Root: `F:/JericoEngine/_local_reference/blender_addons/`.
Scope: package metadata, selected Python declarations, control semantics,
generation/selection/UV boundaries and downstream data.
Status: **PARTIAL study**, not addon conversion, dependency admission or generator
equivalence. The user asked explicitly how this folder will inform Jerico.

The offline worker was independently written from the owner directive and native
Blender APIs before this inventory. This inspection is performed by the task, not
by the worker. The worker does not read this tree. No reference addon was imported,
registered, executed, installed, modified or copied into the package. No reference
`.blend` was deserialized in the initial inventory. The subsequent authorized
metadata inspection, described below and in `TRANSFORMER.md`, read the four binary
libraries without executing addons or evaluating their graphs. Full node behavior
and complete asset licensing remain unexamined; the mere presence of a package is
not evidence of algorithm equivalence.

| Reference found | Evidence read | Useful concept to investigate for Jerico | Boundary/next action |
| --- | --- | --- | --- |
| RockForm 1.4.1 | `rockform_1.4.1/rockform/blender_manifest.toml`; `__init__.py` lines 1–24, 54, 196–211, 233–253, 444–451 | Separation of shape formation, material/displacement and low-poly preparation; explicit high-to-low workflow | Manifest says GPL-2.0-or-later, Python header says GPL-3-or-later: preserve discrepancy for admission. Operator route requires VIEW_3D/INVOKE_DEFAULT and external `.blend`; extract behavior requirements, then implement Jerico-owned graphs. Full socket/graph inspection still required. |
| Edge Decal Generator / Decal Gen 1.2.1 | `blender_manifest.toml`; `features/README.md`; file inventory | Edge-chain strips, width solving, concavity/AO filtering, UV density and versioned presets | Manifest states GPL-2.0-or-later. A non-interactive generation layer exists alongside viewport tools. Study algorithm contracts separately from UI; Jerico RBrush/decal, material and UV owners retain their interfaces. Existing Unreal exporter is not a Jerico adapter. |
| The Nature Generator | `The_Nature_Generator1.2/The_Nature_Generator/__init__.py` lines 8–16, 197–207, 247–254, 429–469, 837–862; asset-library path inventory | Distinguish scatter, ground assembly and water families; derive typed parameter groups from node interfaces | Folder says 1.2; Python metadata says 1.0.0 with Blender minimum 4.1. No license was established from the inspected files. Its library/VIEW_3D front is not a runtime dependency. Map static candidate fields to PWAS/EPLS and existing material/water owners; do not absorb those systems into this geology worker. |
| OCD 2 / One Click Damage 2.5.2 | `ocd_2/__init__.py` lines 1–25, 233–236, 307–310, 360–363, 1202–1265; `data.blend` inventory | Separate authored damage controls and modifier application from resulting geometric/structural candidates | Header states GPL-3-or-later; minimum Blender 5.2. UI/GPU imports, interactive transforms and external node data require an independently designed data-only seam. Jerico destruction owns physical bonds/anchors and runtime behavior. No damage algorithm equivalence claimed. |
| Smart Segmentation 1.0 | `Smart Segmentation v1.0/Smart_Segmentation.py` lines 1–13, 38–48; declaration search near 399–407, 443, 551, 717 | Thickness/concavity-based part segmentation, proxy quality versus source detail, deterministic region identity and designer corrections | No license established in inspected header. Interactive brush/lasso/box corrections belong in the editor phase. Study the segmentation problem and output semantics; do not transplant NumPy/BVH implementation into Engine or pretend connected-component labels are equivalent segmentation. |

## Translation into Jerico requirements

1. Record observable behavior, exact input/output types, native-node capability,
   quality controls and failure modes of one bounded technique at a time.
2. Keep addon-specific Python, operator names, UI state, node groups, `.blend`
   assets and licensing boundaries in the reference tree.
3. Specify an independent Jerico recipe/schema and own native Geometry Nodes;
   preserve source/layer/material/physical/structural identity through evaluation.
4. Prove shape/semantics and determinism on original fixtures, including no access
   to this reference tree during worker execution.
5. Integrate later through the existing Editor, canonical cook and runtime owners.

This inventory does not expand the writable scope or open UI work. The remaining
technique/graph studies and richer generator behavior are explicitly pending.

## Expanded capability/control study — 2026-09-08

**Actualización posterior:** la instrucción directa amplió el alcance a código.
El [transformador](TRANSFORMER.md) implementa intake, metadatos, correspondencias
y una primera operación propia. Las cuatro bibliotecas ya fueron inspeccionadas
a nivel de interfaz/inventario; [BLEND_LINKS.md](BLEND_LINKS.md) registra la unión
con Python. Los párrafos siguientes conservan el alcance del estudio documental
anterior; no describen como pendientes las interfaces que ahora están leídas.

The owner clarified that controls for future UI integration belong in a Markdown
guide, not a newly implemented metadata/framework layer. The result is
[`UI_CONTROLS.md`](UI_CONTROLS.md): all 32 existing artistic parameters, their real
effects and dependencies, separate pending production controls, lifecycle mapping
and a repeatable extension template. No worker algorithm, dependency or UI changed
as part of that documentation contribution.

The following facts were inspected in Python source. Numerical values below
describe **the reference addon**, not admitted/default Jerico controls.

| Reference | Evidence and established behavior | Requirement for our independent suite |
| --- | --- | --- |
| RockForm | `rockform_1.4.1/rockform/__init__.py`: guide transform/path/extrude/bevel controls around 740–780; volume/face/vertex spawn choices 806–808; procedural/image displacement choices 847–852; low-poly decimation 1101–1137; image blending, displacement strength/scale/rotation and projection 1390–1668; voxel/density/decimation near 1754–1779. Many underlying defaults reside in uninspected node libraries. | Keep source/placement, form, surface fields, quality and LOD as distinct responsibilities. Use metric data and explicit selections instead of viewport object names. Do not copy its UI controls as operational Jerico knobs without implementing the corresponding behavior. |
| OCD | `ocd_2/__init__.py`: native 5.2 input-property access and redraw boundary 80–104; named HERO field/mask utilities 61–63; per-object noise seed/randomization 487–510; GN bake lifecycle 514–592; painted mask/image workflow 630–808. Other `Socket_*` labels/defaults live in `data.blend`, not established by numeric identifiers in Python. | Separate damage operation, selection/masks and bake lifecycle; use deterministic recipe streams instead of global randomization. Painted inputs belong to the existing RBrush/material owner. Do not infer damage parameters from unexplained socket numbers. |
| Nature Generator | `The Nature Generator/The_Nature_Generator1.2/The_Nature_Generator/__init__.py`: typed family tags 197–207; panel/socket traversal 247 onward and 429–469; scatter and water modifier setup 837–907. Group input identifiers/types and panel order are read dynamically; some shader paths use older APIs. | Record every operational control and its actual binding/rebuild requirement; avoid hand-maintained hidden controls. Scatter/ground/water remain owned by PWAS/EPLS/material/water systems. UI traversal is a design reference, not evidence of its binary node algorithms. |
| Edge Decal Generator | `features/core_state.py:3093–3145`: relative width (0.01 of largest source dimension when enabled), metric surface offset (0.002), seed, amount (0..1), maximum length with zero disabling it, and endpoint taper. Further declarations 3163–3320 expose concavity/AO/width/overlap/path controls. `generation.py:531–690` exposes UV handling/density/strip placement. `geometry.py:1988–2022` handles selected-edge ribbons and junctions; 4824–5040 handles filtering/chain inclusion. | A future edge-damage operation must distinguish relative versus metric widths, topology selection, junction treatment and UV outcomes. Reuse the existing Decal/RBrush and geometry pipeline where possible. Require concrete chain/filter and measured density tests before adopting artistic ranges. |
| Edge Decal provenance and layers | `features/core_state.py:1872–1904,3627–3645`: source identity/topology signature and rejection of stale edge ordering. `texture_masks.py:196–261`: UV-space mask sampling/boundary refinement. `layers.py:1469–1580`: subtracting layer footprints while retaining components/weights. `presets.py:12–24,407–545`: explicit preset schema and settings/material references. | Stable Jerico source/selection identity must survive evaluation; stale topology cannot reuse naked edge indices. Specify mask domain and ordering. A preset is a concrete versioned set of values, not another algorithm or an implicit asset dependency. |
| Smart Segmentation | `Smart Segmentation v1.0/Smart_Segmentation.py:548` explicitly names **Shape Diameter Function**: thickness analysis, not the signed-distance grid used by our SDF booleans. Lines 20–52 fix analysis budgets and in-memory cache; 1259–1326 expose parts (8 default, 2..200), boundary refinement, manual merge tools, explode and output policy. Preview/corner colors 1133–1174; split/provenance paths 1659–1660,1901–1920,3057–3093; cache invalidation 3493–3514. | Separate analysis labels on the original mesh, preview state, authored corrections and split candidates. Connected components alone do not implement this segmentation capability. Preserve identity/data for material/destruction/LOD consumers, and make source-topology invalidation explicit. Do not mistake explode preview or hide-source UI policy for runtime behavior. |

Edge Decal and Smart Segmentation inspection included a focused read-only Luna
auxiliary; Astra reviewed the findings and checked selected control declarations.
No addon was imported/executed and no binary node library was opened. Numeric
limits hidden in RockForm/OCD/Nature `.blend` data, full node algorithms, Unreal
export, complete lifecycle/keymaps and all modal UV-pin interactions remain
uninspected. No headless worker adapter was established for those addon workflows.

The useful integration boundary is data: explicit input selection/settings/seed,
evaluated geometry and stable semantic outputs, followed by Jerico validation and
transactions. Viewport picking, temporary colors, modal tools and process-local
analysis caches are not durable worker recipe fields. Each future capability must
fill the template in `UI_CONTROLS.md` and earn its own RED/acceptance evidence.
