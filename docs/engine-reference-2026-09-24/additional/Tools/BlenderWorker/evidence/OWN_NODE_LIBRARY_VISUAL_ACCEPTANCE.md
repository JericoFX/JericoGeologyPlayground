# Jerico Own Node Library — Visual Acceptance

## Superseding review — 2026-09-08

**Visual quality: REJECTED by the owner.** The base rocks do not meet the purchased
addon reference target. Historical PASS cells below record the earlier capture
review only; they do not constitute current Beauty acceptance. File integrity,
image counts and protocol checks remain separate from visual quality.

The old manifest's `graphDigest` contains `schemaHash`, not a digest of graph
connections/defaults. It must not be consumed as graph provenance. Likewise,
`keepClearValidated` with an empty protected-volume list is vacuous. New v2
captures retain the accurate field name `schemaHash` and do not claim a keep-clear
visual proof without a populated fixture.

The approved correction is documented in `../ROCK_V2_PLAN.md`. Paid references
remain private. An original shader preview is not a high-to-low bake or runtime
material proof.

**Run:** `node-library-visual-20260908-1430-v3`  
**Status:** PARTIAL — base generator/pass/view matrix passes; bake, LOD and
non-vacuous keep-clear variants remain integration evidence, not silently waived.  
**Runtime:** Blender 5.2.1 LTS, factory startup, background, offline mode,
auto-exec disabled, two threads, one process.  
**Capture:** 84 original PNG files at 1024×1024: seven generators × four passes ×
three fixed orthographic views.  
**Elapsed:** 42,735 ms. Blender process count after exit: zero.

## Accepted base matrix

Every row was inspected at original resolution in front, profile and
three-quarter views. `beauty` and `unlit` preserve silhouette inspection;
`wireframe` exposes real evaluated triangles through Blender's native Wireframe
shader in Eevee; `masks` visualizes up to three transported `_JRC_MASK_*` point or
corner attributes as RGB. The manifest binds every capture to its source result,
seed, job key, schema/graph identity, geometry hash, semantic hash, LOD and image
SHA-256.

| Generator | Beauty | Unlit | Wireframe | Masks visualized |
| --- | --- | --- | --- | --- |
| `sedimentary_strata` | PASS | PASS | PASS | `_JRC_MASK_HARD_LAYER`, `_JRC_MASK_STRATA_CLASS`, `_JRC_MASK_WEAK_LAYER` |
| `fractured_boulder` | PASS | PASS | PASS | `_JRC_MASK_DURABILITY`, `_JRC_MASK_FRESH_BREAK` |
| `columnar` | PASS | PASS | PASS | `_JRC_MASK_COLUMN_END`, `_JRC_MASK_DURABILITY`, `_JRC_MASK_SEGMENT` |
| `rounded_weathered` | PASS | PASS | PASS | `_JRC_MASK_CONTACT`, `_JRC_MASK_DURABILITY`, `_JRC_MASK_WEATHERED` |
| `cave_cliff_shell` | PASS | PASS | PASS | `_JRC_MASK_DURABILITY`, `_JRC_MASK_SHELF` |
| `destructive_spline` | PASS | PASS | PASS | `_JRC_MASK_DURABILITY`, `_JRC_MASK_FRESH_BREAK` |
| `decal3d` | PASS | PASS | PASS | `_JRC_MASK_DAMAGE`, `_JRC_MASK_FRESH_BREAK` |

The authoritative run is retained at
`evidence/node-library-visual-20260908-1430-v3/`. Its `manifest.json` is validated
by `visual_manifest.validate_manifest`; the original images are under `images/`.
No contact sheet or resized derivative substitutes for those originals.
For quick human review, the same run also contains the explicitly derivative
`beauty-overview.png`, `wireframe-overview.png` and `masks-overview.png`; these are
not included in the 84-capture acceptance count.

## Rejected attempts retained for provenance

- `node-library-visual-20260908-1430`: rejected after original-resolution review
  because Workbench rendered the requested wireframe pass as a solid silhouette.
  The other passes were not promoted independently.
- `node-library-visual-20260908-1430-v2`: aborted fail-closed because the runtime's
  registered Eevee engine identifier is `BLENDER_EEVEE`, not
  `BLENDER_EEVEE_NEXT`. It contains only six incomplete images and no manifest.
- `node-library-visual-20260908-1430-v3`: corrected complete run. Wireframes show
  the actual triangulation in all seven generators.

## Criteria not yet admitted

These omissions keep overall visual acceptance at PARTIAL even though the 84-image
base matrix passes:

- the BSP fixtures used here declare `protected: []`; therefore
  `keepClearValidated=true` is correct but vacuous. A fixture with at least one
  protected gameplay volume must visually and numerically prove no penetration;
- LOD0 is captured; LOD1 side-by-side silhouette and semantic-mask preservation is
  not captured;
- deterministic replay and changed-seed identities passed machine tests, but their
  paired visual captures are not in this run;
- high-to-low normal, AO and material-mask bake images are not present;
- these captures prove the private Blender candidate output only. They do not prove
  native broker lifecycle, DDC, Editor Apply/Undo, cook/runtime consumption or
  source-free Shipping acceptance.

The missing variants belong to the next explicitly coordinated Blender window.
They must extend the manifest rather than overwrite this run.
