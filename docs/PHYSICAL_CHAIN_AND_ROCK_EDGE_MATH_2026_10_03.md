# Physical Chain + Rock Edge Math Playground Directive

**Date:** 2026-10-03
**Repo:** `JericoFX/JericoGeologyPlayground`
**Status:** Playground experiment; not an Engine contract.

## Owner intent

Add two experimental tracks to the Playground:

1. **Physical hanging chains**: a chain suspended between two endpoints must form a natural U under gravity, have weight, collide with the floor/world, sleep/wake, and react to interaction when physics is enabled.
2. **Reuse the new edge-chain mathematics on rocks/cliffs** to improve ridges, cracks, erosion guides, strata edges and damage so current cuts stop looking mechanically swept/Boolean.

## Explicit deferment

**Do NOT implement AnchorPoint/MatePoint as a product system yet.**

For now use only lightweight test endpoints such as `ChainEndpoint { Transform transform; bool fixed; }` or equivalent existing Playground handles. These are test inputs only. No socket compatibility database, authoring UI, serialization contract, snap system or typed MatePoints in this task.

Later, AnchorPoints/MatePoints may simply provide endpoint transforms to this already-tested subsystem.

---

# A. Hanging chain: analytic U + Jolt

## 1. Static/editor solution: catenary

For a uniformly weighted flexible chain, use a catenary as the equilibrium reference:

`y(x) = a * cosh((x - x0) / a) + c`

For symmetric equal-height endpoints with horizontal separation `D` and chain length `L`:

`L = 2 * a * sinh(D / (2 * a))`

Solve `a` deterministically with bracketed bisection or safeguarded Newton. Do not use a parabola as the primary solution.

Use the catenary for:

- instant deterministic editor preview;
- initial pose before physics;
- far/sleeping representation experiments;
- validation oracle for the physics chain.

If `L <= distance(A,B) + epsilon`, treat the chain as taut/near-taut instead of trying to solve an impossible sag.

## 2. Sample by arc length

Do not place links uniformly in X. Sample along **arc length** so link spacing is stable.

At every sample build a local frame:

- tangent = curve derivative;
- reference up = opposite gravity;
- side = cross products with degeneracy protection;
- optional alternating roll of 90 degrees for classic chain links.

Keep deterministic link order and IDs.

## 3. Physical mode

Reuse the Playground's existing **Jolt** runtime. Do not create another physics backend.

Modes to test:

- `StaticCatenary`
- `PhysicalChain`
- `Hybrid`: spawn from catenary, promote to physics on interaction/proximity, optionally keep the settled pose when sleeping.

In `PhysicalChain`:

- intermediate links have mass;
- endpoint links are constrained to the two test endpoints;
- world gravity is active;
- floor and obstacle collisions are active;
- user/world impulses wake the chain;
- the physical chain is free to depart from the ideal catenary while moving.

The analytic curve is the initial pose/reference, **not** a runtime constraint.

## 4. Constraints and collision

Start simple and stable:

- positional connection between adjacent links;
- enough angular freedom to hang naturally;
- cheap convex/capsule-like collision proxies;
- avoid moving triangle-mesh collision as the default;
- adjacent links may use collision filtering if needed to avoid solver fighting.

Expose only useful prototype controls:

- link mass or mass-per-meter;
- link spacing;
- linear/angular damping;
- optional angular limit;
- physics enabled/disabled.

Do not hide instability by increasing global physics quality for the whole app.

## 5. The requested U falling to the floor

With enough slack, the low part of the chain must naturally touch and rest on the floor or obstacles.

In physical mode, **Jolt contact must produce the floor rest naturally**. Do not fake physics by clipping the catenary to the floor.

## 6. Interaction tests

Minimum validation:

- floor collision;
- box/rock obstacle collision;
- impulse into a middle link;
- drag/grab a middle link if the Playground interaction path already supports it;
- wake/sleep;
- fixed endpoint drift stays below tolerance;
- Reset restores deterministic initial pose.

Optional only after the above works:

- detach one endpoint;
- break one constraint/link;
- attach one endpoint to a moving test body.

## 7. Catenary versus physics evidence

Use equal-height endpoints and no floor contact. After the chain settles:

- compare link centers to analytic catenary;
- RMS positional error;
- max positional error;
- endpoint drift;
- settle time;
- active body count;
- CPU cost.

Test at least 16, 32 and 64 links. 128 is stress-only, never assumed as a product default.

---

# B. Reuse edge-chain math on rocks

## 8. Do not fork the shared math

The Playground already owns the shared `SurfaceFeatureField + DamageField` rule.

Do not create separate RockEdgeMath / EdgeDecalMath backends unless a concrete missing primitive is proven.

Add reusable primitives only where required:

- ordered edge chains;
- deterministic straight-continuation pairing;
- signed convex/concave/flat classification;
- split by turn angle;
- arc-length coordinate `s`;
- stable tangent/frame along a chain;
- distance-to-feature-chain;
- bounded falloff around a chain;
- canonical closed-loop seam/direction;
- local strip/self-intersection checks if required.

## 9. Correctness rules

Before geology uses this math:

- never use `unordered_set.begin()` as canonical loop start;
- deterministic tie-breaking for equal pairing scores;
- reject zero-length edges before normalization;
- signed bend must not flip because face adjacency order or edge v0/v1 changes;
- closed loops require canonical seam + direction;
- a closed loop with one split point must become one full open chain, not disappear;
- all epsilons/thresholds must be named and included in evidence.

Straight continuation may use `score = dot(dirA, dirB)`, where the most opposite pair approaches `-1`.

If greedy pairing is used at complex junctions, document it as a policy rather than claiming global optimal matching.

---

# C. Rock experiments using the new math

## 10. Convex ridge chains

Extract/select convex feature chains.

Use arc length `s` and distance `d` to make erosion coherent along the ridge:

`w = 1 - smoothstep(r0, r1, d)`

`amount(s) = base * w * deterministicVariation(s, seed)`

Test:

- soften/erode a ridge;
- sparse chips along selected parts;
- coherent variation along the chain;
- preserve flat surfaces outside the influence radius.

Goal: **feature-coherent erosion**, not simply more noise.

## 11. Concave chains as crack/drainage guides

Use concave chains as candidates for:

- crack seeds;
- drainage/erosion grooves;
- dirt/wetness masks;
- fracture guidance.

Do not crack every concave edge. Build a sparse deterministic score using existing shared fields such as signed curvature, cavity/contact, slope versus gravity, exposure, thickness if available, and seeded variation.

## 12. Remove the Boolean-slot look

Use chain-local coordinates around existing damage paths:

- longitudinal `s`;
- lateral distance;
- bounded depth;
- separate outer-rim and fresh-interior behavior.

Test:

- irregular rim driven coherently along `s`;
- chips concentrated near ridges/high-curvature sections;
- independent roughness/detail for fresh interior;
- bounded asymmetric cut shape;
- preserve strata contacts and deformation guards.

Extend current shared DamagePath/DamageField work. Do not replace it.

## 13. Strata/contact chains

On layered rocks/cliffs:

- derive chains along exposed bed/contact edges;
- parameterize by arc length;
- vary retreat/protrusion coherently per layer;
- preserve exact contact constraints;
- avoid synchronized repetition across all strata;
- use deterministic per-layer seeds with controlled correlation.

Goal: reduce the current stacked-slab/staircase appearance without reopening seams.

---

# D. Required execution order

1. **Pull current `main` first.**
2. Read `GOAL.md`, `docs/SHARED_DAMAGE_MATH_ALL_PROCEDURAL_SYSTEMS_2026_09_27.md` and relevant Evidence.
3. Add RED fixtures for deterministic edge-chain extraction, closed-loop seam, single-break closed loop, signed bend invariance and zero-length edge safety.
4. Add a tiny hanging-chain fixture: two endpoints + gravity + floor.
5. Implement only the missing reusable edge-chain primitives.
6. Implement deterministic catenary solve + arc-length sampling.
7. Reuse Jolt to promote links to physics.
8. Run rock experiments on at least one ordinary rock, one layered rock and one cliff/overhang if available.
9. Compare **before / after** visually and numerically.
10. Keep results that improve the requested behavior; remove failed experiments rather than leaving dead paths.

---

# E. Acceptance / evidence

Do not report PASS because a test executable merely reached the end.

For every experiment record:

- exact recipe/seed;
- Release build result;
- focused test result;
- full CTest result where appropriate;
- deterministic replay/hash where applicable;
- Vulkan screenshots from identical camera;
- triangle/body counts;
- timing;
- limitations and Beauty status.

### Chain minimum PASS

- deterministic analytic U;
- physical chain initializes from it;
- gravity changes the chain;
- floor/obstacle contact works;
- impulse/interaction propagates;
- endpoints remain constrained;
- sleeping/rest works;
- Reset is reproducible.

### Rock-edge minimum PASS

- deterministic chains;
- convex/concave sign survives ordering changes;
- no loop-loss bug;
- one visible geology experiment is measurably/visibly less mechanical;
- no broken closure/contact invariant;
- existing shared-damage regressions continue to pass.

---

# F. Things not to do

- Do not implement AnchorPoint/MatePoint yet.
- Do not create a second geometry/damage backend.
- Do not import a giant Edge Decal library wholesale.
- Do not add Eigen/libigl/OpenVDB just to solve these experiments.
- Do not replace Jolt.
- Do not use random unordered-container iteration to define generated geometry.
- Do not claim Beauty acceptance from topology/tests alone.
- Do not let this task erase or rewrite existing Playground evidence.

## Future hook

When AnchorPoint/MatePoint is designed later, it should feed endpoint transform A, endpoint transform B and a connection policy into this already-tested chain subsystem. The math/physics work should not need to be rewritten.