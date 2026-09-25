# M8 F4 authored material on the retained dense rock

Date: 2026-09-05. Baseline: `38b390f8a90c538549edc73f6d422d47a41ff26c`.
Active slice remains `M8-F4-GEOLOGY-STRUCTURAL-AUTHORING-AND-ROCK-FAMILY-BEAUTY`.
Status: material/texture preservation and native replay PASS; Beauty remains RED.

## Inputs and observable contract

Use the immutable meshes, collisions and recipes from
`build/windows-vulkan-editor/f4-strata-relief-witness-01/`. Geometry generation is
not repeated or changed for a material comparison. The export verifies recipe,
mesh/collision identity and source/recipe correspondence, and requires the eight
cooked geometry/collision payloads to survive byte for byte.

The already integrated source material is Poly Haven `rock_surface` (CC0), selected
from `build/geology-material-integration/shipping-20260828-polyhaven-final/packages/geology-material.epak`.
Material GUID: `0DDA2941DDA80EE8C246DA971E0AEBF7`. The retained source hashes,
official asset/download URLs and licence are recorded in
[the original material integration evidence](M8_GEOLOGY_SYSTEM_GEOLOGICAL_MATERIAL_INTEGRATION_CLOSURE.md#poly-haven-provenance).
No source images are modified, redownloaded or added to Shipping.

The source package already contains cooked base color, normal and ORM textures.
The new fixture export creates an explicitly named material instance under the
existing rock-family palette GUID; the original selected material remains intact.
PBR settings, texture references, source/recipe hashes and cooked texture
payloads must survive. Package provenance records both material identities and
the original provenance. This is instance creation, not byte identity of the
whole `.rmat` (its GUID and display name intentionally differ).

## RED and implementation boundary

`f4-authored-material-red-build.log`: exit0/13.556s.
`f4-authored-material-red.log`: exit1/1.320s. Calling the actual old exporter with
the authored material lost its PBR values and texture references and omitted all
three original texture identities. Its diagnostic behavior remains available;
the new authored path is explicit.

Initial preservation GREEN: build13.495s, export1.416s, existing export contract95ms,
all exit0. Logs: `f4-authored-material-green-build.log`,
`f4-authored-material-green.log`, `f4-authored-material-export-contract.log`.
The first native capture from `f4-authored-material-replay-01/Packages/candidates.epak`
then failed: `render.asset.pipeline_load_failed`, because the retained material
archive lacks the current `Surface/PBR_Geology` pipeline. No image from that attempt
is accepted. The material instance must therefore use the current canonical cooked
shader profile, while retaining authored PBR/texture inputs. Preserving an obsolete
archive byte for byte is not correct runtime compatibility.

Admission review identified four missing negative cases (terrain, emission,
unsupported texture semantics and incompatible sampler settings). They failed in
`f4-authored-material-admission-red.log` (exit1/165ms, build13.056s). The material
check now matches the existing renderer's single-palette PBR restrictions.

Final archive-compatible build `f4-authored-material-final-build.log`:
exit0/13.014s. Export `f4-authored-material-final.log`: exit0/1.506s.
Admission/publisher contract `f4-authored-material-admission-green.log`:
exit0/90ms. Fresh export `f4-authored-material-repeat.log`: exit0/1.648s;
package SHA256 is identical. The original diagnostic 12-entry assertion remains.
No GPU regeneration or old rock-family generator runs in this material-only path.
Final guard verification: `f4-authored-material-verify-build.log` exit0/12.453s,
`f4-authored-material-admission-verify.log` exit0/126ms and
`f4-authored-material-verify.log` exit0/1.613s. The verified output is byte-identical
to the package used by both native captures. All logs are under
`build/windows-vulkan-editor/`. A corrected comparison path is explicitly retained
in the last log; the two final SHA256 values agree.

Input material package SHA256:
`E3021F892D605A213FE22942FB7FE0446B5065B837F55151E5A109CB8E79F048`.
The current canonical profile is
`build/windows-vulkan-editor/QtRuntime/JericoEditor/Debug/AuthoringProfiles/Surface-PBR_Opaque.rmat`,
SHA256 `652CE28401B519479EACA47775C462A3990D2A9A8BAC36357E73AD5D3900D74D`.
The instance uses its archive, archive content version and cook schema, while
preserving the selected authored material's PBR, source/recipe hashes and texture
references. Readers recompute/validate the resulting cooked identities.

Reproduction command (all destinations must be new):

```text
JericoGeologyRockFamilyTests --export-retained-material MATERIAL-PACKAGE MATERIAL-GUID PIPELINE.rmat RETAINED-directory NEW-directory
```

Use the input package/GUID above, the canonical profile and
`build/windows-vulkan-editor/f4-strata-relief-witness-01` as the retained directory.

## Original native views

Output: `build/windows-vulkan-editor/f4-authored-material-witness-02/candidates.epak`,
24009706 bytes,12 entries, SHA256
`3671C9CB17FC357585800BAE234F4417F5B9A9C09D4CB1F4E2A0EE65F3EA9D82`.
Both captures replay only its copy in
`build/windows-vulkan-editor/f4-authored-material-replay-02/Packages/`.
`front-capture.log` and `side-capture.log` exit0: Vulkan, `source_free=true`,
`fallbacks=0`,1 material/3 textures,1760 vertices/8520 indices.
The scene, camera and lighting match the neutral Strata witness:1280x720,
scale1, worldY-1.4, yaw25/135, cameraZ5, pitch-15, verticalFOV40.
Capture GPU timing is unavailable; this RTX3090 run is not a hardware budget.

- [Front](images/m8-f4-authored-material-witness-02/member-1-front.bmp), SHA256
  `6A966A4D11AE467E1794AC8BAF13615438D73D21694B6A7CA13EA9E74B512986`.
- [Side](images/m8-f4-authored-material-witness-02/member-1-side.bmp), SHA256
  `3F923730E9E3FE406E21948F4B8FB607637FE83311EACC1C33AA7055CFB266BF`.

Both originals were inspected at native resolution. Fine surface detail and
material shading are now visible, with the same silhouette and geometric bands.
The chosen single palette is dark brown; the overall form still reads as overly
regular stacked bands with broad cut facets. Texture preservation is demonstrated,
but this is not accepted natural-rock Beauty or the requested lighting target.

The retained-material fixture admits complete packages, opaque single-sided PBR
materials with height mapping off and self-contained cooked textures. It uses
existing material/texture/package readers and writers and the exclusive directory
publisher. Referenced missing, wrong-kind or externally dependent textures do not
silently fall back. Paged texture material export remains on its existing package
path. Inputs are read-only and destination replacement remains forbidden.

## End-to-end boundary

Recipe/source geometry, classifications, structural fields and exact collision
are retained, not regenerated or relabelled. The material instance and all texture
dependencies enter one cooked package; native source-free replay must use it alone.
The engine's existing geological material permutation and triplanar texture
mapping are reused. No engine shader, renderer family, binary format, dependency,
editor publication contract, physics/gameplay, animation, scripting, event,
Sequencer, audio or particle interface changes.

INTEGRATION REQUIRED in F4: authoring more than one geological palette material
and response routes in the editor, actual rock-family Beauty, remaining structural
controls and displaced-source regions, LOD/hardware budgets and localized painting.
A single-material comparison does not close those integrations or establish the
requested CryEngine lighting target. Keep the matched neutral views as separate
structural evidence so texture detail cannot substitute for geometry acceptance.

Persistent Goal: ACTIVE until full JericoEngine 1.0 closure.
