# M8 F4: exact native constraints in the bounded mixed Hermite fit

Date: 2026-09-06. Base: `7d96ac850434d1be6f4892437f524201e2331ef3`.
State: **PARTIAL. Mixed-fit repair verified; structural Beauty remains RED.**
The protected original LOD and the closed PlaneCut diagnostic are unchanged.

## Repaired defect

The retained composed-source cell (16,13,-1) has a feasible point inside its
cell on the exact native plane. The old mixed native/implicit solver rejected
the unconstrained minimum at local y=1.391477 instead of solving that feasible
problem. Its four-constraint QR system now has a GPU known-answer regression:
local (0.385637954,1,1), native z=1 retained, QR RMS approximately0.7442mm.
The regression failed against the old solver before implementation.

The repair keeps at most three independent exact native equations in local
shader state. For mixed cells, it minimizes the existing QR residual over the
intersection of those planes and the existing27 box active sets. Native planes
remain equality constraints; curved implicit tangents remain least-squares
constraints. The legacy feasible result is retained when it satisfies both.
Pure native and sampled post-weathering cells keep their previous solver path.
No source plane is reimposed after weathering. No permanent GPU buffer, schema,
dependency, serialized provenance, new UI or LOD algorithm was introduced.

GPU tests additionally cover repeated and oblique planes, two-plane and
three-plane intersections, conflicting exact constraints and mixed curved
constraints. Multi-crossing rejection and the original QEF rank floor remain.

## Same structural A/B

The source-composition witness from `M8_F4_STRUCTURAL_SOURCE_BLOCKER.md` is
unchanged: both variants disable only Strata shape/groove displacement; B removes
the positiveXYZ source octant. Seed, envelope, cuts, material and resolution stay
fixed. B now passes structural extraction, region assignment and mesh/collision
cook. Independent checks on the final cooked-input mesh dumps:

| Check | Solid A | Notched B |
|---|---:|---:|
| Triangles |1926|2886|
| Regions |7|10|
| Boundary/nonmanifold welded edges |0|0|
| Inconsistent edge winding |0|0|
| Zero-area faces |0|0|
| Winding at removed-octant probe |1|0|
| Winding at three retained probes |1,1,1|1,1,1|
| Signed volume, m3 |1.613571793|1.363029103|

Four structural triangles oppose their final vertex-normal sum in BOTH variants,
at identical negative-X locations. This observation is retained separately from
closure and from the later positive-X post-weathering failure; geometric versus
shading validity of those four prefix faces is not claimed as resolved.

Native1280x720 captures use identical material, light and pose, zero fallbacks,
and exact matching collision inputs. [A](images/m8-f4-native-constraint-01/solid.bmp)
and [B](images/m8-f4-native-constraint-01/notched.bmp) are structural diagnostics.
The notch survives but the brown faceted mass is not accepted natural-rock Beauty.
No four-rock or cliff Beauty PASS follows from this repair.

## Next RED: sampled topology after weathering

Both weathered variants reject BEFORE LOD and cook. The retained raw readback
has a shared edge incident to FOUR triangles, and a rejected geometric face
about174.36deg from the summed sampled normals. For the recorded quad, the other
diagonal also produces opposed triangles (about170deg and131.55deg). Changing
that diagonal, shading normals, correspondence or LOD cannot repair this witness.

Field inspection locates the change on the x=0.9m voxel face with corners
(15,-5,-3),(15,-4,-3),(15,-4,-2),(15,-5,-2), in grid coordinates at6cm:

| A stage | Four signed distances, mm |
|---|---|
| Before weathering | -29.4827,-14.9306,-49.8557,-64.4078 |
| After weathering | -1.8630,+2.4199,-9.1904,+3.9210 |
| After Rebuild | -5.8995,+2.6474,-26.9164,+3.1026 |

The alternating sign mask5 appears in SurfaceWeathering and survives Rebuild.
B has the same mask and nearly identical values. The single dual vertex per
cell merges these crossings into nonmanifold connectivity. This is a concrete
representation limit, not proof that the authored notch or SDFs are impossible.

A single bounded numerical-step control changed4 iterations at6cm maximum wear
to16 iterations at1.5cm, preserving the24cm nominal maximum total. BOTH controls
still reject dense geometry and the SAME recorded face remains mask5. The claim
that quarter-size steps alone repair this blocker is falsified locally; this
does not establish a general stability theorem or justify changing production
weathering parameters. No additional step/preset search follows.

Next repair candidate: associate Hermite constraints and emitted connections
with separate local contour components in ambiguous sampled cells. Keep the
existing path for ordinary cells, the same field, QEF, LOD and region contracts.
First demonstrate a shared-face known-answer test, then the SAME real A/B and
protected LOD. Do not claim success from component labels without valid dense
geometry, closed topology, regional LOD and cook. This is not an octree/meshlet
or general adaptive-LOD project.

Primary technical reference: Schaefer, Ju and Warren,
[Manifold Dual Contouring, section III](https://people.engr.tamu.edu/schaefer/research/dualsimp_tvcg.pdf).
It describes one vertex/QEF per contour cycle on a uniform grid, with incident
edge-to-vertex associations. Its adaptive clustering subsystem is outside this
repair. A topological manifold alone does not certify geometric quality here.

## Verification and evidence

- GPU known-answer RED: `f4-native-constraint-red.log`, CTest exit8.
- Expanded GPU Hermite GREEN:33.06s test wall time.
- Original protected LOD regressions:3/3; actual member remains17.014496deg,
  3000triangles,2.953137mm, closed,7regions/397 boundaries, mesh/collision cook.
- Protected dense/LOD dumps and both cooked formats remain byte-identical in
  fresh runs `run-a30eefc5d37b` and final `run-26632a38bad9` to the prior
  `run-ac84c31ca48e`. Final CTest3/3 passes in122.52s wall time.
- Structural A/B is reproducible; full weathered A/B and quarter-step control
  remain expected exit1. These are diagnostics, not newly passing CTest gates.
- One intermediate probe draft requested68 samples and was correctly rejected
  by the64-sample limit; the final witness uses31. The limit was not increased.
- One relink collided with a running baseline executable on Windows (LNK1168);
  after that test completed, the sequential rebuild succeeded. No process killed.

Data and hashes: `data/m8-f4-native-constraint-01/manifest.json` and `proof.zip`.
Archive:66 files,9118430bytes, SHA256
`51053a71ff82e48a1f991c5b9d8f6e3ccfd335ba55008a625e380af11122c2f2`.
Manual modes take the committed control recipe, retained material-reference mesh
and a NEW output directory: `--structural-source-beauty`,
`--structural-source-weather-substeps`. Geometry metrics, field values, local
topology, RED/GREEN logs and native photos are retained separately.

## Completeness boundary

Lineage: authored source -> native Hermite equations -> GPU local fit -> dense
mesh -> regions/classification -> mesh/collision cook -> native renderer.
This repair retains exact source information only while solving the cell; it
does not claim new downstream semantic provenance. CPU/GPU, error handling,
capacity/cancellation contracts, tests and documentation apply. Existing material,
asset, physics and rendering consumers pass for the structural pair; original
LOD/cook are recertified. Weathered composition and regional LOD remain RED.
Four rocks/cliff Beauty, representative source-free package/performance,
authoring transactions and remaining M8 consumers remain INTEGRATION REQUIRED
under the active slice. No new ECS, animation, Luau, Sequencer, save/load,
streaming, destruction, navigation, particles or audio interface is claimed.
High-to-low bake remains deferred; final UI is M10.5; cache WIP is preserved.

Persistent Goal: **ACTIVE until full JericoEngine1.0 closure**.
