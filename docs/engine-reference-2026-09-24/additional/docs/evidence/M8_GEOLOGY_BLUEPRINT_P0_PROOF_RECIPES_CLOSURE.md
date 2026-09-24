# M8 Geology Blueprint P0 Proof Recipes Closure

**Slice:** `M8-GEOLOGY-BLUEPRINT-P0-PROOF-RECIPES`
**Status:** `PASS`
**Backend:** Diligent/Vulkan, NVIDIA GeForce RTX 3090
**Recipe contract:** existing cumulative schema 12; no schema bump

## Result

Jerico now has three canonical, data-driven P0 geology proof recipes that turn
one identical source box into a sedimentary cliff, a fractured boulder and a
weathered rounded boulder. All use the existing admitted layer payloads,
canonical serialization/hash, productive Diligent/Vulkan session, bounded
inspection and Surface Nets extraction. No C++ geometry generator or new
operator was introduced.

With every `Noise` Detail layer disabled, the three results remain visibly and
cryptographically distinct. Sixty-four bounded samples at each recipe's last
macro/meso stage are exactly equal between full and Detail-OFF evaluations,
demonstrating that Detail embellishes rather than owns the composition.

Frozen conventions and the exact stage/parameter/seed inventory are recorded
in:

`docs/evidence/M8_GEOLOGY_BLUEPRINT_P0_PROOF_RECIPES_CONVENTIONS.md`

## RED to GREEN evidence

The first composition contract linked RED because the A/B/E builders did not
exist. After the minimum data builders were added, canonical validation exposed
that schema 12 treated Curvature as a version-global replacement for Bubble,
even though each weathering layer already serializes an explicit `model`.
Schema 12 now admits both existing variants per layer; schema 11 remains
Bubble-only and legacy schemas remain unchanged. The recipe unit rejection
contract was kept by checking that schema 12 still rejects the legacy model.

The productive Vulkan contract then linked RED because the proof evaluator did
not exist. Its first runs caught test assumptions rather than runtime defects:

- Surface Nets attributes single-source triangles to the document GUID, not
  the individual brush GUID; the proof now follows the existing production
  contract.
- Eight arbitrary points did not guarantee a fracture-boundary hit; the proof
  now reuses the admitted bounded `8 x 8` inspection pattern.
- Whole-result readback includes extracted mesh buffers, so its bound is the
  admitted device allocation rather than one source scalar buffer.
- Dense field hashes include the evaluated plan. The exact invariant across
  Detail toggles is the corresponding pre-Detail stage's 64 world-space
  samples, which pass bit-for-bit.

No production runtime behavior was weakened to satisfy those corrections.

## Productive Vulkan evidence

Command:

```text
JericoGeologyGpuSessionTests.exe --proof-recipes-proof docs/evidence/M8_GEOLOGY_BLUEPRINT_P0_PROOF_RECIPES_VULKAN.svg
```

Observed common source hash: `17187918773788934726`.

| Recipe | Recipe hash | Final field/mesh hash | Vertices | Triangles | Detail-OFF recipe hash | Detail-OFF field/mesh hash | Detail-OFF vertices | Detail-OFF triangles |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| A — Sedimentary cliff | `14818948794839183270` | `3169322655651184595` | 6,956 | 13,940 | `14022971068817083982` | `13842000376116918483` | 6,543 | 13,136 |
| B — Fractured boulder | `3273502845010353272` | `13911905980116102005` | 7,076 | 13,848 | `11279864773970548528` | `10269557167284996595` | 6,942 | 13,576 |
| E — Weathered rounded boulder | `3796958945470491854` | `1307321434462335453` | 5,140 | 10,272 | `16886594776780483251` | `5431423660179035242` | 4,840 | 9,676 |

The evidence run recorded `196,608 / 147,456 / 147,456` active voxels and
required device allocations of `29,895,360 / 25,961,664 / 25,961,700` bytes
for A/B/E. Source-field dispatches measured `170,432 / 132,576 / 134,304 ns`;
Surface Nets extraction measured `5,275 / 932 / 631 us`. These are observed
diagnostics, not shipping budgets. The roughly 74-second focused-test wall
times include Debug device/shader startup, synchronization, repeated positive
and negative evaluations, inspection and evidence readback; they are not
per-layer execution times. Individual recorded layer dispatches were generally
about `0.05–0.55 ms` in this run.

Full stage hashes were emitted for every productive pass. A finished at
`SurfaceWeathering=4988817539081978306`, structural
`18288041980422472165`; B finished its macro/meso stack at
`SurfaceWeathering=274444868151847323`, structural
`9701326960813077552`; E finished at
`CavityCarve=5431423660179035242`. The complete canonical stage definitions
remain in the three recipe artifacts.

## Visible and canonical artifacts

- `docs/evidence/M8_GEOLOGY_BLUEPRINT_P0_PROOF_RECIPES_VULKAN.svg` —
  `1800x1050`, SHA-256
  `4fd9766abc574ce670211e36f46eb8522359f40c4cdbc4639bf6ff0bc5047bc9`;
- `docs/evidence/images/m8-geology-blueprint-p0-proof-recipes-vulkan.png` —
  `1800x1050`, SHA-256
  `84abd3a1d20646d31edc0d523c6df1110fc0e2f91021307b755e5d4b0e396e40`;
- recipe A — SHA-256
  `c9d08333ff37e723029c1a0851eaa9756c2cad5a8ef4bf67b2cc1aaf1db1ecc8`;
- recipe B — SHA-256
  `b13ea76dc2f63636fdad3c0aa8879ea3d0d0f1e3d0c23addf5fe663865e9647d`;
- recipe E — SHA-256
  `9d459ec0c0fd855cbb3e34eeafdc0a621165d5c19532b8deec3ea26503b413d2`.

The top row is the full stack and the bottom row disables every Detail layer.
All six panels are meshes extracted from evaluated Vulkan scalar fields.

JericoEditor does not currently expose a geology mesh/recipe viewport route.
An editor capture would require new or restored UI, prohibited by this slice;
the absence of such a screenshot is not presented as an editor capability.

## Verification

Final Debug verification used the existing Windows Vulkan build tree:

- `cmake --preset windows-vulkan-dev`: generation `PASS`;
- focused build of `JericoGeologyRecipeTests`,
  `JericoGeologyGpuTopologyPlanTests` and
  `JericoGeologyGpuSessionTests`: `PASS`;
- recipe, topology and new proof-recipes CTest selection: `3/3 PASS` in
  `73.86 s`; the Vulkan proof itself took `73.61 s`;
- inherited full session, BubbleWeathering, CurvatureWeathering and Detail
  matrix: `4/4 PASS` in `816.41 s`
  (`591.61 / 73.89 / 73.39 / 77.48 s`);
- focused CavityCarve: `PASS` in `73.75 s`, field/baseline hashes
  `2463036689238531135 / 4703855310837332601`, maximum selected-sample parity
  deviation `7.03335e-08 m` and bounded changed-input dirty work;
- focused FractureCells: `PASS` in `73.60 s`, field/structural hashes
  `15244624087024613006 / 18373946749623678984`, scalar parity
  `5.36442e-09 m`, mask parity `4.50683e-08` and bounded changed-input dirty
  work.

The inherited session covers deterministic reevaluation, cancellation,
workload/device-memory guards, material GUIDs, bounded inspection, structural
channels and Surface Nets integration. `git diff --check` and the exact
foreign-inventory reconciliation are publication gates recorded at commit.

## Scope boundary

Completing Proof Recipes closes P0 Step 14 only. It does not complete the
geology blueprint or JericoEngine 1.0. Folded block and columnar jointing remain
P1 because their required operators are not admitted.

Persistent Goal: `ACTIVE` until full JericoEngine 1.0 closure.
