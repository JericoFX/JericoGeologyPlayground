# M8 Geology Blueprint P2 Dorsey Narrow-Band Mineral Weathering Closure

**Slice:** `M8-GEOLOGY-BLUEPRINT-P2-DORSEY-NARROW-BAND-MINERAL-WEATHERING`

**State:** `PASS`

**Selected boundary:** `BOUNDED_INTERIOR_MINERAL_RESPONSE_BAND`

**Persistent Goal:** `ACTIVE until full JericoEngine 1.0 closure`

## Result

Jerico now admits one schema-24 cumulative geology layer as the final active
operation after Source, active Strata and active Rebuild. It consumes the existing packed Strata
mineral/material class, evaluates one finite inside-surface band, preserves the
signed-distance field and every unrelated classification channel exactly, and
publishes a seed-sensitive monotonic weathered-mineral response through the
existing deposition-response material slot, classified Surface Nets, mesh,
collision, cook, package and static Vulkan PBR consumers.

This is a bounded offline/static material classification, not moisture
transport, porous-media flow, mineral dissolution/recrystallization, scalar
erosion, an extruded slab mesh or a second geology/material pipeline. The
designer retains immutable preview/diff/cancel and explicitly chooses the
canonical recipe.

## Research-first provenance and independent design

The research record is
`docs/evidence/M8_GEOLOGY_BLUEPRINT_P2_DORSEY_NARROW_BAND_MINERAL_WEATHERING_RESEARCH.md`.
It studies Dorsey et al., *Modeling and Rendering of Weathered Stone*
(SIGGRAPH 1999), directly from the primary PDF. That source motivates a thin
surface-aligned volume carrying material/mineral state. Jerico discards the
paper's moisture, flow, transport, dissolution, recrystallization, geometry
alteration, rendering implementation and data structure.

The studied PDF SHA-256 is
`F83423249AB46E15411B01245311502785555788C387097B1B8C0E47BFFC5788`.
The frozen independent contract is
`docs/superpowers/specs/2026-08-29-m8-dorsey-narrow-band-mineral-weathering-design.md`.

## Observable RED chronology

The first CPU recipe/invariant RED was consumed from `main`: the focused
Release build failed because
`GeologyNarrowBandMineralWeathering.hpp` did not exist. The Vulkan/cook test was
then added before backend support and failed because the two required
narrow-band diagnostics did not exist. Finally, the exact CPU/Vulkan
classification-witness test failed because `GeologyGpuFieldSample` did not yet
publish `surfaceClassification`. The final ordering RED then failed because an
active Rebuild after weathering was still admitted; schema validation now
requires weathering to be the final active layer. Each missing production
contract was therefore observed by a called test before the minimum
implementation was added.

The tests required finite band confinement, matching-mineral response,
nonmatching-mineral identity, bit-exact scalar and unrelated-classification
preservation, monotonic output, full-width seed sensitivity, deterministic
repeat/cook, pure diff, invalid/schema/order/budget rejection, cancellation,
exact CPU/Vulkan witnesses and a consumed two-material Shipping replay.

## Frozen contract

- input: the existing normalized packed Strata class plus susceptible class,
  half-width, maximum response, interior band, frequency, strength, mask and
  full-width seed;
- domain: only `-interiorBandMeters <= d <= 0` as the final active operation
  after active Strata and active Rebuild;
- geometry invariant: `d' = d` bit-exactly;
- classification invariant: every byte is exact except
  `deposition' = max(deposition, quantizeUnorm8(candidate))`;
- finite termination: one pointwise pass over admitted active voxels;
- identity: canonical schema-24 TOML and existing recipe hash cover the whole
  payload/header/seed/strength/mask;
- ceilings: 67,108,864 work items, zero additional variable GPU buffers and
  zero additional retained variable bytes;
- authoring: pure typed diff plus existing immutable preview, cancellation and
  explicit designer acceptance; no automatic Apply.

## Implementation and integration

- `GeologyLayerKind::NarrowBandMineralWeathering = 26` and one typed five-scalar
  payload extend only the existing recipe variant/parser/writer/validator/hash;
- one shared CPU header owns payload validation, the pointwise reference, pure
  diff and checked work admission;
- the existing 368-byte per-layer constants carry the five controls, so no new
  GPU descriptor, buffer or global resource limit is introduced;
- the existing resident Vulkan classification field receives the monotonic
  response while scalar ping-pong remains unchanged;
- opt-in bounded field inspection now retains the two packed classification
  words, allowing three exact CPU/Vulkan witnesses without whole-field
  readback;
- classified Surface Nets, JRMS 1.1 GEOL, JRCL, package VFS and static PBR
  runtime consume the result through response material slot two.

## Release verification

Direct Release builds used the existing generated projects with project
references disabled, preventing unrelated foreign CMake changes from
regenerating into this slice. The focused CPU and Vulkan executables returned
zero.

The broader default GPU-test invocation also reached and passed the new bounded
classification inspector (`copies=11`, `maps=9`, `bytes=260056`). It remains
nonzero solely at the pre-existing canonical-BSP assertions that expect `5/5`
physical copies/maps. Baseline `HEAD` already contains those expectations while
the unchanged backend counts the additional final-hash readback (`6/6`). That
unrelated contract is outside this slice and was not rewritten or hidden.

The Vulkan field witness was:

```text
recipe_hash=8725686012333603257
field_hash=7894579602563779244
surface_hash=7894579602563779244
classification_hash=474208821701615791
baseline_classification_hash=12311128466024170702
work_items=147456
additional_buffer_bytes=0
cpu_gpu_inspection_samples=3
```

The identical scalar/surface hash is intentional evidence that weathering did
not change geometry; the packed classification hash proves useful material
output separately.

The deterministic cook reported:

```text
source_hash=18089867140996499955
recipe_hash=8725686012333603257
final_field_hash=7894579602563779244
surface_hash=7894579602563779244
vertices=5328
indices=31944
mesh_bytes=426922
collision_bytes=191970
```

JRMS 1.1 carries `geology=true`, two palette materials and response slot two
mapped to the weathered material. The seven-entry source-free package is
4,350,850 bytes with SHA-256
`47A1501DF967DAE858B2D590D065FA3E41610EB64A6C17B7A13F7A386CAA8F57`.
Shipping validation reports no ABR, brush source images, editor RBrush or
EditorAssets.

Two package-only 1280x720 Release Vulkan replays on RTX 3090 each reported one
draw, two materials, three native textures, zero fallback and
`source_free=true`. Their original-resolution frames are byte-identical with
SHA-256
`9F7F5DF374F95BDB7EA684E157142DAC8907B8E6F51A36C334706E14A2A6BA72`.
The oblique Beauty view visibly separates the pale weathered-mineral response
from the preserved brown base mineral while retaining the same silhouette:

![Dorsey narrow-band mineral weathering Vulkan Beauty](images/m8-geology-blueprint-p2-dorsey-narrow-band-mineral-weathering/narrow-band-mineral-weathering-vulkan.bmp)

Machine-readable metrics are retained in
`docs/evidence/data/m8-geology-blueprint-p2-dorsey-narrow-band-mineral-weathering-release.txt`.

## End-to-end system completeness and data lineage

```text
schema-24 mineral selector + finite response controls + seed/strength/mask
-> canonical recipe serialization and identity
-> checked one-pass work admission
-> existing Vulkan SDF + packed classification field
-> bit-exact scalar + monotonic deposition-response byte
-> classified Surface Nets
-> JRMS GEOL + exact JRCL triangles + two-material palette
-> seven-entry source-free package
-> static Vulkan PBR visual result and static collision consumer
```

The useful upstream data that survives is the original Strata class, the
weathered-mineral response, unchanged geometry, exact cooked triangles and
separate source/recipe/scalar/classification/cooked/package/frame identities.
The response byte is an authoring/material signal, not conserved transported
mass; no downstream consumer may silently reinterpret it as physical
dissolution or recrystallization.

Applicable and proven categories are source/input, recipe serialization,
identity, CPU reference/diff/admission, GPU classification processing,
renderer/material, static collision, cook/package, cancellation/failure,
debugging/inspection, budgets, tests and documentation. Animation, Sequencer
and mutable runtime simulation do not apply to this static authoring slice.

### INTEGRATION REQUIRED

| Subsystem | Required data/interface | Owner/dependency | Milestone |
|---|---|---|---|
| Dedicated editor transaction/UI | Typed payload, pure diff and existing preview/cancel/apply transaction | JericoEditor geology authoring | Later Geology editor integration |
| Production mineral taxonomy/material workflow | Stable class vocabulary, palette authoring and explicit response-channel semantics | Geology plus material/editor owners | Production Geology/material integration |
| Mineral physical properties | Cooked mineral/weathering-to-Jolt property contract | Geology plus Jolt integration | Production Geology/physics |
| Classified LOD/world-cell streaming | Strata and weathered-response preservation across simplification and cell cook | Geometry LOD and World Cell owners | Production streaming |
| Mutable runtime geology/destruction/save | Stable modification identity, remesh/collision replacement and save schema | Future destruction/runtime modification owner | Post-blueprint production expansion |
| Navigation/gameplay/Luau/events/particles/audio | Read-only cooked mineral/weathering query contract | Respective downstream owners | Feature-driven production slices |

Moisture transport, mineral dissolution and multi-scale microstructure synthesis
remain the three ordered P2/research features. None was implemented or silently
claimed complete. The complete Geology blueprint must close before the
PWAS/EPLS production expansion begins.

Completing this slice does not complete P2, the Geology blueprint, the later
production expansion or JericoEngine 1.0.

Persistent Goal: `ACTIVE until full JericoEngine 1.0 closure`
