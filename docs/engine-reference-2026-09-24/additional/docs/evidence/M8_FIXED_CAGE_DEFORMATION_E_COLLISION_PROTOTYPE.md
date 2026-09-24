# M8 Fixed-Cage Stage E — Measured Collision Prototype Evidence

Date: 2026-09-01
Slice: `M8-FIXED-CAGE-DEFORMATION-E-MEASURED-COLLISION-PROTOTYPE`
Local status: `PASS / MEASURED DECISION REJECT`
Persistent Goal: `ACTIVE until full JericoEngine 1.0 closure`

## Question being measured

Stage E does not assume that deforming collision should ship. It compares the
accepted `VisualOnly` baseline, the cheapest single-shape hypothesis and the
smallest fixed-cage cell compound that can represent a local dent. Severe
deformation remains `FractureDominant` through the accepted Stage D handoff.

No proxy count, public mode enum, runtime schema, cooked default or product
policy is admitted by this document. The final result must be an explicit
`ADMIT`, `REJECT` or `DEFER` supported by Jerico measurements.

## Current ownership and legal update seam

The repository trace before production changes establishes:

- `ColliderComponent` owns the stable collision GUID and
  `RigidBodyComponent` owns motion type, mass, friction and restitution in ECS.
- `PhysicsWorld::Impl` owns one body binding per rigid body, including the
  entity, opaque Jolt body identity, cooked collision/material lineage and the
  last private prototype publication/shape snapshot. It also owns one
  `Scene::World` pointer. Jolt shapes remain private to this adapter.
- `PhysicsWorld::addEntity` validates the alive root entity, component/collision
  GUID match, transform, motion/shape compatibility and cooked geometry before
  building and adding one Jolt body.
- `PhysicsWorld::removeEntity` removes held-body and authored constraints before
  destroying the body. Therefore remove/add is not an admissible dent-update
  seam: it would discard live constraint ownership and body state.
- The pinned Jolt `BodyInterface::SetShape` operation retains the `BodyID`, can
  recalculate dynamic mass properties, invalidates the body's contact cache,
  updates its broadphase AABB and optionally activates it. A candidate shape can
  be built and validated before this single mutation, leaving the old body
  untouched on failure.
- `FixedCageImpactResponse` publishes only when its current 27-point state
  changes. `takeDirtyPublication` consumes that dirty bit; settled state and
  same-event replay publish no work. Recovery publishes only while offsets
  change.
- Stage D retires the intact response when topology-changing fracture replaces
  it. Detached bodies continue through the existing PreparedBreakable/Jolt
  authority and never inherit the intact cage state.

The prototype may add one narrow Jerico-owned Physics operation around this
existing Jolt seam. It must not expose Jolt types or add a serialized collision
policy.

## Candidate hypotheses — not admitted

```text
VisualOnly
    base cooked collision remains unchanged
    zero collision rebuild/update work

single convex hull
    one convex proxy built from deformed cage controls
    cheapest shape-update hypothesis
    expected to lose inward local dents; measurement must confirm or refute

fixed-cage cell compound
    one convex hull per non-degenerate cell of the fixed 3x3x3 lattice
    bounded independently of render vertices/triangles
    candidate count is measured, not copied from an external implementation

FractureDominant
    topology-changing/severe response uses Stage D
    proxy complexity never grows to chase fracture
```

## Authentic RED / prototype contract

The focused RED belongs in `Tests/Physics/PhysicsWorldTests.cpp` and must fail
before a production shape-update operation exists. It must prove:

1. `VisualOnly` is named and measured as the zero-update baseline.
2. A local controlled dent is queried against the base collision, the
   single-hull hypothesis and the cell-compound hypothesis using declared
   positional tolerances.
3. The admitted prototype candidate retains one entity/body mapping, uses a
   caller-declared hard convex-part budget and never depends on render vertex or
   triangle count.
4. The candidate shape is built completely before Jolt mutation. Invalid/stale
   entity, mismatched collision GUID/material lineage, malformed geometry and
   exceeded proxy budget reject without changing the prior ray/query result,
   body count, constraint state or held-body state.
5. Repeating identical cage bytes/state returns no rebuild/update work. Rest,
   same-event replay and settled state schedule zero collision work.
6. A representative static surface produces coherent ray queries and stable
   dynamic contacts; an updated dynamic body preserves motion/body ownership and
   contacts the representative floor without remove/add.
7. Severe deformation is not represented by adding proxies; the accepted Stage
   D fracture disposition retires the intact collision owner.
8. Evidence reports actual metric kinds separately: proxy build CPU, Jolt shape
   update CPU, Physics step CPU, query/contact behavior, proxy count, retained
   body count and estimated candidate geometry bytes. Single samples are never
   labeled p50/p95.
9. A repeatable object-count sweep compares `VisualOnly`, the rejected/retained
   single-hull hypothesis and the cell-compound candidate before any decision.

Forbidden satisfactions remain per-frame triangle-mesh recook, per-frame convex
decomposition, one proxy per render feature, copied 24-box topology, soft body or
FEA.

## Authentic RED observed

Adding the focused source first caused the repository scheduling inventory to
reject an unnecessary new CTest entry. The source was therefore integrated into
the existing `physics.world_jolt` target instead of expanding the global test
inventory.

The authentic production RED then reached compilation and exited non-zero at
`FixedCageCollisionPrototypeTests.cpp`: `PhysicsWorld` had no
`updateEntityConvexShape` member and no convex-update request contract. No
production shape-update operation existed at that point.

The minimum implementation remains private to `PhysicsWorld` and is reachable
only through the focused prototype access type. It exposes no public mode enum,
runtime schema, cooked policy or Jolt type. The operation validates the live
World/entity/collision/material/budget plus an opaque object/event/generation/
state fingerprint and expected canonical candidate-geometry fingerprint from
the consumed dirty publication. Physics recomputes the actual convex geometry
fingerprint before accepting it. Exact duplicate work is a zero-build/
zero-`SetShape` result; conflicting or older generations fail. A
retired intact owner and classified triangle-material body also fail before
mutation. A complete Jolt convex shape and recursive native-byte count are built
before the single `SetShape` call.

## Pre-implementation data lineage audit

```text
cooked .rmesh fixed 3x3x3 cage/rest points/maximum displacement
+ accepted FixedCageStatePublication {object, event, generation, 27 offsets}
+ ECS Collider/RigidBody and resolved cooked .rcoll Physical Material lineage
-> dirty publication only
-> bounded candidate convex geometry in object space
-> Physics validates entity, GUID, material, transform and caller budget
-> Jolt shape built before mutation
-> one BodyID-preserving SetShape at the Physics safe seam
-> broadphase/contact-cache refresh and optional dynamic activation
-> representative ray/contact result

severe/topology-changing state
-> Stage D PreparedBreakable bond/support/materialization authority
-> intact owner retired
-> no later cage collision update
```

Useful upstream information preserved downstream is object identity, collision
GUID, Physical Material identity/definition hash, fixed-cage generation/bytes,
motion ownership, proxy budget and exact metric kind. No render topology or
source asset path becomes runtime collision authority.

## Pre-implementation end-to-end system completeness audit

| Area | Stage E applicability and required result |
|---|---|
| Source/input data | Use only admitted cooked cage/collision/material data plus runtime dirty publications |
| Asset pipeline | No new cook or source format for the measurement; source-free proof is required only if a production serialized seam is admitted |
| Editor/authoring | Not applicable in Stage E; `INTEGRATION REQUIRED` remains Stage F |
| ECS/components | Existing Collider/RigidBody ownership must remain unchanged; no second collision component |
| Runtime representation | Bounded transient convex geometry only; no public mode/schema before the decision |
| CPU processing | Dirty-event work only; settled/replay/rest state must show zero rebuilds |
| GPU/renderer/shaders | Existing Stage A-C rendering is unchanged; GPU measurements remain separate from collision metrics |
| Materials | Existing cooked Physical Material identity/hash must match; no new material authority |
| Physics/collision | One BodyID-preserving Jolt shape update after full validation; no remove/add dent path |
| Animation | Not applicable to this static-mesh Stage E prototype |
| Luau/events | Not a collision writer; the accepted causal event identity remains upstream |
| Sequencer | Not an owner of collision or deformation state |
| Serialization/save-load | Existing Stage C state remains authority; no new serialized policy during measurement |
| Streaming/package | No new package claim unless final decision admits runtime/cooked integration |
| LOD | Candidate work is independent of render LOD/vertex count |
| Destruction/runtime modification | Severe deformation and owner retirement remain Stage D PreparedBreakable behavior |
| Navigation/gameplay queries | Ray/query approximation and declared limits must be measured; nav integration is `INTEGRATION REQUIRED` only after an `ADMIT` decision |
| Particles/audio | Not causal owners and not applicable to the bounded prototype |
| Debug/visualization | Text metrics and typed errors now; editor visualization remains Stage F |
| Profiling/budgets | Separate build, SetShape, step, query/contact, count and memory metric kinds over an object-count sweep |
| Testing | Authentic compile RED and focused Physics runtime test; the existing Stage D suite proves the actual coordinator retirement, while Stage E separately proves its private retired/stale-owner guards and deterministic replay/no-churn |
| Failure handling | Stale entity, GUID/material mismatch, invalid geometry, budget/capacity and retired owner reject atomically |
| Documentation | This evidence is updated with RED output, measurements, limits and final `ADMIT`/`REJECT`/`DEFER` |

## Required later integrations — decision dependent

`INTEGRATION REQUIRED` only if Stage E ends in `ADMIT`:

- subsystem: runtime scheduling/instantiation and package validation;
- input: admitted candidate geometry contract and fixed-cage dirty publication;
- expected interface: event-driven collision update with truthful fallback;
- owner/milestone: Physics + Destruction runtime, then Stage F tooling;
- dependency: measured contact tolerance, proxy/work budgets and source-free
  package proof.

Stage F editor work, optional Stage G and M9 remain outside this slice.

## Measured contact and locality result

The controlled top-center dent has an expected query height of `0.68 m` with a
declared `0.06 m` tolerance:

| Representation | Query height | Local dent result | Native shape bytes/object |
|---|---:|---|---:|
| `VisualOnly` base box | `1.00 m` | Explicit approximate baseline | `0` added |
| Single convex hull | `1.00 m` | `REJECT`: inward control is inside the hull | `672` |
| Fixed-cage cell compound | `0.68 m` | Fidelity `PASS` | `6,248` |

The cell compound retained one body mapping and the same opaque BodyID token
before/after every update. A dynamic probe settled on the
updated static collision at `y=0.852522 m`; its subsequent 30-step vertical
delta was exactly `0`. Updating a held dynamic body retained the same BodyID and
held constraint, then followed a moved target after 90 fixed steps before
explicit release. After release it settled stably on a separate representative
floor beneath the moved body.

Invalid collision identity, changed Physical Material lineage, malformed
geometry, classified triangle-material lineage and a budget smaller than the
candidate part count all rejected while body count and the previous `0.68 m`
query result remained unchanged. Incomplete/wrong-object, conflicting/older and
retired publications plus mismatched published-state/candidate geometry also
rejected. Repeating the exact publication/candidate
returned `changed=false`, zero build CPU and zero Jolt update CPU. Consumed dirty
publication plus same-event replay produced no later collision work.

The Stage E retired-owner check is deliberately a private caller-provided
prototype guard. Actual topology-changing retirement remains proven by
`physics.prepared_breakable_bond_graph_runtime`; stale/removed intact Physics
owners independently reject later updates here. No new production coordinator
or scheduler contract is admitted after the `REJECT` decision.

## Three-sample Debug count sweep

Each row is the observed range of three independent Debug samples. Ranges are
not p50/p95. Every object count contains one static candidate surface and one
dynamic contact probe per candidate object; Physics time is the total for 120
fixed steps.

| Candidate | Objects | Total proxies | Build + SetShape CPU | Jolt SetShape CPU | Physics 120 steps | Native shape bytes |
|---|---:|---:|---:|---:|---:|---:|
| `VisualOnly` | 1 | 0 | `0 ms` | `0 ms` | `3.5154-3.6040 ms` | 0 |
| Single hull | 1 | 1 | `0.1188-0.1251 ms` | `0.0077-0.0078 ms` | `3.5505-3.6082 ms` | 672 |
| Cage cells | 1 | 8 | `0.6351-0.6675 ms` | `0.0123-0.0130 ms` | `10.6301-10.8136 ms` | 6,248 |
| `VisualOnly` | 16 | 0 | `0 ms` | `0 ms` | `24.7316-25.1824 ms` | 0 |
| Single hull | 16 | 16 | `1.7344-1.7752 ms` | `0.1062-0.1080 ms` | `24.7687-25.1847 ms` | 10,752 |
| Cage cells | 16 | 128 | `9.8960-10.0913 ms` | `0.1818-0.1990 ms` | `111.511-113.195 ms` | 99,968 |
| `VisualOnly` | 64 | 0 | `0 ms` | `0 ms` | `95.7450-98.1362 ms` | 0 |
| Single hull | 64 | 64 | `7.0021-7.3213 ms` | `0.4343-0.4470 ms` | `98.3837-98.5844 ms` | 43,008 |
| Cage cells | 64 | 512 | `40.3149-40.6019 ms` | `0.7339-0.7510 ms` | `443.748-448.752 ms` | 399,872 |

The faithful candidate adds approximately `0.059-0.060 ms`,
`0.719-0.736 ms` and `2.900-2.926 ms` per fixed step at 1, 16 and 64 objects
respectively. At 64 objects its Physics-step cost is approximately `4.56-4.63x` the
explicit `VisualOnly` baseline. Most event cost is convex construction rather
than `SetShape`; most ongoing cost is contact/compound traversal rather than the
event-driven update itself.

## Stage E decision — `REJECT`

`REJECT` admitting fixed-cage collision following as an M8 runtime policy or
default.

Reasoning:

1. The one-proxy hull is cheap and scales like the baseline, but it cannot
   represent the intended inward local dent at all.
2. The smallest tested representation that does preserve the dent is the natural
   eight-cell cage compound. It is deterministic, bounded and contact-coherent,
   but its fixed-step cost grows to about `4.6x` baseline and `2.9 ms/step`
   additional cost at 64 objects in this small fixture.
3. Admitting the compound would require a production scheduler, cooked/runtime
   policy, source-free package proof, navigation/query limits and Stage F tooling.
   The measured cost does not justify introducing those contracts.
4. `VisualOnly` therefore remains the truthful default: zero collision updates,
   ordinary cooked collision and explicit approximation for small dents.
5. Severe/topology-changing response remains `FractureDominant` through the
   accepted Stage D PreparedBreakable handoff. Proxy complexity never grows.

This decision rejects the measured Stage E production admission; it does not
silently promise a future higher-fidelity tier. Any later reconsideration needs
new product evidence and a new legal slice.

## Final end-to-end system completeness audit

| Area | Final Stage E disposition |
|---|---|
| Source/input and lineage | One consumed dirty publication is bound to live object, event, generation, state fingerprint, expected convex geometry fingerprint, collision GUID and Physical Material lineage; missing, conflicting, older and mismatched work fails |
| Asset pipeline/runtime formats | No new cook path, `.rcoll` schema, mode enum, serialized default or source path was admitted |
| ECS/runtime ownership | Existing Collider/RigidBody and one PhysicsWorld BodyID remain authoritative; the prototype does not add a component or second collision owner |
| CPU/GPU | Proxy build, Jolt update and Physics-step CPU are measured separately; existing Stage A-C GPU behavior is unchanged |
| Physics/materials | `SetShape` retains BodyID and live held constraints; static/dynamic contact fixtures remain stable; classified per-triangle materials fail closed instead of losing lineage |
| Destruction | The existing Stage D coordinator remains the topology-changing authority. Its regression proves actual retirement; Stage E proves only its private retired/stale-owner collision guards |
| Serialization/package/streaming | The `REJECT` result admits no runtime scheduler or package payload. The accepted Stage D source-free package gate remains green |
| Editor/tooling/debug | Collision-proxy authoring is not required after `REJECT`. Stage F still owns Inspector/cage visualization and production hardening of the accepted VisualOnly/FractureDominant behavior |
| Navigation/gameplay/audio/particles | No new approximation contract is admitted and therefore no downstream collision-proxy integration is required |
| Failure handling/testing | Typed atomic failures cover entity/body/collision/material/publication/geometry/budget/retirement; real queries, contacts, BodyID, held constraints, deterministic no-churn and 1/16/64-object samples are exercised |
| Documentation/decision | Limits, raw metric kinds and the explicit `REJECT` decision are recorded here; no future tier is promised |

There are no Stage E `INTEGRATION REQUIRED` items for the rejected collision
candidate. Stage F remains required for the already accepted fixed-cage editor
and production-hardening scope, not for exposing this rejected proxy.

## Final verification

```text
authentic RED: missing private Physics shape-update contract              PASS
Debug build: four focused Physics/PreparedBreakable targets              PASS
repository scheduling + focused runtime/package regressions              PASS 5/5, 3.85 s
physics.prepared_breakable_source_free                                   PASS 1/1, 101.29 s
focused executable contact/query/BodyID/constraint checks                PASS
focused executable Debug samples                                         PASS 27 rows (3 x 3 candidates x 3 counts)
independent read-only review                                              PASS, 0 Critical / 0 Important
git diff --check                                                         PASS
foreign-work preservation                                                PASS 185 / exact SHA-256
```

The final Git whitespace check and exact 185-entry foreign-work preservation
check are recorded in the published slice checkpoint.

Persistent Goal: ACTIVE until full JericoEngine 1.0 closure.
