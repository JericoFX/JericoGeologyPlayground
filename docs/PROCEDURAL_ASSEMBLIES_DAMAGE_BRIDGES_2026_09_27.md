# JericoGeologyPlayground — Procedural Assemblies, Damage, Rails, Piers and Bridges Lab

**Owner request:** 2026-09-27  
**Status:** IMPLEMENT / BREAK / VERIFY / LEAVE INTERACTIVE  
**Repository role:** experimental C++/Diligent/ImGui proving ground for reusable JericoEngine procedural construction.  
**Companion:** `docs/SPLINE_CONSTRUCTION_PIPE_UV_PLAYGROUND_2026_09_27.md`

---

# 0. Direct instruction to the coding agent

Extend the Playground beyond a single swept pipe.

Build and test a reusable **Procedural Assembly + Damage Lab** using the path/sweep/repeat mathematics already requested in the companion Spline Construction directive.

The owner wants to be able to open the normal Playground and interactively create, damage and inspect:

- pipes with repeated couplings / collars / clamps / flanges;
- railway track with rails, sleepers, plates/fasteners and broken sections;
- railings / guardrails with missing, bent or broken pieces;
- wooden piers / docks with piles, beams, deck planks, side boards and rails;
- catwalks and simple scaffold-like runs;
- simple beam/truss bridges;
- suspension bridges;
- rope / plank suspension bridges;
- selected broken or missing boards, supports, rails, cables and other semantic parts.

Do not solve each family with unrelated one-off code.

The goal is to prove a small set of reusable mathematical primitives:

```text
Path / Spline
    + Profile Sweep
    + OffsetPath
    + RepeatAlongPath
    + FitCountSpacing
    + Semantic Part IDs
    + Damage Selectors / Part States
    + Optional Jolt promotion
    = Procedural Assembly
```

This is a Playground experiment. **Do not modify JericoEngine from this repository.**

---

# 1. JericoEngine authorities to read

Before implementation, read the current Engine equivalents and preserve their ownership model:

- `docs/JERICO_PROCEDURAL_STUDIO_1_0.md`
- `docs/JERICO_SPLINE_AND_GEOMETRY_MATH_BACKENDS_1_0.md`
- `docs/PROCEDURAL_PWAS_EPLS_MASTER.md`
- `docs/superpowers/specs/2026-08-02-soft-physics-strands-and-webs-design.md`
- `docs/JERICO_JOLT_FULL_CAPABILITY_INTEGRATION_DIRECTIVE.md`
- current `Engine/Physics` Jolt adapter and admitted constraints.

Relevant Engine intent already exists for Pipe, Duct, CableTray, Beam, RailwayTrack, Guardrail, Fence, Bridge, Catwalk and ScaffoldRun. This Playground must test mathematics and UX, not create a competing permanent authority.

---

# 2. Core assembly representation

A generated object is not one anonymous triangle soup.

Conceptually preserve stable semantic elements:

```cpp
enum class AssemblyPartState
{
    Intact,
    Missing,
    Bent,
    Cracked,
    Broken,
    Loose,
    Detached,
    Decayed,
    Collapsed
};

struct AssemblyPartKey
{
    StableId recipe;
    StableId role;
    std::uint32_t primaryIndex;
    std::uint32_t secondaryIndex;
    std::uint32_t variant;
};
```

Exact Playground types may differ, but identity must be deterministic.

Examples:

```text
Pier / DeckPlank / row=18
Railway / Sleeper / station=42
Railing / TopRail / bay=7
Pipe / Coupling / station=5
SuspensionBridge / Hanger / side=Left / station=12
```

Do not derive stable identity from pointer values, hash-table iteration order or final tessellation indices.

A geometry quality change must not randomly renumber semantic boards, sleepers, clamps or hangers.

---

# 3. Reusable stationing by arc length

Let the path arc-length coordinate be `s in [0,L]`.

All repeated structural parts use world-space stationing:

```text
s_k = startPadding + k * spacing
```

with the last accepted station satisfying:

```text
s_k <= L - endPadding
```

Use the common arc-length map to obtain path parameter, position and RMF frame at `s_k`.

For a fit-to-length mode with preferred piece size `b` and gap `g`:

```text
n = max(1, floor((L + g) / (b + g)))
b_actual = (L - (n - 1) * g) / n
```

Alternative policies must be explicit:

- PreserveSize
- PreserveGap
- FitExactly
- AllowPartial
- CenterRemainder

This same primitive serves sleepers, deck boards, clamps, bridge hangers, fence posts and catwalk bays.

---

# 4. Offset paths

Given a path frame `(P,T,N,B)`, a lateral offset path is:

```text
P_offset(s) = P(s) + N(s) * verticalOffset + B(s) * lateralOffset
```

Use the same RMF frame authority as Sweep.

Required uses:

- left/right railway rail;
- two side cables of a suspension bridge;
- handrails;
- cable-tray rails;
- bridge girders;
- fence/guardrail rails;
- side beams of a wooden pier.

Large offsets on tight curves can self-intersect. Detect and report this; do not silently publish invalid geometry.

---

# 5. Pipe assemblies with couplings: ===O===O===

A pipe run is not only one smooth cylinder.

Base body:

```text
PipeBody = Sweep(Circle/RingProfile, Path)
```

Repeated couplings/collars:

```text
station s_k
frame F_k = FrameAtDistance(s_k)
coupling axis = T_k
coupling center = P_k
```

A simple coupling may be a short enlarged sleeve:

```text
inner radius = pipe outer radius + clearance
outer radius = inner radius + couplingThickness
half length   = couplingLength / 2
```

or a ring/collar generated by revolving/extruding a small radial profile around the tangent axis.

Required prototype roles:

- Coupling
- Clamp
- Flange
- Support/Hanger marker
- optional Valve placeholder

Controls:

- spacing
- start/end padding
- coupling width
- coupling radial thickness
- seam/UV policy
- random but deterministic variant selection
- enable/disable individual couplings by stable part ID

Damage experiments:

- missing coupling
- displaced coupling
- broken pipe segment / gap
- bent local run
- hanging physical end promoted to Jolt later

---

# 6. Railway track

Use one path as centerline.

Gauge `G`:

```text
leftRail(s)  = P(s) - B(s) * G/2
rightRail(s) = P(s) + B(s) * G/2
```

Rails are profile sweeps using a rail or simplified I-like profile.

Sleepers/ties are placed by true arc length:

```text
s_k = s0 + k * sleeperSpacing
```

At station `s_k`:

- center = `P(s_k)`
- long axis = `B(s_k)`
- vertical/up = `N(s_k)`
- rail direction = `T(s_k)`

Sleeper length:

```text
sleeperLength = gauge + 2 * sideOverhang
```

Optional parts:

- tie plates
- spikes/bolts
- rail joint bars / fishplates
- periodic supports

### Cant / superelevation

Apply authored roll `phi(s)` to the path frame before placing rails/sleepers:

```text
N' = N cos(phi) + B sin(phi)
B' = B cos(phi) - N sin(phi)
```

### Broken railway states

Per stable rail segment/sleeper:

- MissingSleeper
- BrokenSleeper
- TiltedSleeper
- MissingRailSpan
- RailGap
- BentRail
- DetachedJoint

For a missing rail interval `[s_a,s_b]`, split the sweep at those stations and do not bridge the gap.

For a visual bent span over normalized local coordinate `u in [0,1]`:

```text
d(u) = bendAmplitude * sin(pi*u)
P_bent(u) = P(u) + bendDirection * d(u)
```

Use this only as a bounded authored deformation; reject amplitudes that create inverted or self-intersecting local geometry.

---

# 7. Railings / guardrails

Represent a railing as repeated bays, not one anonymous mesh.

A bay between stations `s_i` and `s_{i+1}` can contain:

- PostLeft
- PostRight / next shared post
- TopRail
- MidRail
- BottomRail
- Balusters / boards / mesh infill

Posts use `RepeatAlongPath`.

Rails use either continuous offset Sweep or one segment per bay depending on the damage policy.

### Damage-friendly rule

If individual breakage matters, keep stable bay/member boundaries even when the intact visual result could be merged later.

States:

- MissingPost
- LeaningPost
- MissingRail
- BentRail
- BrokenRail
- LooseRail
- MissingBoard
- BrokenBoard

### Broken rail split

For a member from `A` to `B`, choose deterministic break fraction:

```text
beta in [betaMin,betaMax]
C = A + beta * (B-A)
```

Create a bounded gap `g` around `C` and two child pieces.

Optional droop/rotation:

```text
theta_left  = -theta
theta_right = +theta
```

around declared local sockets/pivots.

If physics is enabled, the detached child can be promoted to a Jolt rigid body instead of remaining procedurally rotated.

---

# 8. Wooden pier / dock

A wooden pier is an assembly driven by a center path and width.

Primary parts:

```text
deck planks
left/right longitudinal beams
cross beams
vertical piles/posts
diagonal braces
side fascia boards
optional railings
optional ladder
optional ropes/chains
```

## 8.1 Deck frame

At distance `s` use frame `(P,T,N,B)`.

Deck left/right boundaries:

```text
L(s) = P(s) - B(s) * deckWidth/2
R(s) = P(s) + B(s) * deckWidth/2
```

Longitudinal beams are offset sweeps.

Cross beams repeat by station.

## 8.2 Deck planks

For crosswise deck boards repeated along the path:

```text
n = max(1, floor((L + gap) / (preferredBoardWidth + gap)))
boardWidthActual = (L - (n-1)*gap) / n
```

Each board receives a stable index and transform from the frame at its station.

Board length normally spans:

```text
deckWidth + leftOverhang + rightOverhang
```

Add deterministic healthy variation only inside bounded limits:

- width variation
- tiny vertical offset
- tiny yaw
- material/UV offset
- edge wear seed

Do not make an intact pier look structurally exploded.

## 8.3 Piles/supports

Piles repeat at structural stations, usually left/right pairs.

Prototype support target may be a ground/water reference plane in the Playground.

For a support ray from deck underside position `Q` in direction `-Up` hitting support plane at height `h`:

```text
t = (Q.y - h) / dot(Up, worldUp)
supportEnd = Q - Up * t
```

General Engine integration later must use actual host/world queries rather than a hard-coded plane.

## 8.4 Diagonal braces

For bay corners `A,B,C,D`, braces are ordinary beam segments between stable sockets.

Use:

```text
direction = normalize(B-A)
length = |B-A|
center = (A+B)/2
```

Generate by rigid profile extrusion or short Sweep.

## 8.5 Selectable board damage

Two selection modes are mandatory:

### Procedural selection

Use a stable hash:

```text
r_i = Hash01(recipeSeed, partRole, partIndex)
```

Then:

```text
Missing if r_i < pMissing
Broken if pMissing <= r_i < pMissing + pBroken
Loose if ...
```

Never use iteration-order RNG.

### Manual selection

UI must allow selecting a board by stable ID/index and assigning a state.

Required states:

- Intact
- Missing
- Broken
- Loose
- Tilted
- Decayed

### Broken board geometry

First Playground implementation may use deterministic split geometry rather than a full fracture solver.

For board local length axis `x`:

```text
x_break = lerp(xMin, xMax, beta)
```

Generate two pieces with a small gap and a jagged end profile.

A bounded jagged cut can be:

```text
x_cut(y) = x_break
         + A1 * sin(2*pi*f1*y + phase1)
         + A2 * sin(2*pi*f2*y + phase2)
```

with amplitudes clamped relative to board width/thickness.

This is authored damage, not physical fracture simulation.

## 8.6 Local collapse field

Loss of supports may optionally drive a bounded preview sag/collapse envelope around damage center `s0`:

Gaussian form:

```text
w(s) = exp(-0.5 * ((s-s0)/sigma)^2)
verticalOffset(s) = -collapseDepth * w(s)
rollOffset(s) = collapseRoll * signedSide * w(s)
```

or compact cosine support:

```text
u = |s-s0|/R
w = 0.5 * (1 + cos(pi*u)) for u < 1
w = 0 otherwise
```

This is an art-directed procedural state. Do not present it as structural engineering simulation.

---

# 9. Catwalk / industrial walkway

Reuse the pier primitives with metal profiles:

- two longitudinal beams;
- deck plate/grating panels;
- cross beams;
- railings;
- support posts/legs;
- braces;
- optional pipe/cable tray attached to one side.

Damage states:

- missing grating panel
- bent railing bay
- missing post
- detached side pipe
- collapsed local bay

A catwalk should prove that the same assembly system works with different profiles/material roles, not only wood.

---

# 10. Simple beam and truss bridges

## 10.1 Beam bridge

Use:

- deck path
- left/right girders from OffsetPath
- cross beams by RepeatAlongPath
- optional piers
- guardrails

## 10.2 Truss panels

For panel length `p` and span `L`:

```text
n = max(1, round(L / p))
panelLength = L / n
```

At each panel station build nodes:

```text
bottomLeft, bottomRight
topLeft, topRight
```

and connect members according to preset:

- Warren
- Pratt-like
- Howe-like
- simple X brace

All members are rigid profile segments between semantic sockets.

Do not claim engineering load certification. This is procedural visual/level construction.

Damage can remove or detach selected braces while preserving IDs.

---

# 11. Suspension bridge — rigid/semi-rigid deck

This is distinct from a rope/plank bridge.

Use:

- deck center path;
- towers / anchor points;
- left/right main cables;
- vertical hangers;
- deck beams/planks/panels;
- optional stiffening truss/girders;
- side rail/cable roles.

## 11.1 Parabolic main cable under uniform horizontal deck load

For a symmetric span of horizontal length `L` with endpoints at equal height and sag `f`, a useful suspension-bridge authoring approximation under uniform load per horizontal metre is:

Using `x in [0,L]` and downward sag positive:

```text
y(x) = 4*f*x*(L-x) / L^2
```

This gives:

```text
y(0) = 0
y(L) = 0
y(L/2) = f
```

Derivative:

```text
dy/dx = 4*f*(L - 2*x) / L^2
```

If uniform load per horizontal metre is `w`, the horizontal cable force for the ideal parabolic model is:

```text
H = w*L^2 / (8*f)
```

Vertical reaction magnitude at each end:

```text
V_end = w*L/2
```

Approximate cable tension at horizontal position `x`:

```text
V(x) = w*(L/2 - x)
T(x) = sqrt(H^2 + V(x)^2)
```

Use these force values only as diagnostics/authoring estimates unless a later real structural simulation is explicitly implemented.

The Playground's primary job is geometric construction.

## 11.2 Unequal anchor/tower height

Add a linear baseline plus sag term.

Let endpoint heights be `h0` and `h1`:

```text
baseline(x) = h0 + (h1-h0)*(x/L)
cableY(x) = baseline(x) - sagShape(x)
```

where for a parabolic sag:

```text
sagShape(x) = 4*f*x*(L-x)/L^2
```

Sign convention depends on world up.

## 11.3 Side cables

For bridge half width `W/2`:

```text
leftCable(x)  = centerCable(x) - sideAxis(x) * W/2
rightCable(x) = centerCable(x) + sideAxis(x) * W/2
```

For a straight span, sideAxis is constant. For a curved experimental span, derive it from the deck RMF and validate offset self-intersection.

## 11.4 Hangers

Place hangers by true deck arc/horizontal station.

For station `x_k`:

```text
deckPoint = D(x_k)
cablePoint = C(x_k)
hangerLength = |cablePoint - deckPoint|
```

Generate a cable/rod segment between them.

Each hanger has a stable semantic identity.

Damage states:

- MissingHanger
- BrokenHanger
- LooseHanger

Selected hangers can later be promoted to physical strands; most remain static.

## 11.5 Towers and anchors

Towers are procedural assemblies with:

- left/right legs;
- cross beam;
- cable saddle points;
- deck connection;
- optional braces.

Anchor blocks are static endpoints in the Playground.

No need for a general foundation solver.

---

# 12. Catenary — rope/cable under self-weight

For a freely hanging uniform flexible cable under its own weight, use the catenary:

```text
y(x) = a*cosh((x-x0)/a) + c
```

For a symmetric cable with lowest point at `x=0`, shifted so `y(0)=0`:

```text
y(x) = a*(cosh(x/a) - 1)
```

For span `L` and desired sag `f`:

```text
f = a*(cosh(L/(2*a)) - 1)
```

Solve `a > 0` numerically.

Use a bounded root solve such as bisection/Newton with finite iteration/tolerance limits.

Do not iterate without bounds.

Once `a` is known, sample by error/arc-length policy rather than fixed arbitrary segment count.

When weight per unit cable length is `w_c`, the ideal horizontal tension parameter relates as:

```text
a = H / w_c
```

Treat the tension result as diagnostic in the Playground unless driven by the actual Jolt simulation.

---

# 13. Rope / plank suspension bridge

This family is intentionally less rigid.

Suggested assembly:

- two lower support ropes;
- deck planks repeated between them;
- two hand ropes / side ropes;
- vertical/diagonal connecting strands;
- anchor posts / rock anchors.

## 13.1 Deck support curve

Use a catenary or authored spline for each lower support rope.

Deck plank station `s_k`:

```text
leftSupport  = C_left(s_k)
rightSupport = C_right(s_k)
plankCenter  = (leftSupport + rightSupport) / 2
plankAcross  = normalize(rightSupport-leftSupport)
```

Forward orientation uses a smoothed tangent from the midpoint path.

Plank up should be derived from the cross/tangent frame, with optional authored stabilization so boards do not roll randomly.

## 13.2 Irregularity

Use bounded deterministic variation:

```text
offset_k = A * signedHash(seed, k)
yaw_k    = Y * signedHash(seed2, k)
```

Clamp these well below dimensions that would create impossible overlaps.

## 13.3 Damage

Selectable plank states:

- missing
- broken
- hanging
- loose
- tilted

A hanging board can be static/art-directed or promoted to a Jolt rigid body attached at one remaining side.

Selected side-rope spans can use the Strands/Webs Jolt representation later.

---

# 14. Damage selectors

One reusable selector stack should work for all assemblies.

Required selectors:

## 14.1 Index range

```text
role == DeckPlank && index in [12,18]
```

## 14.2 Distance interval

```text
s in [a,b]
```

## 14.3 Deterministic probability

```text
Hash01(seed, role, stableIndex) < probability
```

## 14.4 Cluster around distance

Gaussian:

```text
p(s) = strength * exp(-0.5*((s-s0)/sigma)^2)
```

or compact falloff.

Compare stable hash against `p(s)`.

## 14.5 Painted/manual selection

The owner clicks or selects the semantic part and sets its state.

Manual state overrides generated proposal until reset.

## 14.6 Protected parts

Allow protected/locked parts so procedural regeneration never destroys critical puzzle/navigation structure.

---

# 15. Geometry damage versus state damage

Do not force every damage state through boolean fracture.

Use the cheapest representation that preserves the intended silhouette/gameplay.

Examples:

```text
Missing board     -> omit semantic part
Loose board       -> transform or Jolt rigid body
Broken board      -> deterministic two-piece split
Bent rail         -> bounded deformed path/profile
Missing rail      -> split Sweep + gap
Broken cable      -> edge disabled / two strand ends
Collapsed bay     -> local assembly state + selected physical pieces
Surface cracks    -> material/decal/EPLS when silhouette does not need geometry
```

Reserve expensive fracture/boolean work for cases where the resulting geometry actually needs it.

---

# 16. Optional Jolt promotion

The assembly generator owns semantic construction. Jolt owns runtime physics.

Do not simulate the whole world because one board is loose.

```text
Generated Assembly
     |
     +-> Static parts -> ordinary baked mesh/collision
     |
     +-> Promoted physical parts
            -> rigid body / admitted constraint
            -> Jolt
```

Examples:

- loose pier board -> one rigid body;
- hanging broken rail -> rigid child with constraint;
- detached railing section -> rigid body;
- suspension bridge selected cable span -> strand/soft path when the Jerico soft-body seam is actually available;
- rope bridge hand ropes -> strands;
- most hangers/deck parts -> static.

### Current integration truth

JericoEngine currently has real Jolt rigid/constraint integration, while the broad SoftBody/Cosserat strand runtime seam remains planned/partial.

Therefore this Playground may:

1. use existing Jolt rigid/constraint capabilities where cleanly available in its dependency environment;
2. prototype soft/strand representation only if the necessary Jolt adapter can be isolated locally without claiming JericoEngine already supports it;
3. never add a second physics engine.

Keep Static / Hybrid / Dynamic modes explicit.

---

# 17. Suspension bridge physics preview policy

For the first bridge vertical:

### Static
All cables/deck geometry are procedural/baked.

### Hybrid
- deck/towers static;
- selected ropes/hangers physical;
- selected broken boards physical.

### Dynamic experimental
Only after bounded proof:
- simplified physical cable graph;
- render geometry mapped to it;
- fixed-step;
- strict node/edge budgets.

Do not simulate every rendered cable vertex.

The physical graph should be much coarser than the visual sweep.

---

# 18. LOD strategy

Every assembly needs a cheap runtime representation.

Examples:

## Pipe
- LOD0 detailed sweep + collars
- LOD1 reduced sweep + simplified collars
- LOD2 simplified run / baked detail where appropriate

## Railway
- LOD0 rail profile + sleepers + hardware
- LOD1 simpler rail + sleepers
- LOD2 merged/simplified track silhouette

## Railing
- LOD0 individual parts
- LOD1 merged simple members
- LOD2 simplified silhouette/card only where acceptable

## Wooden pier
- LOD0 individual boards/beams
- LOD1 merged deck groups, simplified braces
- LOD2 coarse deck/support silhouette

## Suspension bridge
- LOD0 deck members + main cables + hangers
- LOD1 simplified deck + main cables + reduced hanger density
- LOD2 silhouette-critical cables/towers/deck only

Semantic damage visibility must survive LOD changes. Do not show a missing board again at LOD2 because an intact merged mesh ignored its state.

---

# 19. Interactive Playground UI — mandatory

Add to the normal executable a visible workspace/panel approximately:

```text
Procedural Assemblies
```

It may live beside or inside Spline Construction if that keeps the UI coherent.

Preset shelf:

- Pipe + Couplings
- Railway Track
- Broken Railway
- Metal Railing
- Broken Railing
- Wooden Pier
- Damaged Wooden Pier
- Catwalk
- Beam/Truss Bridge
- Suspension Bridge
- Rope/Plank Bridge

At least **six** of these must become genuinely interactive before marking the assembly slice PASS, including:

- Pipe + Couplings
- Railway
- Broken Railing
- Wooden Pier
- Suspension Bridge
- Rope/Plank Bridge

## 19.1 Assembly controls

Expose useful parameters:

- path controls from Spline Construction
- width/height
- bay spacing
- post/support spacing
- board width/thickness/gap
- sleeper spacing/gauge
- coupling spacing/size
- rail profile dimensions
- bridge span/width
- tower height
- sag
- hanger spacing
- cable radius
- plank irregularity
- quality

## 19.2 Damage controls

- Enable Damage
- Seed
- Missing %
- Broken %
- Bent %
- Loose %
- Cluster center along path
- Cluster radius
- Select Part
- State dropdown
- Reset selected
- Lock/protect selected

## 19.3 Debug

- semantic IDs
- role colors
- path/frame
- support stations
- damage influence
- collision
- physical/promoted parts
- LOD
- wireframe
- UV checker where relevant

No placeholder control counts as implemented.

---

# 20. Required automated tests

## Stable assembly identity

- same recipe/seed -> same part IDs;
- quality/tessellation change -> same semantic part IDs;
- unrelated damage change -> unaffected part IDs stable.

## Repeat/stationing

- sleepers equal by arc distance;
- couplings equal by arc distance;
- bridge hangers at expected stations;
- pier boards obey fit policy.

## Pipe

- collars oriented to tangent;
- ring/sleeve bounds valid;
- body UV remains metric around collars or explicit region split.

## Railway

- gauge measured perpendicular to tangent;
- sleeper centers on station path;
- reverse deterministic;
- missing rail interval produces real gap.

## Railing

- bay membership stable;
- broken member split has finite nondegenerate geometry;
- missing state removes only intended member.

## Pier

- deck board count/fit exact;
- support pairs stable;
- broken/missing manual board survives regeneration;
- deterministic damage seed.

## Suspension bridge parabola

For equal-height anchors:

- endpoints match anchors;
- midpoint sag equals requested `f`;
- symmetry holds;
- hangers terminate on both deck and cable.

## Catenary

- solver converges within bounded iterations;
- endpoints/sag satisfy tolerance;
- extreme/invalid sag rejects;
- no NaN/Inf;
- reverse deterministic.

## Rope bridge

- plank endpoints lie on support ropes within tolerance;
- board orientation finite;
- missing board state creates actual gap.

## Damage selectors

- deterministic random selection;
- cluster falloff reproducible;
- manual override precedence;
- protected part never altered by procedural damage.

## Robustness

Reject/diagnose:

- negative widths/thicknesses;
- zero spacing;
- impossible gauge;
- sag <= invalid tolerance;
- sag/span that makes configured solver/budget invalid;
- support count over hard budget;
- self-intersecting offset path when detected;
- NaN/Inf;
- runaway tessellation.

---

# 21. Evidence command

Add one reproducible command, for example:

```text
JericoGeologyPlayground.exe --procedural-assemblies-test Evidence/procedural-assemblies
```

Exact name may differ.

Retain:

- recipe JSON
- metrics
- semantic part counts
- damage state counts
- deterministic hashes
- screenshots
- wireframe/debug screenshots
- optional Jolt diagnostics when physics is tested

Required retained visual cases:

1. Pipe with repeated couplings
2. Railway intact
3. Railway with missing sleeper + rail gap
4. Railing intact/broken comparison
5. Wooden pier
6. Same pier with manually selected broken/missing boards
7. Catwalk or truss bridge
8. Suspension bridge
9. Suspension bridge cable/hanger debug
10. Rope/plank bridge
11. Rope bridge with missing/hanging board
12. optional physical promoted piece

---

# 22. Performance measurements

Record at minimum:

- generation CPU ms;
- vertices/triangles;
- number of semantic parts;
- number of repeated stations;
- number of damage overrides;
- rebuild time after one local state edit;
- rebuild time after one path control-point edit;
- approximate physical body/constraint count where Jolt is enabled.

Do not claim incremental rebuild efficiency unless measured.

Future optimization should allow semantic sub-assemblies/bays to rebuild locally, but first prove correctness.

---

# 23. External mathematical references

Use these as mathematical references, not runtime dependencies:

- Engineering LibreTexts / TU Delft-derived cable chapter:  
  https://eng.libretexts.org/Bookshelves/Mechanical_Engineering/Introduction_to_Aerospace_Structures_and_Materials_%28Alderliesten%29/02%3A_Analysis_of_Statically_Determinate_Structures/06%3A_Arches_and_Cables/6.02%3A_Cables  
  Useful distinction: a cable changes shape with load; under horizontal uniform distributed load the ideal cable is parabolic.

- TinySpline:  
  https://github.com/msteinbeck/tinyspline  
  Reference for arc-length reparameterization and rotation-minimizing frames already selected/researched by Jerico.

Do not copy unrelated engine architecture from examples. Jerico owns its semantic recipe, identity, editor and cooked data model.

---

# 24. Acceptance gates

This slice is **not PASS** because it compiles.

PASS requires:

1. Release build succeeds.
2. Existing CTest remains green.
3. New assembly/damage tests pass.
4. Normal Playground UI exposes the assembly presets.
5. Owner can change path/structural dimensions and see live regeneration.
6. Owner can select a semantic board/rail/member and change its damage state.
7. Pipe couplings are placed by true arc distance.
8. Railway sleepers are placed by true arc distance.
9. A wooden pier has individually addressable deck boards.
10. A suspension bridge is generated from span/width/sag/hanger spacing controls.
11. A rope/plank bridge is generated from catenary/sag controls.
12. At least one broken/missing state survives LOD/debug switching without ghost geometry.
13. Evidence command retains images + metrics + recipe/hashes.
14. Documentation separates IMPLEMENTED / AUTOMATED VERIFIED / VISUALLY VERIFIED / EXPERIMENTAL / FAILED / PENDING.
15. JericoEngine checkout remains untouched.

---

# 25. Owner-facing definition of success

The owner must be able to do this without editing code:

```text
Open JericoGeologyPlayground
-> Procedural Assemblies

Pipe + Couplings
-> bend path
-> change spacing
-> see ===O===O=== follow the path

Railway
-> change gauge/sleeper spacing
-> mark one sleeper missing
-> create a rail gap

Broken Railing
-> select a bay
-> remove a post
-> break the top rail

Wooden Pier
-> change width/board gap/support spacing
-> click/select individual boards
-> mark boards Missing/Broken/Loose

Suspension Bridge
-> change span, width, tower height, sag and hanger spacing
-> inspect main cable/hanger debug

Rope/Plank Bridge
-> change catenary sag
-> remove one plank
-> optionally promote a hanging piece/strand to physics
```

If the owner cannot do that in the normal executable, the task is not finished.

And above all: keep this a reusable **Procedural Assembly** system. Do not create eleven unrelated generators that only happen to live in the same menu.
