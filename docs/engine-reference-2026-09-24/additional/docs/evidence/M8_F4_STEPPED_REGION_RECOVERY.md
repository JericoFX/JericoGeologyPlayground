# F4 stepped source: bounded contour and region recovery

2026-09-06. Parent `e351da02614e508e38f9137740aa58feff9e1955`.
Status: **technical RED repaired and regression verified; Beauty RED**.
Persistent Goal ACTIVE. PlaneCut remains closed; no M8 completion claim.

## Fixed input and diagnosed losses

The optional `--stepped-source-beauty` mode authors one fixed 52-of-64-cell
source: a continuous lower mass, upper back shelf and half-height front shelf.
It retains the seed, six cuts, 0.06 m field spacing, working density16, QEF and
post-cut Convex wear (0.06 m maximum per iteration, four iterations). This is a
structural diagnostic: the existing harness disables Strata displacement/groove.
It is not a full artistic stack or a preset/seed search.

Three successive native failures exposed distinct stages:

1. The original diagonal of one actual quad produced face/field angles
   93.661107 and 9.469094 degrees. The other diagonal, with identical four
   positions and boundary winding, gives 30.505623 and 13.737044 degrees.
   The sampled post-weathering shader now changes a diagonal only if an original
   face opposes the field and BOTH replacement faces agree. No triangle is
   dropped, individually flipped or moved. GPU known-answer tests cover the
   actual quad, reversed winding, a valid flat quad and an invalid alternative.
2. Face2389 has a complete nonoverlapping footprint of17 patches/four owners.
   Reproving each fan child rejected that valid parent cover. The bounded proof
   now clips and checks coverage/overlap in the original parent coordinates;
   back-facing participation, missing coverage and overlapping coverage reject.
   The precise floating-point amplification mechanism was not established.
   The actual37-vertex/51-triangle conformance witness and negative cases are
   documented in [the CPU proof](M8_F4_FOUR_REGION_TRIANGULATION.md).
3. Region `952339307EE80C6754760038CC283A5B` remained missing because a narrow
   source footprint could intersect output faces without owning their centroids.
   Only when a region is missing, the finalizer inspects faces overlapping its
   bounds expanded by the EXISTING projection distance. It accepts only complete
   certified footprints containing a missing owner, then uses the existing shared
   edge conformance. The strict final all-region check still rejects any loss.
   Work is capped at16,777,216 source-triangle visits and remains cancellable.

The third repair restores classification of existing geometric support; it does
not restore eroded volume. It uses temporary CPU arrays, with no permanent GPU
buffer, schema, generalized provenance, new operator or post-wear plane constraint.

## Native A/B and downstream verification

Before recovery: `f4-stepped-region-identity-01` rejects the named region.
After: `f4-stepped-region-recovery-01` exits0 with the SAME source/weathering
recipe. Its structural prefix's four files remain byte-identical to the preceding
run. The registered fixture stores that structural recipe; the existing verifier
inserts weathering once. Its generated candidate recipe is byte-identical to the
native witness recipe.

| Metric | Dense | LOD |
| --- | ---: | ---: |
| Triangles | 5358 | 3000 |
| Regions | 12 | 12 |
| Invalid welded edges / vertex links | 0 / 0 | 0 / 0 |
| Zero-area / opposed output-frame faces | 0 / 0 | 0 / 0 |
| Recovered-region triangles | 122 | 70 |
| Recovered-region area, m² | 0.006446039759002277 | 0.006443566461405206 |

LOD fidelity is **19.752209466925141 degrees** against the unchanged35-degree
gate. Bidirectional vertex-to-surface deviation is **0.0022134278301261124 m**.
Closed mesh, exact regional boundary edges, repeated field/mesh/classification,
64 structural-field parity probes, cancellation and invalid-operation rejection,
mesh/collision cook and readback all pass the existing downstream verifier.

The new test initially failed because the fixture already contained weathering;
the verifier correctly rejected its duplicate insertion. After using the exact
structural input, two test assumptions failed: this smaller resident field fits
in full, and a budget one sample below the full field can still retain a compact
covered region. The verifier now checks that route before testing a budget below
the compact requirement. Production export/caps were not changed.

Final Release build exits0. Final five tests pass in38.12 s: topology, original
actual LOD, contour LOD, region LOD, new stepped LOD (10.53 s). The preceding
working-LOD/Hermite/three-native regression run passed5/5 in149.01 s; shader code
did not change afterward. Protected original baseline run-ea23b10387bd matches
ALL SIX golden mesh/collision/text files byte for byte, still17.014496 degrees.
The new test participates in the existing Vulkan resource lock and scheduling
inventory. No production thresholds or triangle budgets were relaxed.

## Appearance and integration limits

Three native1280x720 views were inspected at original resolution. The mass is
still rounded, brown and artificial: **Beauty remains RED**. These structural
controls do not establish the ceiling of the complete authored layer stack.
The separate face-versus-summed-output-normal maximum is51.563681 degrees dense
and89.110764 degrees LOD; this is NOT the19.752209-degree LOD fidelity metric.
The unchanged pre-weathering prefix has four opposed output-frame faces
(maximum178.790860 degrees). Those shading issues are retained, not hidden.

Data lineage is unchanged: authored source/layers -> GPU field/contour -> bounded
CPU region conformance -> classified dense surface -> regional LOD -> canonical
mesh/collision cook -> native renderer. Classification/regions are tested through
the existing consumers. Serialization formats, physics authority, gameplay,
streaming and destruction contracts are unchanged. Their broader M8 acceptance,
four rocks/cliff and final Beauty remain required under the active slice; this
local repair does not recertify those systems. High-to-low bake stays deferred.

The designer's Release preview PID33112 remained open and responsive. Bounded
correctness tests ran while it was idle; these are not performance benchmarks.
Do not close the user window, overwrite its draft or run competing timing tests.
Existing shader-cache WIP remains unadmitted and excluded from this change.

Archive `data/m8-f4-stepped-region-recovery-01/proof.zip`:76 rehashed entries,
10,204,694 bytes, SHA256
`d2b25df749c0ca953b7627f46fc9e314696bf38503ebe70ce9319ec8d2b91b34`.
The adjacent manifest records every entry hash. It retains native products/views,
earlier rejection, independent metrics, final four downstream runs, build/test
logs, test-harness failures and the six-file golden comparison.

Next: return to the existing authored Beauty stack on the now-valid source,
with matched structural/full-layer views and the designer's live controls.
No more region-theory slice, PlaneCut reopening, new macroform subsystem or bake.
