# Jerico Procedural Math Playground

**Date:** 2026-10-03  
**Status:** ACTIVE LAB DIRECTIVE  
**Owner:** Dot / Playground  
**Scope:** mathematics, algorithms, isolated demos and tests only

---

## 0. Hard boundary

This repository is a mathematics and procedural-systems laboratory.

For this phase:

- DO NOT integrate anything into JericoEngine2.
- DO NOT edit JericoEngine2.
- DO NOT create engine-facing glue, runtime adapters or migration code.
- DO NOT copy Playground code into the engine.
- DO NOT change engine contracts to make a Playground experiment fit.
- DO NOT treat a successful Playground demo as engine-ready.
- DO NOT coordinate implementation work with the Codex agent working on JericoEngine2.

Dot owns only the Playground and must continue proving the mathematics here.

The future engine integration is a separate task and remains explicitly blocked until a new instruction opens that gate.

---

# 1. Purpose

The Playground now exists to develop reusable mathematical building blocks for procedural construction and interaction.

Primary areas:

- edge graphs and connectivity;
- ordered chains and loops;
- splines and path resampling;
- stable curve frames;
- anchors and sockets;
- snapping and hysteresis;
- cable, hose and chain behavior;
- pipe/duct/rail routing;
- intersections and fittings;
- automatic supports;
- surface projection and clearance;
- cut/placement volumes;
- shared procedural-damage mathematics;
- projection/bake mathematics for replacing expensive temporary geometry with cheaper data.

The Playground is not responsible for final artistic asset production.

External tools such as Blender may later create artistic meshes. The Playground solves the mathematics that makes those pieces connect, orient, route, sag, adapt and react.

---

# 2. Preserve useful mathematics

Before deleting or replacing an existing subsystem, classify it as:

- KEEP
- GENERALIZE
- ISOLATE
- EXPERIMENTAL
- DEPRECATE
- REMOVE

REMOVE requires evidence that the code has no reusable mathematical or testing value.

Do not delete a solver because its original demonstration is no longer a priority. If useful mathematics is coupled to an old demo, isolate the reusable mathematics behind a clean Playground-side API.

---

# 3. Graphs and ordered chains

Keep and improve:

- edge adjacency;
- connected components;
- endpoint detection;
- junction detection;
- ordered chain extraction;
- loop extraction;
- stable traversal;
- continuation through vertices with degree greater than 2;
- straightest-continuation selection;
- deterministic tie breaking;
- branch classification;
- cumulative path length;
- per-segment metadata.

A chain result should conceptually expose:

- points;
- segments;
- total length;
- open/closed state;
- endpoints;
- junction references;
- cumulative distance;
- normalized U.

Normalized U must be stable enough to drive placement, longitudinal coordinates, weights, falloff and physics influence.

---

# 4. Curves and resampling

Provide reusable utilities for:

- linear paths;
- Hermite curves;
- Catmull-Rom when useful;
- arc-length approximation;
- arc-length parameterization;
- uniform-distance resampling;
- adaptive resampling by curvature;
- tangent evaluation;
- curvature estimation;
- smoothing with controllable error;
- simplification with tolerance;
- degenerate-segment handling.

Callers should be able to request samples by physical distance, not only by control-point index.

---

# 5. Stable curve frames

Build robust orientation for geometry following curves.

Do not rely only on raw Frenet frames where they can flip or become unstable. Prefer rotation-minimizing / parallel-transport behavior.

Each evaluated point should conceptually provide:

- position;
- rotation;
- tangent;
- normal;
- binormal;
- distance along curve;
- normalized U;
- curvature.

Required behavior:

- stable initial frame;
- controllable initial up;
- minimal unwanted twist;
- explicit twist offset;
- loop closure correction where possible;
- continuity across long paths;
- deterministic output.

This is foundational for cables, chains, pipes, rails, repeated modules and sockets.

---

# 6. Unified anchors and sockets

Anchor and socket mathematics must share one infrastructure.

The shared semantic data should include at least:

- stable ID;
- position;
- rotation;
- forward/up axes;
- semantic type;
- snap radius;
- angular tolerance;
- compatibility mask;
- translation permission;
- rotation permission;
- physics permission;
- physics mode.

Useful semantic categories include:

- Generic
- Cable
- Pipe
- Chain
- Rail
- Support
- Structural
- Hinge
- Handle
- Lock
- DoorFrame
- DoorLeaf
- WindowFrame
- WindowLeaf
- Valve
- Vent
- Electrical
- Fluid
- Mechanical
- GrabPoint
- IK
- Damage

Do not create a separate anchor architecture for each procedural feature.

---

# 7. Connection solver

Proximity alone is never enough to connect two items.

Connection resolution must evaluate:

1. semantic/type compatibility;
2. compatibility mask;
3. distance threshold;
4. angular threshold;
5. orientation solution;
6. hysteresis;
7. final accepted or rejected result.

Required properties:

- deterministic candidate ordering;
- stable snap target;
- explicit rejection reason for diagnostics;
- distance tolerance;
- angular tolerance;
- reversible connect/disconnect behavior;
- hysteresis to stop flicker near thresholds.

---

# 8. Cable, chain and hose sag

Connections between anchors should support modes equivalent to:

- Rigid
- Straight
- Spline
- Sag
- Physics
- Hybrid

For cheap sag, evaluate practical solutions such as:

- catenary when suitable;
- parabolic approximation when suitable;
- spline approximation driven by gravity/slack.

Useful parameters:

- length;
- slack;
- mass per meter;
- gravity;
- stiffness;
- tension;
- anchor distance.

The solver must behave safely for degenerate and impossible parameter combinations.

---

# 9. U-weighted anchor influence and physics handoff

Use normalized U to define how strongly each anchor influences a chain.

Example:

- near U=0, anchor A has high influence;
- toward the interior, A fades;
- near U=1, anchor B has high influence.

This weighting may drive:

- positional constraint strength;
- stiffness;
- damping;
- transition to physics;
- pin weights;
- deformation falloff.

A chain may be mathematically controlled near anchors and physically reactive farther away.

Required Playground tests:

- two fixed anchors;
- one fixed anchor plus one free end;
- moving anchor;
- cut in the middle;
- external impulse/collision;
- stable sleep;
- deterministic reattachment when supported.

Do not require one rigid body for every visible segment when a cheaper representation achieves the behavior.

---

# 10. Routing

Develop routing as a generic solver.

Inputs may include:

- start anchor;
- end anchor;
- obstacles;
- preferred directions;
- clearance;
- bend constraints;
- surface preference;
- manual waypoints.

Outputs should include:

- route polyline;
- smoothed route;
- segment classifications;
- junctions;
- bend angles;
- required fitting descriptors;
- support candidates;
- failure reason.

A failed route is a valid result when it explains why it failed.

---

# 11. Fitting classification

Create a generic junction classifier for concepts equivalent to:

- I / straight continuation
- L / elbow
- T
- Y
- X / cross
- END

Classification should consider:

- branch count;
- branch directions;
- pairwise angles;
- radius/diameter class;
- compatibility;
- continuation priority.

The output should be a fitting descriptor containing semantic type, transform, branch frames, branch radii and confidence.

Do not hardcode one production mesh. Choosing the final representation is outside this phase.

---

# 12. Repeated modules along a path

Support deterministic repeated placement for things such as:

- chain links;
- rail pieces;
- cable clips;
- brackets;
- modular structural elements.

Useful modes:

- fixed spacing;
- fixed count;
- fit spacing;
- anchor-to-anchor;
- curvature-aware placement.

Each instance should expose a stable index, transform, path U, distance and local frame.

---

# 13. Supports

Build generic support placement.

Potential inputs:

- maximum/minimum span;
- curvature;
- mass/load proxy;
- junction proximity;
- fitting proximity;
- manual anchors;
- forbidden zones;
- surface candidates;
- wall/ceiling/floor preference.

Support states:

- AUTO
- REQUIRED
- FORBIDDEN
- MANUAL

Every proposed support should carry a reason, for example:

- MaxSpanExceeded
- HighCurvature
- NearJunction
- ManualAnchor

---

# 14. Surface and clearance mathematics

Keep generic utilities for:

- ray/triangle queries;
- closest point;
- projection;
- normals;
- tangent basis;
- floor/wall/ceiling classification;
- orientation against surfaces;
- clearance offsets;
- intersection tests;
- distance queries;
- local sampling;
- segment/segment proximity;
- segment/triangle intersection;
- capsule-like clearance;
- path versus obstacle clearance;
- self-intersection detection;
- junction clustering;
- near-parallel handling.

Use a centralized tolerance policy where practical. Avoid unrelated hardcoded epsilon values.

---

# 15. Cut and placement volumes

Support simple mathematical volumes such as:

- box;
- oriented box;
- capsule;
- cylinder;
- convex volume.

Use cases:

- opening reservation;
- object clearance;
- socket approach region;
- forbidden placement;
- procedural cut request;
- interaction reach volume.

The Playground only proves geometry/overlap math. It must not modify JericoEngine2 walls or runtime scene data during this phase.

---

# 16. Shared procedural-damage mathematics

Keep general math that can operate on arbitrary meshes or regions:

- local damage fields;
- distance falloff;
- edge masks;
- crack path descriptors;
- local subdivision descriptors;
- fracture seeds;
- region classification;
- inside/outside classification;
- dirty-region tracking;
- deterministic random seeds;
- geometry-to-mask projection.

Keep visual-damage descriptors separate from structural topology changes.

Do not integrate production destruction into the engine yet.

---

# 17. Bake/projection mathematics

Keep experiments that convert expensive temporary geometry into cheaper data.

Potential outputs include:

- masks;
- height fields;
- normal-like fields;
- distance fields;
- coverage maps;
- card-placement data.

The goal is the transformation and validation only. Do not build engine material/shader integration yet.

---

# 18. Blender boundary

Future production assets may be authored with Blender/Blender MCP, but this Playground must not depend on Blender for core validation.

Use simple synthetic proxies to test semantics such as:

- component;
- pivot;
- socket;
- anchor;
- collision helper;
- cut volume.

Example proxy test: a modular door made only from boxes with frame sockets, leaf sockets, hinge axis, handle socket and opening volume.

The purpose is to prove the mathematics independently from production art.

Do not implement the final JericoEngine importer here.

---

# 19. Required demo suite

## Demo A — Ordered chains

Prove:

- stable chains;
- correct endpoints;
- correct junctions;
- deterministic traversal;
- stable U.

## Demo B — Stable path frames

Use paths with straight segments, sharp bends, near-vertical regions and loops.

Prove:

- no unexpected frame flips;
- controlled twist;
- deterministic repeated execution.

## Demo C — Cable between anchors

Prove:

- straight mode;
- sag mode;
- moving endpoint;
- U-based anchor influence;
- free-end physical behavior.

## Demo D — Pipe route

Prove:

- route creation;
- bend classification;
- T/Y/X junction classification;
- fitting descriptors;
- support candidates.

## Demo E — Socket snapping

Prove:

- type compatibility;
- orientation;
- distance rejection;
- angular rejection;
- hysteresis;
- deterministic target selection.

## Demo F — Modular door proxy

Use simple boxes only.

Prove:

- frame socket;
- leaf socket;
- hinge-axis alignment;
- correct pivot relationship;
- handle socket;
- cut-volume overlap.

No engine integration.

## Demo G — Cut cable

Prove:

- chain split;
- new endpoints;
- preserved U or documented reparameterization;
- detached section behavior;
- stable reconnect if supported.

---

# 20. Tests

Prefer invariant tests over screenshots alone.

Examples:

- every expected edge belongs to the correct chain;
- arc-length samples remain inside tolerance;
- frame basis stays orthonormal;
- no NaNs appear;
- snap target selection is deterministic;
- hysteresis prevents target flicker;
- fitting branch frames match route branches;
- support spacing respects configured limits;
- cutting a chain produces the expected components.

For numerical tests:

- state tolerance explicitly;
- avoid direct float equality unless intentionally exact;
- cover degenerate and near-degenerate inputs;
- cover different scales where relevant.

---

# 21. Visual diagnostics

Difficult mathematics must have debug visualization.

Useful overlays:

- control points;
- sampled points;
- tangents/normals/binormals;
- anchor axes;
- socket compatibility;
- snap radius;
- angular cone;
- route clearance;
- U gradient;
- support reasons;
- junction type;
- fitting branch frames;
- physical/control influence weights.

Debug rendering is validation infrastructure, not cosmetic UI work.

---

# 22. Determinism and performance

Identical inputs should produce identical outputs.

Any randomness must use an explicit seed. Avoid hidden time-based randomness.

Track performance when relevant:

- input size;
- graph nodes/edges;
- samples generated;
- candidate pairs;
- route iterations;
- support candidates;
- CPU time;
- GPU time when applicable;
- allocations.

Do not prematurely optimize every experiment, but identify accidental O(N²) work in systems expected to scale.

---

# 23. Result descriptors before final meshes

Prefer this flow:

input description -> reusable math -> semantic result descriptor -> debug/proxy visualization

Examples:

- route descriptor -> pipe proxy;
- fitting descriptor -> fitting proxy;
- support descriptor -> support proxy;
- anchor connection -> debug line/proxy.

Do not make a generated mesh the only representation of a mathematical solution.

---

# 24. Git working rule for Dot

Before coding:

1. pull latest changes;
2. inspect current status/diff;
3. do not overwrite unrelated work;
4. select one mathematical slice;
5. implement;
6. build;
7. run tests;
8. run the relevant Playground demo;
9. record evidence;
10. commit;
11. push.

Do not bundle unrelated experiments into one commit.

---

# 25. Evidence standard

A feature is not DONE merely because it compiles.

For each mathematical slice record:

- IMPLEMENTED
- TESTED
- VISUALLY VERIFIED
- PERFORMANCE CHECKED when relevant
- KNOWN LIMITATIONS

If something is unverified, say so explicitly.

Never turn an assumption into a PASS.

---

# 26. Priority order

Unless an active defect requires otherwise:

1. graph + ordered-chain correctness;
2. stable U / arc-length parameterization;
3. robust curve frames;
4. unified anchors/sockets;
5. snapping + hysteresis;
6. cable/chain sag;
7. U-weighted physical influence and free ends;
8. routing;
9. fitting classification;
10. supports;
11. surface projection and clearance;
12. cut volumes;
13. shared damage math;
14. bake/projection experiments.

Completing this list does NOT authorize engine integration.

When the Playground side is mature, document the public data contract and stop at the boundary.

---

# 27. Explicit non-goals

Do NOT:

- integrate with JericoEngine2;
- edit JericoEngine2 build files;
- add engine plugins;
- design the final production importer;
- create final production art assets;
- require Blender to validate core math;
- rewrite engine rendering;
- implement production Jolt integration;
- implement production Luau bindings;
- implement production editor panels;
- create a second socket/anchor architecture;
- create a giant abstraction framework without proving demos.

---

# 28. Integration gate

The gate between Playground and engine remains CLOSED.

Until an explicit future instruction says otherwise:

PLAYGROUND = research + implementation + tests + demos

JERICOENGINE2 = untouched by this work

When a system looks ready, Dot documents:

- what it solves;
- public data contract;
- inputs;
- outputs;
- invariants;
- tolerances;
- performance characteristics;
- known limitations;
- tests/evidence.

Then stop.

Do not perform integration.

---

# Final rule

**Prove the mathematics first. Integrate nothing yet.**

The Playground is the laboratory. JericoEngine2 only receives mature systems later through a separate explicit integration task.
