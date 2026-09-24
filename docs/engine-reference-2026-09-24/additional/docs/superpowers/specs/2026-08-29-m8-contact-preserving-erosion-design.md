# M8 Bounded Contact-Preserving Erosion Design

**Date:** 2026-08-29
**Slice:** `M8-GEOLOGY-BLUEPRINT-P2-CONTACT-PRESERVING-EROSION`
**Selected boundary:** `BOUNDED_EXPLICIT_CONTACT_DAMPED_EROSION`
**Research:**
`docs/evidence/M8_GEOLOGY_BLUEPRINT_P2_CONTACT_PRESERVING_EROSION_RESEARCH.md`

## Observable goal

Add one bounded typed erosion layer whose explicit contacts retain their input
field exactly, whose seed and authored erosion inputs change useful geometry
away from contacts, and whose exact output reaches the existing Vulkan field,
classified Surface Nets, cook, package and source-free static runtime.

The layer is an offline/static authoring operation. It does not discover
contacts, simulate physics, run neighborhood erosion, mutate runtime geometry
or create another recipe, field, mesher or cooker.

## Frozen schema and contact input

Schema 23 adds one layer kind and payload:

```cpp
enum class GeologyLayerKind : std::uint16_t
{
    // Existing values 1..24 remain unchanged.
    ContactPreservingErosion = 25U
};

struct GeologyContactPreservingErosionPayload
{
    std::vector<Vec3d> contactPointsWorld;
    double preservationRadiusMeters{0.125};
    double transitionWidthMeters{0.125};
    double maximumWearMeters{0.0625};
    double surfaceBandMeters{0.25};
    double wearFrequencyPerMeter{1.0};
};
```

Contact points are explicit designer/authoring witnesses. The layer does not
claim that it inferred physical stability or adjacency. A later placement or
physics authoring owner may generate the same typed points, but this slice adds
no such producer.

Admission is exact:

```text
layer count                         at most one in a recipe
recipe order                        after Source and one enabled Rebuild
contact count                       1..8
contact coordinates                 finite, exactly float-preserved
contact order                       strictly lexicographic and unique
preservation radius                 finite, float-preserved and > 0
transition width                    finite, float-preserved and > 0
maximum wear                        finite, float-preserved and > 0
surface band                        finite, float-preserved and >= maximum wear
wear frequency                      finite, float-preserved and > 0
header transform                    identity
header seed                         full-width and semantically active
```

The evaluator additionally rejects any contact whose protected sphere does not
intersect the finite admitted field topology. This prevents silent, paid-for
irrelevant inputs. Disabled-layer identity remains the existing layer-stack
identity behavior; it cannot publish modified field data.

Canonical TOML writes the ordered contact array and all five scalar parameters.
The existing canonical recipe hash covers every contact, parameter, header
field, mask and seed. Earlier schemas and layer/source values do not change.

## Erosion and preservation invariant

For an input signed distance `d(p)`, contact `c_i`, protected radius `a` and
transition width `w`, define:

```text
omega_i(p) = clamp((length(p-c_i)-a)/w, 0, 1)
omega(p)   = min_i omega_i(p)
```

Therefore any point at distance `<= a` from any declared contact has
`omega(p) == 0` and must return the input field exactly.

The finite surface-domain weight is:

```text
surface(p) = clamp(1 - abs(d(p))/surfaceBandMeters, 0, 1)
```

Only inside or on the current surface (`d <= 0`) is material removed. The
existing Jerico value-noise primitive supplies deterministic seed-sensitive
variation:

```text
pattern(p) = 0.75 + 0.25 * valueNoise(
    p * wearFrequencyPerMeter,
    foldFullWidthSeed(layerSeed))

removal(p) = maximumWearMeters
    * clamp(layerStrength, 0, 1)
    * clamp(maskWeight, 0, 1)
    * surface(p)
    * omega(p)
    * pattern(p)

d'(p) = d(p) + removal(p)
```

The native value-noise range keeps `pattern` in `[0.5, 1.0]`; useful erosion is
strictly positive on an enabled, unmasked surface point outside all transition
regions. There are no iterations, neighbor reads or evolving state.

The existing packed erosion classification is updated with the maximum of its
previous value and `removal / maximumWearMeters`. Geometry and semantic output
therefore share one authoritative calculation. Contact-protected samples retain
both their input distance and previous erosion classification exactly.

## Finite domain, work, memory and cancellation

The erosion domain is exactly the active voxel topology already admitted by
`GeologyGpuSession`; no infinite extent or secondary grid exists.

Hard constants:

```text
maximum contacts                    8
maximum contact-distance work       67,108,864
packed GPU bytes per contact        16
maximum retained contact bytes      128
```

Exact work is checked multiplication of active voxel count by contact count.
The packed contacts occupy a fixed 128-byte block inside the existing per-layer
constant buffer, so the layer adds no storage-buffer descriptor. Their retained
bytes are included in existing device-memory admission.
Overflow, excessive work, excessive contact bytes or field-domain mismatch
fails before allocation/dispatch.

The existing cancellation callback is checked before preflight, before contact
upload, before the layer dispatch and before result publication. A dispatched
GPU pass is not interruptible mid-dispatch, but no partial field, mesh, preview
or recipe escapes cancellation or any later failure.

Stable failure families are:

```text
procedural.geology.contact_erosion_invalid
procedural.geology.contact_erosion_contact_outside_domain
procedural.geology.contact_erosion_work_budget_exceeded
procedural.geology.contact_erosion_memory_budget_exceeded
procedural.geology.gpu_cancelled
```

Diagnostics expose contact count, exact work items and retained contact bytes in
the existing `GeologyGpuDiagnostics` result.

## Preview, diff and designer choice

`GeologyGpuEvaluationResult` remains the immutable full preview. It carries the
canonical recipe/source/final-field identities, field inspections, classified
surface and diagnostics.

A focused pure `geologyContactPreservingErosionDiff` compares two admitted
payload/seed pairs and reports contact, preservation, transition, wear, band,
frequency and seed changes without mutating either recipe.

The designer accepts or rejects the canonical recipe. Only an explicitly
submitted recipe reaches `GeologyGpuSession`. No dedicated UI, automatic Apply,
new transaction owner or bespoke preset is added.

## Existing vertical integration

The data lineage is:

```text
schema-23 ordered contact witnesses + layer seed/strength/mask
-> canonical recipe identity
-> checked finite contact-constant block and work admission
-> existing resident Vulkan signed-distance/classification ping-pong
-> contact-damped SDF + packed erosion classification
-> existing classified Surface Nets
-> existing GeologySurfaceMesh / JRMS / JRCL / material / textures
-> existing source-free package
-> existing static PBR Vulkan consumer
```

No runtime format changes are required: the existing optional geological
classification stream already carries erosion. Runtime consumes cooked Jerico
assets only; authored contacts and recipe data remain source-side.

## Observable RED and verification

The first production RED is a `GeologyGpuSessionTests` mode that builds a valid
Source -> Rebuild -> ContactPreservingErosion recipe and requires:

- exact input-field preservation at protected contact witnesses;
- useful positive erosion at surface witnesses outside transition bands;
- same recipe/seed deterministic field/surface identity;
- changed seed and changed contact position alter the relevant identity/output;
- non-empty erosion classification in the emitted Surface Nets mesh;
- exact contact/work/retained diagnostics;
- cancellation and exhausted-work failure with no result;
- non-empty classified surface through real Vulkan.

It initially fails to compile because schema 23, the layer kind, payload and
evaluator do not exist. Focused CPU RED then proves the analytic invariant,
canonical round-trip/hash/diff, invalid/ambiguous input rejection and exact
budget arithmetic.

After GREEN, the existing geology cook helper writes ordinary `.rmesh`,
`.rcoll`, `.rmat` and `.rtext` products. Package-only Vulkan replay must report
non-empty geometry, the existing cooked pipeline, zero fallback/errors and
`source_free=true`; an original-resolution frame must visibly retain the contact
while erosion changes the surrounding surface.

## End-to-end completeness disposition before implementation

| Category | Disposition |
|---|---|
| Source/input | Schema-23 explicit canonical contact witnesses; no inferred physics. |
| Asset/recipe | Existing canonical stack/hash plus one typed payload. |
| Editor/authoring | Existing immutable preview/cancel and explicit recipe choice; dedicated UI/transaction wrapper remains `INTEGRATION REQUIRED`. |
| CPU | Analytic pointwise reference, diff and checked work admission. |
| GPU | One bounded pass using the 128-byte contact block in the existing per-layer constant buffer and resident Vulkan field. |
| Field/mesh/cook/runtime | Existing erosion classification, Surface Nets, JRMS/JRCL and static cooked consumers. |
| Renderer/materials | Existing classified static PBR; no shader/material system change. |
| Physics/collision | Exact final static triangles; physical contact generation/properties remain `INTEGRATION REQUIRED`. |
| Serialization/save | Canonical authoring recipe and static products; no mutable runtime erosion state. |
| Streaming/LOD | Existing package participation; classified LOD/world cells remain `INTEGRATION REQUIRED`. |
| Destruction/runtime modification | Forbidden here and remains `INTEGRATION REQUIRED`. |
| Navigation/gameplay/Luau/events/particles/audio | Later bounded consumers remain `INTEGRATION REQUIRED`. |
| Debug/profiling/failure | Field witnesses, identities, counts, exact work/bytes and stable errors. |
| Testing/documentation | RED/GREEN, CPU/GPU parity, cook/package/runtime evidence and repeated system audit. |

## Explicit non-goals

- reopening P0/P1, block grammar, bounded search or corner-lattice construction;
- deriving contacts from Voronoi/BSP/physics or solving a stability graph;
- generic neighborhood erosion, Dorsey weathering, moisture, dissolution or
  microstructure;
- a general erosion/destruction/remesh/retopology framework;
- iterative, unlimited, distributed or learned simulation;
- VDB restoration, dedicated UI, bespoke final presets or a second field/cook;
- runtime destruction, mutable collision, save state or post-Geology expansion.

`Persistent Goal: ACTIVE until full JericoEngine 1.0 closure`
