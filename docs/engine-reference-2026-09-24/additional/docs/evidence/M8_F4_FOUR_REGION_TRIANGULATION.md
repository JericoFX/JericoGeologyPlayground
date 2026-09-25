# F4 actual four-region face: CPU conformance proof

2026-09-06. Parent `cae675318f7578ae95eef7d3e8e72707b707443c`.
Status: **CPU component verified; experimental native recovery remains PARTIAL**.
Persistent Goal ACTIVE. This is evidence, not M8/Beauty acceptance.

The existing stepped-source diagnostic reached face2389 after the quad-diagonal
experiment. Its normal-directed footprint contains17 source patches from four
owners. The earlier per-child projection proof rejected that complete coverage;
the experimental parent-coordinate proof accepts it. Its native continuation
then rejected a different condition: missing region
`952339307EE80C6754760038CC283A5B`. Those earlier results are retained separately
and are not reclassified here.

This turn extended the actual numerical witness in
`regionalPartitionCoversTheActualFourOwnerFootprint` through the existing
`conformWorkingRegionPartitions`, rather than checking only a patch list. The
original source triangles and output-face coordinates are unchanged.

The test verifies:

- all four owners survive and each owner's world-space area matches its projected
  source coverage within1e-12 square meters;
- every output triangle has positive orientation relative to the original face;
- every output vertex remains coplanar and inside the original face;
- each interior edge has exactly two opposing incidences, and every singly used
  edge lies on the original outer boundary (no internal crack/T-junction);
- deleting a participating source patch or duplicating it rejects the footprint.

Result: **37 vertices,51 triangles**, four owners, original face area approximately
`0.000941002 m^2`. This is a single-face proof, not a closed rock. It establishes
that this exact partition can reach a conforming triangulation without changing
the supporting geometry or losing an owner. It does not establish recovery of the
other small region, a completed LOD/cook, shading quality or realistic appearance.

## Execution and retained state

Build target `JericoGeologyGpuTopologyPlanTests`, Debug: exit0.
`ctest --test-dir build/windows-vulkan-editor -C Debug -R '^procedural.geology_gpu_topology$' --output-on-failure`:
exit0,2.02s. A verbose replay retained the numerical output.
`procedural.geology_working_lod`: exit0,0.10s. These are CPU tests despite the
historical name of the topology target; no geology GPU session was started.
An initial test-harness compile error referenced a nonexistent cross-product
helper; it was corrected locally before running the test and is not an algorithm
RED. The earlier actual projection RED/GREEN logs are archived as historical data.

Archive `data/m8-f4-four-region-cpu-01/proof.zip`:16 verified entries,86,220 bytes,
SHA256 `76d132ab26053baee5774de8159067b5ebb90c805fb668f70356ad92bfdf22cb`.
It retains the exact experimental helper/test sources, their diff against the
parent, CPU logs, the earlier native rejection, source mesh coordinates, recipes
and the independent projected-footprint data. Every entry was rehashed after
compression. Experimental source changes remain uncommitted pending the native
downstream gates; this publication only records the bounded CPU evidence.

No generator/preset/quality/threshold change was made in this turn. The user's
native preview was verified live and responsive at PID33112; it was neither
closed nor subjected to a competing GPU test. No new subsystem or PlaneCut
diagnosis was opened.

Next when the GPU test lane is available: run the already-built SAME stepped
recipe with the experimental missing-region recovery, preserving rejection if
the complete footprint cannot be certified. Then native geometry/LOD/cook and
the protected baseline must be verified before publishing production changes.
Do not claim that the missing region was physically eroded or can be recovered
without that evidence. Beauty/four rocks/cliff and the remaining M8 gates stay open.
