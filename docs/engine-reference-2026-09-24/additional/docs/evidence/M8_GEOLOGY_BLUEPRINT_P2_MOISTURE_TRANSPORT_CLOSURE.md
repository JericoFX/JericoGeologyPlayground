# M8 Geology Blueprint P2 Moisture Transport Closure

**Slice:** `M8-GEOLOGY-BLUEPRINT-P2-MOISTURE-TRANSPORT`

**State:** `PASS`

**Selected boundary:** `BOUNDED_NORMALIZED_MOISTURE_PRESSURE_PROFILE`

**Persistent Goal:** `ACTIVE until full JericoEngine 1.0 closure`

## Result

Jerico now admits one schema-25 cumulative geology layer as the final active
operation after Source, active Strata, active Rebuild and the admitted Dorsey
narrow-band mineral response. It evaluates a deterministic normalized pressure
potential in one finite inside-surface band, stores it in one float per admitted
voxel, and publishes its own field hash and bounded inspection samples without
changing signed distance, Surface Nets, packed classification, structural Strata
state or durability.

The result is an immutable authoring field. It is not physical pressure in Pa,
water mass, water volume, saturation or a conserved quantity. It does not add
lateral porous flow, a moving wet front, iterative time integration, mineral
transport, dissolution, recrystallization, erosion or runtime mutable moisture.
The designer retains pure preview/diff/cancel and explicitly chooses the
canonical recipe.

## Research-first provenance and independent design

The research record is
`docs/evidence/M8_GEOLOGY_BLUEPRINT_P2_MOISTURE_TRANSPORT_RESEARCH.md`. It studies
Dorsey et al., *Modeling and Rendering of Weathered Stone* (SIGGRAPH 1999),
directly from the primary PDF. The source distinguishes an authored surface
pressure, an interior pressure field and an inner zero-pressure front, but it
does not provide a complete SI pressure closure, lateral boundary conditions,
time step, finite iteration count or deterministic convergence rule.

The inspected PDF SHA-256 is
`F83423249AB46E15411B01245311502785555788C387097B1B8C0E47BFFC5788`. No donor
implementation, shader, source asset or reference library was inspected, copied
or ported. Jerico's independent frozen contract is
`docs/superpowers/specs/2026-08-29-m8-bounded-moisture-transport-design.md`.

## Observable RED chronology

The first focused Release CPU build consumed the recipe/field RED and failed
because `GeologyMoistureTransport.hpp` did not exist. After the typed CPU
contract went green, the Vulkan test was written before backend support and
failed because moisture work/byte diagnostics, field hash and pressure samples
did not exist. The first real Vulkan pipeline then failed observably because a
seventeenth dynamic storage buffer exceeded the device's limit of sixteen.

The implementation was narrowed to one existing per-dispatch auxiliary output
slot: weathering retains the same four-float-per-voxel allocation and semantics,
while the moisture dispatch binds its separately owned one-float field to that
slot. The moisture hash uses the second pair of words in the existing layer-hash
buffer. This restored the sixteen-buffer pipeline without raising a global limit
or adding a framework. Bubble weathering, curvature weathering, bounded
inspection and Dorsey gates then consumed the compatibility path.

The final RED/GREEN sequence proves canonical schema/order/hash, exact boundary
values, zero outside the band, binary32-safe validation, deterministic seed
sensitivity, pure diff, checked work/memory rejection, cancellation without a
published result, CPU/Vulkan witnesses, bit-exact prior fields and deterministic
cook provenance.

## Frozen source, storage and transport contract

For signed distance `d <= 0`, depth `z = -d`, band depth `L`, authored effective
diffusivity `D` in m2/s and finite exposure `t` in seconds:

```text
lambda  = sqrt(D * t)
tail    = exp(-L / lambda)
profile = (exp(-z / lambda) - tail) / (1 - tail)
source  = maximumPressure * strength * mask
        * (0.75 + 0.25 * oneOctaveJericoFbm(position * frequency, seed))
p*      = clamp(source * profile, 0, 1) for 0 <= z <= L
p*      = 0 otherwise
```

`p*` is a dimensionless normalized pressure potential. `D` is an authored
effective response scale rather than permeability. Each surface-normal column
is independent: there is no tangential exchange. The surface boundary is
`p*(0) = source`, the fixed inner boundary is `p*(L) = 0`, and no initial/history
state exists. One pointwise dispatch terminates the operation; numerical
iterations are zero.

Every payload scalar and the derived `lambda` must be finite, positive where
applicable and exactly representable as binary32. Maximum pressure is in
`(0, 1]`, and `L / lambda >= 1 / 4096` keeps the normalized exponential
productive on CPU and GPU. `L` must fit inside the active Rebuild extent after
one half-voxel saturation guard; recipes whose fixed inner boundary is not
representable fail with `procedural.geology.moisture_transport_domain_exceeded`.

## Finite work, memory and identity

```text
maximum work items          67,108,864 active voxels
bytes per active voxel      4
maximum moisture bytes      268,435,456
retained moisture buffers   1 optional float field
GPU layer dispatches        1
numerical iterations        0
```

Checked admission occurs before field allocation or dispatch. Zero work and
more than 67,108,864 work items fail with stable moisture-specific diagnostics;
existing topology/device budgets include the admitted moisture bytes and are not
raised. Cancellation, invalid input or later failure returns no
`GeologyGpuEvaluationResult`, so no partial field, mesh or accepted recipe is
published.

Canonical schema-25 serialization and the existing recipe hash cover all five
payload values plus kind/order, strength, mask and full-width seed. The output
field owns a separate hash; scalar, surface and classification identities retain
their existing meanings.

## Implementation and integration

- `GeologyLayerKind::MoistureTransport = 27` and one typed five-scalar payload
  extend only the cumulative recipe/parser/writer/validator/hash path;
- `GeologyMoistureTransport.hpp` owns validation, pure diff, the pointwise CPU
  reference and checked work/memory admission;
- the existing 368-byte layer constants carry the authored controls and the
  CPU-precomputed binary32 `lambda`; the shader never forms a potentially
  underflowing or overflowing binary32 `D * t` product;
- one optional resident float field owns pressure output at exactly four bytes
  per admitted voxel;
- one existing auxiliary UAV descriptor is shared by mutually exclusive
  dispatches, so the Vulkan pipeline remains at the device limit of sixteen
  dynamic storage buffers;
- the second two words of the existing layer-hash allocation own deterministic
  moisture identity without another descriptor;
- opt-in field inspection exposes bounded pressure witnesses and the field hash
  without whole-field readback;
- the existing deterministic cook records schema-25 `recipeHash` in JRMS and
  preserves the same JRCL triangles.

The moisture float field is authoring-only and is deliberately absent from JRMS
runtime state, collision, materials, package entries and save data. A static
source-free package can carry the cooked mesh's changed recipe provenance, but
the runtime does not consume moisture in this slice. No Beauty claim is made for
an output with no renderer/material consumer.

## Release verification

Focused Release builds used the existing generated projects with project
references disabled, avoiding regeneration from unrelated foreign CMake edits.
The CPU recipe executable and all focused Vulkan executables returned zero on an
NVIDIA GeForce RTX 3090.

The moisture field witness was:

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
sample0_distance=-0.112529
sample0_pressure=0.431292
sample1_distance=-0.116616
sample1_pressure=0.453202
sample2_distance=-0.112591
sample2_pressure=0.505477
```

The identical scalar/surface hashes prove that moisture did not alter geometry.
The non-zero dedicated pressure-field hash proves useful output separately. A
seed change altered recipe and moisture identities while retaining scalar and
surface identities. Repeated evaluation and cook produced byte-identical mesh
and collision payloads; the cooked mesh records the schema-25 recipe hash.

Compatibility regressors returned:

```text
bubble weathering           EXIT=0, scalar parity max 5.20394e-09 m
curvature weathering        EXIT=0
bounded field inspection    EXIT=0, copies=11, maps=9, bytes=260056
Dorsey mineral response     EXIT=0, classification hash=474208821701615791
```

Bubble inspection accounting now matches the already-present classification
readbacks (`copy_delta=272`, `map_delta=7`, `byte_delta=2176`); its former
hard-coded values omitted those reads even though the baseline backend already
performed them. No P0 behavior was reopened or changed.

Machine-readable metrics are retained in
`docs/evidence/data/m8-geology-blueprint-p2-moisture-transport-release.txt`.

## End-to-end system completeness and data lineage

```text
BSP source + schema-25 controls + strength/mask/seed
-> canonical recipe serialization and identity
-> checked one-pass work/memory admission
-> existing resident Vulkan SDF/classification/structural/durability fields
-> separately owned bounded normalized pressure-potential field
-> bounded inspection samples + dedicated moisture hash
-> unchanged classified Surface Nets geometry/classification
-> JRMS/JRCL with schema-25 recipe provenance and unchanged static triangles
-> existing source-free static package/runtime lineage, without moisture state
```

The useful upstream pressure potential survives as an authoring field, bounded
samples and a separate identity until evaluation ends. It is not packed into an
existing classification byte and is not silently reinterpreted as water quantity
or mineral response. The Dorsey weathered-mineral classification remains intact.

Applicable and proven categories are typed source/input, recipe serialization,
identity, CPU reference/diff/admission, GPU field processing, debugging/
inspection, deterministic static cook lineage, cancellation/failure, budgets,
tests and documentation. Static renderer/material/collision products are exact
unchanged downstream results. Animation and Sequencer do not apply to this
offline field; runtime moisture simulation is outside this boundary.

### INTEGRATION REQUIRED

| Subsystem | Required data/interface | Owner/dependency | Milestone |
|---|---|---|---|
| Dedicated editor transaction/UI | Typed payload, pressure preview visualization, pure diff and existing preview/cancel/apply transaction | JericoEditor geology authoring | Later Geology editor integration |
| Mineral dissolution | Explicit pressure-field input contract, mineral inventory/units and bounded reaction output | Next ordered Geology P2 owner | `M8-GEOLOGY-BLUEPRINT-P2-MINERAL-DISSOLUTION` |
| Multi-scale microstructure | Explicit scale hierarchy and bounded synthesis inputs; no implicit pressure reuse | Later Geology P2 owner | Ordered microstructure slice |
| Runtime moisture/material response | Cooked bounded field schema and renderer/material semantics if a real product requires them | Geology plus runtime/material owners | Feature-driven production integration |
| Moisture-dependent Jolt properties | Cooked pressure-to-physical-property contract | Geology plus Jolt integration | Production Geology/physics |
| LOD/world-cell streaming | Defined pressure preservation or deliberate discard across simplification/cell cook | Geometry LOD and World Cell owners | Production streaming |
| Mutable weather/destruction/save | Stable modification identity, time/state schema, remesh/collision replacement and persistence | Future runtime modification owner | Post-blueprint production expansion |
| Navigation/gameplay/Luau/events/particles/audio | Read-only cooked moisture query/event seam if demanded by a product | Respective downstream owners | Feature-driven production slices |

Mineral dissolution and multi-scale microstructure synthesis remain the two
ordered P2/research features. Neither was implemented or silently claimed
complete. The complete Geology blueprint must close before the later PWAS/EPLS
production expansion begins.

Completing this slice does not complete P2, the Geology blueprint, the later
production expansion or JericoEngine 1.0.

Persistent Goal: `ACTIVE until full JericoEngine 1.0 closure`
