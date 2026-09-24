# Purchased addon reuse — current private-backend direction

Date: 2026-09-08. Scope: `Tools/BlenderWorker/**` only.

## User authority and ownership

Latest explicit user direction, preserved verbatim:

> Tu usa código de los addons, los compre y me pertenecen el engine no va a ser open source

This supersedes the earlier local preference for independently authored replacements
and a future open-source release. Reuse purchased installed addon code and node
implementations where suitable for the private Blender backend. Do not sacrifice
quality to claim independent authorship. Do not call reused third-party work original
Jerico code. Preserve original names, copyright notices, licenses and source hashes.

The main owner acknowledged this change on 2026-09-08. Its explicit boundary is:
private inspection and folder-local tests are authorized; shared authorities still
describe independence and must be reconciled before dependency integration. This
document neither changes the Persistent Goal nor authorizes Engine/Editor changes,
Git publication, vendor-code redistribution or a new native broker/scheduler.

## Concrete installed-component audit

Source root: `F:/JericoEngine/_local_reference/blender_addons/`. Findings concern
the inspected local versions, not every version or the user's purchase receipt.

| Component | Code evidence | Technical reuse value | Remaining admission boundary |
| --- | --- | --- | --- |
| RockForm 1.4.1 | `rockform_1.4.1/rockform/__init__.py:1` GPL-3.0-or-later; `blender_manifest.toml` declares GPL-2.0-or-later | `SNA_OT_Create_Lowpoly_Dbd1C`, lines 219–257, duplicates high mesh, applies visible modifiers, adds Collapse Decimate at approximately 0.1 | Preserve both declarations; their version ranges overlap at GPLv3+, but do not silently choose a distribution policy. Confirm package-specific terms and asset coverage before packaging |
| OCD 2.5.2 | `ocd_2/__init__.py:1` identifies VFXGuide and GPL-3.0-or-later | Lines 233–285 load original groups from `data.blend`; lines 514–552 invoke Geometry Nodes bake | Python header does not independently establish the license of `data.blend`; GN bake is not high-to-low normal baking |
| Edge Decal Generator 1.2.1 | Manifest and inspected Blender feature modules declare GPL-2.0-or-later | `features/uv_processing.py:1642` onward performs source-polygon/barycentric loop UV transfer; generated decal modifiers include shrinkwrap, weld, bevel and decimation | Audit assets and remote helper separately; do not merge Blender modules into proprietary native Engine code |
| Smart Segmentation 1.0 | No explicit license found in inspected Python/package files | Temporary decimated proxy for analysis, then original-face segmentation | Keep installed execution/reference separate from copied code until component reuse terms are verified |
| The Nature Generator 1.2 | Original installed `Nature_Generator_Assets.blend` already evaluated privately | Fractured formation preset provides the strongest inspected structural visual reference | Preserve original graph identity; exact purchased asset/distribution terms are not established by another addon's GPL header |

These findings identify notices and implementation behavior, not a legal opinion.
Closed-source intent does not erase third-party notices or asset-specific terms.
Conversely, a GPL notice is not a blanket prohibition on private local use. Keep
installation/execution, code modification, and distribution as separate decisions.

## Reuse architecture

1. Keep original installed packages and BLEND assets in the private intake area.
   Do not commit purchases, textures, original libraries or unreviewed dependencies.
2. Inspect interfaces in an isolated, offline Blender worker. Loading a BLEND with
   auto-execution disabled does not make explicitly imported addon Python safe;
   registration executes trusted code and requires component admission.
3. Record exact package version, file digest, declared author/license and runtime
   pin. An adapter must bind real RNA properties, operator parameters or node-group
   socket identifiers; a renamed generator label is not an implementation.
4. Expose the adapter's discovered schema through Jerico's existing backend
   catalogue. JericoEditor remains the only front; do not require Blender UI or
   create Qt controls in this pre-M10.5 task. Unsupported/context-dependent controls
   must be explicit, not silently ignored.
5. Run original graph evaluation behind the existing job/recipe route only after
   source mapping, selection, determinism, masks, cancellation and keep-clear
   gates pass. A private preset render is not production generator admission.
6. Return evaluated products and provenance through existing result contracts.
   Do not ship the Blender runtime, Python package or purchased source libraries
   inside source-free runtime packages by default. Asset-output rights still need
   package-specific verification.

## LOD finding and required fidelity gate

Of the inspected Python implementations, only RockForm constructs a genuine low
mesh from a retained high object. Its fixed-ratio Decimate is not an automatic
guarantee of high-mesh appearance. OCD's GN cache bake, Smart Segmentation's proxy
reduction and Edge Decal's evaluated export are not high-to-low normal baking.

Use a source-derived low mesh, preserve silhouette and semantic/material boundaries,
unwrap the low mesh, and evaluate Blender Cycles selected-to-active normal baking
with an explicit cage/ray-distance policy. Validate surface distance, projected
silhouette, shading, seams, thin strata and keep-clear at every LOD. Lower polygon
count cannot preserve identical topology; the user's requirement is visual and
spatial fidelity, not a false claim of byte-identical high and low geometry.

### Native owner contract response — received 2026-09-08

The following paths and obligations were supplied by Astra's read-only audit;
they are existing contracts to preserve, not a newly admitted native interface.

- `docs/PROCEDURAL_PWAS_EPLS_MASTER.md:5` and `:591`: PWAS owns placement and
  distribution. EPLS (Environmental Procedural Layer System) owns environmental
  surface state; it can request PWAS objects but does not modify topology.
- `PwasRecipeGraph.hpp:338`, `PwasLevelBake.cpp:400`,
  `EditorAutomationBridge::bakePwas`: preserve GUID/revision/hash/seed, ordered
  nodes/layers, masks/dependencies, placement keys/generated GUIDs, locks/removes
  and dirty regions.
- `Tools/Epls/Include/Jerico/Epls/EplsRecipe.hpp:16`, `EplsMask.hpp:18`,
  `EplsCooker.cpp:232`: preserve layer/stroke GUID, semantic/scope, ordered
  include/exclude, seed/overrides, hashes, surface/page owner and material GUID.
  The established cook output is `.rtext`.
- Under `Tools/ProceduralAuthoring/Include/Jerico/ProceduralAuthoring/Geology`,
  `GeologyRecipe.hpp:55`, `GeologyLayer.hpp:586`, `GeologyMask.hpp:16`,
  `GeologySurface.hpp:14`: retain classification/strata, fracture/erosion/deposition/
  cavity/exposure, region/source/layer/material/physical IDs and protected bounds.
- `docs/JERICO_PHYSICAL_MATERIAL_LOCALIZED_DESTRUCTION.md:23`,
  `ProjectPhysicalMaterialResolver.cpp:9` and `CollisionAsset.hpp:37`: slot GUIDs
  alone are not resolved physical definitions, hashes or structural values.
  Bonds, anchors, chunk topology and prepared-set/mesh/collision identity remain
  `INTEGRATION REQUIRED`.

Concrete current gaps reported by Astra: `output.py:118` stamps only the last
layer slot per object, so it does not establish spatial multi-layer provenance.
Its `0.5^LOD` reduction ratios do not prove surface/silhouette error or high-to-low
bake fidelity. Preserve per-LOD provenance, regions/seams/semantics,
collision-source-LOD, bake profile/cage/target and texture hashes. Do not invent
numerical acceptance caps or create a parallel simplifier. GLB remains transport
with `publishable:false`; native validators, DDC, RuntimeFormats and packaging
retain authority. `BACKEND_COMPLETENESS_AUDIT.md` already records these boundaries.

## Current evidence, without overstating completion

- Original Nature Generator fractured preset was evaluated with its original
  nodes in the previous coordinated window: 17,987 evaluated vertices. Private
  evidence: `_local/private-nature-fractured-20260908-1836/`.
- This was a normalized preset comparison, not the same BSP input. No original
  node groups were exported or presented as Jerico-authored.
- Own v2 rock Beauty remains rejected; technical PASS does not reverse that.
- `cellular_fracture`, `block_strata`, `karst_cave_wall` and `glacial_ice_wall`
  currently expose candidate metadata, not executable completed families.
  `require_implementation` rejects them with `plugin.headless_unsupported` so
  discovery cannot silently become a successful no-op job.
- This policy update copies no addon source and makes no claim that all purchased
  addons are already adapted. The next implementation decision uses the audited
  original package and its actual interface, preserving attribution.
