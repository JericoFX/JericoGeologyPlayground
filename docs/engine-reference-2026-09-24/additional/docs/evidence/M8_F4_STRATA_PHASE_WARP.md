# M8 F4 Strata phase variation on dense working geometry

Date: 2026-09-05. Baseline: `0c1508bde2438212abc81942cbe4fe0506289928`.
Slice: `M8-F4-GEOLOGY-STRUCTURAL-AUTHORING-AND-ROCK-FAMILY-BEAUTY`.
State: bounded admission, generation, cook and native replay PASS;
Beauty remains RED and F4 PARTIAL.

## Reference and diagnosis

The original side/front images in `m8-f4-authored-material-witness-02` show
straight, repeated geometric bands around member1, together with broad exact-cut
facets. The material already contains real authored PBR textures. This increment
tests phase variation; it does not claim that phase alone repairs the full form.

The binding blueprint section15 defines optional low-frequency deformation of
the oriented bedding coordinate, followed by a periodic profile and a shared
structural mask. This is explicitly Jerico synthesis with R3/R8 research basis.
`GeologyStrataField.cpp` and the existing GPU Strata branch already implement it.
The working-surface region validator alone excluded nonzero phase warp.

The local reference `C:/Users/Jerico/Desktop/FULLTEXT01.pdf`, Anders Soderlund,
*Procedural Modeling of Rocks* (2015), is not evidence for geological bedding.
Its growth phases concern recursive sphere inflation. Printed p21/PDF p25 warns
of artificial ripples; printed p23/PDF p27 discusses coherent local movement;
printed p32/PDF p36 distinguishes contour-changing geometry from shaded fine
detail. Printed p11/PDF p15 discusses high frequency and sampling limits.
These constrain the visual judgment; no thesis algorithm/code is imported.

## Pointwise proof and retained limits

For a fixed world point p, the existing operation has
`F_p(d)=d+k(p)*max(1-|d|/B,0)`, with `k=groove-shape`.
Profiles are bounded, so `|k(p)| <= |shapeAmplitude|+|grooveAmplitude|`.
Phase warp changes the profile at p, not this bound. Inside the band the two
slopes with respect to d are `1+k/B` and `1-k/B`; outside, the slope is1.
The unchanged strict bound `|shape|+|groove| < B*(1-1e-5)` preserves ordering.
Thus the source/block maximum has the same winning operand after applying
the same F_p to both operands.

This is NOT spatial monotonicity or an analytic root/topology guarantee.
`grad(F)=(1-sign(d)*k/B)*grad(d)+(1-|d|/B)*grad(k)` can vanish or reverse.
The existing sampled-field contract remains: one interpolated crossing per
grid edge; unresolved analytic features are not reconstructed or certified.
No frequency-independent natural-form or resolution claim follows.

`projectWorkingSourceRegions` still proves block dominance over each complete
extracted triangle with the existing adaptive source/block Lipschitz bound,
depth4 and262144-probe cap. The phase profile cancels from that pointwise
comparison. Reaching native source regions still rejects; no source region is
invented or relabelled as a workaround. Sampled extraction, nonzero/finite
gradients, bounded QEF, closed positive volume, exact native cuts and output
capacities remain unchanged. Transformed/masked/partial/multiple relief layers
and relief after a cut remain unsupported in this route.

## Executable RED and verification

All logs below are under `build/windows-vulkan-editor/`.
`f4-strata-warp-red-build.log`: build exit0.
`f4-strata-warp-topology-red.log`: exit1 at admission of nonzero phase warp.
`f4-strata-warp-member-red.log`: exit1 in actual member generation,
`procedural.geology.gpu_working_surface_region_operator_unsupported`.
The GPU pipelines completed creation before that rejection; this is not a
shader compilation failure or missing-symbol RED.

`f4-strata-warp-green-build.log`: build exit0 for topology, rock-family and GPU
session targets. `f4-strata-warp-topology-green.log`: exit0/2.263s.
`f4-strata-warp-member-green.log`: exit0/105.869s,3072 working ->3512 final
triangles. `f4-strata-warp-parity.log`: exit0/89.896s; maximum CPU/GPU structural
deviation1.5731e-7. These elapsed process times include pipeline creation and
multiple evaluations; they are not frame times or hardware-budget acceptance.

The candidate preserves member1 density16, all six exact cuts, spacing0.50m,
shape0.045m and groove0.015m. The sole effective recipe change from the retained
Strata witness is phase warp0.15m with existing XYZ frequencies0.1 cycles/m.
The control sets warp back to0 with unchanged displacement and groove.
The full test requires changed field/mesh/classification, closed volume and
support, surviving cut regions, rejection of an adverse source intersection,
and identical regeneration after that rejection. Export separately tests fresh
session mesh/collision determinism for the four-member package; the other three
members remain historical controls, not new accepted rocks.

`f4-strata-warp-export.log`: exit0/223.567s, including the fresh-session repeat
and all four cooked candidates. `f4-strata-warp-material-export.log`:
exit0/1.723s. `f4-strata-warp-editor-build.log`: JericoEditor build exit0,
including current canonical shader profiles and Qt deployment. Source review
found no actionable admission/region defect; it retained the sampled-field
limitations above. No implementation changed after these tests.

## Matched native views and actual result

Package: `build/windows-vulkan-editor/f4-strata-warp-material-01/candidates.epak`,
24070442 bytes, SHA256
`7C623ED57ACB8CF6F36F341834EBF06A25EF76E74838E9C5D5D2DE7305FDDC1D`.
It uses the same admitted Poly Haven rock_surface instance/three textures and
canonical shader archive as `M8_F4_AUTHORED_MATERIAL_WITNESS.md`.
Members0,2,3 retain byte-exact recipes, meshes and collisions versus that witness.
The existing material exporter verifies exact retained geometry/collision.

Replay folder: `build/windows-vulkan-editor/f4-strata-warp-replay-01/`.
Its only package is the exact copy under `Packages/`. Correct invocation uses
`--package Packages:/candidates.epak`; the first filesystem-style argument
failed at VFS parsing with exit1. Those non-rendering attempts are retained in
`front-invalid-path.log` and `side-invalid-path.log`, not counted as captures.
Final `front-capture.log` and `side-capture.log` both exit0: Vulkan,
1280x720,source_free=true,fallbacks=0,2416 vertices/10536 indices,1 material/3 textures.
Same scale1,worldY-1.4,cameraZ5,pitch-15,verticalFOV40,yaw25/135 as the control.
GPU timing is unavailable; this RTX3090 capture establishes no hardware budget.

- [Front](images/m8-f4-strata-warp-witness-01/member-1-front.bmp), SHA256
  `BF1A6D5A1AFF1D0A8694F5A4A04E206AB61C47AE29B606E4B66438D081F23E81`.
- [Side](images/m8-f4-strata-warp-witness-01/member-1-side.bmp), SHA256
  `07FF7591E7ABAA0440EABC9821CEE1349BB0E89CB3E65DAB39B074E8C060F165`.

Both originals were inspected at native resolution. The broad low-frequency
variation shifts/bends bands slightly, but the regular tiered shape and broad
flat cut faces remain artificial. Beauty stays RED. This candidate does NOT
demonstrate a sufficient visual repair; no owner aesthetic approval is claimed.
Keep structural form work as an explicit next step,
rather than repeatedly adjusting noise/resolution without a falsifiable defect.

## End-to-end completeness and semantic lineage audit

The pre-change audit selected the existing chain:
recipe/seed -> GPU phase/profile and distance -> sampled surface -> whole-face
region certificate -> exact native cuts -> classified vertices/structural fields
-> GeologyCook mesh/collision -> authored material package -> native replay.
Phase, structural mask and resulting classifications remain shared inputs to
durability/material/physical consumers. No intermediate semantic field is dropped.
The final audit confirms those boundaries against the executed tests, cooked
package and native views; the visual and integration omissions remain below.

| Area | Applicability and boundary |
| --- | --- |
| Source/input, serialization, save/load | Existing recipe fields and schema; round-trip and source identity required. |
| Asset pipeline, runtime representation | Existing mesh/collision/package formats and cooked identities; no source dependency added. |
| CPU/GPU, shaders | CPU admission changes; existing GPU formula and sampled extraction reused. CPU/GPU Strata parity regression required. |
| Renderer, materials, visualization | Same geological PBR permutation/textures, camera and lighting; original native images required. |
| Physics/collision | Closed exact cooked collision from the changed surface; full product Jolt acceptance remains in F4. |
| Editor/authoring, tooling/UI | Existing recipe/session admits the bounded operation; new ordinary UI phase controls are deferred to M10.5 by the explicit owner correction on2026-09-05. |
| Debugging, testing, failure handling | Authentic RED/GREEN, ordered-stack negatives, source-contact rejection and deterministic recovery. |
| Profiling, performance budgets | Preserve query/region/extraction limits; report elapsed test time separately from frame/generation budgets. |
| LOD, streaming, destruction/runtime modification, navigation/gameplay queries | Preserve cooked classification/structural/collision interfaces; actual-member F4 integration gates remain open. No new runtime interface. |
| ECS/components, animation, scripting/Luau, events, Sequencer, particles, audio | No interface or consumer changes in this admission increment; no new completeness claim for these subsystems. |

INTEGRATION REQUIRED, M10.5: ordinary editor phase amplitude/frequency controls
(owner JericoEditor, input existing Strata payload, interface existing panel and
transaction bridge; dependency bounded admission verified here). The attempted
uncommitted UI increment was withdrawn after the owner reiterated that UI belongs
to M10.5. Its successful tests do not represent a delivered F4 UI capability.

INTEGRATION REQUIRED, M8 F4: displaced source regions and multi-face continuity
(owner procedural geology, source topology and
deformation correspondence); localized painting and multimaterial response
(owner editor/materials, accepted generated surface and classification/palette);
actual-member LOD and hardware budgets (owner geology/runtime, final cooked
products); four convincing rocks and affected criteria6-12 (owner F4, final
identities and native evidence). These remain mandatory F4 acceptance work.

Persistent Goal: ACTIVE until full JericoEngine 1.0 closure.
