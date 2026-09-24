# M8 Geology Blueprint P2 Mineral Dissolution Closure

**Slice:** `M8-GEOLOGY-BLUEPRINT-P2-MINERAL-DISSOLUTION`

**State:** `PASS`

**Selected boundary:** `BOUNDED_LOCAL_MINERAL_DISSOLUTION_FRACTION`

**Persistent Goal:** `ACTIVE until full JericoEngine 1.0 closure`

## Result

Jerico now admits one schema-26 cumulative geology layer as the final active
operation after Source, active Strata, active Rebuild, schema-24 narrow-band
mineral response and schema-25 normalized moisture exposure. It evaluates one
deterministic local mineral partition, overwrites the existing auxiliary float
field with a separately identified normalized dissolved fraction, and publishes
bounded inspection samples without changing signed distance, Surface Nets,
packed classification, structural Strata state, durability or collision.

The result is dimensionless authoring data. It is not mineral concentration,
mass, molar quantity, a transported or globally conserved value, erosion, or
runtime mutable geology. It does not add cross-voxel mineral transport,
recrystallization, deposition, physical chemistry, geometry removal, remesh or
retopology. The designer retains pure preview/diff/cancel and explicitly chooses
the canonical recipe.

## Research-first provenance and independent design

The research record is
`docs/evidence/M8_GEOLOGY_BLUEPRINT_P2_MINERAL_DISSOLUTION_RESEARCH.md`. It studies
Dorsey et al., *Modeling and Rendering of Weathered Stone* (SIGGRAPH 1999),
directly from the primary PDF. The source motivates separate solid, dissolved,
transported and later recrystallized mineral states, but it does not provide a
complete deterministic per-voxel solid-inventory update, unit closure, boundary
state, time step or finite solver budget. Its printed dissolution equation also
conflicts in sign with the surrounding prose, so Jerico does not transplant it
or claim physical conservation from it.

The inspected PDF SHA-256 is
`F83423249AB46E15411B01245311502785555788C387097B1B8C0E47BFFC5788`. No donor
implementation, shader, source asset or reference library was inspected, copied
or ported. No additional source was necessary to freeze this deliberately local
phenomenological boundary. Jerico's independent contract is
`docs/superpowers/specs/2026-08-29-m8-bounded-mineral-dissolution-design.md`.

## Observable RED chronology

The first focused Release CPU build consumed the recipe/field contract and failed
because `GeologyMineralDissolution.hpp` did not exist. The typed parser,
validation, pure diff, pointwise reference and budget tests were then implemented
minimally until that executable passed.

The Vulkan test was written before backend support and failed because the public
result did not expose dissolution work/byte diagnostics, a dissolution field
hash or dissolved-fraction inspection samples. Backend and shader support then
reused the existing auxiliary field and hash allocation. The focused real Vulkan
test consumed a non-zero schema-25 field, produced a distinct non-zero
dissolution-field identity, matched three CPU witnesses and passed cancellation,
identity, preservation and deterministic-cook checks.

The final RED/GREEN sequence proves canonical schema/order/hash, zero and
monotonic response, local partition bounds, deterministic seed sensitivity, pure
diff, checked work rejection, cancellation without a published result,
CPU/Vulkan parity, exact prior fields and deterministic cook provenance.

## Frozen local partition contract

For an admitted stone voxel:

```text
w       = schema-24 weathered-response byte / 255
p       = schema-25 normalized moisture exposure
t       = authored minimum productive moisture exposure
a       = clamp((p - t) / (1 - t), 0, 1)
pattern = 0.75 + 0.25 * normalized existing one-octave value noise
dose    = dissolutionRatePerSecond * exposureDurationSeconds
M       = availableMineralFractionNormalized * w
D       = M * (1 - exp(-dose * a * strength * mask * pattern))
R       = M - D
```

`D` is stored as `dissolvedMineralFractionNormalized`; `R` is derived for
validation and inspection and requires no second field. The operation is zero
outside represented stone or when moisture activation, susceptibility, mask or
strength is zero. Every admitted term is finite and non-negative, so:

```text
0 <= D <= M <= availableMineralFractionNormalized <= 1
0 <= R <= M
D + R = M within the shared CPU/GPU tolerance
```

This is a local partition only. The schema-24 response remains susceptibility and
is not decremented. The schema-25 field remains normalized exposure and is
inspected and hashed before its buffer changes semantic. No result moves between
voxels.

## Typed controls and validation

Schema 26 owns exactly five payload values:

```text
availableMineralFractionNormalized  (0, 1]
dissolutionRatePerSecond            (0, +inf)
exposureDurationSeconds             (0, +inf)
minimumMoisturePressureNormalized   [0, 255 / 256]
reactionFrequencyPerMeter           (0, +inf)
derived dose = rate * duration       [1 / 4096, 8]
```

Every scalar and the derived dose must be finite and exactly representable as
binary32. CPU computes and uploads the validated dose so the shader does not
introduce a second multiplication contract. Kind, payload, strength, regional
mask, active state and full-width seed are all covered by canonical schema-26
serialization and recipe identity.

Exactly one active dissolution layer is legal, and it must follow all five
required cumulative predecessors. Schema-24 and schema-25 recipes remain valid
without schema 26. Invalid payload, order, count, budget, cancellation or device
failure returns no `GeologyGpuEvaluationResult`.

## Finite execution, storage and identity

```text
maximum work items              67,108,864 active voxels
shared bytes per active voxel   4
maximum shared auxiliary bytes  268,435,456
additional dissolution bytes    0
GPU layer dispatches            1
numerical iterations            0
```

Checked admission occurs before dispatch. The schema-25 stage is sampled and
hashed before schema 26 reads the moisture value and overwrites the same
one-float-per-voxel auxiliary field with `D`. This adds no allocation, descriptor
or global storage-buffer limit. The final stage owns a separate
`mineralDissolutionFieldHash` and sampled dissolved fractions; the already
captured moisture identity and samples retain their original meaning.

The output field is authoring-only. It does not modify extracted vertices,
indices, collision, runtime materials, runtime formats or package payloads. Cook
changes only through canonical recipe provenance. Equal source plus equal recipe
remains byte-identical; a semantic recipe change has a distinct identity without
pretending the field ships.

## Implementation and integration

- `GeologyLayerKind::MineralDissolution = 28` and one typed five-scalar payload
  extend only the cumulative recipe/parser/writer/validator/hash path;
- `GeologyMineralDissolution.hpp` owns validation, pure diff, the pointwise CPU
  reference and checked work admission;
- the existing layer constants carry inventory, CPU-validated dose, moisture
  threshold and reaction frequency;
- the Vulkan shader reads moisture and writes dissolved fraction through the same
  auxiliary UAV, then accumulates the existing auxiliary-field hash pair;
- the session captures the buffer with one explicit semantic at each stage and
  publishes distinct moisture and dissolution identities/samples;
- the existing deterministic cook records schema-26 `recipeHash` in JRMS and
  preserves byte-identical mesh and collision payloads for repeated input.

No framework, secondary field system, device-limit increase, VDB/UI, bespoke
preset, runtime component or parallel implementation was added.

## Release verification

Focused Release builds used the existing generated projects with project
references disabled, avoiding regeneration from the unrelated preserved CMake
edit. The recipe executable and focused Vulkan executable returned zero on an
NVIDIA GeForce RTX 3090 using Diligent/Vulkan.

The consumed dissolution witness was:

```text
recipe_hash=5351109586537887712
field_hash=7894579602563779244
surface_hash=7894579602563779244
moisture_field_hash=11485586198880431429
dissolution_field_hash=5998300488029934755
work_items=147456
additional_buffer_bytes=0
mesh_bytes=426922
collision_bytes=191970
cpu_gpu_inspection_samples=3
sample0_dissolved=0.0353054
sample1_dissolved=0.0233622
sample2_dissolved=0.053419
```

The unchanged scalar/surface identities prove that dissolution did not alter
geometry. The retained non-zero moisture hash proves that the owned input was
observed before overwrite, while the distinct non-zero dissolution hash proves a
separate consumed output. Repeated evaluation and cook were deterministic. A
seed change altered recipe and dissolution identities while preserving scalar,
surface and moisture-stage identities.

The schema-25 regression returned:

```text
recipe_hash=11317994414215098535
field_hash=7894579602563779244
surface_hash=7894579602563779244
moisture_field_hash=11485586198880431429
work_items=147456
additional_buffer_bytes=589824
mesh_bytes=426922
collision_bytes=191970
cpu_gpu_inspection_samples=3
sample0_pressure=0.431292
sample1_pressure=0.453202
sample2_pressure=0.505477
```

The schema-24 narrow-band mineral-response regressor also returned zero with
`classification_hash=474208821701615791`, proving that the new final layer did
not reopen or replace its susceptibility contract.

Machine-readable metrics are retained in
`docs/evidence/data/m8-geology-blueprint-p2-mineral-dissolution-release.txt`.

## End-to-end system completeness and data lineage

```text
schema-24 susceptibility + schema-25 exposure + explicit schema-26 inventory
-> canonical typed recipe and identity
-> checked one-pass work admission
-> resident classification plus inspected/hashed auxiliary moisture field
-> one local dissolution dispatch
-> shared auxiliary dissolved-fraction field
-> dedicated hash and bounded authoring inspection
-> unchanged mesh/collision/runtime products plus schema-26 recipe provenance
```

Applicable and proven categories are typed source/input, recipe serialization,
identity, CPU reference/diff/admission, GPU field processing, debugging and
inspection, deterministic static cook lineage, cancellation/failure, budgets,
tests and documentation. Static renderer/material/collision products are exact
unchanged downstream results. Animation and Sequencer do not apply to this
offline fraction; runtime mineral simulation is outside the boundary.

The useful upstream semantics remain distinct: classification remains class,
schema 24 remains susceptibility, schema 25 remains exposure, schema 26 inventory
is explicit local capacity, and schema 26 output remains locally dissolved
fraction. Transported concentration, remaining-solid runtime material response
and surface recrystallization require separate explicit contracts and are not
silently implied.

### INTEGRATION REQUIRED

| Subsystem | Required data/interface | Owner/dependency | Milestone |
|---|---|---|---|
| Dedicated editor transaction/UI | Typed payload, dissolved-fraction preview visualization, pure diff and existing preview/cancel/apply transaction | JericoEditor geology authoring | Later Geology editor integration |
| Multi-scale microstructure | Explicit scale hierarchy and bounded synthesis inputs; no implicit dissolution-field reuse | Next ordered Geology P2 owner | `M8-GEOLOGY-BLUEPRINT-P2-MULTI-SCALE-MICROSTRUCTURE-SYNTHESIS` |
| Transport/recrystallization | Explicit species/units, conservation domain, boundaries, finite solver and output ownership | Separately authorized future Geology owner | Feature-driven future research slice |
| Runtime mineral/material response | Cooked bounded field schema and renderer/material semantics if a real product requires them | Geology plus runtime/material owners | Feature-driven production integration |
| Mineral-dependent Jolt properties | Cooked mineral-to-physical-property contract | Geology plus Jolt integration | Production Geology/physics |
| LOD/world-cell streaming | Defined field preservation or deliberate discard across simplification/cell cook | Geometry LOD and World Cell owners | Production streaming |
| Mutable destruction/save | Stable modification identity, state schema, remesh/collision replacement and persistence | Future runtime modification owner | Post-blueprint production expansion |
| Navigation/gameplay/Luau/events/particles/audio | Read-only cooked mineral query/event seam if demanded by a product | Respective downstream owners | Feature-driven production slices |

Multi-scale microstructure synthesis remains the only open ordered P2/research
entry. It was not implemented or silently claimed complete. The complete Geology
blueprint must close before the later PWAS/EPLS production expansion begins.

Completing this slice does not complete P2, the Geology blueprint, the later
production expansion or JericoEngine 1.0.

Persistent Goal: `ACTIVE until full JericoEngine 1.0 closure`
