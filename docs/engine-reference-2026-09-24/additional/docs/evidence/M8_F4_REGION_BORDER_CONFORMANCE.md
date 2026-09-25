# M8 F4 projected region-border conformance

2026-09-06. Status: local native repair demonstrated; structural Beauty RED.
Same F4 slice, same checkout. This does not close M8 or the persistent Goal.

## RED and minimum repair

The fixed structural A/B in M8_F4_SAMPLED_CONTOUR_REPAIR.md advanced B to a real
source-region crossing at triangle1444. A whole-face single-owner proof could
not assign this face: its normal-directed footprint crosses two actual regions.
Selecting one centroid/GUID winner would erase that border. The local oracle
retained five nonoverlapping projected patches with total barycentric area0.5.

The bounded private partition helper clips source patches onto the output face
and proves complete unoccluded coverage. It uses the existing local patch proof;
source depth does not displace the weathered face. Its transient polygons are
triangulated with their source owner. New boundary points are inserted into every
incident polygon, including neighbors whose projection direction produced a
different cut on the shared edge. Their union makes the triangulation conform.
Only double-roundoff coincident points are merged (64epsilon times world scale).
No angular/spatial acceptance threshold or voxel resolution changed.

The conformance helper caps positions/indices and work, checks cancellation,
and returns a separate candidate. The existing finalizer canonicalizes float32
positions, verifies closed topology, rebuilds region frames, resamples semantic
classifications through existing GPU buffers and checks admitted capacities.
Production integration is limited to ambiguous post-weathering correspondence.
Unambiguous faces keep their existing mapping; the original LOD baseline is exact.
No new provenance, schema, permanent GPU buffer, mesher, dependency or UI.

## Independent geometry and shading evidence

Native SAME A/B now exits0. Neither recipe, source, material, seed, wear step,
QEF, Surface Nets nor LOD thresholds changed for this comparison.

| Product | Dense triangles | LOD triangles | LOD normal fidelity | Spatial deviation | Regions |
| --- | ---: | ---: | ---: | ---: | ---: |
| Solid A | 5372 | 3000 | 19.7384deg | 2.95314mm | 7 |
| Notched B | 5382 | 3000 | 19.7437deg | 2.41451mm | 10 |

Automatic B proof run-b9a5c80549f2 measures19.743747560524753deg,
0.0024145056513109335m and358706us CPU reduction. The control A regression is
run-b616f16aa37a. All thresholds are the original verifier's thresholds.

Both dense/LOD products have zero invalid welded edges, invalid vertex links,
zero-area faces and faces opposed to their summed output normals. B volume is
1.2434479322440175m3 dense and1.2427153515989906m3 LOD. Native validation also
requires exact dense/LOD region-boundary edges and successful mesh/collision cook.
A and both structural prefixes remain byte-identical to the previous A/B.
The unchanged prefixes still have four opposed output-frame faces; this repair
does not silently relabel that pre-weathering issue as fixed.

Shading is a DIFFERENT measurement: B's maximum geometric face versus summed
output-normal angle is45.9301deg dense and89.0662deg LOD. Four LOD faces exceed
60deg and occupy0.00159256percent of its area. Their witness is retained; this
does not alter or replace the19.7437deg field-fidelity result. No normal bake,
frame camouflage or geometric-quality acceptance follows from LOD fidelity.

Native1280x720 capture shows the notch and broad planes but still reads as an
artificial rounded/faceted block, with visible shading seams. It remains a
diagnostic preview, not the required grounded four-member family Beauty scene.
Natural mass, readable fractures, material response and multiview Beauty remain RED.

## Tests and regression protection

The new CPU RED failed conformed.has_value(). GREEN proves two adjacent tetrahedron
faces can choose different shared-edge cuts while retaining both cuts, closed
topology, original surface and volume. Cancellation and unchanged-input behavior
are covered. Existing partition tests reject holes, overlap and occlusion.

The native notched fixture is registered as procedural.geology_region_working_lod
through the EXISTING actual-working verifier and GPU scheduling lock. It retains
the original six cuts, structural-field lineage, deterministic regeneration,
closed meshes, region boundaries, 3000-triangle/35deg limits and both cooks.
Final six-test integration run PASSES in443.13s: topology1.98s, working LOD0.10s,
Hermite74.94s, protected actual member114.90s, contour A105.08s and region B145.73s.
These are test wall times, not GPU generation/frame times.

Publication review also found that e810110c's selective staging misplaced the
existing ambiguity guard, leaving an unconditional rejection block in Git while
the tested working file had the correct placement. This publication includes
the complete corrected function and compares its staged bytes to the tested
working function. Earlier retained native evidence describes the working tree;
it was not a clean-checkout verification of that misplaced guard.

Protected baseline run-a1f1a099ff68 passes17.014496000952558deg and
2.9531370682149912mm at3000tri; all SIX files match run-ac84c31ca48e exactly.
CPU LOD reduction351699us is Debug CPU time, not GPU generation or frame time.

Immutable evidence: data/m8-f4-region-conformance-01/proof.zip,69files,
6291889bytes, SHA256
e8b800c65d1ca1ae4a79fb008e3a735ed1ac9b7ef868f14ba37d4ac5cf2e581f.
manifest.json records every file hash; all69 archive entries were verified.
Original image: images/m8-f4-region-conformance-01/notched-weathered.bmp.

## Completeness boundary and next action

Lineage: unchanged source/cuts -> weathered sampled field -> contour extraction
-> transient region partition -> conforming dense mesh -> existing region frames
and GPU classifications -> regional LOD -> rmesh/rcoll -> native renderer.
CPU/GPU admission, cancellation, materials/semantics, cook, collision, debugging,
tests and runtime rendering apply and are verified to the local bounds above.
No new ECS, events, animation, Luau, save/load, streaming, destruction, navigation,
particles or audio interface is introduced; their existing F4 consumer audit holds.

INTEGRATION REQUIRED in F4/M8: four independent authored rocks with support and
three useful silhouette directions, grounded family/landmark Beauty, affected
paint/publication transactions, package-only Shipping, measured performance and
remaining criteria6-12. Owners/interfaces remain the existing Geology authoring,
renderer/materials, asset/collision cook and package consumers in ACTIVE_SLICE.
This local repair cannot waive any of those requirements. New UI stays M10.5;
high-to-low bake and saved macroform research remain deferred. PlaneCut stays CLOSED.

Next: return to authored structural Beauty using the now-demonstrated source
composition/cuts/weathering path; preserve these regressions. No further region
framework or parameter search is justified by this repaired witness.

Persistent Goal: ACTIVE until full JericoEngine1.0 closure.
