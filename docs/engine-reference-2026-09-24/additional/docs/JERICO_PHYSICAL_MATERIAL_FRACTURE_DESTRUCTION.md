# JericoEngine — Physical Material, Fracture Authoring and Runtime Destruction

**Status:** Design contract / implementation direction  
**Date:** 2026-08-30  
**Scope:** JericoEngine 1.0 destruction integration  
**Primary rule:** Jerico decides how geometry is allowed to break; Physical Material decides how the material physically reacts; Jolt simulates detached bodies.

---

# 0. Executive decision

Jerico destruction must not be built as a disconnected damage system where an arbitrary `health` value reaches zero and a mesh is replaced by random debris.

The intended authority split is:

```text
Visual Material
    -> appearance

Physical Material
    -> contact response
    -> density / mass contribution
    -> structural resistance
    -> fracture response characteristics

Fracture Authoring
    -> where geometry may separate
    -> chunk geometry
    -> bond topology
    -> anchors / supports
    -> weak zones
    -> preferred crack structure

Runtime Destruction
    -> evaluate an admitted impact/event
    -> use Physical Material + authored fracture data
    -> break local bonds
    -> find unsupported components
    -> activate only required rigid bodies

Jolt
    -> contact solving
    -> rigid-body motion
    -> constraints
    -> sleeping
```

The designer must not have to describe the same physical material twice.

If an entity already uses a Jerico Physical Material, the fracture/destruction system consumes that physical definition instead of asking the designer to separately choose `Concrete`, `Wood`, `Glass`, `Metal`, etc. inside the fracture tool.

The fracture tool may select or suggest a geometric fracture recipe based on Physical Material, but the two remain separate authorities.

---

# 1. Current repository reality

This document is not proof that the full system already exists.

At the time this document is written, Jerico has already proven a bounded physical-surface route for classified dense geology collision:

```text
Geology surface classification
    -> Jerico-owned physical palette
    -> material index per dense collision triangle
    -> cooked collision
    -> private Jolt material mapping
    -> real friction / restitution
    -> surface material returned by physics queries
```

The currently admitted runtime boundary contains `CollisionPhysicalMaterial` with:

```text
identity
friction
restitution
```

The existing closure explicitly does **not** claim generic material authoring, density/mass, destruction, editor authoring, gameplay, VFX or audio.

Therefore the destruction implementation must extend the existing Jerico-owned physical-material path rather than invent a second material authority.

Hard rule:

```text
DO NOT create:

DestructionMaterial
BreakableMaterial
FractureMaterial
PhysicsMaterial2

if those structures duplicate properties whose owner should be Physical Material.
```

---

# 2. Physical Material is the physical truth

A Jerico entity should conceptually resolve to:

```text
Entity
├── Visual Material
├── Physical Material
└── optional Fracture Profile / Destruction Authoring Data
```

The Visual Material controls how the surface looks.

The Physical Material controls how the surface/body physically reacts.

The Fracture Profile controls how this particular asset has been prepared geometrically to break.

Example:

```text
Wall_A
├── Visual: OldConcretePainted
├── Physical: Concrete
└── Fracture: LargeIrregularChunks

Wall_B
├── Visual: OldConcretePainted
├── Physical: Concrete
└── Fracture: SmallDamagedChunks
```

Both walls remain physically concrete.

They may have different authored fracture geometry because the designer wants different break silhouettes or gameplay behavior.

---

# 3. Physical Material expansion for destruction

The current `friction` and `restitution` properties remain valid and must not be bypassed.

Destruction requires a richer physical response model.

Exact repository names and serialization layout must be selected during implementation, but the conceptual data should cover the following categories.

## 3.1 Contact properties

Already partially proven:

```text
friction
restitution
```

These remain contact-solver inputs.

## 3.2 Mass property

```text
density
```

Density should contribute to mass when Jerico owns mass derivation from geometry/volume.

Do not silently mix authored explicit mass and density-derived mass without a declared precedence rule.

## 3.3 Structural resistance

Conceptual properties:

```text
tensileStrength
compressiveStrength
shearStrength
fractureToughness
```

These values exist so that a material can resist different loading modes differently.

Jerico does not need to become an engineering finite-element solver to benefit from these distinctions.

A bounded gameplay approximation is sufficient if it is deterministic and physically coherent.

## 3.4 Response character

Conceptual properties:

```text
brittleness
ductility
impactAbsorption
penetrationResistance
```

These influence the admitted response mode.

Examples:

```text
Glass
    high brittleness
    low fracture toughness
    -> crack / fragment readily

Concrete
    high compressive resistance
    lower tensile resistance
    -> cracking and spall under suitable loading

Wood
    anisotropic structural response
    -> prefers splitting with grain direction

Metal
    greater ductile response
    -> deformation may be preferred before separation
```

Do not pretend all materials fail through the same scalar threshold.

## 3.5 Optional anisotropy

Some materials require a preferred structural direction.

Conceptually:

```text
anisotropyMode
preferredAxis / fiberDirection
longitudinalStrengthMultiplier
transverseStrengthMultiplier
```

This is particularly useful for:

- timber;
- boards;
- layered composites;
- stratified geology;
- other direction-dependent materials.

Keep this optional.

Do not charge every material for anisotropic evaluation when it is not needed.

---

# 4. Physical Material must not dictate exact chunk geometry

The Physical Material may provide a **default fracture-response family** or editor recommendation.

Example:

```text
PhysicalMaterial: Concrete
    suggested fracture family -> BrittleIrregular

PhysicalMaterial: Wood
    suggested fracture family -> FiberSplit

PhysicalMaterial: Glass
    suggested fracture family -> RadialBrittle
```

This is an authoring convenience, not a hard coupling.

Why:

- two concrete walls may require different chunk scales;
- old/damaged concrete may have a different authored crack layout;
- a hero object may need hand-authored fracture boundaries;
- a gameplay-critical opening may require controlled silhouettes;
- the same material can appear in assets with very different dimensions.

Therefore:

```text
Physical Material
    decides physical resistance / response

Fracture Profile
    decides geometric preparation
```

---

# 5. Fracture Editor

**BSP Physical Material reference**: the source author's choice of an existing
reusable physical definition, not a copy of its coefficients or strengths.
**Prepared World body**: an explicitly published geometry/collision snapshot
whose own cooked Physical Material is authoritative. Editing the source reference
alone does not update that snapshot; explicit preparation is required. The narrow
assignment and stale-source contract is owned by
[ADR-031, decision 9](decisions/ADR-031-EDITOR-PROCEDURAL-AUTHORITY-AND-WORLD-PUBLICATION.md).

Jerico should expose destruction authoring as an editor workflow rather than requiring hand-created fragments in an external DCC for ordinary cases.

Conceptual Inspector panel:

```text
Fracture / Destruction

Physical Material
    Concrete

Suggested Fracture Family
    Brittle / Irregular

Generation Mode
    Voronoi
    Radial Impact
    Layered
    Directed Crack
    Structural
    Custom Field / Noise
    Hand Authored

Target Pieces
    [32]

Chunk Scale
    [----------]

Irregularity
    [----------]

Noise / Warp Source
    None / Perlin / FBM / Voronoi / Texture / Field

Interior Surface Material
    ConcreteBrokenInterior

[ Paint Anchors ]
[ Paint Weak Zones ]
[ Paint Protected Zones ]
[ Add Preferred Impact Point ]

[ Generate Preview ]
[ Simulate ]
[ Reset ]
[ Bake / Cook ]
```

The exact UI follows Jerico editor conventions.

No separate material selector should exist inside this panel when the entity already resolves an authoritative Physical Material.

---

# 6. Noise is an input, not the fracture system

A noise texture or procedural noise can help create organic boundaries, but noise alone is not sufficient to describe convincing destruction.

Preferred conceptual path:

```text
Base fracture cells / directed cracks
            +
noise / field warp
            +
material structural response
            +
weak / protected masks
            +
anchors / support constraints
            +
asset-specific authored controls
            ↓
final fracture chunks + bonds
```

Example for concrete:

```text
irregular fracture cells
+ local radial bias near authored impact zones
+ boundary warp
+ smaller fragments near high-energy impact zones
+ larger retained chunks away from impact
```

Example for timber:

```text
longitudinal split tendency
+ fiber direction
+ limited transverse fracture
+ optional splinter/debris representation
```

Example for brickwork:

```text
mortar regions -> weak zones
brick bodies -> stronger regions
some single-brick chunks
some bonded brick groups
```

The system must not produce uniform cheap-looking Voronoi for every material.

---

# 7. Reuse Jerico Structural Geometry Math

The existing production direction already requires Geology and destruction to reuse structural mathematics.

Maintain that rule.

Do not create unrelated implementations of the same math.

Preferred conceptual ownership:

```text
Jerico Structural Geometry Math
├── fracture cells
├── adjacency
├── region identity
├── directed cracks
├── anisotropic fracture
├── structural masks
├── support / anchor topology
└── durability influence

Consumers
├── Geology
└── Destruction
```

Where the Geology SDF/field stack already provides a useful operation, destruction should consume or share that implementation through a clean boundary.

However, do **not** force every ordinary mesh through an expensive volumetric/SDF conversion if a cheaper deterministic mesh-space fracture path produces the required result.

Performance remains a design constraint.

---

# 8. SDF use is selective

SDF is especially useful when Jerico needs:

- volumetric cuts;
- non-trivial interior surfaces;
- rock/geology fracture;
- boolean fracture shapes;
- complex tunnels or thick structures;
- procedural crack volumes.

But the default rule is:

```text
use the cheapest fracture representation that preserves the required result
```

For a simple board or glass pane, a direct geometric partition may be enough.

For a thick irregular rock or geological wall, the shared SDF/structural path may be justified.

Never make runtime SDF remeshing the universal destruction solution.

---

# 9. Chunk generation

The editor/cook stage should generate stable chunk identities.

Conceptually:

```text
DestructionAsset
├── Chunk_000
├── Chunk_001
├── Chunk_002
└── ...
```

Each chunk may need:

```text
stable identity
render geometry
interior/exterior surface classification
collision geometry
local transform
volume / mass contribution
physical material references
adjacency information
anchor state
optional importance / debris class
```

Chunk generation must be deterministic for identical source + recipe + seed.

A repeated cook should not arbitrarily reorder chunk identities.

---

# 10. Interior broken surfaces

Fractured pieces must distinguish existing exterior surfaces from newly exposed interior surfaces.

Conceptual classification:

```text
Chunk surface
├── ExteriorOriginal
└── InteriorFracture
```

This allows Jerico to assign:

- concrete interior;
- raw timber interior;
- rock fracture face;
- glass edge treatment;
- painted wall exterior vs broken plaster/concrete interior.

Do not require the Visual Material to contain the fracture logic itself.

The fracture authoring/cook path may bind an appropriate interior visual treatment while preserving the same Physical Material unless the asset explicitly contains physically distinct layers.

---

# 11. Bond graph is the structural runtime representation

Pre-fractured chunks must not begin runtime as dozens of unrelated active rigid bodies.

Build a bond graph.

Conceptually:

```text
Chunk_A ---- Chunk_B
   |            |
Chunk_C ---- Chunk_D
```

Each bond has an admitted structural resistance derived from meaningful inputs.

Conceptually:

```text
BondStrength =
    contactArea
    * physicalMaterialStructuralResponse
    * localDurability
    * authoredWeaknessMultiplier
    * optional directionalFactor
```

This equation is illustrative, not a frozen implementation formula.

The implementation must choose units and normalization deliberately and cover them with tests.

Important principle:

```text
The graph owns connectivity.
Physical Material contributes resistance.
Authored masks modify local intent.
```

---

# 12. Anchors and structural support

The editor must allow chunks or regions to be anchored.

Examples:

- wall attached to foundation;
- beam attached at both ends;
- ceiling attached to structural supports;
- rock mass attached to parent geology;
- door frame attached to wall.

Conceptual graph:

```text
        ANCHOR
          |
      A---B---C
      |   |   |
      D---E---F
      |   |   |
      G---H---I
```

When local bonds break, Jerico can test whether a connected component still has a path to an anchor.

If it does, it may remain part of the intact/kinematic structure.

If it does not, that separated component becomes eligible for rigid-body activation.

This graph traversal happens on structural change, not continuously every frame.

---

# 13. Runtime impact flow

Preferred event-driven flow:

```text
Impact / explosion / admitted gameplay force
        ↓
resolve hit surface + Physical Material
        ↓
compute bounded impact descriptor
        ↓
query nearby fracture chunks / bonds
        ↓
evaluate only relevant local bonds
        ↓
mark failed bonds
        ↓
if topology changed:
    find affected connected components
        ↓
check path to anchors
        ↓
activate only detached components
        ↓
Jolt simulates resulting rigid bodies
```

Do not evaluate every bond of every destructible object each frame.

---

# 14. Impact descriptor

Destruction needs a Jerico-owned input representation between gameplay/contact events and fracture evaluation.

Conceptually:

```text
DestructionImpact
{
    position
    direction
    impulse / energy representation
    affected radius
    source entity / provenance
    optional penetration context
}
```

Exact data depends on the admitted physics/gameplay API.

The destruction subsystem should not directly depend on weapon-specific classes.

Examples of producers:

- hammer hit;
- thrown rock;
- explosion;
- falling heavy prop;
- scripted collapse event;
- geological rockfall trigger.

All should be able to route through the same generic destruction boundary when appropriate.

---

# 15. Do not reduce destruction to `damage > health`

A scalar damage/health system may still exist for gameplay objects, but structural fracture should not depend exclusively on it.

Avoid:

```text
wallHealth -= weaponDamage
if wallHealth <= 0:
    explode all chunks
```

Prefer a bounded physical response:

```text
impact
    + Physical Material resistance
    + local geometry / bond
    + local weakness / durability
    -> admitted bond failures
```

This does not require full continuum mechanics.

It requires only enough structural meaning to make materials and supports matter.

---

# 16. Material-specific qualitative behavior

The system should allow the same impact to produce different results because the Physical Material differs.

## Glass

Typical desired behavior:

```text
low fracture toughness
high brittle tendency
radial cracking
many small fragments near impact
```

## Concrete

Typical desired behavior:

```text
strong compression response
weaker tension response
crack propagation
spall / fragments near impact
larger structural chunks away from impact
```

## Wood

Typical desired behavior:

```text
anisotropic response
splits with grain direction
stronger/weaker response depending load direction
potential splinter representation
```

## Metal

Typical desired behavior:

```text
high ductile tendency
possible deformation before fracture
higher separation threshold
```

Do not implement expensive deformation simulation merely because metal is ductile.

For 1.0, a designer-controlled deformation/bend response or non-destructible threshold may be cheaper than general plastic deformation.

---

# 17. Default strategy: prefracture in editor/cook

The normal production path should be:

```text
Source Mesh / BSP / Geology
        ↓
Fracture Authoring
        ↓
Generate chunks
        ↓
Build bond graph
        ↓
Generate collision
        ↓
Cook
        ↓
Runtime intact representation
        ↓
activate chunks only after structural failure
```

This should cover the large majority of Jerico gameplay destruction.

Advantages:

- deterministic;
- cacheable;
- inspectable;
- testable;
- no arbitrary runtime remeshing spike;
- collision can be prepared ahead of time;
- gameplay openings can be designed deliberately;
- shipping content is source-free.

---

# 18. Intact runtime representation must stay cheap

A destructible asset with 100 authored chunks must not automatically cost as 100 active rigid bodies while intact.

Preferred hierarchy:

```text
Intact state
    -> one primary render representation where possible
    -> one admitted intact collision representation where possible
    -> dormant/cooked chunk data
    -> dormant bond graph

After local failure
    -> activate only detached chunks/components
```

If visual chunk geometry must already be present for fast activation, it should still avoid unnecessary active physics bodies and per-frame work.

---

# 19. Destruction activation budget

The runtime needs explicit budgets.

Conceptual configuration:

```text
maxActivatedChunksPerEvent
maxActiveDestructionBodiesPerWorld
maxBondEvaluationsPerEvent
maxPropagationDepth / workItems
minimumChunkImportanceForRigidBody
smallDebrisPolicy
```

When a fracture contains many tiny pieces, not every piece deserves a Jolt body.

Possible policy:

```text
large chunks
    -> real rigid bodies

medium chunks
    -> bounded rigid bodies / pooled debris

small fragments
    -> VFX / particles / decals / PWAS aftermath when appropriate
```

The exact handoff to VFX/PWAS should be integrated later through existing subsystem boundaries rather than hardcoded inside physics.

---

# 20. Small debris must not become a physics-body explosion

A believable wall may visually produce hundreds of tiny fragments.

That does not mean Jerico should create hundreds of persistent rigid bodies.

Preferred principle:

```text
structurally meaningful chunks -> physics
visual micro-debris -> cheaper representation
```

Possible representations include:

- short-lived GPU/CPU particles;
- instanced debris;
- decals;
- PWAS-authored persistent rubble after the event;
- baked aftermath state.

Choose by measured cost and gameplay relevance.

---

# 21. Dynamic runtime fracture is optional and bounded

Jerico may eventually support runtime-generated fracture for selected cases.

It must not become the default destruction path.

Gate it behind explicit authoring/capability.

Conceptual policy:

```text
if asset has prepared fracture data:
    use prepared fracture
else if asset explicitly allows bounded dynamic fracture
     and size / complexity / budget admit it:
    generate local runtime fracture
else:
    use non-fracture response
```

Good candidates:

- small hero props;
- glass-like panels;
- bounded local rock fragments;
- explicitly budgeted gameplay objects.

Bad default candidates:

- whole buildings;
- huge caves;
- arbitrary environment meshes;
- large numbers of simultaneous objects.

Runtime SDF + remesh + collider cooking should be treated as expensive work and measured before admission.

---

# 22. Geology destruction integration

Geology already owns useful structural concepts such as durability, fracture influence, region identity and rockfall-related fields.

Destruction should consume those when meaningful.

Conceptual path:

```text
Geology
├── durability
├── strata
├── fracture influence
├── support / region data
└── material classification
        ↓
Destruction authoring / cook
        ↓
chunk + bond resistance
        ↓
rockfall / impact / gameplay trigger
        ↓
Jolt activation
```

Do not assign arbitrary unrelated random bond strengths where existing geology data already describes structural weakness.

---

# 23. Rockfall bridge

Rockfall is a natural destruction consumer.

Conceptual event:

```text
weathering / authored event
        ↓
local support weakens
        ↓
bond/support topology changes
        ↓
component loses anchor path
        ↓
large geological chunk becomes rigid body
        ↓
Jolt
```

This should remain bounded and event-driven.

Jerico 1.0 does not require continuous whole-world structural simulation.

---

# 24. Editor previews

The Fracture Editor should provide useful diagnostic visualization.

Desired overlays:

```text
Chunk boundaries
Bond strength
Anchor state
Weak zones
Protected zones
Physical Material
Detached-component preview
Expected activation count
```

Possible color semantics:

```text
blue   -> anchor
normal -> supported
warning -> weak support
red    -> would detach under current preview event
```

Exact colors belong to editor UX conventions.

The important requirement is explainability.

The designer should be able to answer:

```text
Why did this piece detach?
Why did this bond survive?
Which Physical Material was used?
Which weakness/anchor affected the result?
How many physics bodies will this event activate?
```

---

# 25. Fracture simulation preview

Editor preview should allow a test impact without committing changes.

Conceptual workflow:

```text
select destructible entity
    ↓
place preview impact
    ↓
set impulse / radius
    ↓
Preview
    ↓
Jerico highlights evaluated bonds
    ↓
Jerico highlights bonds that fail
    ↓
Jerico highlights detached components
    ↓
optional bounded physics simulation
    ↓
Reset
```

Preview must not mutate source assets until the designer applies/bakes the result.

---

# 26. Determinism

Identical inputs must produce identical prepared fracture data.

Inputs include at least:

```text
source geometry identity/hash
physical material identity/version
fracture recipe
seed
painted masks
anchors
structural source fields
```

Desired invariant:

```text
same inputs
    -> same chunks
    -> same stable chunk ordering/identity
    -> same bond graph
    -> same cooked bytes where the format guarantees byte determinism
```

Runtime fixed-step destruction tests should prove repeatable topology and body activation for deterministic input events.

---

# 27. Dirty / partial authoring

Fracture authoring should reuse Jerico's existing philosophy of bounded dirty work where practical.

If the designer modifies one local weak zone, avoid rebuilding unrelated data when a safe partial path exists.

However:

```text
correct deterministic full rebuild
    is preferable to
unsafe complex partial rebuild
```

Measure first.

Do not introduce a complicated dirty-fracture system without profiling evidence that full authoring recook is a bottleneck.

Runtime destruction itself remains local/event-driven from the start.

---

# 28. Serialization / cooked format direction

Do not freeze a new runtime format name in this document without checking repository conventions at implementation time.

The cooked destruction representation needs enough data for:

```text
asset identity
source / recipe hashes
stable chunk identities
chunk geometry references
collision references
physical material references
bond graph
bond structural parameters
anchors
interior surface classifications
optional destruction tiers / budgets
```

The source authoring representation and shipping representation may differ.

Shipping must remain source-free.

Do not require editor-only procedural graphs, source textures, DCC files or uncooked fracture recipes at runtime unless explicitly admitted.

---

# 29. Physical Material serialization compatibility

The currently proven collision physical palette is intentionally bounded and backward-compatible.

When expanding Physical Material:

- preserve existing collision compatibility where possible;
- do not silently reinterpret existing `friction` / `restitution` values;
- use explicit format versioning;
- validate finite ranges before allocation/physics use;
- preserve legacy byte behavior where a format contract promises it;
- keep Jolt-native implementation details private.

Destruction-specific cooked data may reference a richer Jerico Physical Material asset while collision surfaces continue to carry the minimal contact data they require.

Do not blindly duplicate every structural property into every triangle if the runtime can resolve them efficiently through a stable material identity.

---

# 30. Jolt boundary

Jolt remains the motion/contact solver.

Jerico remains authoritative for destruction topology.

```text
Jerico
    -> determines failed bonds
    -> determines detached components
    -> provides prepared collision / mass / transforms

Jolt
    -> simulates active bodies
    -> contact response
    -> constraints
    -> sleeping
```

Do not make destruction depend on undocumented Jolt internals.

Do not expose Jolt material pointers/IDs as Jerico public API.

Follow the existing private-adapter pattern already used by Physical Material surface mapping.

---

# 31. Failure and fallback behavior

A bad destruction asset must fail predictably.

Examples to validate:

- missing chunk referenced by a bond;
- duplicate chunk identity;
- self-bond;
- invalid anchor reference;
- NaN/Inf structural coefficient;
- negative volume/density;
- collision missing for a required rigid chunk;
- material identity unavailable;
- graph cardinality beyond admitted limits;
- corrupt cooked data.

Do not partially create a corrupted destruction body graph and continue silently.

Prefer explicit load/cook failure with a Jerico-owned error contract.

---

# 32. Testing requirements

A destruction slice is not complete because fragments can be seen falling in the editor.

Minimum test families should eventually include:

## 32.1 Physical Material

- generic asset authoring -> cook -> runtime propagation;
- friction/restitution legacy compatibility;
- density/mass behavior where admitted;
- structural property validation;
- material identity stability.

## 32.2 Fracture authoring

- deterministic chunk generation;
- deterministic stable ordering;
- interior/exterior classification;
- masks and anchors;
- malformed recipe rejection.

## 32.3 Bond graph

- valid connectivity;
- local bond failure;
- component separation;
- anchor path preservation;
- multiple-anchor cases;
- no detach when support remains;
- detach when support is removed.

## 32.4 Runtime

- impact evaluates only bounded local work;
- same fixed-step event produces same topology;
- only detached chunks become bodies;
- intact object does not instantiate all dormant bodies;
- body sleeping / cleanup policy works;
- world budgets cannot be exceeded silently.

## 32.5 Shipping

- cooked package contains no editor/source fracture dependencies;
- destruction works from source-free package;
- no forbidden editor imports in Shipping;
- deterministic content/hash contract where applicable.

---

# 33. Performance gates

Performance must be evaluated before expanding visual/physical complexity.

Required measurements should include at least:

```text
intact destructible memory overhead
cook time
chunk count
bond count
impact bond evaluations
component traversal work
bodies activated per event
physics step cost after activation
peak simultaneous destruction-body count
cleanup/sleep behavior
```

Representative fixtures should include:

- small wooden board;
- glass panel;
- concrete wall;
- large geological chunk / rockfall case.

Avoid benchmark fixtures so tiny that graph/runtime scaling cannot be observed.

---

# 34. Recommended 1.0 tiers

Jerico 1.0 should support practical quality tiers without requiring separate systems.

Conceptually:

```text
Tier 0 — Non-destructible
    Physical Material only

Tier 1 — Simple Breakable
    small pre-authored/prefractured object
    bounded chunk count

Tier 2 — Structural Breakable
    bond graph
    anchors
    local propagation

Tier 3 — Geological / Hero Destruction
    shared structural fields / richer authoring
    explicitly budgeted

Dynamic runtime fracture
    opt-in only
    not a default tier
```

These names are conceptual; implementation may choose different configuration representation.

---

# 35. Recommended implementation order

Do not attempt the complete system in one giant slice.

## Phase A — General Physical Material vertical

Close the already identified repository gap:

```text
generic material/model authoring
    -> Physical Material selection
    -> cook
    -> package
    -> collider/runtime
    -> queries/contact behavior
```

Preserve the currently proven classified-geology route.

Add density only when mass ownership/preference rules are explicitly defined and tested.

## Phase B — Destruction cooked data + simple prefracture

Implement the smallest production-complete breakable path:

```text
source mesh
    -> prepared chunks
    -> stable identities
    -> collision
    -> cooked destruction asset
    -> intact runtime
    -> explicit trigger activates selected chunk(s)
```

This proves asset/runtime plumbing before advanced fracture response.

## Phase C — Bond graph + anchors

Add:

```text
adjacency
bond resistance
anchor connectivity
component detach
local graph traversal
```

## Phase D — Physical Material structural response

Route physical structural properties into bond evaluation.

Prove at least two materially different responses using identical geometry/impact.

## Phase E — Fracture Editor

Add production authoring:

```text
fracture generation
noise/warp
masks
anchors
preview
interior material
simulate/reset
```

## Phase F — Geology bridge

Reuse durability/fracture/structural fields for geological breakables and rockfall.

The bounded bridge under implementation consumes the explicit productive
JRMS 1.2/GSTR export defined in `RUNTIME_FORMATS_AND_CHUNKS.md`; it does not
repurpose diagnostic inspection or derive physical strength from visual materials.
The first admission is one validated closed dense surface, one exterior palette
material and no alternate LOD. Existing source byte/triangle limits still apply.
GEOL without its exact source structural field remains unsupported for fracture.

For each actual plane cut, sample normalized durability trilinearly at the
centroid of every triangle on the shared cut face. Its area-weighted mean
multiplies the authored base resistance before authored masks. Existing Physical
Material load-mode strength remains the runtime upper limit. This is a bounded
prepared-bond authoring rule, not a stress solver or a localized-impact model.
Uncovered samples and exhausted work budgets reject the preview before publication.

Exterior classifications interpolate the three original triangle samples using
the same barycentric coordinates as other preserved surface attributes. New cut
vertices sample the actual volume trilinearly, retaining geological classification
and setting fresh=true, exposure=1 and curvature=0 for the newly exposed planar
surface. The fresh bit selects the greatest interpolation weight (first on ties)
on existing exterior faces. Classification packing uses the existing codec.
Source/final-field hashes survive; each chunk gets a surface identity from its
source content identity, chunk identity, actual positions and classifications.
The source volume is not copied into every moving chunk.

Work accounting charges three source samples per exterior vertex and eight
lattice samples per new vertex or cut-triangle centroid, in addition to the
existing chunk/bond/mask/anchor work. Classified vertex bytes are included in the
generation byte ceiling. Cancellation is checked during classified generation.
Support anchors remain explicit authored intent mapped by the existing slab
generator. Offline Rockfall diagnostics do not silently become runtime anchors.

Geological fracture collision explicitly selects the source-vertex partition
policy. For a proven star kernel it forms bounded tetrahedral cells around one
stored float interior point and merges exactly convex neighbours, grouping
coplanar source patches first. Surface coordinates are unchanged; coverage,
convexity, non-overlap and existing part/vertex/work ceilings remain mandatory.
Non-geological Phase E producers retain their native-plane policy and accepted
output/rejection contracts. This does not close deferred arbitrary-notch hardening.

A structural-load simulation may start with no pre-failed bonds. It then applies
only the explicit selected load and evaluates existing anchor connectivity; it
must not manufacture an unrelated prior failure. Nonempty pre-failed lists still
require sorted unique in-range indices distinct from the loaded bond. An anchored
component stays supported, even when the selected bond actually fails.
First-load runtime evaluation resolves validated intact graph state temporarily;
invalid, unavailable and survived loads consume no persistent graph slot. Actual
detachment publishes through the existing capacity-checked failure transaction.

For classified fracture chunks, exterior submeshes bind GEOL palette entry zero.
An explicit interior Visual Material outside that palette uses its ordinary
material shader. The complete GEOL stream remains attached to the mesh for CPU
and future semantic consumers; ordinary interior shading does not replace the
sampled classification or physical authority. Direct draws binding a non-base
palette entry remain rejected because palette selection belongs to GEOL.

## Phase G — Optional bounded dynamic fracture

Only after profiling proves it is needed and a strict budget can be enforced.

---

# 36. Explicit non-goals for the first implementation

Do not require:

- full finite-element analysis;
- continuous world structural simulation;
- per-frame evaluation of every bond;
- general plastic deformation solver;
- arbitrary whole-building runtime remeshing;
- universal runtime SDF fracture;
- hundreds of persistent rigid bodies for micro-debris;
- duplicated destruction-only material definitions;
- weapon-specific code inside the fracture subsystem;
- editor source data in Shipping.

These can only be admitted later with measured need.

---

# 37. Codex implementation guardrails

When Codex implements this design:

1. **Inspect repository reality first.** Do not assume conceptual names in this document already exist.
2. **Reuse the existing Physical Material route.** Do not create a parallel authority.
3. **Preserve current JRCL/classified-geology compatibility.** Existing friction/restitution behavior must remain valid.
4. **Prefer prefracture/cook over runtime remeshing.** Runtime fracture is opt-in future work.
5. **Keep destruction event-driven.** No all-world bond update loop.
6. **Activate only detached chunks.** Intact authored chunks are not automatically active Jolt bodies.
7. **Share structural math with Geology where appropriate.** Do not duplicate fracture math families.
8. **Do not force SDF on cheap cases.** Choose the least expensive representation that produces the required result.
9. **Budget graph work and rigid bodies.** Hard limits and cancellation/bounded work are required where applicable.
10. **Test deterministic behavior.** A visible editor demo is not sufficient closure evidence.
11. **Keep Jolt private.** Jerico types remain the public/runtime format boundary.
12. **Do not claim unimplemented capabilities as complete.** Evidence documents must match actual behavior.

---

# 38. Final architecture

The intended production chain is:

```text
                         AUTHORING

Visual Material -------------------------------> appearance

Physical Material
├── friction
├── restitution
├── density
├── structural resistance
├── fracture toughness
├── brittle/ductile response
└── optional anisotropy
        |
        v
Mesh / BSP / Geology
        |
        v
Fracture Editor
├── fracture cells / directed cracks
├── noise / field warp
├── weak/protected masks
├── anchors
├── chunk scale
└── interior surface classification
        |
        v
Jerico Structural Geometry Math
        |
        v
Prepared Chunks + Bond Graph + Collision
        |
        v
Cook / Package

                         RUNTIME

Intact Object
    |
Impact / Explosion / Structural Trigger
    |
    v
Physical Material response
    +
local bond evaluation
    |
    v
failed bonds
    |
    v
local connected-component / anchor test
    |
    +---- supported component -> remains intact
    |
    `---- detached component -> activate Jolt rigid body/bodies
                                   |
                                   v
                            motion / contacts / sleep
```

The key product rule is simple:

> **The fracture editor defines where an object can break. Physical Material defines what it takes to break it and how the material responds. Jerico owns the topology. Jolt owns the motion after separation.**

This preserves physical coherence, designer control, deterministic cooking and scalable runtime cost without turning Jerico into a continuous structural-engineering simulator.
