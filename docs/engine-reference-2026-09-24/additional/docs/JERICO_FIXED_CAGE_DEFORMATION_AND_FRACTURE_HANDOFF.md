# JericoEngine — Fixed-Cage Deformation and Fracture Handoff

**Status:** Design companion / future implementation contract  
**Date:** 2026-08-31  
**Scope:** Cheap deterministic bend/crush/dent response for eligible static geometry  
**Extends:** `docs/JERICO_PHYSICAL_MATERIAL_FRACTURE_DESTRUCTION.md` and `docs/JERICO_PHYSICAL_MATERIAL_LOCALIZED_DESTRUCTION.md`  
**Active-slice rule:** This document does **not** change `docs/work/ACTIVE_SLICE.md` and does not authorize implementation outside the currently legal slice.

---

# 0. Executive decision

Jerico should support a bounded deformation stage between intact rigid geometry and structural fracture for assets whose intended response includes dents, bends, crushing or limited elastic recovery.

The preferred baseline is **not** a general soft-body solver and **not** runtime CPU mesh rebuilding.

The preferred baseline is:

```text
admitted impact / load event
        ↓
resolve authoritative Physical Material + asset deformation policy
        ↓
evaluate bounded elastic / plastic response
        ↓
update a fixed 3×3×3 object-space control cage
        ↓
render deformation in the vertex stage
        ↓
if the admitted fracture condition is exceeded:
    hand the same causal event/state to existing fracture/destruction authority
        ↓
    break bonds / detach unsupported components
        ↓
    Jolt simulates detached rigid bodies
```

The fixed cage contains exactly:

```text
3 × 3 × 3 = 27 control points
```

Its dimensions are a deliberate performance contract for the first implementation. Do not turn the baseline into an arbitrary-resolution lattice.

This capability exists to produce convincing low-cost deformation for selected objects such as:

- sheet metal;
- lockers and cabinets;
- thin doors and panels;
- barrels and cans;
- vents and grilles;
- cardboard and crushable containers;
- selected timber or thin composite props where a bounded bend is appropriate;
- other static-mesh props whose deformation can be represented adequately by a coarse volume cage.

It is not the universal solution for:

- cloth;
- characters or skeletal soft tissue;
- ropes/strands;
- fluids;
- granular material;
- geological fracture;
- concrete/rock destruction whose primary response is cracking, spall and separation;
- arbitrary continuum simulation.

---

# 1. External reference and independent Jerico design

This design was prompted by the public Unreal Engine community example:

```text
https://www.reddit.com/r/UnrealEngine5/comments/1w3ao6a/
```

The author describes a runtime deformation system based on:

- a fixed 27-point cage around a static mesh;
- vertex-stage position deformation;
- impact-driven changes to cage points rather than CPU mesh rebuilding;
- no general soft-body solver;
- a fixed 3×3×3 lattice so shader work can remain statically bounded;
- optional moving primitive collision proxies;
- distance disabling and sleeping of inactive CPU-side work.

Those observations are useful as public research evidence, but Jerico must implement its own architecture, math, data layout, shaders and Jolt integration.

Hard rule:

```text
Do not copy plugin code, shaders, data formats, APIs or Unreal-specific structure.
```

Jerico adopts only the general idea of a **small fixed deformation lattice driven by deterministic impact math**, then implements it independently through Jerico/Diligent/Jolt boundaries.

---

# 2. Authority split remains unchanged

This feature must fit the authority already established by the Physical Material and destruction documents.

```text
Visual Material
    -> appearance
    -> optional wrinkle/crumple detail inputs

Physical Material
    -> contact response
    -> material resistance / absorption / ductile-vs-brittle character
    -> future admitted plastic/yield response inputs when an executable slice needs them

Asset deformation policy/profile
    -> whether this asset may visually/physically deform
    -> cage domain / limits
    -> deformation scale and permitted modes
    -> collision approximation policy

Fracture Profile
    -> prepared break geometry
    -> chunks
    -> bonds
    -> anchors/support
    -> weak/protected zones

Impact Descriptor
    -> causal spatial/physical event

Fixed-Cage Deformation
    -> bounded pre-fracture shape response

Runtime Destruction
    -> local bond failure / structural detachment

Jolt
    -> rigid-body contacts and detached-body simulation
```

Do **not** introduce duplicate material authorities such as:

```text
DeformationMaterial
DentMaterial
SoftBodyMaterial
MetalDamageMaterial
```

if their properties belong to the existing Jerico Physical Material.

Likewise, do not create a second weapon-specific or deformation-specific impact event when the existing generic impact/destruction descriptor can be extended or reused cleanly.

---

# 3. Product goal: a bounded response ladder

Jerico should be capable of expressing the following conceptual response ladder when the asset/material combination allows it:

```text
intact
  ↓
small elastic response
  ↓
recover toward rest

or

intact
  ↓
yield
  ↓
plastic dent / bend / crush
  ↓
persistent deformed state

or

intact / already deformed
  ↓
higher admitted structural load
  ↓
fracture threshold / bond failure reached
  ↓
existing localized destruction path
  ↓
detached rigid pieces handled by Jolt
```

Not every material traverses every state.

Examples:

```text
thin ductile sheet
    -> plastic deformation is likely before separation

brittle glass
    -> fracture can dominate with little or no cage deformation

concrete / rock
    -> localized fracture/spall usually dominates; cage path normally disabled

cardboard
    -> large visible crush may be allowed without expensive continuum simulation
```

The response is driven by quantitative data and asset policy, not by branching on names such as `Metal`, `Wood` or `Cardboard`.

---

# 4. Fixed 3×3×3 lattice contract

The baseline lattice is always:

```text
C[i][j][k]

i ∈ {0,1,2}
j ∈ {0,1,2}
k ∈ {0,1,2}
```

Total:

```text
27 points
```

Each eligible mesh has a rest cage in object/local space and a deformed cage state.

Prefer storing deformation as offsets from rest:

```text
DeltaC[i][j][k] = CurrentC[i][j][k] - RestC[i][j][k]
```

Benefits:

- zero offsets represent the ordinary undeformed mesh;
- deterministic clamping is straightforward;
- reset is cheap;
- save/network state can encode only deformation state or causes;
- shader input can be compact and object-relative.

The default cage domain is derived from the asset local bounds.

Authoring may later allow a designer-adjusted cage domain for assets where the raw AABB is a poor deformation volume, but the baseline remains one fixed 3×3×3 cage.

Do not add arbitrary point counts in the first production implementation.

If a later measured product requirement genuinely needs higher-resolution deformation, it must be a separate design decision with its own performance evidence.

---

# 5. Independent deformation math

Jerico can implement the lattice as its own quadratic free-form deformation volume.

For a mesh vertex `P`, first map the rest/object-local position to normalized cage coordinates:

```text
u, v, w ∈ [0, 1]
```

A suitable quadratic Bernstein basis is:

```text
B0(t) = (1 - t)^2
B1(t) = 2 t (1 - t)
B2(t) = t^2
```

Then a straightforward offset form is:

```text
P' = P + Σ_i Σ_j Σ_k
         B_i(u) * B_j(v) * B_k(w) * DeltaC[i][j][k]
```

for `i,j,k ∈ {0,1,2}`.

This is a Jerico-owned mathematical choice. It is not a claim about the exact internal formula used by the referenced Unreal plugin.

Implementation may use an algebraically equivalent factorization if profiling proves it cheaper.

Required properties:

- deterministic for identical inputs;
- no dynamic lattice-size loop;
- no CPU vertex rewriting;
- no per-frame topology generation;
- bounded memory per deformed object;
- stable behavior across supported rendering backends through Diligent shader conventions.

---

# 6. Impact-to-cage response

The cage update should be event-driven.

Conceptual input:

```text
ImpactDescriptor
{
    position
    surfaceNormal
    direction
    impulse / admitted energy proxy
    affectedRadius
    provenance
}
```

Conceptual CPU-side cage update:

```text
impact
    -> transform event into object/cage space
    -> compute normalized hit location
    -> evaluate a bounded influence for 27 control points
    -> derive deformation amplitude from admitted physical response
    -> move only affected control points
    -> clamp against asset/material deformation limits
    -> mark cage state dirty once
```

A cheap deterministic polynomial/smooth radial falloff is preferred over unnecessary iterative solving.

The exact response function, units and normalization must be frozen by an implementation slice with tests. The design requirement is that it remains a **small fixed event calculation over 27 points**, not a hidden general solver.

Do not evaluate all vertices on the CPU to decide a dent.

Do not run a continuous stress solver merely because the material is ductile.

---

# 7. Physical Material: add only the physics we actually need

The existing Physical Material authority already contains or plans concepts such as:

```text
density
structural strengths
fracture toughness
brittleness
ductility
impact absorption
penetration resistance
optional anisotropy
```

A future deformation slice may require one or more explicit concepts describing the onset and persistence of plastic deformation, for example:

```text
yield resistance / yield threshold
plastic response scale
maximum admitted plastic strain/deformation
optional recovery character
optional hardening-like response if evidence needs it
```

Exact names and units are **not frozen by this document**.

Important performance/science guardrail:

Jerico does **not** need to add a full engineering material model such as Young's modulus, Poisson ratio, continuum stress tensors or finite-element state merely to make a locker dent convincingly.

Add a physical field only when an executable response test needs it and there is a clear owner, unit and consumer.

The cheapest coherent model wins.

---

# 8. Deformation policy belongs to the asset, not only the material

A material may be ductile while a particular asset should not use cage deformation.

Examples:

```text
PhysicalMaterial = MildSteel

Asset A: thin locker door
    deformation policy -> Enabled

Asset B: massive structural column
    deformation policy -> Disabled or fracture/structural path only
```

Therefore the owning asset/component needs an explicit deformation policy.

Conceptual authoring fields:

```text
Deformation
    Enabled
    Mode: FixedCage3x3x3
    MaxDisplacement
    LocalInfluenceRadius
    RecoveryMode
    RecoveryRate / time characteristic when applicable
    CollisionMode
    DeformDisableDistance
    optional cage-domain override
```

These are conceptual controls. Exact persisted schema belongs to the future implementation slice.

Do not encode geometric facts such as panel thickness solely into a material display name.

---

# 9. Fracture handoff is causal, not a second damage system

The cage system must not become an isolated `dentHealth` system.

The same admitted event should be capable of progressing through deformation into existing fracture authority.

Preferred conceptual flow:

```text
ImpactDescriptor
        ↓
Physical Material + geometry + deformation policy
        ↓
response classification
        ├── below deformation threshold
        │       -> no permanent shape change
        │
        ├── elastic range
        │       -> temporary cage offset / recovery
        │
        ├── plastic range
        │       -> persistent cage offset
        │
        └── admitted fracture response
                -> evaluate existing local fracture graph
                -> fail causal bonds only
                -> support/anchor traversal only if topology changes
                -> detached components become Jolt bodies
```

A stronger event does not require the cage to reach some arbitrary `100% dent` value before fracture can occur.

Fracture remains a structural decision based on the existing Physical Material + geometry + fracture/bond topology.

The deformation result may contribute to later fracture evaluation if a real slice proves that dependency useful, but it must not silently replace structural truth.

---

# 10. Rendering integration through Diligent

The productive path should use the Jerico rendering abstraction and Diligent shader pipeline.

Conceptually:

```text
StaticMesh instance
    -> deformation-enabled instance flag/index
    -> fixed cage data buffer
    -> vertex-stage deformation function
    -> normal material/shading path
```

Hard rules:

1. Ordinary non-deformable static meshes must not pay the 27-point deformation cost.
2. No CPU mesh copy/rebuild per impact.
3. No skeletal-mesh conversion merely to obtain deformation.
4. No special UVs or vertex colors should be required for the baseline cage.
5. Deformation must use object/cage-local coordinates so all supported LODs can reproduce the same shape response.
6. Deformed bounds must be represented conservatively for culling and shadows.
7. Any depth/shadow pass that needs the deformed silhouette must consume the same cage deformation path.

Shader permutation vs runtime branch is an implementation choice that must be based on actual pipeline/PSO and GPU measurements. The contract is only that undeformed objects retain a cheap path.

---

# 11. Cage state data and memory

A raw baseline state of:

```text
27 × float3 offsets
```

is:

```text
27 × 12 bytes = 324 bytes
```

before alignment/container overhead.

That is small enough to be practical for selected objects, but Jerico should still avoid attaching live deformation state to every static mesh in the world.

Preferred ownership:

```text
object opts into deformation
    -> allocate/register cage state

ordinary static mesh
    -> no cage state
```

Possible later compression such as half-precision offsets may be considered only if visual precision and platform support justify it.

Do not optimize storage speculatively before measuring the real visible-object and bandwidth costs.

---

# 12. Surface detail: large shape vs fine crumple

The cage is intentionally coarse.

It should own:

```text
large-scale bend
large-scale dent
crush silhouette
panel displacement
```

Fine wrinkles should normally come from shading rather than a denser physical lattice.

A future material path may derive a compression/crumple mask from cage deformation and use it to blend:

- wrinkle normal maps;
- roughness variation;
- localized paint cracking/damage masks;
- other purely visual microdetail.

Keep the distinction explicit:

```text
cage -> macro shape
shading -> micro detail
```

Do not increase cage resolution merely to create tiny wrinkles that a normal/detail material can represent more cheaply.

---

# 13. Normal/tangent quality is a measured tier, not a hidden cost

Large vertex displacement can make undeformed normals look incorrect.

The first implementation should not automatically add expensive general normal reconstruction to every deformable mesh.

Preferred staged policy:

```text
baseline
    -> bounded deformation amount
    -> original geometric normals
    -> optional crumple/detail normal shading

if visual evidence proves insufficient
    -> evaluate analytic/approximate normal correction from the deformation field
    -> measure GPU cost
    -> enable only where justified
```

A correct optional normal-deformation tier is preferable to silently paying for it on every object.

---

# 14. Motion vectors and temporal rendering

While cage points are actively changing, temporal rendering may need previous deformation state to produce correct motion vectors.

The future renderer integration must explicitly inspect:

- motion-vector generation;
- TAA/temporal reconstruction;
- shadow temporal behavior;
- any velocity-dependent post effect.

Do not store previous cage state forever if an object is settled and no renderer consumer needs it.

Active deformation can retain transient previous-frame state; settled persistent dents can collapse back to one current state.

---

# 15. Bounds and LOD contract

A deformed mesh can move outside its original asset bounds.

The system must avoid culling/shadow popping by using a conservative deformation allowance.

Conceptually:

```text
RenderBounds = RestBounds expanded by admitted MaxDisplacement
```

The exact bound update strategy must remain cheap and deterministic.

LOD meshes should evaluate the same normalized cage domain so a dent does not jump to a different location when the LOD changes.

Do not require identical topology between LODs.

---

# 16. Collision strategy: do not blindly copy the 24-box reference

The public Unreal reference reports a moving primitive-proxy approach using multiple collision boxes. Jerico should treat that as evidence that coarse collision can follow cage deformation without runtime mesh recooking, **not** as a number or topology to copy.

Twenty-four moving proxies per object would scale poorly if enabled indiscriminately.

Jerico should expose explicit collision tiers.

Conceptual modes:

```text
VisualOnly
    -> render shape deforms
    -> ordinary/base collision remains
    -> valid only when small mismatch is acceptable

CoarseProxy
    -> bounded small set of Jolt-friendly primitive/compound proxies
    -> proxies update only when cage state changes
    -> count and update cost are measured

FractureDominant
    -> severe deformation does not chase an increasingly complex collision shape
    -> admitted structural failure hands off to fracture/destruction instead
```

Forbidden baseline:

```text
per-frame triangle-mesh recook
per-frame convex decomposition
one collision primitive per visual feature
24 proxies by default because another engine/plugin used 24
```

The exact Jolt shape-update seam must be selected only after inspecting current Jerico Physics ownership and measuring the cheapest legal option.

---

# 17. Event-driven lifetime and sleeping

CPU work must be event-driven.

Preferred lifecycle:

```text
Undeformed / Settled
    -> no deformation tick

Impact arrives
    -> bounded 27-point update
    -> dirty state uploaded

Elastic recovery active
    -> short-lived scheduled updates only while recovery is changing state

Plastic settled state
    -> no CPU deformation tick
    -> persistent cage data remains available to rendering

New impact
    -> reactivate bounded update
```

A settled dent may still cost vertex shader work while visible. Therefore:

```text
CPU asleep != GPU free
```

Jerico must report those costs separately.

---

# 18. Distance and scalability tiers

The system must have a distance/scalability policy from the start.

Conceptually:

```text
Near
    -> full visual cage deformation
    -> collision mode selected by gameplay need

Mid
    -> visual deformation retained if useful
    -> collision may simplify or stop updating if gameplay permits

Far
    -> deformation shader path may be disabled or replaced by cheaper baked/undeformed representation
```

Exact distances must come from project scalability settings and profiling, not be hard-coded because the reference plugin used a particular default distance.

Deformation distance, collision distance and object simulation activity are separate controls.

---

# 19. Network and deterministic replay

JericoEngine is generalist, so deterministic compact state is valuable even if the first product is primarily single-player.

Preferred future network principle:

```text
replicate cause or compact cage state
not deformed vertex buffers
```

If the impact-to-cage function is proven deterministic across supported platforms, a network implementation may replicate a compact admitted event and reconstruct the cage result locally.

If cross-platform floating-point behavior makes exact event replay unsafe, replicate a quantized cage/result state instead.

Do not assume deterministic networking merely because the formula is deterministic in one local test.

---

# 20. Save-game and authored pose semantics

Persistent runtime dents may matter to save/load.

The future save contract should store the smallest authoritative state needed to reconstruct the visible/physical deformation.

Possible choices:

```text
quantized 27-point offsets
or
stable sequence of admitted deformation events
```

The implementation slice must choose based on determinism, save size and compatibility.

Separately, JericoEditor may later support an authored/baked deformation pose for set dressing or cinematics.

An authored pose is an asset-authoring feature. It must not be confused with runtime structural damage state.

---

# 21. Editor authoring direction

When this capability becomes a legal editor slice, the ordinary Inspector should expose it without creating a separate external deformation application.

Conceptual panel:

```text
Deformation

Enabled                 [x]
Mode                    Fixed Cage 3×3×3
Physical Material       <resolved authoritative reference>
Max Displacement        [...]
Influence Radius        [...]
Recovery                None / Elastic
Collision               Visual Only / Coarse Proxy / Fracture Dominant
Distance Tier           <project/scalability driven>

[ Show Cage ]
[ Detailed Cage Handles ]
[ Reset Pose ]
[ Preview Impact ]
[ Apply ] [Cancel]
```

The exact UI must follow JericoEditor conventions:

- preview/diff;
- cancel;
- undo/redo;
- non-destructive source handling;
- deterministic authoring;
- no duplicated Physical Material definition.

Do not implement this UI during the current Phase F slice merely because the contract exists.

---

# 22. Asset eligibility rules

A fixed cage is appropriate when the required visual deformation is smooth/coarse over the object volume.

Good candidates:

```text
thin metal panel
locker/cabinet
barrel/can
cardboard box
vent/grille
simple crushable prop
```

Poor candidates:

```text
very long articulated object needing many independent bends
highly concave object where one global AABB cage causes unacceptable influence
cloth-like surface requiring folds
skeletal character
object whose dominant response is brittle fracture
complex geological body
```

When the cage cannot preserve the intended result cheaply, Jerico should choose another admitted system rather than escalating this one into a universal simulator.

---

# 23. Performance contract

This feature exists specifically because the intended result can be obtained more cheaply than general soft-body simulation.

Binding performance rules:

1. No CPU vertex deformation loop.
2. No general soft-body/FEA solver.
3. No runtime topology work for ordinary dents.
4. Exactly 27 cage points in the baseline implementation.
5. Cage response is event-driven.
6. Stable plastic deformation performs zero CPU cage updates until another event.
7. Ordinary non-deformable meshes retain the ordinary static-mesh render path.
8. Collision deformation is optional and separately budgeted.
9. Distance/visibility scalability is mandatory.
10. GPU cost is measured against visible vertex count, not only object count.
11. CPU, GPU, memory and Jolt proxy costs are reported separately.
12. No claimed performance number is accepted without a Jerico benchmark.

The Unreal reference author reports a fixed 27-point vertex-shader path and a material instruction count for that plugin. Those numbers are external evidence only. Jerico must profile its own generated shaders on its own renderer and target hardware.

---

# 24. Required profiling evidence

A future implementation slice must produce repeatable evidence for at least:

```text
baseline static mesh vs cage-enabled mesh

vertex-count sweep
    low
    medium
    high

visible deformable-object sweep
    representative small count
    representative medium count
    stress count

states
    idle undeformed
    actively deforming
    settled plastic deformation

collision
    disabled
    cheapest admitted proxy mode

renderer
    main pass
    depth/shadow passes that consume deformation
```

Report at minimum:

```text
CPU update time
GPU frame/pass delta
uploaded deformation bytes
visible vertex count
active cage count
settled cage count
collision proxy count
Jolt shape/proxy update time when enabled
VRAM/buffer footprint where meaningful
```

Do not approve the feature based only on one attractive dent demo.

---

# 25. Required behavioral tests

The eventual implementation must prove at least the following.

## 25.1 Rest identity

Zero cage offsets produce the exact normal static-mesh position path within defined numeric tolerance.

## 25.2 Deterministic event response

Identical admitted inputs produce identical cage state on repeated runs for the supported deterministic boundary.

## 25.3 Locality

A left-side impact affects the expected local cage neighborhood more strongly than the opposite side.

## 25.4 Energy/strength ordering

A stronger admitted impact cannot produce less plastic deformation than an otherwise identical weaker impact unless an explicit material/model rule explains it.

## 25.5 Material distinction by data

Two Physical Materials with distinct quantitative response values can produce distinct deformation/fracture outcomes without branching on display names.

## 25.6 Asset-policy distinction

The same Physical Material can be deformable on one asset and deformation-disabled on another.

## 25.7 Elastic recovery

When recovery is enabled, the cage approaches its admitted rest/plastic target and stops scheduling CPU updates when settled.

## 25.8 Plastic persistence

A plastic dent remains after CPU deformation work sleeps.

## 25.9 Fracture handoff

A controlled stronger event can cross into the existing fracture path without inventing a second global-health destruction system.

## 25.10 Bounded work

One impact updates a bounded fixed cage and does not traverse unrelated world objects.

## 25.11 Distance disable

The selected far-distance policy removes the intended deformation render cost without corrupting persistent authoritative state.

## 25.12 LOD consistency

The same cage state deforms supported LODs coherently in the same object-space region.

## 25.13 Bounds correctness

Maximum admitted deformation does not cause erroneous frustum/shadow culling.

## 25.14 Collision policy

Visual-only mode is explicitly identified as approximate. Any collision-following mode proves bounded proxy count and coherent contact behavior without runtime triangle-mesh recook.

---

# 26. Future implementation order

This order is intentionally staged and **must not preempt the current active slice**.

When roadmap/slice authority admits this feature, prefer:

```text
Stage A — Visual mathematical proof
    fixed 3×3×3 rest cage
    object-space shader deformation
    deterministic direct cage offsets
    bounds/LOD/shadow witness
    no collision deformation

Stage B — Event-driven impact response
    reuse admitted generic impact descriptor
    27-point CPU cage update
    settled sleep/no-tick behavior
    Physical Material quantitative response seam

Stage C — Plastic + optional elastic response
    persistence
    recovery scheduling
    explicit limits
    save-state implications identified

Stage D — Fracture handoff
    same causal event
    no duplicate damage authority
    existing localized bond/support path

Stage E — Collision prototype
    inspect Jolt shape ownership
    prototype cheapest bounded proxy strategy
    benchmark before admitting a default

Stage F — Editor authoring / production hardening
    inspector
    cage visualization
    preview/cancel/apply/undo/redo
    scalability
    profiling/debug views
    source-free cooked behavior where required

Stage G — Optional later features only if product evidence needs them
    deformation-aware normal correction
    authored deformation poses
    compact networking
    higher-fidelity collision tier
```

Do not start at Stage E with a complex collision proxy system before proving the visual/event model.

---

# 27. Codex guardrails

When Codex encounters this document:

1. Read it only when the active slice concerns physical deformation, impact response, static-mesh deformation, fracture handoff or directly affected renderer/physics integration.
2. Do not change `ACTIVE_SLICE` just because this document exists.
3. Do not implement it opportunistically during unrelated Phase F/Geology work.
4. Reuse the existing Physical Material authority.
5. Reuse the existing generic/localized impact and fracture authority where legal.
6. Inspect actual repository seams before naming C++ types, runtime formats or serialized fields.
7. Do not add a new material authority.
8. Do not add a general soft-body solver.
9. Do not add FEA.
10. Do not make lattice size configurable in the baseline implementation.
11. Do not copy the Unreal plugin implementation.
12. Do not copy the reference collision-box count as a Jerico default.
13. Performance evidence is required before enabling collision-following deformation broadly.
14. Preserve the rule: cheapest representation that keeps the intended result.

---

# 28. Final binding rules

1. Fixed-cage deformation is an optional **pre-fracture response layer**, not a replacement for structural destruction.
2. Baseline cage resolution is exactly 3×3×3 / 27 points.
3. Jerico owns an independent fixed-lattice deformation implementation.
4. Deformation is evaluated from admitted causal events and quantitative physical data, not material names.
5. Physical Material remains the physical truth; asset deformation policy controls whether/how this geometry uses the cage.
6. Fracture Profile remains the break-geometry/topology authority.
7. Jolt remains the runtime rigid-body backend.
8. Ordinary dents do not rebuild CPU mesh topology.
9. Stable deformation does not require continuous CPU simulation.
10. Visible settled deformation can still cost GPU vertex work; report CPU and GPU cost separately.
11. Fine crumple detail belongs primarily in shading, not in a denser baseline cage.
12. Collision-following deformation is optional, bounded and benchmark-gated.
13. Extreme deformation should hand off to fracture or another explicit system rather than growing an unbounded collision/deformation solution.
14. Normal correction is an optional measured tier, not a mandatory hidden cost.
15. Deformed render bounds and LOD consistency are production requirements.
16. No full engineering material/soft-body model is introduced without a demonstrated need.
17. This document does not alter the current `ACTIVE_SLICE`; implementation begins only when the roadmap and single active Codex task legally admit it.
